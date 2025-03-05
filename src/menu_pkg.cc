#include "menu_pkg.hh"
#include "gui.hh"
#include "game.hh"
#include "pkgman.hh"
#include "progress.hh"
#include "menu-play.hh"
#include "ui.hh"
#include "misc.hh"
#include "widget_manager.hh"

#include <unistd.h>

// Disables level completion check in packages
#define UNLOCK_ALL_LVLS false

//#define MAX_P 20
// defined in game.hh

#define _BASE_X (_tms.xppcm*1.f)
#define _ICON_WIDTH  (_tms.window_height/5.f)
#define _BASE_Y (-(_ICON_WIDTH / 2.f))
#define _ICON_SPACING (_tms.xppcm/2.f)
#define _ICON_OUTER  (_ICON_SPACING+_ICON_WIDTH)
#define _ICON_HEIGHT _ICON_WIDTH//(1.5f*_tms.yppcm)
#define _BLOCK_SPACING (_tms.xppcm)

static void
menu_pkg_render(struct tms_wdg *w, struct tms_surface *s)
{
    principia_wdg *pwdg = static_cast<principia_wdg*>(w->data2);
    float px, py;

    if (pwdg->render_pos.use) {
        px = pwdg->render_pos.x;
        py = pwdg->render_pos.y;
    } else {
        px = w->pos.x;
        py = w->pos.y;
    }

    float r = 0.f;

    if (_tms.emulating_portrait) {
        int xx = (int)px, yy = (int)py;
        tms_convert_to_portrait(&xx, &yy);
        px = (float)xx;
        py = (float)yy;

        r = -90.f;
    }

    tms_ddraw_sprite_r(s->ddraw, w->s[0], px, py, w->size.x, w->size.y, r);

    if (w->s[1]) {
        tms_ddraw_sprite_r(s->ddraw, w->s[1], px, py, w->size.x/2.f, w->size.y/2.f,r);
    }
}

static bool                down[MAX_P];
static tvec2               touch_pos[MAX_P];
static uint64_t            touch_time[MAX_P];
static bool                dragging[MAX_P];
static struct tms_texture *_tex_bg    = 0;
static struct tms_texture *tex_overlay = 0;
static struct tms_atlas   *tex_icons = 0;

static pkginfo main_pkg;

struct lvlcache {
    uint32_t           id;
    struct tms_sprite *sprite;
    uint8_t            icon[128*128];
    lvl_progress *progress;
    bool show_score;
};

static int unlock_count = 0;
static struct lvlcache *cache = 0;

bool
menu_pkg::widget_clicked(principia_wdg *w, uint8_t button_id, int pid)
{
    if (menu_base::widget_clicked(w, button_id, pid)) {
        return true;
    }

    switch (button_id) {
        case BTN_BACK:
            P.add_action(ACTION_GOTO_MAINMENU, 0x1);
            //P.add_action(ACTION_GOTO_PLAY, 0x1);
            break;

        default: return false;
    }

    return true;
}

menu_pkg::menu_pkg()
    : menu_base(false)
{
    this->scale = 1.f;

    base_x = _BASE_X * scale;
    base_y = _BASE_Y * scale;
    icon_width = _ICON_WIDTH * scale;
    icon_spacing = _ICON_SPACING * scale;
    icon_outer = _ICON_OUTER * scale;
    icon_height = _ICON_HEIGHT * scale;
    block_spacing = _BLOCK_SPACING * scale;

    main_pkg.num_levels = 2;
    main_pkg.levels = (uint32_t*)malloc(2*sizeof(uint32_t));
    main_pkg.levels[0] = 118;
    main_pkg.levels[1] = 119;
    this->pkg = main_pkg;

    this->dd = this->get_surface()->ddraw;
    this->cam = tms_camera_alloc();
    this->cam_screen = tms_camera_alloc();

    tex_icons = tms_atlas_alloc(1024,1024,3);

    this->cam->_position = (tvec3){0.f, -base_y - _tms.window_height/2.f, 0.f};
    this->cam->_direction = (tvec3){0.f, 0.f, -1.f};
    this->cam->owidth = this->cam->width = _tms.window_width;
    this->cam->oheight = this->cam->height = _tms.window_height;

    this->cam->_velocity.x = .5f;

    this->wm->remove_all();

    this->wdg_back = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            BTN_BACK, AREA_MENU_TOP_LEFT,
            gui_spritesheet::get_sprite(S_LEFT), 0,
            1.5f);

    this->wdg_back->render = menu_pkg_render;

    this->wdg_back->priority = 500;
    this->wdg_back->add();
}

