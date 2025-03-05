#include "game.hh"
#include "main.hh"
#include "tms/core/err.h"
#include "loading_screen.hh"
#include "soundmanager.hh"
#include "ui.hh"
#include "menu_shared.hh"
#include "menu_pkg.hh"
#include "menu_main.hh"
#include "menu_create.hh"
#include "menu-play.hh"
#include "settings.hh"
#include "material.hh"
#include "object_factory.hh"
#include "model.hh"
#include "cable.hh"
#include "ledbuffer.hh"
#include "spritebuffer.hh"
#include "textbuffer.hh"
#include "fluidbuffer.hh"
#include "linebuffer.hh"
#include "sticky.hh"
#include "rope.hh"
#include "polygon.hh"
#include "progress.hh"
#include "worker.hh"
#include "emitter.hh"
#include "display.hh"
#include "group.hh"
#include "tiles.hh"
#include "faction.hh"
#include "robot_base.hh"
#include "factory.hh"
#include "adventure.hh"
#include "gui.hh"

#include <ctime>
#include <errno.h>
#include <unistd.h>

#ifdef TMS_BACKEND_WINDOWS
#include <direct.h>
#define mkdir(dirname, ...) _mkdir(dirname)
#define create_dir(dirname, ...) _create_dir(dirname, 0)
#else
#define create_dir _create_dir

#endif

#ifdef BUILD_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include <sys/stat.h>

#include <tms/core/tms.h>
#include <tms/core/framebuffer.h>
#include <tms/core/entity.h>
#include <tms/core/pipeline.h>
#include <tms/bindings/cpp/cpp.hh>

#include "network.hh"

principia P={0};
static struct tms_fb *gi_fb;
static struct tms_fb *ao_fb;

static bool quitting = false;

static volatile int loading_counter = 0;


static Uint32 time_start;


static void
create_thread(int (SDLCALL *fn)(void*),
        const char *name, void *data)
{
    SDL_Thread *t = SDL_CreateThread(fn, name, data);
    SDL_DetachThread(t);
}

static void
menu_begin(void)
{

}

