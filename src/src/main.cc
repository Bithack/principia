#include "game.hh"
#include "main.hh"
#include "version.hh"
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
#include "text.hh"
#include "tiles.hh"
#include "faction.hh"
#include "robot_base.hh"
#include "factory.hh"
#include "crc.hh"

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

#if defined(TMS_BACKEND_LINUX) && defined(DEBUG)
#include <valgrind/valgrind.h>
#include <csignal>
#endif

#include <sys/stat.h>

#include <tms/core/tms.h>
#include <tms/core/framebuffer.h>
#include <tms/core/entity.h>
#include <tms/core/pipeline.h>
#include <tms/bindings/cpp/cpp.hh>

#include "zlib.h"

#ifndef TMS_BACKEND_LINUX_SS
#define BUILD_CURL
#endif

#ifdef BUILD_CURL
#include <curl/curl.h>

#define CURL_CUDDLES \
        part = curl_mime_addpart(mime); \
        curl_mime_name(part, "key"); \
        curl_mime_data(part, "cuddles", CURL_ZERO_TERMINATED);

#endif

extern "C" void tmod_3ds_init(void);

enum {
    DOWNLOAD_GENERIC_ERROR              = 1,
    DOWNLOAD_WRITE_ERROR                = 2,
    DOWNLOAD_CHECK_INTERNET_CONNECTION  = 3,
};

principia P={0};
static struct tms_fb *gi_fb;
static struct tms_fb *ao_fb;

static char          featured_data_path[1024];
static char          featured_data_time_path[1024];
static char          cookie_file[1024];
static char          username[256];

static bool          is_very_shitty = false;

struct header_data {
    char *error_message;
    char *notify_message;
    int error_action;
};

static bool quitting = false;

static volatile int loading_counter = 0;

static int fl_fetch_time = 0;

static uint32_t      _play_id;
static char          _community_host[512] = {0}; /* Temporary input host from principia:// url, not to be confused with P.community_host */
static uint32_t      _play_type;
static bool          _play_lock;
static volatile bool _play_downloading = false;
static volatile bool _play_download_for_pkg = false;
static volatile int  _play_downloading_error = 0;
static struct header_data _play_header_data = {0};

/* Download pkg variables */
static uint32_t      _play_pkg_id;
static uint32_t      _play_pkg_type;
static uint32_t      _play_pkg_downloading = false;
static uint32_t      _play_pkg_downloading_error = false;

/* Publish level variables */
static uint32_t      _publish_lvl_community_id;
static uint32_t      _publish_lvl_id;
static bool          _publish_lvl_with_pkg = false;
static bool          _publish_lvl_set_locked = false;
static uint8_t       _publish_lvl_pkg_index = 0;
static bool          _publish_lvl_lock = false;
static volatile bool _publish_lvl_uploading = false;
static bool          _publish_lvl_uploading_error = false;
static char          _publish_lvl_error_msg[512];

/* Publish PKG variables */
static uint32_t      _publish_pkg_id;
static volatile bool _publish_pkg_done = false;
static bool          _publish_pkg_error = false;

/* Submit score variables */
static bool         _submit_score_done = false;

/** --Login **/
int _login(void *p);

/** --Register **/
int _register(void *p);

static Uint32 time_start;

struct MemoryStruct {
    char *memory;
    size_t size;
};

static void
create_thread(int (SDLCALL *fn)(void*),
        const char *name, void *data)
{
    SDL_Thread *t = SDL_CreateThread(fn, name, data);
#ifdef TMS_BACKEND_PC
    /* SDL_DetachThread was implemented in SDL 2.0.2, which is only available
     * for our PC backend as of now. */
    SDL_DetachThread(t);
#endif
}

#ifdef BUILD_CURL

static void
lock_curl(const char *invoker="N/A")
{
    tms_infof("%s locking curl...", invoker);
    SDL_LockMutex(P.curl_mutex);
    tms_infof("%s locked curl!", invoker);
}

static void
unlock_curl(const char *invoker="N/A")
{
    tms_infof("%s unlocking curl...", invoker);
    SDL_UnlockMutex(P.curl_mutex);
    tms_infof("%s unlocked curl!", invoker);
}

static void
print_cookies(CURL *curl)
{
    CURLcode res;
    struct curl_slist *cookies;
    struct curl_slist *nc;
    int i;

    tms_infof("Cookies, curl knows:");
    res = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
    if (res != CURLE_OK) {
        tms_errorf("Curl curl_easy_getinfo failed: %s", curl_easy_strerror(res));
        exit(1);
    }

    nc = cookies, i = 1;
    while (nc) {
        tms_infof("[%d]: %s", i, nc->data);
        nc = nc->next;
        i++;
    }

    if (i == 1) {
        tms_infof("(none)\n");
    }

    curl_slist_free_all(cookies);
}