bool
menu_pkg::set_pkg(int type, uint32_t id)
{
    tms_infof("set pkg");
    if (!(this->pkg.open(type,id))) {
        tms_errorf("could not open package!");
        return false;
    }

    if (this->pkg.num_levels <= 0)
        return false;

    lvlinfo info;
    lvlbuf  buf;

    buf.clear();
    buf.ensure(sizeof(lvlinfo));

    tms_atlas_reset(tex_icons);

    char filename[1024];

    if (cache) free(cache);
    cache = (struct lvlcache*)malloc(this->pkg.num_levels*sizeof(struct lvlcache));

    for (int x=0; x<this->pkg.num_levels; x++) {
        uint32_t lvl_id = this->pkg.levels[x];
        memset(info.icon, 0, sizeof(info.icon));

        snprintf(filename, 1023, "%s/%d.plvl", pkgman::get_level_path(this->pkg.type), lvl_id);

        FILE_IN_ASSET(this->pkg.type == LEVEL_MAIN);
        FILE *fp = _fopen(filename, "rb");

        if (fp) {
            buf.reset();
            _fread(buf.buf, 1, sizeof(lvlinfo), fp);
            buf.size = sizeof(lvlinfo);

            info.read(&buf, true);

            tms_infof("read level %.*s", info.name_len, info.name);

            memcpy(cache[x].icon, info.icon, 128*128);
            int nn = 0;
            int n_max = 0;
            for (int y=0; y<128*128; y++) {
                if (cache[x].icon[y] != 0) nn ++;
                if (cache[x].icon[y] > n_max) n_max = cache[x].icon[y];
            }
            tms_infof("num not zero: %d, max:%d", nn, n_max);
            cache[x].id = lvl_id;
            cache[x].sprite = tms_atlas_add_bitmap(tex_icons, 128, -128, 1, info.icon);
            cache[x].progress = progress::get_level_progress(type, lvl_id);
            cache[x].show_score = info.show_score;
        } else {
            tms_errorf("file in package was missing");
            return false;
        }

        _fclose(fp);
    }

    tms_texture_upload(&tex_icons->texture);
    tms_infof("texture id of icons tex %u", tex_icons->texture.gl_texture);

    tms_infof("successfully set pkg");

    return true;
}

int
menu_pkg::resume(void)
{
    tms_infof("Resume menu_pkg");
    if (_tex_bg) tms_texture_free(_tex_bg);
    if (tex_overlay) tms_texture_free(tex_overlay);

    _tex_bg = tms_texture_alloc();

    tms_texture_load(_tex_bg, "data/textures/pkgmenubg.png");
    _tex_bg->format = GL_RGBA;
    tms_texture_set_filtering(_tex_bg, GL_LINEAR);
    tms_texture_upload(_tex_bg);

    tex_overlay = tms_texture_alloc();
    tms_texture_load(tex_overlay, "data/textures/ui/icon_overlay.png");
    tex_overlay->format = GL_RGBA;
    tms_texture_set_filtering(tex_overlay, GL_LINEAR);
    tms_texture_upload(tex_overlay);

    for (int x=0; x<MAX_P; x++) {
        down[x] = false;
        dragging[x] = false;
    }

    if (this->pkg.first_is_menu) {
        tms_infof("playing lvl!");
        /* TODO: the level selector should revent to main menu,
         * and if you're in a level you should return to the level selector. */
        G->screen_back = this;
        G->open_play(this->pkg.type, this->pkg.levels[0], &this->pkg);
        G->resume_action = GAME_RESUME_OPEN;
        G->state.waiting = false;
        tms::set_screen(G);
    }

    /* calculate unlock count */
    unlock_count = 0;
    for (int x=0; x<this->pkg.num_levels; x++) {
        if (cache[x].progress->completed) {
            unlock_count ++;
        }
    }

    unlock_count += this->pkg.unlock_count;

    tms_infof("pkg resume finished");
    return T_OK;
}

int
menu_pkg::pause(void)
{
    tms_infof("PAUSE");
    if (_tex_bg) {
        tms_texture_free(_tex_bg);
        _tex_bg = 0;
    }
    if (tex_overlay) {
        tms_texture_free(tex_overlay);
        tex_overlay = 0;
    }

    return T_OK;
}

int
menu_pkg::step(double dt)
{
    float damping = powf(.025f, dt);
    this->cam->_velocity.x *= damping;
    this->cam->_position.x += this->cam->_velocity.x * dt;
    //this->cam->_position.x += .002f;
    this->cam->near = -1.f;
    this->cam->far = 1.f;

    float max_x = this->pkg.num_levels/9 * _tms.xppcm*4.f*1.2f + 2000;
    float min_x = _tms.window_width/2.f - 200.f;

    this->cam->_position.x = tclampf(this->cam->_position.x, min_x, max_x);

    tms_camera_calculate(this->cam);

    return T_OK;
}