static void
gi_begin(void)
{
    time_start = SDL_GetTicks();

    if (P.best_variable_in_the_world2 != 1337) {
        glClearColor(0.f, 1.f, 0.f, 0.f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glColorMask(1,1,1,1);
    } else {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    if (settings["shadow_map_depth_texture"]->is_true()) {
        glColorMask(0,0,0,0);
    }

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glDisable(GL_CULL_FACE);
}

static void
set_connection_strength(entity *e, void *userdata)
{
    float strength = VOID_TO_INT(userdata) / 100.f;

    connection *c = e->conn_ll;
    while (c) {
        connection *next = c->get_next(e);

        G->set_connection_strength(c, strength);

        c = next;
    }
}

static void
set_color(entity *e, void *userdata)
{
    e->set_color(*((tvec4*)userdata));
}

static void
set_density(entity *e, void *userdata)
{
    float value = VOID_TO_INT(userdata) / 100.f;

    e->help_set_density_scale(value);
}

static void
set_render_type(entity *e, void *userdata)
{
    uint8_t render_type = VOID_TO_UINT8(userdata);

    connection *c = e->conn_ll;
    while (c) {
        connection *next = c->get_next(e);

        c->render_type = render_type;

        c->update_render_type();

        c = next;
    }

    if (e->gr) {
        W->add_action(e->id, ACTION_REBUILD_GROUP);
    }
}


static void
unlock_all(entity *e, void *userdata)
{
    if (e->is_locked()) {
        G->toggle_entity_lock(e);
    }
}

static void
disconnect_all(entity *e, void *userdata)
{
    e->disconnect_all();
}

void (* _glDiscardFramebufferEXT)(GLenum target, GLsizei num, const GLenum *d) = 0;

static int shader_loader(int step);
static int level_loader(int step);
static int open_loader(int step);
static int edit_loader(int step);
static int pkg_loader(int step);
static int publish_loader(int step);
static int submit_score_loader(int step);
#ifdef BUILD_PKGMGR
static int publish_pkg_loader(int step);
#endif

static void
gi_end(void)
{
    glColorMask(1,1,1,1);

    if (settings["shadow_quality"]->v.i != 2 && !settings["shadow_map_depth_texture"]->is_true()) {
        GLenum discards[] = {GL_DEPTH_ATTACHMENT};

        if (settings["discard_framebuffer"]->v.b && _glDiscardFramebufferEXT == 0) {
            _glDiscardFramebufferEXT = (void (*)(GLenum , GLsizei , const GLenum *))SDL_GL_GetProcAddress("glDiscardFramebufferEXT");
        }

        if (_glDiscardFramebufferEXT) {
            _glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, discards);
        }
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

static void
ao_begin(void)
{
    time_start = SDL_GetTicks();
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(0);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDisable(GL_BLEND);

    glColorMask(1,1,1,1);
    glDisable(GL_CULL_FACE);
    //glColorMask(0,0,0,0);
    //glCullFace(GL_FRONT);
}

static void
ao_end(void)
{
    //glColorMask(1,1,1,1);
    //glFinish();
    //
    glColorMask(1,1,1,1);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(1);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

static void
begin(void)
{
    time_start = SDL_GetTicks();

    /*if (P.best_variable_in_the_world != 1337) {
        glClearColor(.05f, .05f, .05f, 1.f);
        glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    }*/

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    //glDisable(GL_ALPHA_TEST);
    glDisable(GL_DITHER);
    //glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glDepthMask(0xff);
    glCullFace(GL_BACK);

    //glActiveTexture(GL_TEXTURE3);
    //glBindTexture(GL_TEXTURE_2D, gi_fb->fb_texture[gi_fb->toggle][0]);
    //glBindTexture(GL_TEXTURE_2D, gi_fb->fb_texture[gi_fb->toggle][0]);
    //tms_fb_bind_current_textures(gi_fb, GL_TEXTURE3);

    if (gi_fb) {
        if (settings["shadow_map_depth_texture"]->is_true()) {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, gi_fb->fb_depth[gi_fb->toggle]);
        } else {
            tms_fb_bind_current_textures(gi_fb, GL_TEXTURE3);
        }
    }
    if (ao_fb) tms_fb_bind_current_textures(ao_fb, GL_TEXTURE4);
    //glBindTexture(GL_TEXTURE_2D, gi_fb->fb_texture[gi_fb->toggle][0]);
    glActiveTexture(GL_TEXTURE0);

    //glFinish();
    //Uint32 delta = SDL_GetTicks() - time_start;
    //tms_infof("bind shadow map time: %u", delta);
    //
}

static void
end(void)
{
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

#if 0
    if (settings["postprocess"]->v.b) {
        GLenum discards[] = {GL_DEPTH_ATTACHMENT};
        if (settings["discard_framebuffer"]->v.b && _glDiscardFramebufferEXT) {
            _glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, discards);
        }
    }
#endif

}

void
init_framebuffers(void)
{
    tms_infof("Initializing framebuffers ");

    if (gi_fb) {
        tms_fb_free(gi_fb);
        gi_fb = 0;
    }

    if (ao_fb) {
        tms_fb_free(ao_fb);
        ao_fb = 0;
    }

    if (settings["enable_shadows"]->v.b) {
        tms_infof("SM(%u,%u)", settings["shadow_map_resx"]->v.i,settings["shadow_map_resy"]->v.i);
        gi_fb = tms_fb_alloc(settings["shadow_map_resx"]->v.i,settings["shadow_map_resy"]->v.i,  (settings["swap_shadow_map"]->v.b?1:0));
        /* XXX use RGB is only shadows, RGBA if shadows+gi */
        //tms_fb_add_texture(gi_fb, GL_RGB, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        //tms_fb_add_texture(gi_fb, GL_RGB32F, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        //

        int shadow_map_precision = GL_RGB;

        if (settings["shadow_map_depth_texture"]->is_true()) {
            tms_fb_add_texture(gi_fb, GL_RGB, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
            tms_fb_enable_depth_texture(gi_fb, GL_DEPTH_COMPONENT16);
        } else {
            if (settings["shadow_map_precision"]->v.i == 0) {
                shadow_map_precision = GL_RGB;
            } else if (settings["shadow_map_precision"]->v.i == 1) {
#ifdef TMS_USE_GLES
                /* Android does not seem to have either GL_RGB16F or GL_RGBA16F defined */
                shadow_map_precision = GL_RGB;
#else
                shadow_map_precision = GL_RGB16F;
#endif
            } else if (settings["shadow_map_precision"]->v.i == 2) {
#ifdef TMS_USE_GLES
                /* Android does not seem to have either GL_RGB32F or GL_RGBA32F defined */
                shadow_map_precision = GL_RGB;
#else
                shadow_map_precision = GL_RGB32F;
#endif
            }

            tms_fb_add_texture(gi_fb, shadow_map_precision, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
            tms_fb_enable_depth(gi_fb, GL_DEPTH_COMPONENT16);
        }
    }

    if (settings["enable_ao"]->v.i) {
        tms_infof("AO!!!!!!");
        int res = settings["ao_map_res"]->v.i == 512 ? 512 : (
                  settings["ao_map_res"]->v.i == 256 ? 256 :
                  128);
        ao_fb = tms_fb_alloc(res, res, (settings["swap_ao_map"]->v.b?1:0));
        tms_fb_add_texture(ao_fb, GL_RGB, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    }

    tms_pipeline_set_framebuffer(1, gi_fb);
    tms_pipeline_set_framebuffer(3, ao_fb);
}

/* called by TMS when we receive the initial command
 * line arguments or a new set of arguments from
 * another principia process */
void
tproject_set_args(int argc, char **argv)
{
    if (argc > 1) {

        char *s = argv[1];
        int n;

        do {
            if (strncmp(s, "principia://", 12) != 0)
                break;

            s+=12;

            /* extract the host */
            _community_host[0] = '\0';
            for (n=0; n<511; n++) {
                if (*(s+n) == '/' || *(s+n) == '\0') {
                    break;
                }
            }

            strncpy(_community_host, s, n);
            _community_host[n] = '\0';
            s += n;

            if (*s == '\0') {
                break;
            }

            s ++;

            /* backwards compatibility, if the "host" equals any of the reserved words below we default to the
             * currently signed-in community host instead and backset the pointer */
            if (strcmp(_community_host, "play") == 0 ||
                strcmp(_community_host, "sandbox") == 0 ||
                strcmp(_community_host, "edit") == 0) {
                s -= strlen(_community_host) + 1;
                _community_host[0] = '\0';
            }

            if (strncmp(s, "play/", 5) == 0) {
                s+=5;

                int action = ACTION_IGNORE;

                if (strncmp(s, "lvl/", 4) == 0)
                    action = ACTION_OPEN_PLAY;
                else if (strncmp(s, "pkg/", 4) == 0)
                    action = ACTION_PLAY_PKG;

                s+=4;

                if (strncmp(s, "local/", 6) == 0) {
                    s+=6;
                    _play_type = LEVEL_LOCAL;
                } else if (strncmp(s, "db/", 3) == 0) {
                    s+=3;
                    _play_type = LEVEL_DB;
                } else if (strncmp(s, "main/", 5) == 0) {
                    s+=5;
                    _play_type = LEVEL_MAIN;
                } else {
                    break;
                }

                _play_id = atoi(s);
                P.add_action(action, 0);
            } else if (strncmp(s, "sandbox/", 8) == 0) {
                s+=8;

                int action = ACTION_IGNORE;

                if (strncmp(s, "local/", 6) == 0) {
                    s+=6;
                    _play_type = LEVEL_LOCAL;
                    action = ACTION_OPEN;
                } else if (strncmp(s, "db/", 3) == 0) {
                    s+=3;
                    _play_type = LEVEL_DB;
                    action = ACTION_DERIVE;
                }

                _play_id = atoi(s);
                P.add_action(action, _play_id);
            } else if (strncmp(s, "edit/", 5) == 0) {
                s+=5;

                int action = ACTION_IGNORE;

                if (strncmp(s, "db/", 3) == 0) {
                    s+=3;
                    _play_type = LEVEL_DB;
                    action = ACTION_EDIT;
                }

                _play_id = atoi(s);
                P.add_action(action, _play_id);
            }

        } while (0);
    }
}

void
tproject_window_size_changed(void)
{
    if (!settings["window_maximized"]->v.b && settings["autosave_screensize"]->v.b) {
        settings["window_width"]->v.i = _tms.window_width;
        settings["window_height"]->v.i = _tms.window_height;
    }

    if (G) G->window_size_changed();
    if (P.s_menu_main) P.s_menu_main->window_size_changed();
    if (P.s_menu_create) P.s_menu_create->window_size_changed();
    if (P.s_menu_play) P.s_menu_play->window_size_changed();
    if (P.s_loading_screen) P.s_loading_screen->window_size_changed();
    if (P.s_intermediary) P.s_intermediary->window_size_changed();
    if (P.s_menu_pkg) P.s_menu_pkg->window_size_changed();

    /*
    P.can_reload_graphics = true;
    P.can_set_settings = true;
    P.add_action(ACTION_RELOAD_GRAPHICS, 0);
    */
}

void
tproject_init_pipelines(void)
{
    init_framebuffers();

    tms_infof("Initializing pipelines...");

    tms_pipeline_init();
    tms_pipeline_declare(0, "M", TMS_MAT4, offsetof(struct tms_entity, M));
    tms_pipeline_declare(0, "MV", TMS_MV, 0);
    tms_pipeline_declare(0, "MVP", TMS_MVP, 0);
    tms_pipeline_declare(0, "N", TMS_MAT3, offsetof(struct tms_entity, N));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
    tms_pipeline_declare_global(0, "ao_layer", TMS_INT, offsetof(game, tmp_ao_layer));
    tms_pipeline_declare_global(0, "ao_mask", TMS_VEC3, offsetof(game, tmp_ao_mask));
    tms_pipeline_declare_global(0, "SMVP", TMS_MAT4, offsetof(game, SMVP));
    tms_pipeline_declare_global(0, "AOMVP", TMS_MAT4, offsetof(game, AOMVP));

    tms_pipeline_declare_global(0, "_AMBIENTDIFFUSE", TMS_VEC2, offsetof(game, tmp_ambientdiffuse));
#pragma GCC diagnostic pop

    tms_pipeline_set_begin_fn(0, begin);
    tms_pipeline_set_end_fn(0, end);

    /* shadow mapping pipeline */
    tms_pipeline_declare(1, "M", TMS_MAT4, offsetof(struct tms_entity, M));
    tms_pipeline_declare(1, "MV", TMS_MV, 0);
    tms_pipeline_declare(1, "MVP", TMS_MVP, 0);
    tms_pipeline_declare(1, "N", TMS_MAT3, offsetof(struct tms_entity, N));

    tms_pipeline_set_begin_fn(1, gi_begin);
    tms_pipeline_set_end_fn(1, gi_end);

    /* menu pipeline */
    tms_pipeline_declare(2, "MVP", TMS_MVP, 0);
    tms_pipeline_set_begin_fn(2, menu_begin);

    /* ao pipeline */
    tms_pipeline_declare(3, "M", TMS_MAT4, offsetof(struct tms_entity, M));
    tms_pipeline_declare(3, "MV", TMS_MV, 0);
    tms_pipeline_declare(3, "MVP", TMS_MVP, 0);
    tms_pipeline_declare(3, "N", TMS_MAT3, offsetof(struct tms_entity, N));

    tms_pipeline_set_begin_fn(3, ao_begin);
    tms_pipeline_set_end_fn(3, ao_end);
}

void
intermediary::prepare(int (*loader)(int), tms::screen *s)
{
    this->loader = loader;
    this->next = s;
    P.s_loading_screen->set_next_screen(this);
}

int
intermediary::render(void)
{
    P.s_loading_screen->load(loader, next);

    return T_OK;
}

int
intermediary::resume(void)
{
    return T_OK;
}

void
intermediary::window_size_changed()
{
    if (this->get_surface() && this->get_surface()->ddraw) {
        float projection[16];
        tmat4_set_ortho(projection, 0, _tms.window_width, 0, _tms.window_height, 1, -1);
        tms_ddraw_set_matrices(this->get_surface()->ddraw, 0, projection);
    }
}


static int featured_levels_left = 0;

void
tproject_step(void)
{
    // XXX: Can we move the mutex lock/unlock to within the if-state?
    SDL_LockMutex(P.action_mutex);

    if (P.num_actions && P.loaded) {
        for (int x=0; x<P.num_actions; x++) {
            tms_debugf("Performing action %d", P.actions[x].id);
            void *data = P.actions[x].data;
            /* ACTION_ALL */
            switch (P.actions[x].id) {
                case ACTION_IGNORE:
                    break;

#ifdef BUILD_CURL
                case ACTION_GET_FEATURED_LEVELS: {
                    uint32_t num_featured_levels = VOID_TO_UINT32(data);

                    if (num_featured_levels > MAX_FEATURED_LEVELS_FETCHED) {
                        num_featured_levels = MAX_FEATURED_LEVELS_FETCHED;
                    }

                    featured_levels_left = num_featured_levels;

                    /* This thread will fetch the data */
                    create_thread(
                            _get_featured_levels,
                            "_get_featured_levels",
                            UINT_TO_VOID(num_featured_levels));
                } break;

                case ACTION_VERSION_CHECK:
                    create_thread(_check_version_code,"_version_check",  (void*)0);
                    break;

                case ACTION_LOGIN:
                    create_thread(_login, "_login", data);
                    break;

                case ACTION_REGISTER:
                    create_thread(_register, "_register", data);
                    break;

                case ACTION_PUBLISH_PKG:
#ifdef BUILD_PKGMGR
                    _publish_pkg_id = VOID_TO_UINT32(data);
                    G->resume_action = GAME_RESUME_OPEN;
                    if (_tms.screen == &P.s_loading_screen->super) {
                        P.s_intermediary->prepare(publish_pkg_loader, G);
                    } else {
                        P.s_loading_screen->load(publish_pkg_loader, G);
                    }
#endif
                    break;

                case ACTION_PUBLISH:
                    tms_debugf("action publish");
                    P.s_loading_screen->load(publish_loader, G);
                    G->resume_action = GAME_RESUME_CONTINUE;
                    break;

                case ACTION_SUBMIT_SCORE:
                    P.s_loading_screen->load(submit_score_loader, G);
                    G->resume_action = GAME_RESUME_CONTINUE;
                    break;

#endif

                case ACTION_RELOAD_DISPLAY:
                    ((display*)G->selection.e)->load_symbols();
                    break;

                case ACTION_ENTITY_MODIFIED:
                    if (G) {
                        G->state.modified = true;
                    }
                    break;

                case ACTION_SET_LEVEL_TYPE:
                    W->set_level_type(VOID_TO_UINT32(data));
                    break;

                case ACTION_REMOVE_AUTOSAVE: {
                    char tmp[1024];
                    snprintf(tmp, 1023, "%s/.autosave", pkgman::get_level_path(LEVEL_LOCAL));
                    unlink(tmp);
                } break;

                case ACTION_AUTOSAVE: {
                    if (G->state.sandbox && W->is_paused() && !G->state.test_playing) {
                        tms_infof("Autosaving...");

                        if (W->save(SAVE_TYPE_AUTOSAVE)) {
                            G->state.modified = false;
                        }
                    }
                } break;

                case ACTION_MULTI_JOINT_STRENGTH:
                    G->multiselect_perform(&set_connection_strength, data);
                    break;

                case ACTION_MULTI_PLASTIC_COLOR:
                    G->multiselect_perform(&set_color, data);
                    free(data);
                    break;

                case ACTION_MULTI_PLASTIC_DENSITY:
                    G->multiselect_perform(&set_density, data);
                    break;

                case ACTION_MULTI_CHANGE_CONNECTION_RENDER_TYPE:
                    G->multiselect_perform(&set_render_type, data);
                    break;

                case ACTION_MULTI_UNLOCK_ALL:
                    G->multiselect_perform(&unlock_all);
                    break;

                case ACTION_MULTI_DISCONNECT_ALL:
                    G->multiselect_perform(&disconnect_all);
                    G->selection.disable();
                    break;

                case ACTION_OPEN_AUTOSAVE:
                    G->open_sandbox(LEVEL_LOCAL,0);
                    break;

                case ACTION_PUZZLEPLAY: {
                    uint32_t id = VOID_TO_UINT32(data);
                    G->puzzle_play(id);
                } break;

                case ACTION_CONSTRUCT_ENTITY: {
                    uint32_t g_id = VOID_TO_UINT32(data);
                    G->editor_construct_entity(g_id);
                    G->state.modified = true;
                } break;

                case ACTION_CREATE_ADVENTURE_ROBOT: {
                    b2Vec2 *pos = (b2Vec2*)data;

                    if (!adventure::player) {
                        entity *e = of::create(O_ROBOT);
                        e->_pos.x = pos->x;
                        e->_pos.y = pos->y;
                        e->_angle = 0.f;
                        e->prio = 0;
                        ((robot_base*)e)->set_faction(FACTION_FRIENDLY);
                        e->on_load(true, false);
                        e->set_layer(G->state.edit_layer);
                        W->add(e);
                        adventure::player = static_cast<robot_base*>(e);
                        G->add_entity(e);

                        W->level.set_adventure_id(e->id);
                        G->state.adventure_id = e->id;
                    }

                    free(data);
                } break;

                case ACTION_CONSTRUCT_ITEM: {
                    uint32_t item_id = VOID_TO_UINT32(data);
                    G->editor_construct_item(item_id);
                    G->state.modified = true;
                } break;

                case ACTION_CONSTRUCT_DECORATION: {
                    uint32_t decoration_id = VOID_TO_UINT32(P.actions[x].data);
                    G->editor_construct_decoration(decoration_id);
                    G->state.modified = true;
                } break;

                case ACTION_GOTO_MAINMENU:
                    if (!data) {
                        sm::stop_all();
                    }
                    tms::set_screen(P.s_menu_main);
                    break;

                case ACTION_GOTO_CREATE:
                    if (!data) {
                        sm::stop_all();
                    }
                    tms::set_screen(P.s_menu_create);
                    break;

                case ACTION_GOTO_PLAY:
                    if (!data) {
                        sm::stop_all();
                    }
                    tms::set_screen(P.s_menu_play);
                    break;

                case ACTION_SAVE_STATE:
                    if (W->is_playing() && W->level.flag_active(LVL_ALLOW_QUICKSAVING)) {
                        G->save_state();
                        ui::message("Saved!");
                    } else {
                        ui::message("This level does not support quick saving.");
                    }
                    break;

                case ACTION_MULTIEMITTER_SET: {
                    if (G->selection.e && G->selection.e->g_id == O_MULTI_EMITTER) {
                        uint32_t id = VOID_TO_UINT32(data);
                        ((emitter*)G->selection.e)->set_partial(id);
                    }
                    G->state.modified = true;
                } break;

                case ACTION_EXPORT_OBJECT: {
                    char* name = (char*)data;
                    if (name) {
                        G->export_object(name);
                        free(name);
                    }
                } break;

                case ACTION_IMPORT_OBJECT: {
                    uint32_t id = VOID_TO_UINT32(data);
                    G->import_object(id);
                    G->state.modified = true;
                } break;

                case ACTION_SELECT_IMPORT_OBJECT: {
                    uint32_t id = VOID_TO_UINT32(data);
                    G->select_import_object(id);
                    G->state.modified = true;
                } break;

                case ACTION_DELETE_LEVEL: {
                    uint32_t *vec = (uint32_t*)data;
                    uint32_t lvltype = vec[0];
                    uint32_t id = vec[1];
                    uint32_t save_id = vec[2];

                    if (G->delete_level(lvltype, id, save_id)) {
                        ui::message("Successfully deleted level.");
                    } else {
                        ui::message("Unable to delete level.");
                    }

                    free(vec);
                } break;

                case ACTION_MULTI_DELETE:
                    G->_multidelete();
                    break;

                case ACTION_DELETE_SELECTION:
                    G->delete_selected_entity();
                    break;

                case ACTION_DELETE_PARTIAL: {
                    uint32_t id = VOID_TO_UINT32(data);
                    if (G->delete_partial(id)) {
                        ui::message("Successfully deleted object.");
                    } else {
                        ui::message("Unable to delete object.");
                    }
                } break;

                case ACTION_OPEN_STATE: {
                    if (data) {
                        uint32_t *info = (uint32_t*)data;

                        tms_debugf("open state called, %u %u %u", info[0], info[1], info[2]);
                        G->resume_action = GAME_RESUME_OPEN;

                        G->open_state(info[0], info[1], info[2]);

                        if (_tms.screen == &P.s_loading_screen->super) {
                            P.s_loading_screen->set_next_screen(G);
                        } else if (_tms.screen != &G->super){
                            tms::set_screen(G);
                        }

                        free(data);
                    }
                } break;

                case ACTION_OPEN: {
                    uint32_t id = VOID_TO_UINT32(data);
                    G->resume_action = GAME_RESUME_OPEN;
                    tms_debugf("ACTION_OPEN: %u", id);
                    G->open_sandbox(LEVEL_LOCAL, id);

                    if (_tms.screen == &P.s_loading_screen->super) {
                        P.s_loading_screen->set_next_screen(G);
                    } else if (_tms.screen != &G->super){
                        tms::set_screen(G);
                    }
                } break;

                case ACTION_STICKY: {
                    if (G->selection.e && G->selection.e->g_id == O_STICKY_NOTE){
                        static_cast<sticky*>(G->selection.e)->update_text();
                    }
                    G->state.modified = true;
                } break;

                case ACTION_SET_STICKY_TEXT: {
                    if (G->selection.e && G->selection.e->g_id == O_STICKY_NOTE){
                        static_cast<sticky*>(G->selection.e)->set_text((const char*)data);
                    }
                    G->state.modified = true;
                } break;

                case ACTION_NEW_GENERATED_LEVEL: {
                    int level_type = VOID_TO_INT(data);

                    tms_debugf("ACTION_NEW_GENERATED_LEVEL: %u", level_type);
                    G->create_level(level_type, false, false);
                } break;

                case ACTION_NEW_LEVEL: {
                    int level_type = VOID_TO_INT(data);

                    tms_debugf("ACTION_NEW_LEVEL: %u", level_type);
                    G->create_level(level_type, true, false);
                } break;

                case ACTION_RELOAD_GRAPHICS: {
                    tms_debugf("Reloading graphics...");
                    if (_tms.screen == &G->super) {
                        P.s_loading_screen->load(shader_loader, G);
                        G->resume_action = GAME_RESUME_CONTINUE;
                    } else if (_tms.screen == &P.s_menu_main->super) {
                        P.s_loading_screen->load(shader_loader, P.s_menu_main);
                    } else if (_tms.screen == &P.s_menu_create->super) {
                        P.s_loading_screen->load(shader_loader, P.s_menu_create);
                    } else if (_tms.screen == &P.s_menu_play->super) {
                        P.s_loading_screen->load(shader_loader, P.s_menu_play);
                    }
                } break;

                case ACTION_SAVE:
                    if (G->save()) {
                        ui::message("Saved!");
                    } else {
                        ui::message("Unable to save level.");
                    }
                    break;

                case ACTION_SAVE_COPY:
                    if (G->save_copy()) {
                        ui::message("Saved copy!");
                    } else {
                        ui::message("Unable to save copy.");
                    }
                    break;

                case ACTION_UPGRADE_LEVEL: {
                    /* make sure the chunk preloader has loaded all chunks */
                    W->level.version = LEVEL_VERSION;
                    G->save();
                    G->open_sandbox(LEVEL_LOCAL, W->level.local_id);
                    ui::message("level version upgraded!");
                } break;

                case ACTION_BACK:
                    G->back();
                    break;

                case ACTION_RESELECT: {
                    G->reselect();
                    G->state.modified = true;
                } break;

                case ACTION_MAIN_MENU_PKG: {
                    int type = VOID_TO_INT(data);
                    if (type == 0) {
                        P.s_menu_pkg->set_pkg(LEVEL_MAIN, 7);
                    } else {
                        P.s_menu_pkg->set_pkg(LEVEL_MAIN, 9);
                    }

                    tms::set_screen(P.s_menu_pkg);
                } break;

                case ACTION_SET_MODE:
                    G->set_mode(VOID_TO_INT(data));
                    break;

                case ACTION_HIGHLIGHT_SELECTED: {
                    if (G->selection.e) {
                        /* XXX: Should the highlight time be chosen from the optional userdata? */
                        G->add_highlight(G->selection.e, false, 0.5f);
                    }
                } break;

                case ACTION_RESTART_LEVEL: {
                    if (W->is_paused() && W->is_puzzle()) {
                        G->open_play(W->level_id_type, W->level.local_id, G->state.pkg);
                    } else {
                        G->do_pause(); /* NOTE: for custom and adventure, do_pause will trigger do_play when done */
                    }
                } break;

                case ACTION_WORLD_PAUSE:
                    G->do_pause();
                    break;

                case ACTION_AUTOFIT_LEVEL_BORDERS:
                    G->fit_level_borders();
                    G->state.modified = true;
                    break;

                case ACTION_RELOAD_LEVEL:
                    G->apply_level_properties();
                    W->init_level(true);
                    break;

                case ACTION_PLAY_PKG:
                    if (data != (void*)0) {
                        _play_id = VOID_TO_UINT32(data);
                    }

                    if (_tms.screen == &P.s_loading_screen->super) {
                        P.s_intermediary->prepare(pkg_loader, P.s_menu_pkg);
                    } else {
                        P.s_loading_screen->load(pkg_loader, P.s_menu_pkg);
                    }
                    break;

                case ACTION_WARP: {
                    uint32_t id = VOID_TO_UINT32(data);
                    tms_debugf("ACTION_WARP %u", id);
                    G->open_play(G->state.pkg->type, id, G->state.pkg);
                } break;

                case ACTION_OPEN_PLAY:
                    G->resume_action = GAME_RESUME_OPEN;
                    G->screen_back = 0;

                    if (_tms.screen == &P.s_loading_screen->super) {
                        /* we set the screen following the loading screen to an
                         * intermediary screen that runs the loading screen again but
                         * with another loader */
                        P.s_intermediary->prepare(level_loader, G);
                    } else {
                        P.s_loading_screen->load(level_loader, G);
                    }
                    break;

                case ACTION_DERIVE:
                    G->resume_action = GAME_RESUME_OPEN;
                    G->screen_back = 0;

                    if (_tms.screen == &P.s_loading_screen->super) {
                        P.s_intermediary->prepare(open_loader, G);
                    } else {
                        P.s_loading_screen->load(open_loader, G);
                    }
                    break;

                case ACTION_EDIT:
                    G->resume_action = GAME_RESUME_OPEN;
                    G->screen_back = 0;

                    if (_tms.screen == &P.s_loading_screen->super) {
                        P.s_intermediary->prepare(edit_loader, G);
                    } else {
                        P.s_loading_screen->load(edit_loader, G);
                    }
                    break;

                case ACTION_REFRESH_WIDGETS:
                    for (std::vector<pscreen*>::iterator it = P.screens.begin();
                            it != P.screens.end(); ++it) {
                        pscreen *ps = *it;

                        ps->refresh_widgets();
                    }
                    break;

                case ACTION_REFRESH_HEADER_DATA: {
                    pscreen::refresh_username();
                    menu_shared::refresh_message();

                    for (std::vector<pscreen*>::iterator it = P.screens.begin();
                            it != P.screens.end(); ++it) {
                        pscreen *ps = *it;

                        ps->refresh_widgets();
                    }
                } break;

                case ACTION_OPEN_MAIN_PUZZLE_SOLUTION: {
                    tms_infof("action open puzzle sol");
                    open_play_data *opd = static_cast<open_play_data*>(data);

                    G->open_play(opd->id_type, opd->id, opd->pkg, opd->test_playing, opd->is_main_puzzle);
                    G->resume_action = GAME_RESUME_OPEN;
                    tms::set_screen(G);

                    delete opd;
                } break;

                case ACTION_CREATE_MAIN_PUZZLE_SOLUTION: {
                    tms_infof("action create puzzle sol");
                    open_play_data *opd = static_cast<open_play_data*>(data);

                    uint8_t pkg_type = opd->id_type;
                    uint32_t level_id = opd->id;

                    char main_filename[1024];
                    snprintf(main_filename, 1023, "%s/%d.plvl", pkgman::get_level_path(LEVEL_MAIN), level_id);

                    char filename[1024];
                    snprintf(filename, 1023, "%s/7.%d.psol", pkgman::get_level_path(pkg_type), level_id);

                    pkg_type = LEVEL_LOCAL;

                    FILE_IN_ASSET(1);

                    _FILE *fp = _fopen(main_filename, "rb");

                    if (fp) {
                        _fseek(fp, 0, SEEK_END);
                        long size = _ftell(fp);
                        _fseek(fp, 0, SEEK_SET);

                        if (size > 8*1024*1024) {
                            tms_fatalf("Puzzle solution file too big");
                        }

                        char *buf = (char*)malloc(size);
                        _fread(buf, 1, size, fp);

                        _fclose(fp);

                        FILE *ofp = fopen(filename, "wb");

                        if (ofp) {
                            fwrite(buf, 1, size, ofp);
                            fclose(ofp);
                        }

                        free(buf);
                    }

                    G->open_play(pkg_type, level_id, opd->pkg, opd->test_playing, opd->is_main_puzzle);
                    G->resume_action = GAME_RESUME_OPEN;
                    tms::set_screen(G);

                    delete opd;
                } break;

                case ACTION_OPEN_LATEST_STATE: {
                    tms::screen *previous_screen = P.s_menu_main;

                    if (data) {
                        previous_screen = static_cast<tms::screen*>(data);
                    }

                    G->open_latest_state(false, previous_screen);
                } break;

                case ACTION_OPEN_URL:
                    if (data) {
                        ui::open_url((char*)data);
                        free(data);
                    }
                    break;

                case ACTION_SELF_DESTRUCT:
                    if (W->is_adventure() && adventure::player) {
                        adventure::player->damage(10000.f, 0, DAMAGE_TYPE_OTHER, DAMAGE_SOURCE_WORLD, 0);
                    }
                    break;
            }
        }
        P.num_actions = 0;
    }

    SDL_UnlockMutex(P.action_mutex);

    if (_tms.screen == &P.s_loading_screen->super) {
        int status = P.s_loading_screen->step_loading();

        switch (status) {
            case LOAD_ERROR: tms_errorf("LOADING SCREEN ERROR"); tms::set_screen(P.s_menu_main); break;
            case LOAD_RETRY: P.s_loading_screen->retry(); break;
            case LOAD_DONE:tms::set_screen(P.s_loading_screen->get_next_screen()); break;
            default: break;
        }
    }
}

void
tproject_soft_resume(void)
{
    ui::open_dialog(CLOSE_ABSOLUTELY_ALL_DIALOGS);

    int ierr;
    tms_assertf((ierr = glGetError()) == 0, "gl error %d before soft resume", ierr);

    tms_infof("SOFT RESUME ---------------------");
    for (int x=0; x<5; x++) {
        glActiveTexture(GL_TEXTURE0+x);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);

    tms_assertf((ierr = glGetError()) == 0, "gl error %d after texture cool bind", ierr);

    init_framebuffers();

    soft_resume_curl();

    tms_assertf((ierr = glGetError()) == 0, "gl error %d after soft resume", ierr);
    sm::resume_all();
}

void
tproject_soft_pause(void)
{
    sm::pause_all();

    if (_tms.screen == &G->super && G->state.sandbox && W->is_paused() && !G->state.test_playing && G->state.modified) {
        tms_infof("saving level");
        W->save(true);
    }

    tms_infof("SOFT PAUSE ---------------------");

    tms_infof("Saving settings...");
    settings.save();
    tms_infof("Saving progress...");
    progress::commit();

    soft_pause_curl();

    /* TODO: Save current level as a backup */
}

/**
 * This function is called when SDL gives us the SDL_Quit command.
 **/
void
tproject_quit(void)
{
    quitting = true;

    tms_infof("tproject_quit");

    tms_infof("Saving settings...");
    settings.save();
    tms_infof("Saving progress...");
    progress::commit();

    quit_curl();

    tms_infof("Cleaning settings...");
    settings.clean();

    delete G;

    sticky::_deinit();

    gui_spritesheet::deinit();

    /* TODO: Save current level as a backup */
}

void
setup_opengl_settings()
{
    if (settings["discard_framebuffer"]->is_uninitialized()) {
        settings["discard_framebuffer"]->v.b = ((bool)strstr(_tms.gl_extensions, "discard_framebuffer") ? 1 : 0);
    }

#ifdef TMS_BACKEND_MOBILE
    if (settings["shadow_map_precision"]->is_uninitialized()) {
        settings["shadow_map_precision"]->v.i = 0;
    }

    if (settings["shadow_map_depth_texture"]->is_uninitialized()) {
        if (strstr(_tms.gl_extensions, "GL_OES_depth_texture") != 0) {
            tms_infof("GL_OES_depth_texture: YES");
            settings["shadow_map_depth_texture"]->v.b = 1;
        } else {
            tms_infof("GL_OES_depth_texture: NO");
            settings["shadow_map_depth_texture"]->v.b = 0;
        }
    }
#else
    if (settings["shadow_map_precision"]->is_uninitialized()) {
        if (strstr(_tms.gl_extensions, "GL_ARB_texture_float") != 0
            || strstr(_tms.gl_extensions, "GL_ATI_texture_float") != 0) {
            tms_infof("GL_ARB_texture_float: YES");
            settings["shadow_map_precision"]->v.i = 1;
        } else {
            tms_infof("GL_ARB_texture_float: NO");
            settings["shadow_map_precision"]->v.i = 0;
        }
    }

    if (settings["gamma_correct"]->is_uninitialized()) {
        if (strstr(_tms.gl_extensions, "GL_EXT_texture_sRGB") != 0
            && strstr(_tms.gl_extensions, "GL_EXT_framebuffer_sRGB") != 0) {
            tms_infof("GL_EXT_texture_sRGB+GL_EXT_framebuffer_sRGB: YES");
            settings["gamma_correct"]->v.b = 1;
        } else {
            tms_infof("GL_EXT_texture_sRGB+GL_EXT_framebuffer_sRGB: NO");
            settings["gamma_correct"]->v.b = 0;
        }
    }

    if (settings["shadow_map_depth_texture"]->is_uninitialized()) {
        if (strstr(_tms.gl_extensions, "GL_ARB_depth_texture") != 0) {
            tms_infof("GL_ARB_depth_texture: YES");
            settings["shadow_map_depth_texture"]->v.b = 1;
        } else {
            tms_infof("GL_ARB_depth_texture: NO");
            settings["shadow_map_depth_texture"]->v.b = 0;
        }
    }
#endif
}

void
tproject_preinit(void)
{
    settings.init();
    tms_infof("Loading settings...");
    if (!settings.load())
        tms_infof("ERROR!");

    settings.save();

    tms_infof("Shadow quality: %d (%dx%d)",
        settings["shadow_quality"]->v.i8,
        settings["shadow_map_resx"]->v.i,
        settings["shadow_map_resy"]->v.i);
}

void
tproject_init(void)
{
    P.username = 0;
    P.user_id = 0;
    P.num_unread_messages = 0;
    P.message = 0;
    P.new_version_available = false;
    P.curl = 0;

    tms_infof("tproject_init called");
    srand((unsigned)time(0));

    setup_opengl_settings();
    settings.save();

    _tms.xppcm *= settings["uiscale"]->v.f;
    _tms.yppcm *= settings["uiscale"]->v.f;

#ifdef NO_UI
    settings["render_gui"]->set(false);
#endif

    P.action_mutex = SDL_CreateMutex();
    if (!P.action_mutex) {
        tms_fatalf("Unable to create action mutex.");
    }

    init_curl();
}

static int
shader_loader(int step)
{
    P.can_set_settings = true;

    switch (step) {
        case 0: break;
        case 1:
            if (!P.can_reload_graphics) {
                tms_debugf("Waiting for can_reload_graphics...");
                return LOAD_RETRY;
            }
            break;
        case 2: tms_scene_clear_graphs(G->get_scene()); break;
        case 3: material_factory::free_shaders(); break;
        case 4:
            material_factory::init_shaders();
            material_factory::init_materials();
            break;
        case 5: init_framebuffers(); G->init_framebuffers(); break;
        case 6: tms_scene_fill_graphs(G->get_scene()); break;
        case 7: return LOAD_DONE;

        case LOAD_RETURN_NUM_STEPS: return 7;
        default: LOAD_ERROR;
    }

    return LOAD_CONT;
}

#ifdef BUILD_CURL

static void
handle_downloading_error(int error)
{
    tms_debugf("Handling download error...");
    if (error) {
        if (_play_header_data.error_message) {
            tms_debugf("Outputting message from headers");
            // output any error message received from the headers
            ui::message(_play_header_data.error_message);
        } else if (_play_header_data.error_action == 0) {
            switch (error) {
                case DOWNLOAD_WRITE_ERROR:
                    ui::message("Unable to save level to disk.");
                    break;

                case DOWNLOAD_CHECK_INTERNET_CONNECTION:
                    ui::message("An error occured while attempting to download the file. Check your internet connection.");
                    break;

                case DOWNLOAD_GENERIC_ERROR:
                default:
                    ui::messagef("An error occured while downloading the level. (%d)", error);
                    break;
            }
        }
    }
}



#endif

static int
level_loader(int step)
{
    int num = __sync_fetch_and_add(&loading_counter, 1) % 30;

    switch (step) {
        case 0:
            _play_lock = true;
            _play_downloading = false;
            _play_download_for_pkg = false;
            // For Linux SS manager we will always assume it has DB levels downloaded :)
#ifdef BUILD_CURL
            if (_play_type == LEVEL_DB) {
                _play_downloading = true;
                create_thread(_download_level, "_download_level", 0);
            }
#endif
            break;
        case 1:
#ifdef BUILD_CURL
            if (num < 10) {
                P.s_loading_screen->set_text("Downloading level.");
            } else if (num < 20) {
                P.s_loading_screen->set_text("Downloading level..");
            } else {
                P.s_loading_screen->set_text("Downloading level...");
            }

            if (_play_downloading) return LOAD_RETRY;

            if (_play_downloading_error) {
                handle_downloading_error(_play_downloading_error);

                return LOAD_ERROR;
            } else {
                if (_play_header_data.notify_message)
                    ui::message(_play_header_data.notify_message, 1);
            }
#endif

            G->screen_back = 0;
            G->open_play(_play_type, _play_id, 0);
            break;
        case 2: default:
            P.s_loading_screen->set_text(0);
            _play_lock = false;
            _play_downloading = false;
            _play_downloading_error = 0;
            return LOAD_DONE;
        case LOAD_RETURN_NUM_STEPS: return 2;
    }

    return LOAD_CONT;
}

/**
 * Function used for deriving levels.
*/
static int
open_loader(int step)
{
#ifdef BUILD_CURL
    switch (step) {
        case 0:
            _play_lock = true;
            _play_downloading = false;
            _play_download_for_pkg = false;
            if (_play_type == LEVEL_DB) {
                _play_downloading = true;
                create_thread(_download_level, "_download_level", (void*)1); /* send 1 to derive */
            }
            break;
        case 1:
            if (_play_downloading) return LOAD_RETRY;

            if (_play_downloading_error) {
                handle_downloading_error(_play_downloading_error);

                return LOAD_ERROR;
            } else {
                if (_play_header_data.notify_message)
                    ui::message(_play_header_data.notify_message);
            }

            G->open_sandbox(LEVEL_LOCAL, _play_id);
            break;
        case 2: default: _play_lock = false; _play_downloading = false; _play_downloading_error = 0; return LOAD_DONE;
        case LOAD_RETURN_NUM_STEPS: return 2;
    }
#endif

    return LOAD_CONT;
}

static int
edit_loader(int step)
{
#ifdef BUILD_CURL
    switch (step) {
        case 0:
            _play_lock = true;
            _play_downloading = false;
            _play_download_for_pkg = false;
            if (_play_type == LEVEL_DB) {
                _play_downloading = true;
                create_thread(_download_level, "_download_level", (void*)2); /* send 2 to edit */
            }
            break;
        case 1:
            if (_play_downloading) return LOAD_RETRY;

            if (_play_downloading_error) {
                handle_downloading_error(_play_downloading_error);

                return LOAD_ERROR;
            } else {
                if (_play_header_data.notify_message)
                    ui::message(_play_header_data.notify_message);
            }

            G->open_sandbox(LEVEL_LOCAL, _play_id);
            break;
        case 2: default: _play_lock = false; _play_downloading = false; _play_downloading_error = 0; return LOAD_DONE;
        case LOAD_RETURN_NUM_STEPS: return 2;
    }
#endif

    return LOAD_CONT;
}

static int
pkg_loader(int step)
{
#ifdef BUILD_CURL
    switch (step) {
        case 0:
            _play_lock = true;
            _play_pkg_id = _play_id;
            _play_pkg_type = _play_type;
            _play_pkg_downloading_error = false;

            if (_play_pkg_type == LEVEL_DB) {
                _play_pkg_downloading = true;
                create_thread(_download_pkg, "_download_pkg", 0);
            }
            break;
        case 1:
            if (_play_pkg_downloading)
                return LOAD_RETRY;

            if (!_play_pkg_downloading_error) {
                if (!P.s_menu_pkg->set_pkg(_play_pkg_type, _play_pkg_id)) {
                    ui::message("Error loading package");
                    return LOAD_ERROR;
                }
            } else {
                ui::message("An error occured while downloading the package.");
                return LOAD_ERROR;
            }
            break;
        case 2: default: _play_lock = false; _play_pkg_downloading = false; _play_pkg_downloading_error = false; return LOAD_DONE;
        case LOAD_RETURN_NUM_STEPS: return 2;
    }
#endif

    return LOAD_CONT;
}

#ifdef BUILD_CURL

#ifdef BUILD_PKGMGR
static int
publish_pkg_loader(int step)
{
    switch (step) {
        case 0:
            break;

        case 1:
            _publish_pkg_error = false;
            _publish_pkg_done = false;
            create_thread(_publish_pkg, "_publish_pkg", 0);
            break;

        case 2:
            if (!_publish_pkg_done) {
                return LOAD_RETRY;
            }

            if (_publish_pkg_error) {
                tms_debugf("publish pkg error");
                return LOAD_DONE;
            }
            break;

        case 3:
            break;

        case 4: default:
            ui::message("Package published successfully!");
            return LOAD_DONE;

        case LOAD_RETURN_NUM_STEPS: return 4;
    }

    return LOAD_CONT;
}
#endif

static int
publish_loader(int step)
{
    switch (step) {
        case 0:
            G->save();
            _publish_lvl_id = W->level.local_id;
            _publish_lvl_community_id = 0;
            _publish_lvl_with_pkg = false;
            _publish_lvl_lock = true;
            _publish_lvl_uploading = false;
            _publish_lvl_uploading_error = false;
            break;

        case 1:
            _publish_lvl_uploading = true;
            create_thread(_publish_level, "_publish_level", 0);
            break;

        case 2:
            if (_publish_lvl_uploading) {
                return LOAD_RETRY;
            }

            if (_publish_lvl_uploading_error) {
                /* An error occured while publishing the level */
                return LOAD_DONE;
            }
            break;

        case 3:
            if (_publish_lvl_community_id != 0 && _publish_lvl_community_id != W->level.community_id) {
                W->level.community_id = _publish_lvl_community_id;
                /* no need to save, the community id was written by the publish thread */
                //G->save();
            }
            break;

        case 4: default:
            /* TODO: display another message if the community level was updated instead of published (already had a community id) */
            ui::open_dialog(DIALOG_PUBLISHED);
            _publish_lvl_community_id = 0;
            _publish_lvl_lock = false;
            _publish_lvl_uploading = false;
            _publish_lvl_uploading_error = false;
            return LOAD_DONE;

        case LOAD_RETURN_NUM_STEPS: return 4;
    }

    return LOAD_CONT;
}

static int
submit_score_loader(int step)
{
    int num = __sync_fetch_and_add(&loading_counter, 1);

    switch (step) {
        case 0:
            _submit_score_done = false;
            create_thread(_submit_score, "_submit_score", 0);
            break;

        case 1:
            switch (num % 3) {
                case 0:
                    P.s_loading_screen->set_text("Submitting highscores.");
                    break;
                case 1:
                    P.s_loading_screen->set_text("Submitting highscores..");
                    break;
                case 2:
                    P.s_loading_screen->set_text("Submitting highscores...");
                    break;
            }

            if (!_submit_score_done) {
                return LOAD_RETRY;
            }
            break;

        case 2: default:
            P.s_loading_screen->set_text(0);
            _submit_score_done = false;
            /* FIXME
            if (G->w_submit_score && G->w_submit_score->surface) {
                tms_surface_remove_widget(G->w_submit_score->surface, G->w_submit_score);
            }
            */
            return LOAD_DONE;

        case LOAD_RETURN_NUM_STEPS: return 2;
    }

    return LOAD_CONT;
}

#endif

static Uint32 loader_times[32] = {0,};

static const char *load_step_name[] = {
    /* 0  */ "Initialize atlases",
    /* 1  */ "Initialize fonts",
    /* 2  */ "Initialize more fonts",
    /* 3  */ "Loading GUI+Sounds",
    /* 4  */ "Initialize workers",
    /* 5  */ "Load materials",
    /* 6  */ "Allocating models",
    /* 7  */ "Loading models",
    /* 8  */ "Uploading models",
    /* 9 */ "Loading tilemaps",
    /* 10 */ "Initializing buffers",
    /* 11 */ "Init world",
    /* 12 */ "Init game",
    /* 13 */ "Init game GUI",
    /* 14 */ "Init menus",
    /* 15 */ "Load progress",
    /* 16 */ "Save settings",
};

bool
_create_dir(const char *path, mode_t mode)
{
    if (mkdir(path, mode) != 0) {
        switch (errno) {
            case EACCES:
                tms_errorf("We lack permissions to create folder %s", path);
                return false;

            case EEXIST:
                /* this should not be considered an error */
                return true;

            case ENAMETOOLONG:
                tms_errorf("Name of directory %s is too long.", path);
                return false;

            case ENOENT:
                tms_errorf("Parent directory for %s not found.", path);
                return false;

            default:
                tms_errorf("An unknown error occurs when attempting to create directory %s (%d)", path, errno);
                return false;
        }
    }

    return true;
}

static void
populate_community_host()
{
    P.community_host = "principia-web.se";

    char path[1024];
    snprintf(path, 1023, "%s/community_host.txt", tbackend_get_storage_path());
    FILE *fh = fopen(path, "r");

    if (!fh) return;

    static char buf[256];
    fgets(buf, 256, fh);

    for (size_t i = 0; i < 255; i++) {
        if (buf[i] == '\n')
            buf[i] = 0x0;
    }

    tms_infof("Overriding community host: %s", buf);

    P.community_host = buf;
}

static int
initial_loader(int step)
{
    static uint32_t last_time = SDL_GetTicks();

    char tmp[512];
    int retval = LOAD_CONT;

    switch (step) {
        case 0:
            {
                populate_community_host();

                static const char *s_dirs[]={
                    "",
                    "/cache", "/cache/db", "/cache/local", "/cache/main", "/cache/sav",
                    "/lvl", "/lvl/db", "/lvl/local", "/lvl/main",
                    "/pkg", "/pkg/db", "/pkg/local", "/pkg/main",
                    "/sav"
                };

                for (int x=0; x<sizeof(s_dirs)/sizeof(const char*); x++) {
                    sprintf(tmp, "%s%s", tbackend_get_storage_path(), s_dirs[x]);
                    create_dir(tmp, S_IRWXU | S_IRWXG | S_IRWXO);
                }

                /**
                 * We must init the recipes for the factories "dynamically" like this,
                 * because we have to make sure the item worths are already initialized.
                 **/
                factory::init_recipes();

                gui_spritesheet::init_atlas();
            }
            break;

        case 1:
            gui_spritesheet::init_loading_font();
            P.s_loading_screen->set_text("Loading fonts...");
            break;

        case 2:
            gui_spritesheet::init_fonts();
            P.s_loading_screen->set_text("Loading GUI...");
            break;

        case 3:
            gui_spritesheet::init();

            pscreen::init();
            menu_shared::init();
            adventure::init();

#ifdef BUILD_VALGRIND
            if (!RUNNING_ON_VALGRIND) {
                sm::init();
            } else {
                tms_debugf("Running on valgrind, disabling soundmanager.");
            }
#else
            sm::init();
#endif

            P.s_loading_screen->set_text("Loading workers...");
            break;

        case 4:
            ui::init();

            /* initialize worker threads */
            w_init();

            of::init();

            P.s_loading_screen->set_text("Loading materials...");
            break;

        case 5:
            material_factory::init();
            P.s_loading_screen->set_text("Allocating models...");
            break;

        case 6:
            {
                mesh_factory::init_models();

                char msg[128];
                snprintf(msg, 127, "Loading model %d/%d", cur_mesh+1, NUM_MODELS);
                P.s_loading_screen->set_text(msg);
            }
            break;

        case 7:
            {
                bool ret = mesh_factory::load_next();

                char msg[128];
                snprintf(msg, 127, "Loading model %d/%d", cur_mesh+1, NUM_MODELS);
                P.s_loading_screen->set_text(msg);

                if (ret) {
                    retval = LOAD_RETRY;
                    break;
                }
            }
            P.s_loading_screen->set_text("Uploading models...");
            break;

        case 8:
            mesh_factory::upload_models();

            P.s_loading_screen->set_text("Loading tilemaps...");
            break;

        case 9:
            tile_factory::init();
            P.s_loading_screen->set_text("Loading buffers...");
            break;

        case 10:
            cable::_init();
            display::_init();
            ledbuffer::_init();
            linebuffer::_init();
            textbuffer::_init();
            spritebuffer::_init();
            fluidbuffer::_init();
            rope::_init();
            polygon::_init();
            sticky::_init();

            P.s_loading_screen->set_text("Loading world...");
            break;

        case 11:
            W = new world();

            P.s_loading_screen->set_text("Loading game...");
            break;

        case 12:
            /* All fonts must be loaded before we can continue here */
            if (!gui_spritesheet::all_fonts_loaded) {
                tms_infof("Waiting for fonts to load...");
                return LOAD_RETRY;
            }

            G = new game();

            P.screens.push_back(G);

            P.s_loading_screen->set_text("Loading game GUI...");
            break;

        case 13:
            G->init_gui();
            break;

        case 14:
            P.s_menu_pkg = new menu_pkg();
            P.s_menu_main = new menu_main();
            P.s_menu_create = new menu_create();
            P.s_menu_play = new menu_play();

            P.screens.push_back(P.s_menu_pkg);
            P.screens.push_back(P.s_menu_main);
            P.screens.push_back(P.s_menu_create);
            P.screens.push_back(P.s_menu_play);

            P.s_loading_screen->set_text("Loading progress...");
            break;

        case 15:
            progress::init();
            break;

        case 16:
            {
#ifdef SCREENSHOT_BUILD
                P.s_loading_screen->set_text("Ready! o.o");
#else
                P.s_loading_screen->set_text(0);
#endif

                P.loaded = true;
                settings.save();

#ifdef BUILD_CURL
                /* do not start version check if we have an initial action like ACTION_OPEN_PLAY */
                if (P.num_actions == 0) {
                    create_thread(_check_version_code,"_version_check",  (void*)0);
                } else {
                    tms_infof("skipping version check");
                }

                uint32_t num_levels = 0;
                int res = _tms.opengl_width;

                do {
                    ++ num_levels;
                    res -= 380;
                } while (res > 0);

                num_levels = std::max(num_levels, 1U);

                num_levels = 4;

                P.add_action(ACTION_GET_FEATURED_LEVELS, VOID_TO_UINT32(num_levels));
#endif
            }
            break;

        case 17:
            {
                uint32_t total = 0;
                for (int x=0; x<step; x++) {
                    tms_infof("%27s: %u", load_step_name[x], loader_times[x]);
                    total += loader_times[x];
                }

                tms_infof("%27s: %u", "Total", total);

                ui::emit_signal(SIGNAL_QUICKADD_REFRESH);
            }

            return LOAD_DONE;

        case LOAD_RETURN_NUM_STEPS: return 17;
        default: return LOAD_ERROR;
    }

    if (retval != LOAD_RETRY) {
        uint32_t now = SDL_GetTicks();
        loader_times[step] = now - last_time;
        last_time = now;
    }

    return retval;
}

void*
tproject_initialize(void)
{
    P.s_loading_screen = new loading_screen();
    P.s_intermediary = new intermediary();

    P.s_loading_screen->load(initial_loader, 0);

    return 0;
}

principia::~principia()
{

}

void
principia::add_action(int id, void *data)
{
    if (id == ACTION_IGNORE) {
        return;
    }

    SDL_LockMutex(P.action_mutex);
    if (P.num_actions < MAX_ACTIONS) {
        P.actions[P.num_actions].id = id;
        P.actions[P.num_actions].data = data;
        P.num_actions ++;
    }
    SDL_UnlockMutex(P.action_mutex);
}

tvec3
principia::get_light_normal()
{
    tvec3 light = (tvec3){0.5, 1.3, 0.6}; // X,Y,Z normals
    tvec3_normalize(&light);
    return light;
}

extern "C" void
P_focus(int focus)
{
    P.focused = focus?true:false;

    if (focus) sm::resume_all();
    else sm::pause_all();
}

extern "C" void
P_add_action(int id, void *data)
{
    P.add_action(id, data);
}