#endif

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
static int publish_pkg_loader(int step);
static int _publish_level(void *p);
static int _download_level(void *p);
static int _check_version_code(void *_unused);
static int _get_featured_levels(void *_unused);

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
            if (settings["is_very_shitty"]->v.b || settings["shadow_map_precision"]->v.i == 0) {
                shadow_map_precision = GL_RGB;
            } else if (settings["shadow_map_precision"]->v.i == 1) {
#ifdef TMS_BACKEND_ANDROID
                /* Android does not seem to have either GL_RGB16F or GL_RGBA16F defined */
                shadow_map_precision = GL_RGB;
#else
                shadow_map_precision = GL_RGB16F;
#endif
            } else if (settings["shadow_map_precision"]->v.i == 2) {
#ifdef TMS_BACKEND_ANDROID
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
#if defined(TMS_BACKEND_WINDOWS) && !defined(DEBUG)
    static bool has_set_log = false;
    if (!has_set_log) {
        has_set_log = true;
        char logfile[1024];
        snprintf(logfile, 1023, "%s" SLASH "run.log", tbackend_get_storage_path());

        if (file_exists(logfile)) {
            char backup_logfile[1024];
            snprintf(backup_logfile, 1023, "%s" SLASH "bkp.run.log", tbackend_get_storage_path());

            if (file_exists(backup_logfile)) {
                unlink(backup_logfile);
            }

            tms_infof("Copying log file from %s to %s", logfile, backup_logfile);
            int r = rename(logfile, backup_logfile);

            if (r == 0) {
                tms_infof("Success!");
            } else {
                tms_infof("Error copying log file.. :(");
            }
        }

        tms_infof("changing log file to %s", logfile);
        FILE *log = fopen(logfile, "w+"); // XXX: why "w+" instead of "w"?
        if (log) {
            tms_set_log_file(log, log);
        } else {
            tms_infof("could not open log file for writing!");
        }
    }
#endif

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

    tms_pipeline_declare_global(0, "ao_layer", TMS_INT, offsetof(game, tmp_ao_layer));
    tms_pipeline_declare_global(0, "ao_mask", TMS_VEC3, offsetof(game, tmp_ao_mask));
    tms_pipeline_declare_global(0, "SMVP", TMS_MAT4, offsetof(game, SMVP));
    tms_pipeline_declare_global(0, "AOMVP", TMS_MAT4, offsetof(game, AOMVP));

    tms_pipeline_declare_global(0, "_AMBIENTDIFFUSE", TMS_VEC2, offsetof(game, tmp_ambientdiffuse));

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

char *featured_levels_buf = 0;
size_t featured_levels_buf_size = 0;
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
                    settings["is_very_shitty"]->v.b = false;
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
                        ui::emit_signal(SIGNAL_SAVE_LEVEL);
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
                    ui::emit_signal(SIGNAL_PLAY_COMMUNITY_LEVEL);
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
                            tms_fatalf("file too big");
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

#ifdef BUILD_CURL
    lock_curl("tproject_soft_resume");
    CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
    if (r != CURLE_OK) {
        tms_infof("ERR: %s", curl_easy_strerror(r));
        exit(1);
    }
    P.curl = curl_easy_init();
    unlock_curl("tproject_soft_resume");
#endif

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

#ifdef BUILD_CURL
    lock_curl("tproject_soft_pause");

    if (P.curl) {
        curl_easy_cleanup(P.curl);
        P.curl = 0;
    }
    curl_global_cleanup();

    unlock_curl("tproject_soft_pause");
#endif

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

#ifdef BUILD_CURL
    tms_infof("CURL easy cleanup...");
    lock_curl("tproject_quit");
    if (P.curl) {
        curl_easy_cleanup(P.curl);
        P.curl = 0;
    }
    curl_global_cleanup();
    unlock_curl("tproject_quit");
#endif

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

#ifdef TMS_BACKEND_MOBILE
    settings.init();
    settings.load();
    P.loaded_correctly_last_run = settings["loaded_correctly"]->v.b;
    if (!settings["fixed_uiscale"]->v.b && settings["uiscale"]->v.f == 1.f) {
        settings["uiscale"]->v.f = 1.3f;
    }

    /* TODO: Move this into a settings function. settings.post_load? */
    if (settings["fv"]->v.i == 1) {
        settings["fv"]->v.i = 2;
        settings["cam_speed_modifier"]->v.f = 1.f;
        settings["menu_speed"]->v.f = 1.f;
        settings["smooth_zoom"]->v.b = false;
        settings["smooth_cam"]->v.b = false;
        tms_infof("Modified cam settings.");
    }
    settings["fixed_uiscale"]->v.b = true;
    is_very_shitty = (!settings["loaded_correctly"]->v.b || settings["is_very_shitty"]->v.b);
    settings["loaded_correctly"]->v.b = false;
    settings["is_very_shitty"]->v.b = is_very_shitty;

#endif

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

#ifdef BUILD_CURL
    tms_debugf("Creating curl mutex");
    P.curl_mutex = SDL_CreateMutex();
    if (!P.curl_mutex) {
        tms_fatalf("Unable to create curl mutex.");
    }

    P.focused = 1;

    tms_infof("Initializing curl (v" LIBCURL_VERSION ")...");
    CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
    if (r != CURLE_OK) {
        tms_infof("ERR: %s", curl_easy_strerror(r));
        exit(1);
    }

    snprintf(cookie_file, 1024, "%s/c", tbackend_get_storage_path());
#endif
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
            material_factory::init_shaders(false);
            material_factory::init_materials(false);
            break;
        case 5: init_framebuffers(); G->init_framebuffers(); break;
        case 6: tms_scene_fill_graphs(G->get_scene()); break;
        case 7: return LOAD_DONE;

        case LOAD_RETURN_NUM_STEPS: return 7;
        default: LOAD_ERROR;
    }

    return LOAD_CONT;
}

struct level_write {
    const char *save_path;
    FILE *stream;
};

#ifdef BUILD_CURL

// Function used for saving a level that gets downloaded from a community site
static size_t _save_level(void *buffer, size_t size, size_t nmemb, void *stream)
{
    tms_infof("Saving level...");
    if (!stream) {
        tms_errorf("No stream!");
        return 0;
    }

    long http_code = 0;
    curl_easy_getinfo(P.curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        struct level_write *out = (struct level_write*)stream;

        if (!out->stream) {
            tms_infof("opening stream at %s", out->save_path);
            out->stream = fopen(out->save_path, "wb");
            if (!out->stream) {
                tms_errorf("Unable to open stream to save_path %s", out->save_path);
                return 0;
            }
        }

        tms_infof("returning fwrite data");
        return fwrite(buffer, size, nmemb, out->stream);
    } else if (http_code == 303) {
        tms_debugf("http_code was 303. do nothing!");
        return 1;
    } else {
        tms_errorf("Unhandled http code: %ld", http_code);
    }

    return 0;
}

static size_t
_parse_headers(void *buffer, size_t size, size_t nmemb, void *data)
{
    char *buf = (char*)buffer;
    char *pch = strchr(buf, ':');

    if (pch && strlen(buf) > pch-buf+1) {
        buf[pch-buf] = '\0';
        buf[nmemb-2] = '\0';
        char *v = pch+2;

        if (data) {
            if (strcmp(buf, "x-error-message") == 0) {
                ((struct header_data*)data)->error_message = strdup(v);
            } else if (strcmp(buf, "x-error-action") == 0) {
                ((struct header_data*)data)->error_action = atoi(v);
            } else if (strcmp(buf, "x-notify-message") == 0) {
                ((struct header_data*)data)->notify_message = strdup(v);
            }
        }

        if (strcmp(buf, "x-principia-user-id") == 0) {
            P.user_id = atoi(v);
        } else if (strcmp(buf, "x-principia-user-name") == 0) {
            P.username = strdup(v);
        } else if (strcmp(buf, "x-principia-unread") == 0) {
            P.num_unread_messages = atoi(v);
        }
    }

    return nmemb;
}