int
menu_pkg::render()
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(0);

    tms_texture_render(_tex_bg);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_icons->texture.gl_texture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    tms_ddraw_set_color(this->dd, 1.f, 1.f, 1.f, 1.f);

    float tmp[16];
    tmat4_set_ortho(tmp, 0, _tms.window_width, 0, _tms.window_height, -1, 1);
    tms_ddraw_set_matrices(this->dd, 0, tmp);

    tms_ddraw_set_matrices(this->dd, this->cam->view, this->cam->projection);

    for (int x=0; x<this->pkg.num_levels; x++) {
        int block = x / 9;
        float sx = base_x + (x%3)*icon_outer + (float)block * (block_spacing + 3.f*icon_outer);
        float sy = base_y - (x%9)/3*icon_outer;

        if (UNLOCK_ALL_LVLS || this->pkg.unlock_count == 0 || x < unlock_count || cache[x].progress->completed)
            tms_ddraw_sprite(this->dd, cache[x].sprite, sx, sy, icon_width*scale, icon_height*scale);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_overlay->gl_texture);

    for (int x=0; x<this->pkg.num_levels; x++) {
        int block = x / 9;
        float sx = base_x + (x%3)*icon_outer + (float)block * (block_spacing + 3.f*icon_outer);
        float sy = base_y - (x%9)/3*icon_outer;

        struct tms_sprite overlay;
        overlay.bl = (tvec2){0.f, 0.f};
        overlay.tr = (tvec2){1.f, 1.f};

        if (UNLOCK_ALL_LVLS || this->pkg.unlock_count == 0 || x < unlock_count || cache[x].progress->completed)
            tms_ddraw_set_color(this->dd, 0.75f, 1.f, .75f, 1.f);
        else
            tms_ddraw_set_color(this->dd, 1.f, .75f, .75f, 1.f);

        tms_ddraw_sprite(this->dd, &overlay, sx, sy, (icon_width+7.f)*scale, (icon_height+7.f)*scale);
    }

    tms_ddraw_set_color(this->dd, 1.f, 1.f, 1.f, 1.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gui_spritesheet::atlas->texture.gl_texture);

    for (int x=0; x<this->pkg.num_levels; x++) {
        int block = x / 9;
        float sx = base_x + (x%3)*icon_outer + (float)block * (block_spacing + 3.f*icon_outer);
        float sy = base_y - (x%9)/3*icon_outer;
        char ss[32];

        if (cache[x].progress->completed) {
            tms_ddraw_sprite(this->dd, gui_spritesheet::get_sprite(S_CHECKMARK),
                    sx + icon_outer/2.f - 48.f*scale,
                    sy - icon_outer/2.f + 64.f*scale,
                    gui_spritesheet::get_sprite(S_CHECKMARK)->width*scale, gui_spritesheet::get_sprite(S_CHECKMARK)->height*scale);
        }

        if (UNLOCK_ALL_LVLS || this->pkg.unlock_count == 0 || x < unlock_count || cache[x].progress->completed) {
            if (cache[x].show_score) {
                sprintf(ss, "%d", cache[x].progress->top_score);

                float x = sx - icon_width/2.f;
                float y = sy - icon_height/2.f + _tms.yppcm*.125f;

                this->add_text(ss, font::xmedium, x, y, TV_WHITE);
            }
        }

        /* render the order number of this level */
        sprintf(ss, "%d", x+1);

        float _x = sx - icon_width/2.f;
        float _y = sy + icon_height/2.f - _tms.yppcm * .25f*scale;

        this->add_text(ss, font::large, _x, _y, TV_WHITE);
    }

    glDepthMask(0xff);

    this->wdg_back->render_pos.x = -_tms.window_width/2.f + this->cam->_position.x + this->wdg_back->size.x/2.f + this->wm->get_margin_x();
    this->wdg_back->render_pos.y = 0 + this->cam->_position.y + _tms.window_height/2.f - this->wdg_back->size.y/2.f - this->wm->get_margin_y();
    this->wdg_back->render_pos.use = true;

    pscreen::render();

    return T_OK;
}