static size_t
_parse_headers_fl(void *buffer, size_t size, size_t nmemb, void *data)
{
    char *buf = (char*)buffer;
    char *pch = strchr(buf, ':');

    if (pch && strlen(buf) > pch-buf+1) {
        buf[pch-buf] = '\0';
        buf[nmemb-2] = '\0';
        char *v = pch+2;

        if (strcmp(buf, "x-fetch-time") == 0) {
            fl_fetch_time = atoi(v);
            tms_infof("fl_feetch_time: %d", fl_fetch_time);

            FILE *fh = fopen(featured_data_time_path, "w");

            if (fh) {
                fwrite(v, 1, strlen(v), fh);
                fclose(fh);
            }
        }

        if (strcmp(buf, "x-principia-user-id") == 0) {
            P.user_id = atoi(v);
        } else if (strcmp(buf, "x-principia-user-name") == 0) {
            P.username = strdup(v);
        } else if (strcmp(buf, "x-principia-unread") == 0) {
            P.num_unread_messages = atoi(v);
        }
    }

    return nmemb;
}

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

static int
progress_cb(
        void *userdata,
        curl_off_t dltotal, curl_off_t dlnow,
        curl_off_t ultotal, curl_off_t ulnow
        )
{
    if (quitting) {
        // force quit!
        return 1;
    }

    return 0;
}

static void
init_curl_defaults(void *curl)
{
    curl_easy_reset(P.curl);

    curl_easy_setopt(P.curl, CURLOPT_SSL_VERIFYHOST, 0); /* XXX */
    curl_easy_setopt(P.curl, CURLOPT_SSL_VERIFYPEER, 0); /* XXX */

    curl_easy_setopt(P.curl, CURLOPT_USERAGENT,
        "Principia/" STR(PRINCIPIA_VERSION_CODE) " (" OS_STRING ") (" PRINCIPIA_VERSION_STRING ")");

    curl_easy_setopt(P.curl, CURLOPT_HEADERFUNCTION, _parse_headers);

    curl_easy_setopt(P.curl, CURLOPT_COOKIEFILE, cookie_file);
    curl_easy_setopt(P.curl, CURLOPT_COOKIEJAR, cookie_file);

    curl_easy_setopt(P.curl, CURLOPT_XFERINFOFUNCTION, progress_cb);
    curl_easy_setopt(P.curl, CURLOPT_NOPROGRESS, 0);

#ifdef DEBUG
    curl_easy_setopt(P.curl, CURLOPT_VERBOSE, 1);
#endif

    // Note: this may put token cookie in the log output
    //print_cookies(P.curl);
}

int
_download_pkg(void *_p)
{
    CURLcode res;

    char save_path[1024];
    sprintf(save_path, "%s/%d.ppkg",
            pkgman::get_pkg_path(_play_pkg_type),
            _play_pkg_id);

    tms_debugf("save: %s", save_path);

    char url[256];
    snprintf(url, 255, "https://%s/internal/get_package?i=%d",
            P.community_host,
            _play_pkg_id);
    long http_code = 0;

    struct level_write save_data = {
        save_path,
        NULL
    };

    lock_curl("download_pkg");
    if (P.curl) {
        init_curl_defaults(P.curl);

        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        curl_easy_setopt(P.curl, CURLOPT_WRITEFUNCTION, _save_level);
        curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, &save_data);
        curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 30L);

        res = curl_easy_perform(P.curl);

        if (res != CURLE_OK) {
            tms_errorf("error while downloadnig file: %s", curl_easy_strerror(res));
            _play_pkg_downloading_error = true;
        } else {
            curl_easy_getinfo(P.curl, CURLINFO_RESPONSE_CODE, &http_code);

            if (http_code == 404) {
                _play_pkg_downloading_error = true;
            } else {
                tms_debugf("got http code %ld", http_code);
            }
        }
    }
    unlock_curl("download_pkg");

    if (save_data.stream) {
        fclose(save_data.stream);
    }

    pkginfo p;
    if (p.open(_play_pkg_type, _play_pkg_id)) {
        tms_debugf("pkg name: %s", p.name);
        tms_debugf("pkg num_levels: %d", p.num_levels);
        tms_debugf("pkg first_is_menu: %d", p.first_is_menu);
        tms_debugf("pkg unlock_count: %d", p.unlock_count);

        /* package was successfully downloaded, now loop through the levels
         * and download them all !!!! */
        for (int x=0; x<p.num_levels; x++) {
            _play_downloading_error = 0;
            _play_id = p.levels[x];
            _play_type = LEVEL_DB;
            _play_download_for_pkg = true;
            _download_level(0);

            if (_play_downloading_error) {
                _play_pkg_downloading_error = true;
                break;
            }
        }
    } else {
        _play_pkg_downloading_error = true;
    }
    //p.save();

    _play_pkg_downloading = false;

    return T_OK;
}

/* start with p = 1 to download a derivative to sandbox, p=2 to edit the level (assuming it's your own) */
int
_download_level(void *p)
{
    // begin by resetting error state
    _play_downloading_error = 0;
    if (_play_header_data.error_message) {
        free(_play_header_data.error_message);
        _play_header_data.error_message = 0;
    }
    if (_play_header_data.notify_message) {
        free(_play_header_data.notify_message);
        _play_header_data.notify_message = 0;
    }

    _play_header_data.error_action = 0;

    CURLcode res;
    uint32_t crc=0;
    struct stat file_stat;

    int arg = (intptr_t)p;
    int type = LEVEL_DB;
    bool derive = true;

    if (arg == 0) {
        type = LEVEL_DB;
        derive = false;
    } else if (arg == 1) {
        type = LEVEL_LOCAL;
        derive = true;
    } else if (arg == 2) {
        type = LEVEL_LOCAL;
        derive = false;
    }

    tms_infof("before: %d ++++++++++++++++++++++ ", _play_id);
    uint32_t new_id = type == LEVEL_LOCAL ? pkgman::get_next_level_id() : _play_id;
    uint32_t old_id = _play_id;

    if (type == LEVEL_LOCAL) {
        if (derive) {
            tms_debugf("downloading derivative level");
        } else {
            tms_debugf("Attempting to edit a level as your own.");
        }
    }

    char save_path[1024];
    sprintf(save_path, "%s/%d.plvl",
            pkgman::get_level_path(type),
            new_id);

    tms_debugf("save: %s", save_path);

    uint32_t r = 0;

    if (type == LEVEL_DB) {
        lvledit e;
        if(e.open(LEVEL_DB, new_id)) {
            /* File already exists, check if we actually need to download a new version. */
            r = e.lvl.revision;
            tms_debugf("we already have this DB level of revision %u", r);
        }
    }

    const char *host = strlen(_community_host) > 0 ? _community_host : P.community_host;

    char url[256];
    snprintf(url, 255, "https://%s/internal/%s_level?i=%d&h=%u",
            host,
            _play_download_for_pkg ? "get_package" :
                (type == LEVEL_DB ? "get" :
                    (derive == true ? "derive" : "edit")),
            _play_id, r);

    tms_infof("url: %s", url);

    long http_code = 0;
    bool require_login = false;

    struct level_write save_data ={
        save_path,
        NULL
    };

    _play_id = new_id;

    tms_infof("_play_id = %d -----------------------", _play_id);

    lock_curl("download_level");
    if (P.curl) {
        init_curl_defaults(P.curl);

        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        curl_easy_setopt(P.curl, CURLOPT_WRITEFUNCTION, _save_level);
        curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, &save_data);

        curl_easy_setopt(P.curl, CURLOPT_WRITEHEADER, &_play_header_data);

        curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 30L);
        curl_easy_setopt(P.curl, CURLOPT_TIMEOUT, 60L);

        tms_infof("we get here first");

        res = curl_easy_perform(P.curl);
        P.add_action(ACTION_REFRESH_HEADER_DATA, 0);

        if (res != CURLE_OK) {
            tms_infof("we get here");
            if (res == CURLE_WRITE_ERROR) {
                _play_downloading_error = DOWNLOAD_WRITE_ERROR;
            } else {
                _play_downloading_error = DOWNLOAD_CHECK_INTERNET_CONNECTION;
            }

            tms_errorf("error while downloading file: [%d]%s", res, curl_easy_strerror(res));
        }

        curl_easy_getinfo(P.curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code == 404) {
            _play_downloading_error = DOWNLOAD_GENERIC_ERROR;
        } else {
            tms_debugf("got http code %ld", http_code);

            if (http_code == 303) {
                if (type == LEVEL_LOCAL && !derive) {
                    require_login = true;
                    tms_errorf("user must log in before editing the level.");
                    _play_downloading_error = 1;
                }
            } else if (http_code == 500) {
                switch (_play_header_data.error_action) {
                    case ERROR_ACTION_LOG_IN:
                        require_login = true;
                        break;
                }

                _play_downloading_error = 1;
            }
        }
    }
    unlock_curl("download_level");

    if (require_login) {
        if (type == LEVEL_DB) {
            ui::next_action = ACTION_OPEN_PLAY;
        } else {
            if (derive) {
                ui::next_action = ACTION_DERIVE;
            } else {
                ui::next_action = ACTION_EDIT;
            }
        }

        ui::open_dialog(DIALOG_LOGIN);
    }

    if (save_data.stream) {
        tms_debugf("Closing save data stream.");
        fclose(save_data.stream);
    }

    if (!_play_downloading_error) {
        lvledit e;
        if (e.open(type, new_id)) {
            if (derive) {
                tms_debugf("derive = true");
            } else {
                tms_debugf("derive = false");
            }
            /* make sure the level has its community_id set correctly, just in case.
             * This should have been set when the level was published but it is possible
             * for the publisher to alter the id stored in the level file using tools
             * such as wireshark */
            if (type == LEVEL_LOCAL && e.lvl.allow_derivatives && derive) {
                tms_debugf("setting derive properties");
                e.lvl.community_id = 0;
                e.lvl.parent_id = old_id;
                e.lvl.parent_revision = e.lvl.revision;
                e.lvl.revision = 0;
                e.lb.size -= 1;
            } else if (type == LEVEL_LOCAL && !derive) {
                tms_debugf("editing level, do nothing");
                e.lb.size -= 1;
            } else {
                e.lvl.community_id = old_id;
            }
            e.save();
        } else {
            tms_errorf("wtf? we just downloaded it and couldnt open it");
        }
    } else {
        tms_debugf("An error occured while downloading the level.");
        _play_id = old_id;
    }

    _play_downloading = false;

    return T_OK;
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

static size_t
write_memory_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        tms_fatalf("wmc out of memory!");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

#ifdef BUILD_CURL

static int
_check_version_code(void *_unused)
{
    int res = T_OK;
    CURLcode r;

    struct MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);
    chunk.size = 0;

    lock_curl("check_version_code");
    if (P.curl) {
        init_curl_defaults(P.curl);

        char url[256];
        snprintf(url, 255, "https://%s/internal/version_code", P.community_host);
        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        curl_easy_setopt(P.curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
        curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, (void*)&chunk);
        curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 35L);

        if ((r = curl_easy_perform(P.curl)) == CURLE_OK) {
            if (chunk.size > 0) {
                int server_version_code = atoi(chunk.memory);

                if (server_version_code > PRINCIPIA_VERSION_CODE) {
                    P.new_version_available = true;
                    ui::message("A new version of Principia is available!", true);
                }

                tms_debugf("Client: %d. Server: %d", PRINCIPIA_VERSION_CODE, server_version_code);
                if (P.message) {
                    free(P.message);
                }
                P.message = strdup(chunk.memory);
            } else {
                tms_errorf("could not check for lateset version: invalid data");
            }
        } else {
            tms_errorf("could not check for latest version: %s", curl_easy_strerror(r));
            res = 1;
        }
    } else {
        tms_errorf("unable to initialize curl handle!");
        res = 1;
    }
    unlock_curl("check_version_code");

    if (quitting) {
        return 0;
    }

    P.add_action(ACTION_REFRESH_HEADER_DATA, 0);

    tms_debugf("exiting version check thread");
    return 0;
}