int
menu_pkg::handle_input(tms::event *ev, int action)
{
    if (pscreen::handle_input(ev, action) == EVENT_DONE) {
        return EVENT_DONE;
    }

    if (ev->type == TMS_EV_KEY_PRESS) {
        switch (ev->data.key.keycode) {
#ifdef TMS_BACKEND_MOBILE
            case SDL_SCANCODE_AC_BACK:
#endif
            case TMS_KEY_B:
            case TMS_KEY_ESC:
                tms::set_screen(P.s_menu_play);
                return T_OK;
        }
    } else if (ev->type == TMS_EV_POINTER_DOWN) {
        int pid = ev->data.motion.pointer_id;
        touch_pos[pid] = (tvec2){ev->data.motion.x, ev->data.motion.y};
        touch_time[pid] = _tms.last_time;
        dragging[pid] = false;
        down[pid] = true;
    } else if (ev->type == TMS_EV_POINTER_DRAG) {
        int pid = ev->data.motion.pointer_id;
        if (!down[pid]) return T_OK;
        tvec2 tdown = (tvec2){ev->data.motion.x, ev->data.motion.y};
        tvec2 td = (tvec2){tdown.x-touch_pos[pid].x, tdown.y-touch_pos[pid].y};

        float td_mag = tvec2_magnitude(&td);

        if (!dragging[pid] && td_mag > DRAG_DIST_MIN_EPS
                && (_tms.last_time - touch_time[pid] > DRAG_TIME_EPS
                    || td_mag > DRAG_DIST_EPS)) {
            dragging[pid] = true;
            touch_time[pid] = _tms.last_time;
        }

        if (dragging[pid]) {
            this->cam->_velocity.x -= td.x * 10.f;

            touch_pos[pid] = tdown;
            touch_time[pid] = _tms.last_time;
        }
    } else if (ev->type == TMS_EV_POINTER_UP) {
        int pid = ev->data.motion.pointer_id;
        if (!down[pid]) return T_OK;
        down[pid] = false;
        if (!dragging[pid]) {
            tms_infof("not dragging");
            tvec2 tdown = (tvec2){ev->data.motion.x, ev->data.motion.y};

            tdown.x += this->cam->_position.x - _tms.window_width /2.f;
            tdown.y += this->cam->_position.y - ( _tms.window_height/2.f);
            tdown.y = -tdown.y;

            int block = floor(tdown.x / (block_spacing + 3.f * icon_outer));

            float block_pos = (block_spacing + 3.f * icon_outer) * block;

            int btn_x = (int)(floor((tdown.x - block_pos) / icon_outer));
            int btn_y = (int)(floor(tdown.y / icon_outer));
            int btn = block * 9 + btn_x + btn_y * 3;

            tms_infof("clicked block %d, btn_x %d btn_y %d", block, btn_x, btn_y);

            if (btn_x >= 0 && btn_x < 3 && btn_y >= 0 && btn_y < 3 && btn >= 0 && btn < this->pkg.num_levels) {
                tms_infof("clicked %d", btn);

                if (UNLOCK_ALL_LVLS || this->pkg.unlock_count == 0 || btn < unlock_count || cache[btn].progress->completed) {
                    uint32_t level_id = this->pkg.levels[btn];

                    bool test_playing = false;
                    G->screen_back = this;
                    if (this->pkg.type == LEVEL_MAIN && this->pkg.id == 7) {
                        // XXX: causes segfaults on android
#ifndef TMS_BACKEND_ANDROID
                        char filename[1024];
                        snprintf(filename, 1023, "%s/7.%d.psol", pkgman::get_level_path(LEVEL_LOCAL), level_id);

                        if (file_exists(filename)) {
                            open_play_data *opd = new open_play_data(LEVEL_LOCAL, level_id, &pkg, false, 1);
                            ui::confirm("Do you want to load your last saved solution?",
                                    "Yes",    principia_action(ACTION_OPEN_MAIN_PUZZLE_SOLUTION, opd),
                                    "No",     principia_action(ACTION_CREATE_MAIN_PUZZLE_SOLUTION, opd),
                                    "Cancel", principia_action(ACTION_IGNORE, 0));
                        } else
#endif
                        {
                            P.add_action(ACTION_CREATE_MAIN_PUZZLE_SOLUTION, new open_play_data(LEVEL_LOCAL, level_id, &pkg, false, 1));
                        }

                        return T_OK;
                    }

                    G->open_play(this->pkg.type, level_id, &this->pkg, test_playing, 0);
                    G->resume_action = GAME_RESUME_OPEN;
                    tms::set_screen(G);
                } else {
                    ui::message("This level is locked. Please complete more levels to access this level.");
                }
            }
        }
    }

    return T_OK;
}

void
menu_pkg::window_size_changed()
{
    if (this->get_surface() && this->get_surface()->ddraw) {
        float projection[16];
        tmat4_set_ortho(projection, 0, _tms.window_width, 0, _tms.window_height, 1, -1);
        tms_ddraw_set_matrices(this->get_surface()->ddraw, 0, projection);
    }
}

void
menu_pkg::refresh_widgets()
{
    this->wm->rearrange();
}