static int
_get_featured_levels(void *_num)
{
    uint32_t num_featured_levels = VOID_TO_UINT32(_num);
    CURLcode r;

    struct MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);
    chunk.size = 0;

    featured_levels_buf = 0;
    featured_levels_buf_size = 0;

    {
        FILE *fh = fopen(featured_data_time_path, "r");
        if (fh) {
            fseek(fh, 0, SEEK_END);
            long size = ftell(fh);
            fseek(fh, 0, SEEK_SET);

            char *buf = (char*)malloc(size+1);

            fread(buf, 1, size, fh);

            buf[size] = '\0';

            fclose(fh);

            fl_fetch_time = atoi(buf);

            free(buf);
        }
    }

    lock_curl("get_featured_levels");
    if (P.curl) {
        init_curl_defaults(P.curl);

        char url[256];
        if (fl_fetch_time && file_exists(featured_data_path)) {
            snprintf(url, 255, "https://%s/internal/get_featured?num=%" PRIu32 "&time=%d", P.community_host, num_featured_levels, fl_fetch_time);
        } else {
            snprintf(url, 255, "https://%s/internal/get_featured?num=%" PRIu32, P.community_host, num_featured_levels);
        }

        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        curl_easy_setopt(P.curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
        curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, (void*)&chunk);

        curl_easy_setopt(P.curl, CURLOPT_HEADERFUNCTION, _parse_headers_fl);
        curl_easy_setopt(P.curl, CURLOPT_WRITEHEADER, 0);

        curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 35L);
        curl_easy_setopt(P.curl, CURLOPT_FOLLOWLOCATION, 1);

        if ((r = curl_easy_perform(P.curl)) == CURLE_OK) {
            long http_code = 0;
            curl_easy_getinfo(P.curl, CURLINFO_RESPONSE_CODE, &http_code);
            /* Anything other than 200 will make us attempt to read from our cache */

            if (http_code == 200) {
                featured_levels_buf = chunk.memory;
                featured_levels_buf_size = chunk.size;
            }
        } else {
            tms_errorf("curl error: %s", curl_easy_strerror(r));
        }
    } else {
        tms_errorf("unable to initialize curl handle!");
    }

    unlock_curl("get_featured_levels");

    if (quitting) {
        return 0;
    }

    if (!featured_levels_buf) {
        FILE *fh = fopen(featured_data_path, "rb");
        if (fh) {
            fseek(fh, 0, SEEK_END);
            featured_levels_buf_size = ftell(fh);
            fseek(fh, 0, SEEK_SET);

            featured_levels_buf = (char*)malloc(featured_levels_buf_size);
            fread(featured_levels_buf, 1, featured_levels_buf_size, fh);

            fclose(fh);
        } else {
            tms_infof("Error opening cache file!");
            return 0;
        }
    }

    /* Second pass! */
    if (!featured_levels_buf) {
        return 0;
    }

    lvlbuf lb;

    lb.reset();
    lb.size = 0;
    lb.ensure(featured_levels_buf_size);

    lb.buf = (uint8_t*)featured_levels_buf;
    lb.size = featured_levels_buf_size;

    tms_debugf("featured data size: %d", (int)featured_levels_buf_size);

    uint32_t count = lb.r_uint32();

    uint32_t n = std::min(count, num_featured_levels);
    for (uint32_t i=0; i<n; ++i) {
        menu_shared::fl[i].id = lb.r_uint32();

        uint32_t name_len = lb.r_uint32();
        char *name = (char*)calloc(name_len+1, 1);
        lb.r_buf(name, name_len);
        strcpy(menu_shared::fl[i].name, name);
        free(name);

        uint32_t creator_len = lb.r_uint32();
        char *creator = (char*)calloc(creator_len+1, 1);
        lb.r_buf(creator, creator_len);
        strcpy(menu_shared::fl[i].creator, creator);
        free(creator);

        uint32_t thumb_len = lb.r_uint32();

        if (thumb_len == 0) {
            tms_errorf("Featured level received with no thumbnail!");
            continue;
        }
        char *thumb = (char*)malloc(thumb_len);
        lb.r_buf(thumb, thumb_len);

        tms::texture *tex = new tms::texture();
        tex->load_mem2(thumb, thumb_len, 0);
        tex->flip_y();
        tex->add_alpha(1.f);

        menu_shared::fl[i].sprite = tms_atlas_add_bitmap(
                gui_spritesheet::atlas,
                tex->width,
                tex->height,
                tex->num_channels,
                tex->data
                );

        free(thumb);
        tex->free_buffer();

        delete tex;
    }

    tms_infof("loading contest stuff..");
    uint32_t contest_active = lb.r_uint32();

    if (contest_active == 1) {
        menu_shared::contest.id = lb.r_uint32();

        uint32_t contest_name_len = lb.r_uint32();
        char *contest_name = (char*)calloc(contest_name_len+1, 1);
        lb.r_buf(contest_name, contest_name_len);

        uint32_t contest_thumb_len = lb.r_uint32();
        if (contest_thumb_len > 0) {
            char *contest_thumb = (char*)calloc(contest_thumb_len, 1);
            lb.r_buf(contest_thumb, contest_thumb_len);

            tms::texture *tex = new tms::texture();
            tex->load_mem2(contest_thumb, contest_thumb_len, 0);
            tex->flip_y();
            tex->add_alpha(1.f);

            menu_shared::contest.sprite = tms_atlas_add_bitmap(
                    gui_spritesheet::atlas,
                    tex->width,
                    tex->height,
                    tex->num_channels,
                    tex->data
                    );

            strcpy(menu_shared::contest.name, contest_name);

            free(contest_name);
            free(contest_thumb);

            tex->free_buffer();

            delete tex;

            uint32_t num_entries = lb.r_uint32();

            n = std::min(num_entries, (uint32_t)MAX_FEATURED_LEVELS_FETCHED);
            for (uint32_t i=0; i<n; ++i) {
                menu_shared::contest_entries[i].id = lb.r_uint32();

                uint32_t thumb_len = lb.r_uint32();

                if (thumb_len == 0) {
                    tms_errorf("We received a contest entry which did not have a thumbnail!");
                    continue;
                }

                char *thumb = (char*)calloc(thumb_len, 1);
                lb.r_buf(thumb, thumb_len);

                tex = new tms::texture();
                tex->load_mem2(thumb, thumb_len, 0);
                tex->flip_y();
                tex->add_alpha(1.f);

                menu_shared::contest_entries[i].sprite = tms_atlas_add_bitmap(
                        gui_spritesheet::atlas,
                        tex->width,
                        tex->height,
                        tex->num_channels,
                        tex->data
                        );

                free(thumb);
                tex->free_buffer();

                delete tex;
            }

            menu_shared::contest_state = FL_WAITING;
        } else {
            tms_errorf("Contest has no thumbnail :(");
        }
    }

    uint32_t num_getting_started_links = lb.r_uint32();

    tms_infof("Num getting started links: %" PRIu32, num_getting_started_links);

    menu_shared::gs_entries.clear();

    for (uint32_t x=0; x<num_getting_started_links; ++x) {
        uint32_t title_len, link_len;
        char    *title,    *link;

        title_len = lb.r_uint32();
        title = (char*)calloc(title_len+1, 1);
        lb.r_buf(title, title_len);

        link_len = lb.r_uint32();
        link = (char*)calloc(link_len+1, 1);
        lb.r_buf(link, link_len);

        menu_shared::gs_entries.push_back(gs_entry(strdup(title), strdup(link)));

        free(title);
        free(link);
    }

    if (num_getting_started_links) {
        menu_shared::gs_state = FL_WAITING;
    }

    {
        FILE *fh = fopen(featured_data_path, "wb");

        if (fh) {
            fwrite(featured_levels_buf, 1, featured_levels_buf_size, fh);

            fclose(fh);
        }
    }

    menu_shared::fl_state = FL_UPLOAD;

    return 0;
}

#ifdef BUILD_PKGMGR
static int
_publish_pkg(void *_unused)
{
    uint32_t pkg_id = _publish_pkg_id;

    pkginfo p;
    if (p.open(LEVEL_LOCAL, pkg_id)) {

        if (p.num_levels) {
            uint32_t *community_ids = (uint32_t*)malloc(p.num_levels*sizeof(uint32_t));
            bool error = false;

            for (int x=0; x<p.num_levels; x++) {
                _publish_lvl_id = p.levels[x];
                _publish_lvl_with_pkg = true;
                _publish_lvl_pkg_index = x;
                _publish_lvl_set_locked = x >= p.unlock_count;

                _publish_level(0);

                if (_publish_lvl_uploading_error || _publish_lvl_community_id == 0) {
                    error = true;
                    break;
                }

                community_ids[x] = _publish_lvl_community_id;
            }

            if (!error) {
                /* ok, all levels are uploaded. now upload the package itself. */
                CURLcode r;

                char level_list[4096];
                char tmp[20];
                level_list[0] = '\0';
                for (int x=0; x<p.num_levels; x++) {
                    sprintf(tmp, "%u,", community_ids[x]);
                    strcat(level_list, tmp);
                }
                level_list[strlen(level_list)-1] ='\0';

                struct MemoryStruct chunk;
                chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
                chunk.size = 0;    /* no data at this point */

                lock_curl("publish_pkg");
                if (P.curl) {
                    init_curl_defaults(P.curl);

                    curl_mime *mime = curl_mime_init(P.curl);
                    curl_mimepart *part = NULL;

                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "xFxlax");
                    curl_mime_data(part, "zPaod", CURL_ZERO_TERMINATED);

                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "name");
                    curl_mime_data(part, p.name, CURL_ZERO_TERMINATED);

                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "levels");
                    curl_mime_data(part, level_list, CURL_ZERO_TERMINATED);

                    sprintf(tmp, "%d", p.unlock_count);
                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "unlock_count");
                    curl_mime_data(part, tmp, CURL_ZERO_TERMINATED);

                    sprintf(tmp, "%u", (uint32_t)p.first_is_menu);
                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "first_is_menu");
                    curl_mime_data(part, tmp, CURL_ZERO_TERMINATED);

                    sprintf(tmp, "%u", (uint32_t)p.return_on_finish);
                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "return_on_finish");
                    curl_mime_data(part, tmp, CURL_ZERO_TERMINATED);

                    sprintf(tmp, "%u", (uint32_t)p.version);
                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "version");
                    curl_mime_data(part, tmp, CURL_ZERO_TERMINATED);

                    sprintf(tmp, "%u", (uint32_t)p.community_id);
                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "id");
                    curl_mime_data(part, tmp, CURL_ZERO_TERMINATED);


                    char url[256];
                    snprintf(url, 255, "https://%s/internal/upload_package", P.community_host);
                    curl_easy_setopt(P.curl, CURLOPT_URL, url);

                    curl_easy_setopt(P.curl, CURLOPT_MIMEPOST, mime);

                    curl_easy_setopt(P.curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
                    curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, (void*)&chunk);
                    curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 15L);

                    tms_debugf("Publishing package..");
                    if ((r = curl_easy_perform(P.curl)) != CURLE_OK) {
                        tms_errorf("pkg publish curl_easy_perform failed: %s\n", curl_easy_strerror(r));
                        unlock_curl("publish_pkg");
                        _publish_pkg_done = true;
                        _publish_pkg_error = true;
                        return 1;
                    }

                    if (chunk.size != 0) {
                        char *pch;

                        pch = strchr(chunk.memory, '-');

                        if ((pch-chunk.memory) == 0) {
                            int notify_id = atoi(chunk.memory);
                            _publish_pkg_error = true;

                            switch (notify_id) {
                                default:
                                    ui::message("Unknown error message");
                                    break;
                            }
                        } else {
                            p.community_id = atoi(chunk.memory);
                            tms_debugf("saved pkg community id: %u", p.community_id);
                            p.save();
                        }
                    } else {
                        /* we did not recieve any data back, an unknown error occured */
                        tms_errorf("no data received");
                        _publish_pkg_error = true;
                    }

                    curl_mime_free(mime);
                } else {
                    tms_errorf("lock_curl failed :3");
                    _publish_pkg_error = true;
                }
                unlock_curl("publish_pkg");
            } else {
                //ui::message("An error occurred while uploading levels in package.");
                tms_errorf("An error occured while uploading the levels contained in the package.");
                _publish_pkg_error = true;
            }

            free(community_ids);
        } else {
            ui::message("Can not upload an empty package.");
        }
    } else {
        _publish_pkg_error = true;
    }

    _publish_pkg_done = true;

    return T_OK;
}
#endif

static int
_publish_level(void *p)
{
    uint32_t level_id = _publish_lvl_id;
    int community_id    = 0;
    int error           = 0;

    _publish_lvl_community_id = 0;
    _publish_lvl_uploading_error = false;

    /* TODO: Check if this simplified version works on linux as well */
    CURLcode r;

    struct MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */

    lvledit lvl;

    if (!lvl.open(LEVEL_LOCAL, level_id)) {
        tms_errorf("could not open level");
        return false;
    }

    /* if we publish this as part of a package, update the locked
     * value according to the packages settings */

    /* TODO: use hidden for non-locked levels */
    if (_publish_lvl_with_pkg) {
        lvl.lvl.visibility = (_publish_lvl_set_locked ? LEVEL_LOCKED : LEVEL_VISIBLE);
    }

    tms_debugf("old revision: %d", lvl.lvl.revision);
    lvl.lvl.revision++;
    tms_debugf("new revision: %d", lvl.lvl.revision);
    lvl.save();

    char level_path[1024];
    pkgman::get_level_full_path(LEVEL_LOCAL, level_id, 0, level_path);

    lock_curl("publish_level");
    if (P.curl) {
        struct header_data hd = {0};
        init_curl_defaults(P.curl);

        curl_mime *mime = curl_mime_init(P.curl);
        curl_mimepart *part;

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "level");
        curl_mime_filedata(part, level_path);

        CURL_CUDDLES;

        char url[256];
        snprintf(url, 255, "https://%s/internal/upload", P.community_host);
        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        curl_easy_setopt(P.curl, CURLOPT_WRITEHEADER, &hd);
        curl_easy_setopt(P.curl, CURLOPT_MIMEPOST, mime);
        curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 15L);

        tms_infof("Publishing level %d...", level_id);
        r = curl_easy_perform(P.curl);
        if (r == CURLE_OK) {
            // Check for messages
            if (hd.error_message) {
                ui::message(hd.error_message);

                _publish_lvl_uploading_error = true;

                free(hd.error_message);
            } else if (hd.notify_message) {
                tms_infof("got data: %s", hd.notify_message);
                community_id = atoi(hd.notify_message);

                W->level.revision = lvl.lvl.revision;
                lvl.lvl.community_id = community_id;
                tms_infof("community id: %d", community_id);
                tms_infof("parent id:    %u", lvl.lvl.parent_id);
                tms_infof("revision:     %u", lvl.lvl.revision);

                free(hd.notify_message);

            } else {
                /* we did not recieve any data back, an unknown error occured */
                tms_errorf("no data received");
                _publish_lvl_uploading_error = true;
            }
        } else {
            tms_errorf("lvl publish curl_easy_perform failed: %s\n", curl_easy_strerror(r));

            switch (r) {
                case CURLE_OPERATION_TIMEDOUT:
                    ui::message("Operation timed out. Your internet connection seems to be unstable! Please try again.", true);
                    break;

                case CURLE_COULDNT_RESOLVE_HOST:
                    ui::message("Error: Unable to resolve hostname. Please check your internet connection.", true);
                    break;

                default:
                    ui::message("An unknown error occured when publishing your level. Check your internet connection.", true);
                    break;
            }
            _publish_lvl_uploading = false;
            _publish_lvl_uploading_error = true;
        }

        curl_mime_free(mime);
    }
    unlock_curl("publish_level");

    P.add_action(ACTION_AUTOSAVE, 0);

    if (!lvl.save()) {
        tms_errorf("Unable to save the level after publish!");
    }

    _publish_lvl_community_id = community_id;
    _publish_lvl_uploading = false;

    return T_OK;
}

static int
_submit_score(void *p)
{
    tms_assertf(W->is_playing(), "submit score called when the level was paused");

    int error = 0;

    CURLcode r;

    char data_path[1024];

    const char *storage = tbackend_get_storage_path();
    snprintf(data_path, 1023, "%s/data.bin", storage);

    uint32_t highscore_level_id = BASE_HIGHSCORE_LEVEL_ID;

    int highscore_level_offset = highscore_offset(W->level.community_id);

    lvledit lvl;

    if (!lvl.open(LEVEL_DB, W->level.local_id)) {
        tms_errorf("could not open level");
        return false;
    }

    uint32_t timestamp = (uint32_t)time(NULL);
    const lvl_progress *base_lp = progress::get_level_progress(W->level_id_type, W->level.local_id);
    uint32_t last_score = base_lp->last_score;

    uint32_t crc32_data[5];
    uint32_t more_data[5];
    more_data[HS_VER_DATA_TIMESTAMP] = timestamp;
    more_data[HS_VER_DATA_REVISION] = lvl.lvl.revision;
    more_data[HS_VER_DATA_TYPE] = W->level_id_type;
    more_data[HS_VER_DATA_PRINCIPIA_VERSION] = PRINCIPIA_VERSION_CODE;
    more_data[HS_VER_DATA_VERSION] = W->level.version;

    for (int x=0; x<5; ++x) {
        tms_infof("fetching progress for level with id %d", BASE_HIGHSCORE_LEVEL_ID+x);
        lvl_progress *lp = progress::get_level_progress(LEVEL_MAIN, BASE_HIGHSCORE_LEVEL_ID+x);
        crc32_data[x] = crc32_level(lvl.lvl, lvl.lb, timestamp, last_score, x);
        tms_debugf("%d got crc: %08X", x, crc32_data[x]);

        lp->last_score = more_data[(x+highscore_level_offset)%5];

        tms_infof("Last score: %" PRIu32, lp->last_score);
    }

    for (int x=0; x<5; ++x) {
        lvl_progress *lp = progress::get_level_progress(LEVEL_MAIN, BASE_HIGHSCORE_LEVEL_ID+x);
        lp->top_score = crc32_data[(x+highscore_level_offset)%5];
        tms_infof("Top score: %08X", lp->top_score);
    }

    progress::commit();

    /* TODO: add cool stuff, like crc32 stuff here */

    /* and when we're done with sbuuuuutmi score, we remove that secret stuff again */

    lock_curl("submit_score");
    if (P.curl) {
        struct header_data hd = {0};
        init_curl_defaults(P.curl);

        curl_mime* mime = curl_mime_init(P.curl);
        curl_mimepart* part;

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "data.bin");
        curl_mime_filedata(part, data_path);

        char tmp[32];
        sprintf(tmp, "%" PRIu32, W->level.community_id);

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "lvl_id");
        curl_mime_data(part, tmp, CURL_ZERO_TERMINATED);

        CURL_CUDDLES;

        char url[256];
        snprintf(url, 255, "https://%s/internal/submit_score", P.community_host);
        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        curl_easy_setopt(P.curl, CURLOPT_WRITEHEADER, &hd);

        curl_easy_setopt(P.curl, CURLOPT_MIMEPOST, mime);

        curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 15L);

        r = curl_easy_perform(P.curl);
        if (r == CURLE_OK) {
            // Check for messages
            bool displayed_message = false;
            if (hd.error_message) {
                if (!displayed_message) {
                    ui::message(hd.error_message);
                    displayed_message = true;
                }

                free(hd.error_message);
            }
            if (hd.notify_message) {
                if (!displayed_message) {
                    ui::message(hd.notify_message);
                    displayed_message = true;
                }

                free(hd.notify_message);

                G->state.submitted_score = true;
            }

            switch (hd.error_action) {
                case ERROR_ACTION_LOG_IN:
                    ui::next_action = ACTION_SUBMIT_SCORE;
                    ui::open_dialog(DIALOG_LOGIN);
                    break;
            }
        } else {
            switch (r) {
                case CURLE_OPERATION_TIMEDOUT:
                    ui::message("Unable to submit your score.\nYour internet connection seems to be unstable! Please try again.", true);
                    break;

                case CURLE_COULDNT_RESOLVE_HOST:
                    ui::message("Unable to submit your score.\nError: Unable to resolve hostname. Please check your internet connection.", true);
                    break;

                default:
                    ui::message("An unknown error occured when submitting your score. Check your internet connection and try again.", true);
                    break;
            }
        }

        curl_mime_free(mime);
    }

    _submit_score_done = true;

    unlock_curl("submit_score");

    return T_OK;
}

/** --Login **/
int
_login(void *p)
{
    struct login_data *data = static_cast<struct login_data*>(p);

    int res = T_OK;

    CURLcode r;

    lock_curl("login");
    if (P.curl) {
        struct header_data hd = {0};
        init_curl_defaults(P.curl);

        curl_mime *mime = curl_mime_init(P.curl);
        curl_mimepart* part;

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "username");
        curl_mime_data(part, data->username, CURL_ZERO_TERMINATED);

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "password");
        curl_mime_data(part, data->password, CURL_ZERO_TERMINATED);

        CURL_CUDDLES;

        char url[256];
        snprintf(url, 255, "https://%s/internal/login", P.community_host);
        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        curl_easy_setopt(P.curl, CURLOPT_WRITEHEADER, &hd);
        curl_easy_setopt(P.curl, CURLOPT_MIMEPOST, mime);
        curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 15L);

        r = curl_easy_perform(P.curl);
        if (r == CURLE_OK) {
            // Check for messages
            if (hd.error_message) {
                ui::message(hd.error_message);
                ui::emit_signal(SIGNAL_LOGIN_FAILED);

                free(hd.error_message);
            }

            if (hd.notify_message) {
                ui::message(hd.notify_message);

                P.username = strdup(data->username);
                P.add_action(ACTION_REFRESH_HEADER_DATA, 0);

                ui::emit_signal(SIGNAL_LOGIN_SUCCESS);

                free(hd.notify_message);
            }
        } else {
            tms_errorf("curl_easy_perform failed: %s\n", curl_easy_strerror(r));
            res = 1;
        }
        curl_mime_free(mime);
    } else {
        tms_errorf("Unable to initialize curl handle.");
        res = 1;
    }
    unlock_curl("login");

    free(data);

    return res;
}

/** --Register **/
int
_register(void *p)
{
    struct register_data *data = static_cast<struct register_data*>(p);
    int res = T_OK;
    int num_tries = 0;

    CURLcode r;

    lock_curl("register");

    if (P.curl) {
        struct header_data hd = {0};
        init_curl_defaults(P.curl);

        curl_mime *mime = curl_mime_init(P.curl);
        curl_mimepart *part;

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "username");
        curl_mime_data(part, data->username, CURL_ZERO_TERMINATED);

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "email");
        curl_mime_data(part, data->email, CURL_ZERO_TERMINATED);

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "password");
        curl_mime_data(part, data->password, CURL_ZERO_TERMINATED);

        CURL_CUDDLES;

        char url[256];
        snprintf(url, 255, "https://%s/internal/register", P.community_host);
        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        curl_easy_setopt(P.curl, CURLOPT_WRITEHEADER, &hd);
        curl_easy_setopt(P.curl, CURLOPT_MIMEPOST, mime);
        curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 15L);

        r = curl_easy_perform(P.curl);

        if (r == CURLE_OK) {
            // Check for messages
            if (hd.error_message) {
                ui::message(hd.error_message);
                ui::emit_signal(SIGNAL_REGISTER_FAILED);

                free(hd.error_message);
            }

            if (hd.notify_message) {
                ui::message(hd.notify_message);
                ui::emit_signal(SIGNAL_REGISTER_SUCCESS);

                free(hd.notify_message);
            }
        } else {
            if (r != CURLE_OK) {
                tms_errorf("curl_easy_perform failed: %s", curl_easy_strerror(r));
            } else {
                tms_errorf("No data received.");
            }
            res = T_ERR;
        }

        curl_mime_free(mime);
    } else {
        tms_errorf("Unable to initialize curl handle.");
        res = T_ERR;
    }

    unlock_curl("register");

    if (res != T_OK) {
        ui::emit_signal(SIGNAL_REGISTER_FAILED);
    }

    free(data);

    return res;
}

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
static Uint32 total_load = 0;

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
generate_paths()
{
    snprintf(featured_data_path, 1023, "%s/fl.cache", tbackend_get_storage_path());
    snprintf(featured_data_time_path, 1023, "%s/fl.time", tbackend_get_storage_path());
}

static int
initial_loader(int step)
{
    static uint32_t last_time = SDL_GetTicks();

    char tmp[512];
    Uint32 ss = SDL_GetTicks();
    int retval = LOAD_CONT;

    switch (step) {
        case 0:
            {
                P.community_host = "principia-web.se";

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

                generate_paths();

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

#if defined(TMS_BACKEND_LINUX) && defined(DEBUG)
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

#ifdef BUILD_CURL
            lock_curl("initial_loader-curl_init");
            P.curl = curl_easy_init();
            unlock_curl("initial_loader-curl_init");
            if (!P.curl) return LOAD_ERROR;
#endif

            /* initialize worker threads */
            w_init();

            tmod_3ds_init();
            of::init();

            P.s_loading_screen->set_text("Loading materials...");
            break;

        case 5:
            material_factory::init(is_very_shitty || settings["is_very_shitty"]->v.b);

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
#ifdef TMS_BACKEND_LINUX_SS
                P.s_loading_screen->set_text("Ready! o.o");
#else
                P.s_loading_screen->set_text(0);
#endif

                P.loaded = true;
                settings["loaded_correctly"]->v.b = true;
                settings.save();

#ifndef TMS_BACKEND_LINUX_SS
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

                tms_infof("%27s: %" PRIu32, "Total", total);

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

#ifdef BUILD_CURL

/**
 * Get the community site login token from cURL, intended for the user to be automatically
 * logged into the Android webview.
*/
extern "C" void
P_get_cookie_data(char **token)
{
    *token = 0;

    if (quitting) {
        return;
    }

    lock_curl("get_cookie_data");
    if (P.curl) {
        init_curl_defaults(P.curl);

        char url[256];
        snprintf(url, 255, "https://%s/internal/login", P.community_host);
        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        struct curl_slist *cookies;
        CURLcode res = curl_easy_getinfo(P.curl, CURLINFO_COOKIELIST, &cookies);

        if (res == CURLE_OK) {
            P.add_action(ACTION_REFRESH_HEADER_DATA, 0);

            while (cookies) {
                int nt = 0;
                int found_token = 0;
                char *d = cookies->data;
                tms_debugf("cookie: %s", d);
                while (*d != '\0') {
                    if (nt == 5) {
                        if (strncmp(d, "_PRINCSECURITY", 14) == 0)
                            found_token = 1;
                    }
                    if (nt == 6) {
                        if (found_token) *token = d;
                        break;
                    }
                    if (*d == '\t') nt++;
                    d ++;
                }
                cookies = cookies->next;
            }
        }
    }
    unlock_curl("get_cookie_data");
}

#endif

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
