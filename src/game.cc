#include "absorber.hh"
#include "adventure.hh"
#include "basepixel.hh"
#include "cable.hh"
#include "chunk.hh"
#include "connection.hh"
#include "cursorfield.hh"
#include "decorations.hh"
#include "display.hh"
#include "dragfield.hh"
#include "emitter.hh"
#include "escript.hh"
#include "fan.hh"
#include "fluid.hh"
#include "fluidbuffer.hh"
#include "font.hh"
#include "fxemitter.hh"
#include "game-message.hh"
#include "game.hh"
#include "gravityman.hh"
#include "grid.hh"
#include "group.hh"
#include "i0o1gate.hh"
#include "i1o1gate.hh"
#include "i2o1gate.hh"
#include "item.hh"
#include "key_listener.hh"
#include "ladder.hh"
#include "ledbuffer.hh"
#include "linebuffer.hh"
#include "loading_screen.hh"
#include "main.hh"
#include "material.hh"
#include "menu_main.hh"
#include "menu_pkg.hh"
#include "motor.hh"
#include "object_factory.hh"
#include "object_finder.hh"
#include "objectfield.hh"
#include "panel.hh"
#include "pixel.hh"
#include "plant.hh"
#include "polygon.hh"
#include "progress.hh"
#include "proximitysensor.hh"
#include "ragdoll.hh"
#include "rand.h"
#include "rc_activator.hh"
#include "robot.hh"
#include "robot_parts.hh"
#include "robotman.hh"
#include "rope.hh"
#include "screenshot_marker.hh"
#include "scup.hh"
#include "settings.hh"
#include "simplebg.hh"
#include "soundman.hh"
#include "soundmanager.hh"
#include "spritebuffer.hh"
#include "text.hh"
#include "textbuffer.hh"
#include "tpixel.hh"
#include "ui.hh"
#include "vendor.hh"
#include "widget_manager.hh"
#include "worker.hh"
#include "world.hh"
#include "player_activator.hh"
#include "gui.hh"
#ifdef DEBUG
/* for print_screen_point_info */
#include "terrain.hh"
#endif

#include <tms/backend/opengl.h>
#include <unistd.h>

#include <iterator>
#include <map>

#include "SDL2_rotozoom.h"

#define MAX_COPY_ENTITIES 10

#define CSCONN_OFFSX .55f
#define CSCONN_OFFSY .0f

#define MAX_BTN 5
#define MAX_DIR_BTN 1
#define MAX_DIR_SLIDERS 1
#define MAX_MISC_WIDGETS 4

#define INTERACT_TRAIL_LEN 10
#define INTERACT_REACH 20
#define INTERACT_REACH_SQUARED (INTERACT_REACH*INTERACT_REACH)

#define DROP_FREQUENCY 50
#define DROP_SPEEDUP 0.05f

#define BAR_WIDTH       1.05f
#define BAR_HEIGHT      0.15f
#define BAR_Y_OFFSET    BAR_HEIGHT + 0.15f

//#define PROFILING

static bool enable_culling = true;

static const float RH_MAX_DIST = 15.f;
static const float RH_MAX_DIST_ALPHA = 13.f;
static const float RH_MIN_DIST = 1.f;

static tvec3 touch_quickplug_pos;
static tvec2 touch_pos[MAX_P];
static tvec2 touch_proj[MAX_P]; /* touch position projected onto z = 0 */
static uint64_t touch_time[MAX_P];

tvec2 move_pos;
static uint64_t move_time = 0;
static bool move_queried = false;
#ifdef TMS_BACKEND_PC
static uint64_t hov_time = 0;
static bool hov_fadeout = false;
static uint64_t hov_fadeout_time = 0;

#define HOVER_TIME 45000
#define HOVER_TIME_ACTIVE 5000

#endif

static float cam_move_x[2];
static float cam_move_y[2];

static cursorfield *in_cursorfield[MAX_P];
static cursorfield *drag_cursorfield[MAX_P];
static cursorfield *hover_cursorfield;

#define MAX_INTERACTING 1

static entity *interacting[MAX_INTERACTING];
static int current_interacting = -1;
static discharge_effect *interacting_discharge[MAX_INTERACTING];
static b2Vec2 interacting_discharge_lp[MAX_INTERACTING];
static float interacting_M[MAX_INTERACTING][INTERACT_TRAIL_LEN][16];
static float interacting_N[MAX_INTERACTING][INTERACT_TRAIL_LEN][9];
static int interacting_p[MAX_INTERACTING];
static uint32_t layer[MAX_P];

static bool dragging[MAX_P];
static bool moving[MAX_P];
static bool down[MAX_P];
static bool snap[MAX_P];
static uint8_t rotating[MAX_P];
static bool resizing[MAX_P];
static int resize_index = -1; /* index of the vertex or edge */
static uint8_t resize_type; /* 0 for vertex, 1 for edge */

static bool disable_menu = false;
static bool zooming = false;
static bool drawing = false;
static bool zoom_stopped = false;
static float zoom_dist = 0.f;
static panel::widget *wdg_up[MAX_DIR_BTN];
static panel::widget *wdg_down[MAX_DIR_BTN];
static panel::widget *wdg_left[MAX_DIR_BTN];
static panel::widget *wdg_right[MAX_DIR_BTN];
static panel::widget *wdg_btn[MAX_BTN];
static panel::widget *wdg_misc[MAX_MISC_WIDGETS];
static uint8_t wdg_up_i = 0;
static uint8_t wdg_down_i = 0;
static uint8_t wdg_left_i = 0;
static uint8_t wdg_right_i = 0;
static uint8_t wdg_btn_i = 0;
static uint8_t wdg_misc_i = 0;
static entity *copy_entity[MAX_COPY_ENTITIES];
static uint64_t prompt_slot[NUM_PROMPT_SLOTS];
static tvec3 old_cam_pos;

static int box_select_pid = 0;
static tvec3 begin_box_select;
static tvec3 end_box_select;
static entity_set box_select_entities;

static b2MotorJoint *mover_joint[MAX_INTERACTING];

static const char *trans_sources[] = {
    "attribute vec2 position;"
    "attribute vec2 texcoord;"
    "varying lowp vec2 FS_texcoord;"

    "uniform vec2 texcoord_trans;"
    "uniform vec2 position_trans;"
    "uniform vec2 position_trans_lower;"

    "void main(void) {"
        "vec2 tx = texcoord+texcoord_trans;"
        "vec2 trans = position_trans;"
        "if (position.y < 0.01) trans = position_trans_lower;"
        "FS_texcoord = tx;"
        "gl_Position = vec4(position+trans, .99, 1.);"
    "}",

    "uniform sampler2D tex_0;"
    "varying lowp vec2 FS_texcoord;"

    "void main(void) {"
        "gl_FragColor = texture2D(tex_0, FS_texcoord);"
    "}"
};

const char *src_brightpass[2] = {
    "attribute vec2 position;"
    "attribute vec2 texcoord;"
    "varying lowp vec2 FS_texcoord;"

    "void main(void) {"
        "FS_texcoord = texcoord;"
        "gl_Position = vec4(position, 0., 1.);"
    "}"
    ,
    "uniform mediump sampler2D tex_0;"
    "varying lowp vec2 FS_texcoord;"

    "void main(void) {"
        "vec3 color = texture2D(tex_0, FS_texcoord).rgb;"
        "float lum = dot(color, vec3(0.33, 0.33, 0.33));"
        "lum = lum*lum;"
        "gl_FragColor = vec4((lum - .5)*2.);"
    "}"
};

const char *src_output[2] = {
    "attribute vec2 position;"
    "attribute vec2 texcoord;"
    "varying lowp vec2 FS_texcoord;"

    "void main(void) {"
        "FS_texcoord = texcoord;"
        "gl_Position = vec4(position, 0., 1.);"
    "}"
    ,

    "uniform mediump sampler2D tex_0;"
    "varying lowp vec2 FS_texcoord;"

    /*
    "const float A = .2;"
    "const float B = .34;"
    "const float C = .3;"
    "const float D = .2;"
    "const float E = .069;"
    "const float F = .25;"
    */

    /*
    "const float A = 0.15;"
    "const float B = 0.50;"
    "const float C = 0.10;"
    "const float D = 0.20;"
    "const float E = 0.02;"
    "const float F = 0.25;"
    */

    "const float A = .25;"
    "const float B = .11;"
    "const float C = .2;"
    "const float D = .3;"
    "const float E = .07;"
    "const float F = .25;"

    "const vec3 W = vec3(0.45, 0.45, 0.45);"

    "vec3 tonemap(vec3 x) {"
        "return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F)) - E/F;"
    "}"

    "void main(void) {"
        /*
        "float vignette;"
        "vignette = distance(vec2(.5, .5), FS_texcoord);"
        "vignette *= vignette;"
        "vignette = 1.-vignette;"
        "vignette = 1.;"
        //"gl_FragColor = vec4(sqrt(vec3(tonemap(texture2D(tex_0, FS_texcoord).rgb)"
        //" * (1./.1880678)))*vignette, 1.);"
        "gl_FragColor = vec4(tonemap(texture2D(tex_0, FS_texcoord).rgb)/tonemap(W)*vignette, 1.);"
        //"gl_FragColor = vec4(sqrt(texture2D(tex_0, FS_texcoord).rgb), 1.);"
        //
        //
        */

        /*
        "vec3 col = texture2D(tex_0, FS_texcoord).rgb;"
        "col = max(vec3(0.), col-.01);"
        "gl_FragColor = vec4("
            "(col*(6.2*col+.5))/(col*(6.2*col+1.7))+.06"
            ", 1.);"
            */

        //"vec3 col = pow(texture2D(tex_0, FS_texcoord).rgb, vec3(1./2.2));"
        //"vec3 col = sqrt(texture2D(tex_0, FS_texcoord).rgb);"
        "vec3 col = sqrt(texture2D(tex_0, FS_texcoord).rgb);"
        "gl_FragColor = vec4(col, 1.);"
    "}"

    //"void main(void) {"
        //"gl_FragColor = pow(texture2D(tex_0, FS_texcoord), vec4(1./2.2, 1./2.2, 1./2.2, 1.));"
        //"gl_FragColor = sqrt(texture2D(tex_0, FS_texcoord));"
        //"gl_FragColor = vec4(sqrt(texture2D(tex_0, FS_texcoord).rgb), 1.);"
        //"gl_FragColor = pow(texture2D(tex_0, FS_texcoord), vec4(1./2.2, 1./2.2, 1./2.2, 1.));"
    //"}"

};

const char *tutorial_texts[NUM_TUTORIAL_TEXTS] =
{
    /* TUTORIAL_TEXT_PICKUP_EQUIPMENT */
    "When you find loose robot equipment, use the\n"
    "compressor tool to pick it up, bring it to a\n"
    "repair station to install it.",

    /* TUTORIAL_TEXT_ZAP_WOOD */
    "Use the Zapper tool to cut down trees.",

    /* TUTORIAL_TEXT_CAVE_FIRST_TIME */
    "Caves can be dangerous, you should gather some\n"
    "wood above ground before going too deep.\n"
    "Wood can be used to create ladder steps using\n"
    "the Builder tool."
        ,

    /* TUTORIAL_TEXT_REPAIR_STATION_DROP */
    "To drop equipment into the repair station,\n"
    "climb the ladder, aim at the blue antenna and\n"
    "charge the Compressor until it releases an item.",

    /* TUTORIAL_TEXT_BUILD_LADDERS */
    "Ladder steps can help you get out of deep caves,\n"
    "Create one by equipping the Builder tool, then \n"
#ifdef TMS_BACKEND_PC
    "click and drag from your character to where you\n"
#else
    "touch and swipe from your character to where you\n"
#endif
    "want to create the ladder step."
};

bool
game_sorter::distance_to_creature::operator()(activator *a, activator *b)
{
    const b2Vec2 &player_pos = this->c->get_position();
    const float dist_a = b2Distance(a->get_activator_pos(), player_pos);
    const float dist_b = b2Distance(b->get_activator_pos(), player_pos);

    return dist_a < dist_b;
}

static tms_program *prg_output;
static tms_program *prg_brightpass;

static tms_program *trans_program;
GLuint trans_program_shift_loc;
GLuint trans_program_scale_loc;
GLuint trans_program_pos_loc;
GLuint trans_program_poslower_loc;

static void
deactive_misc_wdg(panel::widget **wdg)
{
    tms_debugf("DEACTIVATE");
    if (*wdg) {
        tms_debugf("DEACTIVATE WDG %p %p", wdg, *wdg);
        if ((*wdg)->is_slider()) {
            (*wdg)->render = tms_wdg_slider_render;
        } else if ((*wdg)->is_vslider()) {
            (*wdg)->render = tms_wdg_vslider_render;
        } else if ((*wdg)->is_radial()) {
            (*wdg)->render = tms_wdg_radial_render;
        } else if ((*wdg)->is_field()) {
            (*wdg)->render = tms_wdg_field_render;
        }

        tms_wdg_set_active(*wdg, 0);
        *wdg = 0;
    }
}

static void
active_slider_render(struct tms_wdg *w, struct tms_surface *s)
{
    float px = w->pos.x, py = w->pos.y;
    float sx = 1.f, sy = 0.f;
    float r = 0.f;

    if (_tms.emulating_portrait) {
        int xx = (int)px, yy = (int)py;
        tms_convert_to_portrait(&xx, &yy);
        px = (float)xx;
        py = (float)yy;

        sx = 0.f; sy = 1.f;
        r = -90.f;
    }

    /* Save old color */
    tvec4 old = s->ddraw->color;

    /* Set new color to the active greenish tint color */
    tms_ddraw_set_color(s->ddraw, ACTIVE_MISC_WIDGET_COLOR);

    /* Render base sprite */
    tms_ddraw_sprite_r(s->ddraw, w->s[0], px, py, w->size.x, w->size.y, r);

    {
        float pulse = cos((double)_tms.last_time/(2000 * 100.));
        r = pulse*2.f;
        pulse += 1.f;
        pulse /= 2.f;

        float gray  = 1.f - (pulse/8.f);
        float ngray = 1.f + (pulse/8.f);
        tms_ddraw_set_color(s->ddraw, gray, gray, ngray, 1.f);

        struct tms_sprite *spr = gui_spritesheet::get_sprite(S_MOUSE);

        float wmod = spr->width / spr->height;

        tms_ddraw_sprite_r(s->ddraw, spr,
                px + ((w->value[0]-.5f) * 2.f)*w->size.x/2.f*sx,
                py + ((w->value[0]-.5f) * 2.f)*w->size.x/2.f*sy,
                .6f * wmod * _tms.xppcm, .6f * _tms.yppcm, r);
    }

    /* Revert back to old color */
    tms_ddraw_set_color(s->ddraw, TVEC4_INLINE(old));
}

static void
active_vslider_render(struct tms_wdg *w, struct tms_surface *s)
{
    float px = w->pos.x, py = w->pos.y;
    float sx = 0.f, sy = 1.f;
    float r = -90.f; // base rotation of vertical slider is -90

    if (_tms.emulating_portrait) {
        int xx = (int)px, yy = (int)py;
        tms_convert_to_portrait(&xx, &yy);
        px = (float)xx;
        py = (float)yy;

        sx = -1.f; sy = 0.f;
        r = 0.f;
    }

    /* Save old color */
    tvec4 old = s->ddraw->color;

    /* Set new color to the active greenish tint color */
    tms_ddraw_set_color(s->ddraw, ACTIVE_MISC_WIDGET_COLOR);

    /* Render base sprite */
    tms_ddraw_sprite_r(s->ddraw, w->s[0], px, py, w->size.y, w->size.x, r);

    {
        float pulse = cos((double)_tms.last_time/(2000 * 100.));
        r = pulse*2.f;
        pulse += 1.f;
        pulse /= 2.f;

        float gray  = 1.f - (pulse/8.f);
        float ngray = 1.f + (pulse/8.f);
        tms_ddraw_set_color(s->ddraw, gray, gray, ngray, 1.f);

        struct tms_sprite *spr = gui_spritesheet::get_sprite(S_MOUSE);

        float wmod = spr->width / spr->height;

        tms_ddraw_sprite_r(s->ddraw, spr,
                px + ((w->value[0]-.5f) * 2.f)*w->size.y/2.f*sx,
                py + ((w->value[0]-.5f) * 2.f)*w->size.y/2.f*sy,
                .6f * wmod * _tms.xppcm, .6f * _tms.yppcm, r);
    }

    /* Revert back to old color */
    tms_ddraw_set_color(s->ddraw, TVEC4_INLINE(old));
}

static void
active_radial_render(struct tms_wdg *w, struct tms_surface *s)
{
    float px = w->pos.x, py = w->pos.y;
    float r = 0.f;

    if (_tms.emulating_portrait) {
        int xx = (int)px, yy = (int)py;
        tms_convert_to_portrait(&xx, &yy);
        px = (float)xx;
        py = (float)yy;

        r = -90.f;
    }

    /* Save old color */
    tvec4 old = s->ddraw->color;

    /* Set new color to the active greenish tint color */
    tms_ddraw_set_color(s->ddraw, ACTIVE_MISC_WIDGET_COLOR);

    /* Render base sprite */
    tms_ddraw_sprite_r(s->ddraw, w->s[0], px, py, w->size.x, w->size.y, r);

    /* Revert back to old color */
    tms_ddraw_set_color(s->ddraw, TVEC4_INLINE(old));

    float a =  w->value[0] * 2.f * M_PI + (_tms.emulating_portrait ? M_PI/2.f : 0.f);
    float cs = cosf(a);
    float sn = sinf(a);

    float knob_w = w->size.x/5.f;
    float knob_h = w->size.y/5.f;

    if (w->s[1]) {
        tms_ddraw_sprite_r(s->ddraw, w->s[1],
                px + w->size.x/2.1f * cs,
                py + w->size.y/2.1f * sn,
                knob_w,
                knob_h, r);

        if (w->enable_ghost) {
            a =  w->ghost[0] * 2.f * M_PI + (_tms.emulating_portrait ? M_PI/2.f : 0.f);
            cs = cosf(a);
            sn = sinf(a);

            tvec4 col = s->ddraw->color;
            tms_ddraw_set_color(s->ddraw, 1.f, 1.f, 1.f, 0.5f);
            tms_ddraw_sprite_r(s->ddraw, w->s[1],
                    w->pos.x + w->size.x/2.1f * cs,
                    w->pos.y + w->size.y/2.1f * sn,
                    knob_w,
                    knob_h,r);
            tms_ddraw_set_color(s->ddraw, col.x, col.y, col.z, col.w);
        }
    }
}

static void
active_field_render(struct tms_wdg *w, struct tms_surface *s)
{
    float px = w->pos.x, py = w->pos.y;
    float sx = 1.f, sy = 1.f;
    float r = 0.f;

    if (_tms.emulating_portrait) {
        int xx = (int)px, yy = (int)py;
        tms_convert_to_portrait(&xx, &yy);
        px = (float)xx;
        py = (float)yy;

        sx = 1.f; sy = 1.f;
        r = -90.f;
    }

    /* Save old color */
    tvec4 old = s->ddraw->color;

    /* Set new color to the active greenish tint color */
    tms_ddraw_set_color(s->ddraw, ACTIVE_MISC_WIDGET_COLOR);

    /* Render base sprite */
    tms_ddraw_sprite_r(s->ddraw, w->s[0], px, py, w->size.x, w->size.y, r);

    /* Revert back to old color */
    tms_ddraw_set_color(s->ddraw, TVEC4_INLINE(old));

    float knob_w = w->size.x/5.f;
    float knob_h = w->size.y/5.f;

    if (w->s[1]) {
        tms_ddraw_sprite_r(s->ddraw, w->s[1],
                px + ((w->value[0]-.5f)  * 1.8f)*w->size.x/2.f*sx,
                py + ((w->value[1]-.5f) * 1.8f)*w->size.y/2.f*sy,
                knob_w,
                knob_h,
                r);

        if (w->enable_ghost) {
            tvec4 col = s->ddraw->color;
            tms_ddraw_set_color(s->ddraw, 1.f, 1.f, 1.f, 0.5f);
            tms_ddraw_sprite_r(s->ddraw, w->s[1],
                    px + ((w->ghost[0]-.5f)  * 1.8f)*w->size.x/2.f*sx,
                    py + ((w->ghost[1]-.5f) * 1.8f)*w->size.y/2.f*sy,
                    knob_w,
                    knob_h,
                    r);
            tms_ddraw_set_color(s->ddraw, col.x, col.y, col.z, col.w);
        }
    }
}

static bool
set_active_double(panel::widget *wdg)
{
    bool is_self = (G->active_hori_wdg == wdg && G->active_vert_wdg == wdg);

    if (G->active_hori_wdg || G->active_vert_wdg) {
        deactive_misc_wdg(&G->active_hori_wdg);
        deactive_misc_wdg(&G->active_vert_wdg);

        if (is_self) {
            return false;
        }
    }

    if (wdg->is_radial()) {
        wdg->render = active_radial_render;
    } else if (wdg->is_field()) {
        wdg->render = active_field_render;
    }

    G->active_hori_wdg = wdg;
    G->active_vert_wdg = wdg;

    return true;
}

static bool
set_active_slider(panel::widget **active_wdg, panel::widget *wdg)
{
    bool is_self = (*active_wdg == wdg);

    if (*active_wdg) {
        if ((*active_wdg)->type == TMS_WDG_RADIAL || (*active_wdg)->type == TMS_WDG_FIELD) {
            deactive_misc_wdg(&G->active_hori_wdg);
            deactive_misc_wdg(&G->active_vert_wdg);
        } else {
            deactive_misc_wdg(active_wdg);
        }

        if (is_self) {
            return false;
        }
    }

    if (wdg->is_slider()) {
        wdg->render = active_slider_render;
    } else if (wdg->is_vslider()) {
        wdg->render = active_vslider_render;
    }

    *active_wdg = wdg;

    return true;
}

static bool
try_activate_slider(int slot)
{
    if (wdg_misc[slot]) {
        bool ret = true;
        bool warp = false;

        switch (wdg_misc[slot]->type) {
            case TMS_WDG_FIELD:
                warp = true;
            case TMS_WDG_RADIAL:
                ret = set_active_double(wdg_misc[slot]);
                break;
            case TMS_WDG_SLIDER:
                warp = true;
                ret = set_active_slider(&G->active_hori_wdg, wdg_misc[slot]);
                break;
            case TMS_WDG_VSLIDER:
                warp = true;
                ret = set_active_slider(&G->active_vert_wdg, wdg_misc[slot]);
                break;
            default:
                ret = false;
                break;
        }

        if (ret) {
            if (warp) {
                G->wdg_base_x = _tms.window_width/2;
                G->wdg_base_y = _tms.window_height/2;
                SDL_WarpMouseInWindow((SDL_Window*)_tms._window, G->wdg_base_x, G->wdg_base_y);
            } else {
                SDL_GetMouseState(&G->wdg_base_x, &G->wdg_base_y);
                G->wdg_base_y = _tms.window_height - G->wdg_base_y;
            }

            tms_wdg_set_active(wdg_misc[slot], 1);
        }

        return ret;
    }

    return false;
}

int
render_foreground(struct tms_rstate *state, void *value)
{
    int val = VOID_TO_INT(value);

    if (val == 3) {
        glColorMask(1,1,1,1);
        G->tmp_ao_mask = (tvec3){.0f, 0.f, 0.0f};
        return T_OK;
    }

    return 1;
}

int
render_next_prio(struct tms_rstate *state, void *value)
{
    int val = VOID_TO_INT(value);

    switch (val) {
        case 2:
            G->tmp_ao_mask = (tvec3){.0f, 0.f, 0.0f};
            glColorMask(1,1,1,1);
            if (W->is_adventure() && adventure::player && G->caveview_size > 0.f) {
                return 1;
            }
            break;

        case 1:
            G->tmp_ao_mask = (tvec3){.0f, 0.f, 1.0f};
            glColorMask(1,1,1,1);
            break;

        case 0:
            G->tmp_ao_mask = (tvec3){.0f, 1.f, 0.0f};
            glColorMask(1,1,1,1);
            break;

        default: return 1;
    }

    if (!((1 << val) & G->layer_vis)) {
        return 1;
    }

    return T_OK;
}

int
render_hidden_prio(struct tms_rstate *rstate, void *value)
{
    int val = VOID_TO_INT(value);

    switch (val) {
        case 2: G->tmp_ao_mask = (tvec3){.0f, 0.f, 0.0f}; break;
        case 1: G->tmp_ao_mask = (tvec3){.0f, 0.f, 1.0f}; break;
        case 0: G->tmp_ao_mask = (tvec3){.0f, 1.f, 0.0f}; break;

        default: return 1;
    }

    if ((!((1 << val) & G->layer_vis))
        || (W->is_adventure() && adventure::player && G->caveview_size > 0.f  && G->caveview_size < 1.f && val == 2)) {
        glEnable(GL_BLEND);
        /*glDepthMask(true);
        glEnable(GL_DEPTH_TEST);*/
        glBlendColor(1.f, 1.f, 1.f, G->caveview_size > 0.f ? 1.f-(tclampf(G->caveview_size, 0.0f, 1.f)) : .125f);
        glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
    } else {
        return 1;
    }

    return T_OK;
}

void
post_fn(struct tms_rstate *state)
{
    if (W->is_adventure() && adventure::player && G->caveview_size > 0.f) {
        glDepthMask(0);
        struct tms_entity *e = G->caveview;
        glDisable(GL_DEPTH_TEST);
        /* render the caveview fade stuff */
        struct tms_program *prog = m_bg_fixed.pipeline[0].program;
        tms_entity_apply_uniforms(e, state->p);
        tms_pipeline_apply_combined_uniforms(state->p, state, prog, e);
        tms_program_bind(prog);
        tmat4_load_identity(G->caveview->M);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, W->cwindow->caveview->gl_texture);
        tmat4_translate(G->caveview->M,
                //roundf(adventure::player->get_position().x*2.)/2.f- .25f,
                //roundf((adventure::player->get_position().y+.5f)*2.)/2.f- .25f,
                G->caveview_pos.x,
                G->caveview_pos.y,
                //(adventure::player->get_position().x*2.)/2.f- .25f,
                //(adventure::player->get_position().y+.5f)*2./2.f- .25f,
                2.0);
        tmat4_scale(G->caveview->M, CAVEVIEW_SIZE/2, CAVEVIEW_SIZE/2, 0.f);
        tmat4_copy(state->modelview, state->view);
        tmat4_multiply(state->modelview, e->M);
        tms_pipeline_apply_local_uniforms(state->p, state, prog, e);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        tms_mesh_render(const_cast<tms_mesh*>(tms_meshfactory_get_square()), prog);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDepthFunc(GL_LESS);
        glDepthMask(0xff);
    }

    /* TODO: copy blending stuff from material/escript */
    bool base_initialized = false;
    struct tms_program *prog = m_spritebuf.pipeline[0].program;

    for (std::set<escript*>::iterator it = W->escripts.begin(); it != W->escripts.end(); ++it) {
        escript *es = static_cast<escript*>(*it);
        draw_data *draw = es->normal_draw;
        if (!draw || es->coordinate_mode != ESCRIPT_SCREEN) continue;

        if (!base_initialized) {
            tmat4_load_identity(state->view);
            //tmat4_load_identity(state->projection);
            tmat4_set_ortho(state->projection, 0, 100, 0, 100, 1, -1);
            tmat4_load_identity(state->modelview);
            glDisable(GL_DEPTH_TEST);
            tms_program_bind(prog);
            tms_pipeline_apply_global_uniforms(state->p, state, prog);

            base_initialized = true;
        }

        struct tms_entity *e = draw->sprite_ent;
        tms_program_bind(prog);
        tms_entity_apply_uniforms(e, state->p);
        tms_pipeline_apply_combined_uniforms(state->p, state, prog, e);
        tms_pipeline_apply_local_uniforms(state->p, state, prog, e);

        tmat4_load_identity(draw->sprite_ent->M);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, draw->texture->gl_texture);

        if (es->blending_mode == 1) {
            glEnable(GL_BLEND);
            glDepthMask(false);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else if (es->blending_mode == 2) {
            glEnable(GL_BLEND);
            glDepthMask(false);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        } else {
            glDisable(GL_BLEND);
            //glDepthMask(true); do we need to enable depth mask here?
        }

        tms_mesh_render(draw->sprite_ent->mesh, prog);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (base_initialized) {
        glDisable(GL_BLEND);
        glDepthMask(true);
    }

    /* TODO: disable blending, re-enable depth testing */
}

void
ao_post_fn(struct tms_rstate *state)
{
    /// XXX: what?
    return;
    if (W->level.type == LCAT_ADVENTURE && adventure::player) {
        glColorMask(0,0,1,0);
        float caves = G->caveview_size;

        struct tms_entity *e = G->caveview;
        struct tms_mesh *m = e->mesh;
        struct tms_program *prog = m_cavemask.pipeline[state->p].program;

        tms_program_bind(prog);
        tms_entity_apply_uniforms(e, state->p);
        tms_pipeline_apply_combined_uniforms(state->p, state, prog, e);

        tmat4_load_identity(G->caveview->M);
        tmat4_translate(G->caveview->M, adventure::player->get_position().x, adventure::player->get_position().y, 2.05f);
        tmat4_scale(G->caveview->M, caves, caves, 0.0f);
        tmat4_copy(state->modelview, state->view);
        tmat4_multiply(state->modelview, e->M);
        tms_pipeline_apply_local_uniforms(state->p, state, prog, e);
        tms_mesh_render(G->caveview->mesh, prog);
        //tms_varray_unbind_attributes(m->vertex_array, prog->last_locations);
    }
}

int ao_mask_color(struct tms_rstate *state, void *value)
{
    int val = VOID_TO_INT(value);

    switch (val) {
        case 2:
            glColorMask(0,0,1,0);
            break;
        case 1:
            glColorMask(0,1,0,0);
            break;
        case 0:
            glColorMask(1,0,0,0); break;
        default: return 1;
    }

    return T_OK;
}

int sort_blending(struct tms_rstate *rstate, void *value)
{
    int val = VOID_TO_INT(value);

    rstate->graph->sort_reverse_prio = 0;

    if (P.best_variable_in_the_world == 1337) {
        if (val != TMS_BLENDMODE_OFF) {
            return 1;
        }
    } else {
        switch (val) {
            default:
            case TMS_BLENDMODE_OFF:

                if (P.best_variable_in_the_world3 == 1) {
                    rstate->graph->sort_reverse_prio = 1;
                    glEnable(GL_BLEND);
                    glDepthMask(false);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                } else {
                    rstate->graph->sort_reverse_prio = 0;
                    glDisable(GL_BLEND);
                    glDepthMask(true);
                }
                break;

            case TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA:
                rstate->graph->sort_reverse_prio = 1;
                glEnable(GL_BLEND);
                glDepthMask(false);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;

            case TMS_BLENDMODE__SRC_ALPHA__ONE:
                rstate->graph->sort_reverse_prio = 1;
                glEnable(GL_BLEND);
                glDepthMask(false);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;

            case TMS_BLENDMODE__ONE_MINUS_DST_COLOR__ONE_MINUS_SRC_ALPHA:
                rstate->graph->sort_reverse_prio = 1;
                glEnable(GL_BLEND);
                glDepthMask(false);
                glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
                break;
        }
    }

    return T_OK;
}

void
on_panel_change(tms_wdg *w, float value)
{
    if (value == 1.f) {
        if (W->is_adventure()) {
            G->set_control_panel(adventure::player);
        } else {
            G->set_control_panel(0);
        }
    }
}

static std::map<uint32_t, float> hp_offsets;


static const int NUM_ACTIVATOR_BINDINGS = 3;

static const char *activator_strings[NUM_ACTIVATOR_BINDINGS] = {
    "E",
    "S+E",
    "C+E",
};

static p_text *activator_texts[NUM_ACTIVATOR_BINDINGS];

game *G = 0;

game::game()
    : active_hori_wdg(0)
    , active_vert_wdg(0)
    , wdg_base_x(0)
    , wdg_base_y(0)
    , text_small(0)
    , info_label(0)
    , help_dragpanel(0)
    , panel_edit_need_scroll(false)
    , render_controls(false)
    , tex_controls(0)
{
    G = this;

    this->layer_vis = 7;

    this->layer_vis_saved = 7;

    this->current_keymod = 0;
    this->previous_keymod = 0;
    this->follow_options.linear = false;
    this->follow_options.offset_mode = 0;
    this->follow_options.offset.x = 0.f;
    this->follow_options.offset.y = 0.f;

#ifdef TMS_BACKEND_PC
    this->hov_ent = 0;
#endif

    this->caveview_size = 0.f;
    this->caveview_zoom = 0.f;
    this->inventory_highest_y = 0.f;
    this->inventory_scroll_offset = 0.f;
    this->dropping = -1;
    this->drop_step = 0;
    this->drop_amount = 0;
    this->drop_speed = 1.f;
    for (int x=0; x<NUM_PROMPT_SLOTS; ++x) {
        prompt_slot[x] = 0;
    }
    for (int x=0; x<MAX_RECENT; ++x) {
        this->recent[x] = -1;
    }

    this->force_static_update = 1;
    this->do_static_update = false;
    this->last_static_update = (tvec3){0.f,0.f,0.f};

    this->do_drop_interacting = false;
    this->current_prompt = 0;
    this->opened_special_level = 0;
    this->_lock = SDL_CreateMutex();
    this->previous_level = 0;
    this->screen_back = 0;
    this->state.waiting = false;
    this->score_highlight = 0.f;
    this->numfeed_timer = 0.f;
    this->score_text = new p_text(font::large, ALIGN_RIGHT, ALIGN_TOP);
    this->score_text->set_position(_tms.window_width, _tms.window_height);
    this->numfeed_text = new p_text(font::small);
    this->numfeed_text->set_position(_tms.window_width/2.f, _tms.window_height-_tms.yppcm/2.f/2.f);
    this->tmp_ao_layer = 1;
    this->tmp_ao_mask = (tvec3){.0f, 0.f, 0.f};
    this->cam_vel = (tvec3){0,0,0};

    for (unsigned x=0; x<NUM_ACTIVATOR_BINDINGS; ++x) {
        activator_texts[x] = new p_text(font::medium);
        activator_texts[x]->set_text(activator_strings[x]);
    }

    this->tmp_ambientdiffuse.x = P.default_ambient;
    this->tmp_ambientdiffuse.y = P.default_diffuse;

    this->sel_p_ent = 0;
    this->sel_p_body = 0;
    this->sel_p_frame = 0;

    this->current_panel = 0;
    this->follow_object = 0;

    this->cs_conn = 0;
    this->cs_timer = 0;

    this->state.abo_architect_mode = false;
    this->state.sandbox = true;
    this->state.edev_labels = false;
    this->state.gridsize = .25f;
    this->state.finished = false;
    this->set_score(0);

    this->_mode = GAME_MODE_DEFAULT;
    this->set_mode(GAME_MODE_DEFAULT);
    this->ss_edev = 0;
    this->ss_plug = 0;
    this->ss_asker = 0;
    this->ss_anim = .0f;
    this->ss_num_socks = 0;
    this->main_fb = 0;
    this->bloom_fb = 0;

    for (int x=0; x<NUM_CA; x++) {
        this->ca[x].life = -2.f;
    }

    //this->main_fb = tms_fb_alloc(_tms.window_width, _tms.window_height, 0);
    //this->main_fb = tms_fb_alloc(_tms.window_width, _tms.window_height, 1);

    this->icon_fb = 0;

    this->dd = tms_ddraw_alloc();

    this->set_scene(new tms::scene());

    this->cam_iterator = W->cam_markers.end();

    this->light = P.get_light_normal();

    this->bgent = new simplebg();
    this->grident = new grid();

    this->caveview = tms_entity_alloc();
    this->caveview->prio = 3;
    tms_entity_set_mesh(caveview, tms_ddraw_circle_mesh);
    tms_entity_set_material(caveview, &m_cavemask);
    tmat4_load_identity(caveview->M);
    tmat3_load_identity(caveview->N);

    //this->reset();
    //

    adventure::init();

    this->cam_rel_pos = b2Vec2(0,0);
    this->adv_rel_pos = b2Vec2(0,0);

    this->init_framebuffers();
    this->init_shaders();
    this->init_graphs();
    this->init_camera();
}

game::~game()
{
    delete this->wm;
    if (this->score_text) {
        delete this->score_text;
    }

    if (this->numfeed_text) {
        delete this->numfeed_text;
    }
    if (this->text_small) {
        delete this->text_small;
    }
    if (this->info_label) {
        delete this->info_label;
    }
    if (this->help_dragpanel) {
        delete this->help_dragpanel;
    }
    gui_spritesheet::cleanup();
    tms_atlas_free(this->texts);

    for (std::vector<struct menu_obj>::iterator it = menu_objects.begin();
            it != menu_objects.end(); ++it) {
        struct menu_obj &mo = *it;

        free(mo.name);
        delete mo.e;
    }
}

bool
game::occupy_prompt_slot()
{
    uint64_t cur_time = _tms.last_time;

    for (int x=0; x<NUM_PROMPT_SLOTS; ++x) {
        uint64_t dt = cur_time - prompt_slot[x];
        if (dt > PROMPT_TIME) {
            prompt_slot[x] = cur_time;
            tms_infof("Took prompt slot %d", x);
            return true;
        }
    }

    return false;
}

void
game::init_framebuffers()
{
    tms_infof("Initializing game framebuffers");

    if (!this->icon_fb) {
        this->icon_fb = tms_fb_alloc(512, 512, 0);
        tms_fb_add_texture(this->icon_fb, GL_RGB, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        tms_fb_enable_depth(this->icon_fb, GL_DEPTH_COMPONENT16);
    }

    if (this->main_fb) {
        tms_fb_free(this->main_fb);
        this->main_fb = 0;
    }

    if (this->bloom_fb) {
        tms_fb_free(this->bloom_fb);
        this->bloom_fb = 0;
    }

#ifndef TMS_USE_GLES
    if (settings["postprocess"]->v.b) {
        tms_infof("Postprocess time");
        //this->main_fb = tms_fb_alloc(_tms.window_width/2., _tms.window_height/2., 0);
        this->main_fb = tms_fb_alloc(_tms.window_width, _tms.window_height, 0);
        tms_fb_add_texture(this->main_fb, GL_RGBA, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        //tms_fb_add_texture(this->main_fb, GL_RGB, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
        tms_fb_enable_depth(this->main_fb, GL_DEPTH_COMPONENT16);

        this->bloom_fb = tms_fb_alloc(_tms.window_width, _tms.window_height, 1);
        tms_fb_add_texture(this->bloom_fb, GL_RGBA, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    } else {
        this->main_fb = 0;
        this->bloom_fb = 0;
    }
#endif
}

void
game::init_shaders()
{
    tms_infof("Compiling shaders...");

    struct tms_shader *sh;

    sh = tms_shader_alloc();
    tms_shader_compile(sh, GL_VERTEX_SHADER, trans_sources[0]);
    tms_shader_compile(sh, GL_FRAGMENT_SHADER, trans_sources[1]);
    trans_program = (tms_shader_get_program(sh, TMS_NO_PIPELINE));
    trans_program_shift_loc = tms_program_get_uniform(trans_program, "texcoord_trans");
    trans_program_pos_loc = tms_program_get_uniform(trans_program, "position_trans");
    trans_program_poslower_loc = tms_program_get_uniform(trans_program, "position_trans_lower");

    sh = tms_shader_alloc();
    tms_shader_compile(sh, GL_VERTEX_SHADER, src_output[0]);
    tms_shader_compile(sh, GL_FRAGMENT_SHADER, src_output[1]);
    prg_output = tms_shader_get_program(sh, TMS_NO_PIPELINE);

    sh = tms_shader_alloc();
    tms_shader_compile(sh, GL_VERTEX_SHADER, src_brightpass[0]);
    tms_shader_compile(sh, GL_FRAGMENT_SHADER, src_brightpass[1]);
    prg_brightpass = tms_shader_get_program(sh, TMS_NO_PIPELINE);
}

void
game::init_graphs()
{
    tms_infof("Loading graphs...");

    this->graph = this->get_scene()->create_graph(0);
    this->graph->sorting[1] = TMS_SORT_PRIO;
    this->graph->sorting[0] = TMS_SORT_BLENDING;
    this->graph->sorting[2] = TMS_SORT_TEXTURE0;
    this->graph->sorting[3] = TMS_SORT_TEXTURE1;
    this->graph->sorting[4] = TMS_SORT_SHADER;
    this->graph->sorting[5] = TMS_SORT_VARRAY;
    this->graph->sorting[6] = TMS_SORT_MESH;
    this->graph->sort_depth = 7;
    this->graph->post_fn = post_fn;
    tms_graph_set_sort_callback(this->graph, TMS_SORT_BLENDING, sort_blending);
    tms_graph_enable_culling(this->graph, enable_culling);

    this->gi_graph = this->get_scene()->create_graph(1);
    this->gi_graph->sorting[0] = TMS_SORT_PRIO;
    this->gi_graph->sorting[1] = TMS_SORT_SHADER;
    //this->gi_graph->sorting[2] = TMS_SORT_TEXTURE0;
    this->gi_graph->sorting[2] = TMS_SORT_VARRAY;
    this->gi_graph->sorting[3] = TMS_SORT_MESH;
    this->gi_graph->sort_depth = 4;
    tms_graph_enable_culling(this->gi_graph, enable_culling);

    this->ao_graph = this->get_scene()->create_graph(3);
    this->ao_graph->sorting[0] = TMS_SORT_PRIO_BIASED;
    this->ao_graph->sorting[1] = TMS_SORT_SHADER;
    this->ao_graph->sorting[2] = TMS_SORT_VARRAY;
    this->ao_graph->sorting[3] = TMS_SORT_MESH;
    this->ao_graph->sort_depth = 4;
    this->ao_graph->post_fn = ao_post_fn;
    tms_graph_set_sort_callback(this->ao_graph, TMS_SORT_PRIO_BIASED, ao_mask_color);
    tms_graph_enable_culling(this->ao_graph, enable_culling);

    this->outline_graph = new tms::graph(0);
    this->outline_graph->scene_pos = 3;
    tms_graph_init(this->outline_graph, 0, 0);
    this->outline_graph->sorting[0] = TMS_SORT_SHADER;
    this->outline_graph->sorting[1] = TMS_SORT_TEXTURE0;
    this->outline_graph->sorting[2] = TMS_SORT_VARRAY;
    this->outline_graph->sorting[3] = TMS_SORT_MESH;
    this->outline_graph->sort_depth = 4;
    this->outline_graph->full_pipeline = 0;
    tms_graph_enable_culling(this->outline_graph, 0);
}

void
game::init_camera()
{
    tms_infof("Loading camera...");

    this->cam = new tms::camera();

    this->cam->enable(TMS_CAMERA_PERSPECTIVE);
    this->cam->set_direction(0, .0f, -1);
    this->cam->set_position(0, 5, 14);
    this->cam->up = (tvec3){0.f, 1.f, 0.f};
    this->cam->fov = 50;

    this->cam->set_direction(0, 0, -1);
    this->cam->calculate();

    this->ao_cam = new tms::camera();
    this->ao_cam->width = 1024.f/50.f;
    this->ao_cam->height = 1024.f/50.f;

    this->ao_cam->near = 0.0f - 2.0f;
    this->ao_cam->far = LAYER_DEPTH*3 + .5f;// + .75f;

    this->gi_cam = new tms::camera();
    this->gi_cam->width = 1024.f/50.f;
    this->gi_cam->height = 1024.f/50.f;
    this->gi_cam->near = 0.f - 2.f;
    this->gi_cam->far = LAYER_DEPTH*3 + .5f;// + .75f;
}

void
game::reset_touch(bool hard/*=true*/)
{
    hover_cursorfield = 0;
    current_interacting = -1;

    for (int x=0; x<MAX_P; x++) {
        this->mining[x] = false;
        down[x] = false;
        snap[x] = false;
        moving[x] = false;
        dragging[x] = false;
        resizing[x] = false;

        if (!hard) {
            if (in_cursorfield[x]) {
                in_cursorfield[x]->pressed --;
            }
        }
        in_cursorfield[x] = 0;

        if (!hard) {
            if (drag_cursorfield[x]) {
                drag_cursorfield[x]->dragged --;
            }
        }
        drag_cursorfield[x] = 0;

        rotating[x] = 0;
        touch_time[x] = _tms.last_time;
        layer[x] = -1;
    }

    if (hard) {
        for (int x=0; x<MAX_INTERACTING; x++) {
            /* XXX: release objects properly? */
            interacting[x] = 0;
            mover_joint[x] = 0;
        }
    } else {
        this->drop_interacting();
    }

#ifdef TMS_BACKEND_PC
    move_time = _tms.last_time;
#endif

    cam_move_x[0] = 0.f;
    cam_move_x[1] = 0.f;
    cam_move_y[0] = 0.f;
    cam_move_y[1] = 0.f;
    zooming = false;

    this->reset_touch_gui();
}

int
game::pause()
{
    tms_debugf("game::pause");
    sm::stop_all();
    ui::open_dialog(CLOSE_ABSOLUTELY_ALL_DIALOGS);

#ifdef TMS_BACKEND_PC
    SDL_SetWindowGrab((SDL_Window*)_tms._window, SDL_FALSE);
#endif

    return T_OK;
}

int
game::resume(void)
{
    this->render_controls = false;

    if (this->resume_action == GAME_START_NEW_ADVENTURE) {
        tms_infof("Resume action: Start new adventure");
        this->create_level(LCAT_ADVENTURE, false, true);
        this->state.test_playing = false;
        this->state.new_adventure = true;

        if (settings["first_adventure"]->is_true()) {
            this->render_controls = true;
            settings["first_adventure"]->set(false);
        }

        this->begin_play();
    } else if (this->resume_action == GAME_RESUME_NEW || this->resume_action == GAME_RESUME_NEW_EMPTY) {
        tms_infof("Resume action: New");

        uint32_t level_type = resume_level_type;

        this->create_level(level_type, this->resume_action == GAME_RESUME_NEW_EMPTY, false);

        if (!settings["hide_tips"]->v.b) {
            ui::open_sandbox_tips();
        }
    }

    if (this->resume_action == GAME_RESUME_OPEN) {
        /* do not do anything, the level was loaded before game was resumed */
        tms_infof("OPEN");
    }

#ifndef SCREENSHOT_BUILD
    this->state.fade = 1.f;
#endif
    this->resume_action = GAME_RESUME_CONTINUE;

    this->refresh_widgets();
    reset_touch();

    return T_OK;
}

void
game::back()
{
    tms_debugf("BACK PRESSED");
    if (this->state.test_playing) {
        this->state.test_playing = false;
        this->state.sandbox = true;
        this->open_sandbox(LEVEL_LOCAL, W->level.local_id);
    } else {
        if (this->screen_back == 0) {
            tms_infof("Returning to main menu.");

            if (W->level_id_type == LEVEL_DB) {
                ui::open_dialog(DIALOG_COMMUNITY);
            } else {
                if (this->state.sandbox && W->is_paused() && !this->state.test_playing && this->state.modified) {
                    tms_infof("Autosaving.");
                    W->save(SAVE_TYPE_AUTOSAVE);
                }
                sm::stop_all();
                tms::set_screen(P.s_menu_main);
            }

        } else {
            tms_infof("Returning to %p", this->screen_back);
            sm::stop_all();
            tms::set_screen(this->screen_back);
        }
    }

}

b2Joint*
game::create_joint(b2JointDef *jd)
{
    return W->b2->CreateJoint(jd);
}

static float saved_z;

void
game::set_caveview_zoom_limits(bool update)
{
    saved_z = this->cam->_position.z;

    if (caveview_size >= 2.f) {
        if (adventure::player && this->state.new_adventure) {
            if (adventure::player->inventory[RESOURCE_WOOD] > 0) {
                if (!(settings["tutorial"]->v.u32 & TUTORIAL_BUILD_LADDERS)) {
                    this->add_tt(TUTORIAL_TEXT_BUILD_LADDERS, 0, b2Vec2(0.f, 2.f), 15.f);
                    this->finished_tt(TUTORIAL_BUILD_LADDERS);
                }
            } else {
                if (!(settings["tutorial"]->v.u32 & TUTORIAL_CAVE_FIRST_TIME)) {
                    this->add_tt(TUTORIAL_TEXT_CAVE_FIRST_TIME, 0, b2Vec2(0.f, 2.f), 15.f);
                    this->finished_tt(TUTORIAL_CAVE_FIRST_TIME);
                }
            }
        }
    }

#ifdef DEBUG
    if (G->shift_down()) {
        return;
    }
#endif

#define CAVEVIEW_MAX_ZOOM 15.f

    if (update) {
        if (this->caveview_size > 0.f) {
            this->caveview_zoom = std::min(std::max(this->caveview_size, this->caveview_zoom), 1.f);
        } else {
            this->caveview_zoom -= _tms.dt*3.f;
        }
    }

    if (this->caveview_zoom > 0.f && this->cam->_position.z > CAVEVIEW_MAX_ZOOM) {
        /* force the zoom to a lower maximum if we're in a cave */
        float b = 1.f-powf(tclampf(this->caveview_zoom, 0.f, 1.f), 1.f);
        this->cam->_position.z = CAVEVIEW_MAX_ZOOM*(1.f-b) + this->cam->_position.z*b;
    }

    //tms_debugf("cam pos: %f", this->cam->_position.z);
}

void
game::unset_caveview_zoom_limits()
{
    this->cam->_position.z = saved_z;
}

int
game::step(double dt)
{
#ifndef SCREENSHOT_BUILD
    if (!P.focused) {
        _tms.time_accum = 0;
        return T_OK;
    }
#endif

    ++ this->state.step_num;

    if (this->state.waiting) {
        _tms.time_accum = 0;
    }

#ifdef TMS_BACKEND_PC
    if (settings["rc_lock_cursor"]->v.b) {
        if ((this->active_hori_wdg && !this->active_hori_wdg->is_radial())
                || (this->active_vert_wdg && !this->active_vert_wdg->is_radial())) {
            SDL_ShowCursor(0);
            //SDL_SetRelativeMouseMode(SDL_TRUE);
        } else {
            SDL_ShowCursor(1);
            //SDL_SetRelativeMouseMode(SDL_FALSE);
        }
    }
#endif

    this->wm->step();

    /*
    if (this->state.time_mul > 0.f) {
        dt *= 1.f-this->state.time_mul;
        _tms.dt *= 1.f -this->state.time_mul;
        tms_infof("time mulling");
        tms_infof("dt == %f", _tms.dt);
    }
    */

#ifdef TMS_BACKEND_PC
    uint64_t diff = _tms.last_time - move_time;
    if (((this->hov_text->active && diff > HOVER_TIME_ACTIVE) || (this->hov_text->active == false && diff > HOVER_TIME)) && !move_queried) {
        move_queried = true;

        b2Body *_b;
        tvec2 _o;
        uint8_t _f;

        W->query(this->cam, (int)move_pos.x, (int)move_pos.y, &this->hov_ent, &_b, &_o, &_f, this->layer_vis, false, 0, true);

        hov_fadeout = true;
        if (this->hov_ent && this->hov_ent->g_id != O_CHUNK) {
            char tooltip_text[512];
            tooltip_text[0] = '\0';
            this->hov_ent->write_tooltip(tooltip_text);
            if (strlen(tooltip_text)) {
                hov_fadeout = false;
                if (!this->hov_text->active) {
                    hov_time = _tms.last_time;
                }
                this->hov_text->active = true;
                this->hov_text->set_text(tooltip_text);
            }
        } else {
            this->hov_ent = 0;

            if (this->hov_text->active && !hov_fadeout) {
                hov_fadeout = true;
                hov_fadeout_time = _tms.last_time + 175000;
            }
        }
    }

    if (this->hov_text->active) {
        float alpha = 0.f;
        float h = this->hov_text->get_num_lines() * this->hov_text->get_max_height();

        if (hov_fadeout) {
            const float x = 1.f-(float)((int64_t)_tms.last_time - (int64_t)hov_fadeout_time) / 175000.f;
            alpha = tclampf(x, 0.f, 1.f);
            this->hov_text->color.a = alpha;
            this->hov_text->outline_color.a = alpha;

            if (alpha <= 0.05f) {
                this->hov_text->active = false;
                tms_debugf("remove entirely");
                hov_fadeout = false;
            }
        } else {
            if (this->hov_ent) {
                float x = this->hov_ent->get_position().x;
                float y = this->hov_ent->get_position().y + (this->hov_ent->height * 1.2f);

                tvec3 dd = tms_camera_project(this->cam, x, y, this->hov_ent->get_layer()*LAYER_DEPTH);
                this->hov_text->set_position(dd.x, dd.y + this->wm->get_margin_y());
            }

            alpha = tclampf((float)(_tms.last_time - hov_time) / 175000.f, 0.f, 1.f);
            this->hov_text->color.a = alpha;
            this->hov_text->outline_color.a = alpha;
        }

        this->add_rounded_square(
                this->hov_text->get_x(),
                this->hov_text->get_y() + (this->hov_text->get_height() / 2.f),
                this->hov_text->get_width(),
                h * 1.05f,
                tvec4f(.2f, .2f, .2f, alpha*0.65f),
                2.f);
    }
#endif

    if (this->state.sandbox && W->is_paused() && !this->state.test_playing) {
        /* do autosave */
        if (_tms.last_time > this->state.last_autosave_try+GAME_AUTOSAVE_INTERVAL && !down[0] && !down[1]) {
            this->state.last_autosave_try = _tms.last_time;

            if (this->state.modified) {
                tms_infof("autosaving");

                if (W->save(SAVE_TYPE_AUTOSAVE)) {
                    this->state.modified = false;
                }
            } else {
                //tms_infof("autosave: nothing modified");
            }
        }
    }

#ifdef PROFILING
    Uint32 ss = SDL_GetTicks();
#endif

    while (W->step()) {
        /* step never returns true here if we're paused */

        if (this->do_drop_interacting) {
            this->drop_interacting();
            this->do_drop_interacting = false;
        }

        if (W->is_adventure()) {
            adventure::step();
        }

        for(int x=0; x<MAX_INTERACTING; ++x) {
            if (interacting[x] && (W->level.type == LCAT_ADVENTURE || interacting[x]->in_dragfield || W->level.flag_active(LVL_DO_NOT_REQUIRE_DRAGFIELD))) {
                edevice *ed = interacting[x]->get_edevice();
                if (mover_joint[x] && ed && current_interacting != -1 && dragging[current_interacting]) ed->recreate_all_cable_joints();

                tmat4_copy(interacting_M[x][(interacting_p[x]+1)%INTERACT_TRAIL_LEN], interacting[x]->M);
                tmat3_copy(interacting_N[x][(interacting_p[x]+1)%INTERACT_TRAIL_LEN], interacting[x]->N);

                //interacting_M[x][(interacting_p[x]+1)%INTERACT_TRAIL_LEN][14] += .01f;
                tmat4_scale(interacting_M[x][(interacting_p[x]+1)%INTERACT_TRAIL_LEN], 1.05f, 1.05f, 1.05f);

                /*
                tmat4_copy(interacting_M[x][(interacting_p[x]+1)%INTERACT_TRAIL_LEN], interacting[x]->M);
                tmat3_copy(interacting_N[x][(interacting_p[x]+1)%INTERACT_TRAIL_LEN], interacting[x]->N);
                */

                tmat4_lerp(
                        interacting_M[x][interacting_p[x]%INTERACT_TRAIL_LEN],
                        interacting_M[x][(interacting_p[x]-1)%INTERACT_TRAIL_LEN],
                        interacting_M[x][(interacting_p[x]+1)%INTERACT_TRAIL_LEN],
                        .5f
                        );

                interacting_p[x] += 1;

                if (adventure::player && interacting_discharge[x]) {
                    interacting_discharge[x]->set_points(
                                        adventure::player->get_position(),
                                        interacting[x]->get_position(),
                                        //interacting[x]->local_to_world(interacting_discharge_lp[x], 0),
                                        adventure::player->get_layer() * LAYER_DEPTH,
                                        interacting[x]->get_layer() * LAYER_DEPTH);
                }

                //b2Vec2 p1 = mover_joint[x]->GetBodyA()->GetWorldPoint(mover_joint[x]->GetAnchorA());
                //b2Vec2 p2 = mover_joint[x]->GetBodyB()->GetWorldPoint(mover_joint[x]->GetAnchorB());

                if (mover_joint[x] && ((mover_joint[x]->GetBodyB()->GetPosition() - mover_joint[x]->GetBodyA()->GetPosition()) - mover_joint[x]->GetLinearOffset()).Length() > 2.f) {
                    tms_debugf("NO!");
                    this->drop_interacting();
                    continue;
                }

                tvec3 tproj;
                W->get_layer_point(this->cam, last_cursor_pos_x, last_cursor_pos_y, 0, &tproj);

                if (interacting[x]->g_id == O_LADDER_STEP) {
                    sel_p_offs.x = 0;
                    sel_p_offs.y = 0;
                    tproj.x *= 2.f;
                    tproj.y *= 2.f;
                    tproj.x = roundf(tproj.x+.5f);
                    tproj.y = roundf(tproj.y);
                    tproj.x /= 2.f;
                    tproj.y /= 2.f;
                    tproj.x -= .25f;
                }

                if (mover_joint[x] && x == current_interacting) {
                    mover_joint[x]->SetLinearOffset(b2Vec2(tproj.x-sel_p_offs.x, tproj.y-sel_p_offs.y));
                }

                if (mover_joint[x] && x != current_interacting) {
                    mover_joint[x]->SetLinearOffset(interacting[x]->get_body(0)->GetPosition());
                }

                if (W->level.type == LCAT_ADVENTURE && adventure::player) {
                    b2Vec2 p1 = adventure::player->get_position();
                    b2Vec2 p2 = interacting[x]->get_position();
                    if ((p2-p1).LengthSquared() > INTERACT_REACH_SQUARED || (x != current_interacting && (adventure::player->is_moving_left() || adventure::player->is_moving_right()))) {
                        this->destroy_mover(x);
                    }
                }

                if (interacting[x] && interacting[x]->is_creature()) {
                    creature *c = static_cast<creature*>(interacting[x]);
                    tms_infof("We are interacting with a creature---");
                    if (W->level.flag_active(LVL_ABSORB_DEAD_ENEMIES)) {
                        /* XXX FIXME TODO: Is the creature in the list for timed absorbs? */
                        //G->timed_absorb(this, W->level.dead_enemy_absorb_time);
                    }
                }
            } else {
                this->destroy_mover(x);
            }
        }

        if (!W->is_paused() && W->is_adventure()) {
            if (this->dropping != -1 && adventure::player->get_num_resources(this->dropping)) {
                int step_diff = W->step_count - this->drop_step;

                float speedup = std::min(this->drop_speed, 5.f);
                if (step_diff % (int)(DROP_FREQUENCY / speedup) == 0) {
                    adventure::player->drop_resource(this->dropping, this->drop_amount, b2Vec2(adventure::player->look_dir*1.25f, .75f));
                    if (!adventure::player->get_num_resources(this->dropping)) this->refresh_inventory_widgets();
                    this->drop_speed += DROP_SPEEDUP;
                }
            }
        }

        if (!W->is_paused() && this->follow_object) {
#ifndef SCREENSHOT_BUILD
            if (this->follow_options.linear) {
                b2Vec2 p = this->follow_object->get_position();
                b2Vec2 offset(0,0);
                if (this->follow_options.offset_mode == 0) { // global
                    offset.Set(this->follow_options.offset.x, this->follow_options.offset.y);
                } else if (this->follow_options.offset_mode == 1) {
                    float cs, sn;
                    tmath_sincos(this->follow_object->get_angle(), &sn, &cs);

                    offset.x = this->follow_options.offset.x*cs - this->follow_options.offset.y*sn;
                    offset.y = this->follow_options.offset.x*sn + this->follow_options.offset.y*cs;
                }

                p += offset;

                this->cam->_position.x = p.x;
                this->cam->_position.y = p.y;
            } else {
                b2Vec2 p = b2Vec2(this->cam->_position.x, this->cam->_position.y);

                p -= this->follow_object->get_position();

                b2Vec2 offset;
                if (this->follow_options.offset_mode == 0) { // global
                    offset.Set(this->follow_options.offset.x, this->follow_options.offset.y);
                } else { // relative
                    float cs, sn;
                    tmath_sincos(this->follow_object->get_angle(), &sn, &cs);

                    offset.x = this->follow_options.offset.x*cs - this->follow_options.offset.y*sn;
                    offset.y = this->follow_options.offset.x*sn + this->follow_options.offset.y*cs;
                }

                p -= offset;

                if (!W->level.flag_active(LVL_DISABLE_CAM_MOVEMENT)) {
                    p -= this->cam_rel_pos;
                }

                if (W->is_adventure() && this->follow_object == adventure::player) {
                    p -= this->adv_rel_pos;
                }

                double dist = p.Length() / 2.f;
                double rdist = dist;
                if (dist > 0.) {
                    dist*=dist*dist*dist*dist;
                    double s = WORLD_STEP / 10000. * G->get_time_mul();

                    if (dist > rdist) dist = rdist;

                    dist *= s;

                    p *= 1./(double)p.Length();
                    p *= dist;
                } else {
                    p.x = 0.f;
                    p.y = 0.f;
                }

                this->cam->_position.x -= p.x;
                this->cam->_position.y -= p.y;
            }
#endif
        }
    }

#ifdef PROFILING
    tms_infof("box2d ms: %d (num bodies: %d, num joints: %d)", SDL_GetTicks() - ss, W->b2->GetBodyCount(), W->b2->GetJointCount());
    ss = SDL_GetTicks();
#endif

    if (W->is_paused() && this->_mode == GAME_MODE_DRAW && drawing) {
        this->handle_draw(0, touch_pos[0].x, touch_pos[0].y);
    }

#define BASE_BORDER_SCROLL_SPEED 1.f

    if (settings["border_scroll_enabled"]->v.b) {
        double fps_mod = (_tms.fps_mean < 60. ? 60. : _tms.fps_mean) / 60.;

        if (cam_move_x[0]) {
            this->cam->_position.x -= BASE_BORDER_SCROLL_SPEED * settings["border_scroll_speed"]->v.f * cam_move_x[0] * 0.25f / fps_mod;
        } else if (cam_move_x[1]) {
            this->cam->_position.x += BASE_BORDER_SCROLL_SPEED * settings["border_scroll_speed"]->v.f * cam_move_x[1] * 0.25f / fps_mod;
        }
        if (cam_move_y[0]) {
            this->cam->_position.y -= BASE_BORDER_SCROLL_SPEED * settings["border_scroll_speed"]->v.f * cam_move_y[0] * 0.25f / fps_mod;
        } else if (cam_move_y[1]) {
            this->cam->_position.y += BASE_BORDER_SCROLL_SPEED * settings["border_scroll_speed"]->v.f * cam_move_y[1] * 0.25f / fps_mod;
        }
    }

#undef BASE_BORDER_SCROLL_SPEED

    float damping = powf(.025f, dt);
    this->cam_vel.x *= damping;
    this->cam_vel.y *= damping;
    this->cam_vel.z *= damping;

#ifndef SCREENSHOT_BUILD
    if (!W->level.flag_active(LVL_DISABLE_CAM_MOVEMENT) || (this->state.sandbox && W->is_paused())) {
        this->cam->_position.x += this->cam_vel.x * dt;
        this->cam->_position.y += this->cam_vel.y * dt;
    }
#endif

    if (!W->level.flag_active(LVL_DISABLE_ZOOM) || this->state.sandbox) {
        this->cam->_position.z += this->cam_vel.z * dt;
    }

    //this->cam_rel_pos.x += this->cam_vel.x * dt;
    //this->cam_rel_pos.y += this->cam_vel.y * dt;

    if (!settings["smooth_cam"]->v.b) {
        this->cam_vel.x = 0.f;
        this->cam_vel.y = 0.f;
    }

    if (!settings["smooth_zoom"]->v.b) {
        this->cam_vel.z = 0.f;
    }

#ifdef DEBUG
    float max_z = 200.f;
#else
    float max_z = 60.f;
#endif
    float min_z = 4.f;

#ifndef SCREENSHOT_BUILD
    if (!W->level.flag_active(LVL_DISABLE_ADVENTURE_MAX_ZOOM) && !W->is_paused() && W->is_adventure() && this->follow_object == adventure::player) {
        max_z = 20.f;
    }
#endif

    if (this->cam->_position.z > max_z) {
        this->cam->_position.z = max_z;
    } else if (this->cam->_position.z < min_z) {
        this->cam->_position.z = min_z;
    }

#ifndef SCREENSHOT_BUILD
    /* Apply camera movement constraints */
    if (W->is_playing() && W->is_adventure() && adventure::player && this->follow_object == adventure::player) {
        if (this->cam_rel_pos.Length() > 0.1f) {
            float adventure_max_cam_dist = 7.f;

            if (this->caveview_size > 0.f) {
                adventure_max_cam_dist = 4.5f;
            }

            const b2Vec2 player_pos = adventure::player->get_position();

            const float x_diff = this->cam->_position.x-adventure::player->get_position().x;
            const float y_diff = this->cam->_position.y-adventure::player->get_position().y;

#ifdef DEBUG
            if (!this->shift_down()) {
#endif

            if (x_diff > adventure_max_cam_dist) {
                float diff = adventure_max_cam_dist-x_diff;
                this->cam->_position.x += diff;
                this->cam_rel_pos.x += diff;
            } else if (x_diff < -adventure_max_cam_dist) {
                float diff = x_diff + adventure_max_cam_dist;
                this->cam->_position.x -= diff;
                this->cam_rel_pos.x -= diff;
            }

            if (y_diff > adventure_max_cam_dist) {
                float diff = adventure_max_cam_dist-y_diff;
                this->cam->_position.y += diff;
                this->cam_rel_pos.y += diff;
            } else if (y_diff < -adventure_max_cam_dist) {
                float diff = y_diff + adventure_max_cam_dist;
                this->cam->_position.y -= diff;
                this->cam_rel_pos.y -= diff;
            }

#ifdef DEBUG /* shift_down */
            }
#endif
        }
    }
#endif

    this->set_caveview_zoom_limits(true);

    this->cam->far = this->cam->_position.z+1.f;
    this->cam->near = this->cam->_position.z-3*LAYER_DEPTH;

    tvec3 l = this->light;

    tvec3 l_to_c = (tvec3){ l.x, l.y, 1.f};

    tvec3_mul(&l, LAYER_DEPTH*3);

    this->ao_cam->set_direction(0,0,-1.f);//TVEC3_INLINE_N(l));
    this->gi_cam->set_direction(0,0,-1.f);//TVEC3_INLINE_N(l));

    l.x += roundf(this->cam->_position.x);
    l.y += roundf(this->cam->_position.y);
    /*l.x += this->cam->_position.x;
    l.y += this->cam->_position.y;*/

    this->gi_cam->width = 1024.f/50.f * fmaxf(this->cam->_position.z/11.f, .1f);
    this->gi_cam->height = 1024.f/50.f * fmaxf(this->cam->_position.z/11.f, .1f);

    //this->ao_cam->width = 1024.f/50.f * this->cam->_position.z/11.f;
    //this->ao_cam->height = 1024.f/50.f * this->cam->_position.z/11.f;

    this->ao_cam->width = this->gi_cam->width;
    this->ao_cam->height = this->gi_cam->height;

    this->gi_cam->set_position(l.x, l.y, l.z);
    this->ao_cam->set_position(l.x, l.y, l.z);
    //this->ao_cam->set_position(l.x-l_to_c.x*3.f, l.y-l_to_c.y*3.f, l.z);
    //this->gi_cam->set_position(0, 0, l.z);
    this->gi_cam->up = (tvec3){0.f, 1.f, 0.f};
    this->ao_cam->up = (tvec3){0.f, 1.f, 0.f};
    //tvec3_normalize(&this->gi_cam->up);

    if (this->state.abo_architect_mode && W->is_paused()) {

        this->cam->enable(TMS_CAMERA_PERSPECTIVE);
        this->cam->width = _tms.window_width;
        this->cam->height = _tms.window_height;
        this->cam->calculate();

        tvec3 dd = tms_camera_project(this->cam, this->cam->_position.x, this->cam->_position.y, LAYER_DEPTH*1.f);

        tvec3 top = tms_camera_unproject(this->cam, 0.f, _tms.window_height, dd.z);

        this->cam->disable(TMS_CAMERA_PERSPECTIVE);
        /*
        this->cam->width = 20.f;
        this->cam->height = 20.f;
        this->cam->owidth = 20.f;
        this->cam->oheight = 20.f;
        */

        this->cam->width = fabsf(top.x - this->cam->_position.x)*2.f;
        this->cam->height = fabsf(top.y - this->cam->_position.y)*2.f;
        this->cam->owidth = _tms.window_width;
        this->cam->oheight = _tms.window_height;
        //this->cam->owidth = _tms.window_width/50.f;
        //this->cam->oheight = _tms.window_height/50.f;
    } else {
        this->cam->enable(TMS_CAMERA_PERSPECTIVE);
        this->cam->width = _tms.window_width;
        this->cam->height = _tms.window_height;
    }

    this->cam->calculate();
    //this->cam->view[10] = .5f;

    tmat4_copy(this->cam->combined, this->cam->projection);
    tmat4_multiply(this->cam->combined, this->cam->view);

    this->gi_cam->calculate();
    this->ao_cam->calculate();

    if (settings["shadow_ao_combine"]->v.b) {
        /* GI */
        float skew[16];
        tmat4_load_identity(skew);
        //tvec3_normalize(&l_to_c);
        skew[8] = -l_to_c.x / this->light.z;
        skew[9] = -l_to_c.y / this->light.z;
        tmat4_multiply(this->gi_cam->projection, skew);

        tmat4_copy(this->gi_cam->combined, this->gi_cam->projection);
        tmat4_multiply(this->gi_cam->combined, this->gi_cam->view);

        /* AO */

        skew[8] = -l_to_c.x / this->light.z;
        skew[9] = -l_to_c.y / this->light.z;
        tmat4_multiply(this->ao_cam->projection, skew);

        tmat4_load_identity(skew);
        //skew[10] = 0;
        skew[14] = -LAYER_DEPTH;
        tmat4_multiply(this->ao_cam->view, skew);

        tmat4_copy(this->ao_cam->combined, this->ao_cam->projection);
        /*this->ao_cam->view[11] = 0.f;
        this->ao_cam->view[10] = 0.f;
        this->ao_cam->view[9] = 0.f;*/
        //this->ao_cam->view[10] = 0.0f;
        //this->ao_cam->view[12] = -1.0f;
        //this->ao_cam->view[14] = -1.f;
        tmat4_multiply(this->ao_cam->combined, this->ao_cam->view);

        /* create the shadow matrix */
        static float shadow_bias[] = {
            .5f, 0, 0, 0,
            0, .5f, 0, 0,
            0, 0, .5f, 0,
            .5f, .5f, .5f, 1.f
        };

        tmat4_copy(this->SMVP, shadow_bias);
        tmat4_multiply(this->SMVP, this->gi_cam->combined);

        float tmp[16];
        tmat4_copy(tmp, this->cam->combined);
        tmat4_invert(tmp);

        tmat4_multiply(this->SMVP, tmp);
    } else {
        /* create SMVP and AOMVP separately */

        /* create the ao matrix */
        static float bias[] = {
            .5f, 0, 0, 0,
            0, .5f, 0, 0,
            0, 0, .5f, 0,
            .5f, .5f, .5f, 1.f
        };

        float inv[16];
        tmat4_copy(inv, this->cam->combined);
        tmat4_invert(inv);

        tmat4_copy(this->AOMVP, bias);
        tmat4_multiply(this->AOMVP, this->ao_cam->combined);
        tmat4_multiply(this->AOMVP, inv);

        /* shadow matrix */
        float skew[16];
        tmat4_load_identity(skew);
        //tvec3_normalize(&l_to_c);
        skew[8] = -l_to_c.x / this->light.z;
        skew[9] = -l_to_c.y / this->light.z;
        tmat4_multiply(this->gi_cam->projection, skew);

        tmat4_copy(this->gi_cam->combined, this->gi_cam->projection);
        tmat4_multiply(this->gi_cam->combined, this->gi_cam->view);
        tmat4_copy(this->SMVP, bias);
        tmat4_multiply(this->SMVP, this->gi_cam->combined);

        tmat4_multiply(this->SMVP, inv);
    }

    /* XXX place this somewhere else? */
    if (W->is_paused() || W->level.type == LCAT_ADVENTURE) {
        this->update_pairs();
    }

    sm::position.x = this->cam->_position.x;
    sm::position.y = this->cam->_position.y;
    sm::step();

    {
        tvec3 dd;
        tvec3 top;
        tvec3 bottom;
        dd = tms_camera_project(this->cam, this->cam->_position.x, this->cam->_position.y, 0.f);

        if (!_tms.emulating_portrait) {
            top = tms_camera_unproject(this->cam, _tms.window_width, _tms.window_height, dd.z);
            bottom = tms_camera_unproject(this->cam, 0.f, 0.f, dd.z);

#if false && defined DEBUG
            const float CHUNK_EXTRA_CULLING = .4f;
            float ff = CHUNK_EXTRA_CULLING*this->cam->_position.z;
            top.x -= ff;
            top.y -= ff;
            bottom.x += ff;
            bottom.y += ff;
#endif
        } else {
            top = tms_camera_unproject(this->cam, 0, _tms.window_height, dd.z);
            bottom = tms_camera_unproject(this->cam, _tms.window_width, 0.f, dd.z);
        }

        float min_x = std::min(bottom.x, top.x);
        float min_y = std::min(bottom.y, top.y);
        float max_x = std::max(bottom.x, top.x);
        float max_y = std::max(bottom.y, top.y);

        bool update = true;
#ifdef DEBUG
        if (this->shift_down()) {
            update = false;
        }
#endif
        if (update) {
            const b2Vec2 b2min(min_x, min_y);
            const b2Vec2 b2max(max_x, max_y);

            /*
            tms_infof("Setting chunk window min: %.2f/%.2f max: %.2f/%.2f", b2min.x, b2min.y, b2max.x, b2max.y);
            tms_infof("top: %.2f/%.2f/%.2f", top.x, top.y, top.z);
            tms_infof("bottom: %.2f/%.2f/%.2f", bottom.x, bottom.y, bottom.z);
            tms_infof("min: %.2f/%.2f", min_x, min_y);
            tms_infof("max: %.2f/%.2f", max_y, max_y);
            tms_infof("cam pos: %.2f/%.2f %.2f", this->cam->_position.x, this->cam->_position.y, this->cam->_position.z);
            tms_infof("dd: %.2f/%.2f/%.2f", dd.x, dd.y, dd.z);

            tms_infof("cam vel: %.2f/%.2f/%.2f",
                    this->cam->_velocity.x,
                    this->cam->_velocity.y,
                    this->cam->_velocity.z
                    );

            tms_infof("cam dir: %.2f/%.2f/%.2f",
                    this->cam->_direction.x,
                    this->cam->_direction.y,
                    this->cam->_direction.z
                    );

            tms_infof("cam lookat: %.2f/%.2f/%.2f",
                    this->cam->_lookat.x,
                    this->cam->_lookat.y,
                    this->cam->_lookat.z
                    );

            tms_infof("cam up: %.2f/%.2f/%.2f",
                    this->cam->up.x,
                    this->cam->up.y,
                    this->cam->up.z
                    );

            tms_infof("view: ");
            tmat4_dump(this->cam->view);

            tms_infof("proj: ");
            tmat4_dump(this->cam->projection);

            tms_infof("combined: ");
            tmat4_dump(this->cam->combined);
            */

            W->cwindow->set(b2min, b2max);
        }
    }

    if (material_factory::background_id == BG_OUTDOOR && this->bgent && this->bgent->scene) {
        tmat4_load_identity(this->bgent->M);
        tmat4_translate(this->bgent->M, roundf(this->cam->_position.x), -100.f, -0.499f);
        tmat4_scale(this->bgent->M, 200, 200, 1.0f);
    } else if (this->grident && this->grident->scene) {
        tmat4_load_identity(this->grident->M);
        tmat4_translate(this->grident->M, roundf(this->cam->_position.x), roundf(this->cam->_position.y), -0.499f);
        tmat4_scale(this->grident->M, 200, 200, .0f);
    }

    this->unset_caveview_zoom_limits();

    //tms_infof("rest of step ms: %d", SDL_GetTicks() - ss);
    return T_OK;
}

void
game::update_ghost_entity(entity *ths)
{
    if (ths->flag_active(ENTITY_CUSTOM_GHOST_UPDATE)) {
        ths->ghost_update();
    } else {
        tmat4_load_identity(ths->M);
        tmat4_translate(ths->M, ths->_pos.x, ths->_pos.y, ths->get_layer()*LAYER_DEPTH);
        tmat4_rotate(ths->M, ths->_angle * (180.f/M_PI), 0, 0, -1);
        tmat3_copy_mat4_sub3x3(ths->N, ths->M);
    }
}

void
game::update_static_entities()
{
    pixel::reset_counter();
    tpixel::reset_counter();

    for (std::set<entity*>::iterator i = this->u_static.begin();
            i != this->u_static.end(); i++) {
        if (enable_culling && tms_graph_is_entity_culled(this->graph, (*i)))
            continue;
        entity *ee = (*i);
        b2Vec2 p = ee->get_position();
        float a = ee->get_angle();

        float c,s;
        tmath_sincos(a, &s, &c);

        ee->M[0] = c;
        ee->M[1] = s;
        ee->M[4] = -s;
        ee->M[5] = c;
        ee->M[12] = p.x;
        ee->M[13] = p.y;
        ee->M[14] = ee->prio * LAYER_DEPTH;

        tmat3_copy_mat4_sub3x3(ee->N, ee->M);
    }

    for (std::set<entity*>::iterator i = this->u_static_custom.begin();
            i != this->u_static_custom.end(); i++) {
        if (enable_culling && tms_graph_is_entity_culled(this->graph, (*i)))
            continue;
        entity *ths = (*i);
        ths->update();
    }
}

void
game::update_entities()
{
#ifdef PROFILING
    Uint32 ss = SDL_GetTicks();
#endif

    for (std::set<entity*>::iterator i = this->u_ghost.begin();
            i != this->u_ghost.end(); i++)
        game::update_ghost_entity(*i);

#ifdef PROFILING
    tms_infof("counts: fastbody=%d, grouped=%d, custom=%d, joints=%d, pjoints=%d, stepable=%d, mstepable=%d", u_fastbody.size(), u_grouped.size(), u_custom.size(), u_joint.size(), u_joint_pivot.size(), W->stepable.size(), W->mstepable.size());
#endif
    for (std::set<entity*>::iterator i = this->u_fastbody.begin();
            i != this->u_fastbody.end(); i++) {
        entity *ths = (*i);
        if (enable_culling && tms_graph_is_entity_culled(this->graph, ths)) {
            continue;
        }

        ths->fastbody_update();
    }

#ifdef PROFILING
    tms_infof("fastbody: %d", SDL_GetTicks() - ss);
    ss = SDL_GetTicks();
#endif

    if (this->do_static_update || W->is_paused()) {
        this->update_static_entities();
    }

    //tms_infof("num grouped: %d", this->u_grouped.size());
    for (std::set<entity*>::iterator i = this->u_grouped.begin();
            i != this->u_grouped.end(); i++) {
        composable *ee = static_cast<composable*>(*i);

        if (enable_culling && tms_graph_is_entity_culled(this->graph, ee))
            continue;

        ee->grouped_update();
    }

#ifdef PROFILING
    tms_infof("grouped: %d", SDL_GetTicks() - ss);
    ss = SDL_GetTicks();
#endif

    /* update all pivot joints */
    for (std::set<entity*>::iterator i = this->u_joint_pivot.begin();
            i != this->u_joint_pivot.end(); i++) {
        connection_entity *e = static_cast<connection_entity*>(*i);
        connection *conn = e->conn;
        b2Vec2 p = conn->e->local_to_world(conn->p, conn->f[0]);
        e->M[12] = p.x;
        e->M[13] = p.y;
        e->M[14] = conn->layer*LAYER_DEPTH + LAYER_DEPTH*.85f;
    }

#ifdef PROFILING
    tms_infof("pivot joints: %d", SDL_GetTicks() - ss);
    ss = SDL_GetTicks();
#endif

    /* update all other joints */
    for (std::set<entity*>::iterator i = this->u_joint.begin();
            i != this->u_joint.end(); i++) {
        connection_entity *e = static_cast<connection_entity*>(*i);
        connection *conn = e->conn;
        b2Vec2 p = conn->e->local_to_world(conn->p, conn->f[0]);
        tmat4_load_identity(e->M);
        tmat4_translate(e->M, p.x, p.y, conn->layer*LAYER_DEPTH + (conn->multilayer ? LAYER_DEPTH*.85f:0));

        tmat4_rotate(e->M, (conn->e->get_angle(conn->f[0])+conn->angle)*(180.f/M_PI)+90.f, 0, 0, -1);

        if (conn->render_type == CONN_RENDER_NAIL) {
            tmat4_rotate(e->M, 90, 1, 0, 0);
        }

        tmat3_copy_mat4_sub3x3(e->N, e->M);

        if (conn->multilayer && conn->type == CONN_WELD) {
            tmat4_scale(e->M, .10f, .10f, 1.0f);
        } else if (conn->render_type == CONN_RENDER_NAIL) {
            tmat4_scale(e->M, 1.f, 1.f, 0.5f);
        } else if (conn->multilayer && conn->render_type == CONN_RENDER_SMALL) {
            tmat4_scale(e->M, 1.f, 1.f, .75f);
        } else if (!conn->multilayer && conn->render_type == CONN_RENDER_SMALL) {
            tmat4_scale(e->M, .5f, .5f, 1.f);
        }
    }

#ifdef PROFILING
    tms_infof("joints: %d", SDL_GetTicks() - ss);
    ss = SDL_GetTicks();
#endif

    if (w_is_enabled()) {
        w_updatec_set = &this->u_custom;
        w_updatec_graph = enable_culling ? this->graph : 0;

        std::set<entity*>::iterator i = this->u_custom.begin();
        for (int x=0; x<w_get_num_workers() && i != this->u_custom.end(); x++) {
            struct wdata_updatec data;
            data.i = x;
            w_run(W_RUN_UPDATEC, &data);
            i++;
        }

        w_wait(-1);
    } else {
        for (std::set<entity*>::iterator i = this->u_custom.begin();
                i != this->u_custom.end(); i++) {
            if (enable_culling && tms_graph_is_entity_culled(this->graph, *i))
                continue;
            tms_entity_update(static_cast<struct tms_entity*>(*i));
        }
    }

#ifdef PROFILING
    tms_infof("custom: %d", SDL_GetTicks() - ss);
    ss = SDL_GetTicks();
#endif
}

static inline void _uncull(struct tms_entity *e)
{
    if (e->scene && tms_scene_is_entity_culled(G->get_scene(), (struct tms_entity*)e)) {
        tms_scene_uncull_entity(G->get_scene(), (struct tms_entity*)e);
        tms_graph_uncull_entity(G->graph, (struct tms_entity*)e);
        tms_graph_uncull_entity(G->gi_graph, (struct tms_entity*)e);
        tms_graph_uncull_entity(G->ao_graph, (struct tms_entity*)e);
    }
}

static inline void
_uncull_full(entity *e)
{
    _uncull(e);

    edevice *ee;
    if ((ee = e->get_edevice())) {
        for (int x=0; x<ee->num_s_in; x++) {
            if (ee->s_in[x].p) {
                _uncull(ee->s_in[x].p);
                if (ee->s_in[x].p->c)
                    _uncull(ee->s_in[x].p->c);
            }
        }
        for (int x=0; x<ee->num_s_out; x++) {
            if (ee->s_out[x].p) {
                _uncull(ee->s_out[x].p);
                if (ee->s_out[x].p->c)
                    _uncull(ee->s_out[x].p->c);
            }
        }
    }

    connection *c = e->conn_ll;
    while (c) {
        if (c->self_ent) _uncull(c->self_ent);
        connection *next = c->get_next(e);

        c = next;
    }

    if (e->gr) _uncull(e->gr);
}

bool
_uncull_handler::ReportFixture(b2Fixture *f)
{
    entity *e;

    if ((e = static_cast<entity*>(f->GetUserData()))) {
        if (e->g_id == O_CHUNK) {
            return true;
        }
        if (W->is_paused() && e->type == ENTITY_PLUG && e->flag_active(ENTITY_IS_OWNED)) {
            _uncull_full(static_cast<entity*>(e->parent));
            _uncull(e);
        } else {
#if 0
            if (f->GetShape()->GetType() == b2Shape::e_circle) {
                tms_infof("unculling circle");
            }
            if (f->GetShape()->GetType() == b2Shape::e_polygon) {
                tms_infof("unculling poly");
            }
#endif
            _uncull_full(e);
        }
    }
    return true;
}

bool
_box_select_handler::ReportFixture(b2Fixture *f)
{
    entity *e;

    if ((e = static_cast<entity*>(f->GetUserData()))) {
        if (e->g_id == O_CHUNK) return true;
        if (e->is_static()) return true;
        if (e->flag_active(ENTITY_IS_PLUG)) {
            e = e->get_property_entity();
        }

        box_select_entities.insert(e);
    }

    return true;
}

int
game::render()
{
    /* only delay on android */
#ifdef TMS_BACKEND_ANDROID
    if (!P.focused) {
        SDL_Delay(500);
    }
#endif

    int ierr;

    if ((ierr = glGetError()) != 0) {
        tms_errorf("gl error %d in game::render begin", ierr);
    }

#ifdef PROFILING
    Uint32 ss = SDL_GetTicks();
#endif

    this->set_caveview_zoom_limits();

    display::reset();
    ledbuffer::reset();
    //tms_assertf((ierr = glGetError()) == 0, "gl error %d after led reset", ierr);
    spritebuffer::reset();
    fluidbuffer::reset();
    linebuffer::reset();
    tms_assertf((ierr = glGetError()) == 0, "gl error %d after linebuffer reset", ierr);
    textbuffer::reset();
    tms_assertf((ierr = glGetError()) == 0, "gl error %d after textbuffer reset", ierr);
    cable::reset_counter();
    //tms_assertf((ierr = glGetError()) == 0, "gl error %d after cable reset", ierr);
    rope::reset_counter();
    plant::reset_counter();
    //

    if (gui_spritesheet::tmp_atlas_modified) {
        tms_texture_upload(&gui_spritesheet::tmp_atlas->texture);
        gui_spritesheet::tmp_atlas_modified = false;
    }

#ifdef PROFILING
    tms_infof("texture upload: %d", SDL_GetTicks() - ss);
    ss = SDL_GetTicks();
#endif

    tms_graph_enable_culling(this->graph, enable_culling);
    tms_graph_enable_culling(this->gi_graph, enable_culling);
    tms_graph_enable_culling(this->ao_graph, enable_culling);

    tvec3 vdist = this->cam->_position;
    vdist.x -= this->last_static_update.x;
    vdist.y -= this->last_static_update.y;
    vdist.z -= this->last_static_update.z;
    //vdist.z =0;

    float dist = tvec3_magnitude(&vdist);

    if (force_static_update == 1 || dist > 5.f) {
        this->force_static_update = 0;
        this->do_static_update = true;
        this->last_static_update = this->cam->_position;
    } else {
        this->do_static_update = false;
    }

    b2AABB aabb;

    if (enable_culling) {
        tms_scene_cull_all(this->get_scene());
        tms_graph_cull_all(this->graph);
        tms_graph_cull_all(this->gi_graph);
        tms_graph_cull_all(this->ao_graph);

        if (W->is_paused()) {
            if (this->selection.e)
                _uncull_full(this->selection.e);
            entity *e = this->get_pending_ent();
            if (e) _uncull(e);
        }

        tms_graph_uncull_entity(this->graph, display::get_full_entity());
        tms_graph_uncull_entity(this->graph, ledbuffer::get_entity());
        tms_graph_uncull_entity(this->graph, linebuffer::get_entity());
        tms_graph_uncull_entity(this->graph, linebuffer::get_entity2());
        tms_graph_uncull_entity(this->graph, textbuffer::get_entity());
        tms_graph_uncull_entity(this->graph, textbuffer::get_entity2());
        tms_graph_uncull_entity(this->graph, fluidbuffer::get_entity());
        tms_graph_uncull_entity(this->graph, spritebuffer::get_entity());
        tms_graph_uncull_entity(this->graph, spritebuffer::get_entity2());

        _uncull(W->cwindow);
        _uncull(pixel::get_entity(0));
        _uncull(pixel::get_entity(1));
        _uncull(pixel::get_entity(2));
        _uncull(tpixel::get_entity(0));
        _uncull(tpixel::get_entity(1));
        _uncull(tpixel::get_entity(2));

        _uncull(cable::get_entity());
        _uncull(rope::get_entity());

        if (this->grident->scene) {
            _uncull(this->grident);
        }

        if (this->bgent->scene) {
            if (!this->state.abo_architect_mode)
                _uncull(this->bgent);
            else {
                /* only uncull the borders */
                for (int x=0; x<this->bgent->num_children; x++) {
                    tms_graph_uncull_entity(this->graph, this->bgent->children[x]);
                }
            }
        }

        tvec3 dd = tms_camera_project(this->cam, this->cam->_position.x, this->cam->_position.y, 0.f);
        tvec3 projs[4];
        projs[0] = tms_camera_unproject(this->cam, _tms.window_width, _tms.window_height, dd.z);
        projs[1] = tms_camera_unproject(this->cam, _tms.window_width, 0,                  dd.z);
        projs[2] = tms_camera_unproject(this->cam, 0,                 _tms.window_height, dd.z);
        projs[3] = tms_camera_unproject(this->cam, 0,                 0,                  dd.z);

        float minx = projs[0].x;
        float maxx = projs[0].x;
        float miny = projs[0].y;
        float maxy = projs[0].y;

        for (int x=1; x<4; ++x) {
            if (projs[x].x <= minx) {
                minx = projs[x].x;
            } else if (projs[x].x >= maxx) {
                maxx = projs[x].x;
            }

            if (projs[x].y <= miny) {
                miny = projs[x].y;
            } else if (projs[x].y >= maxy) {
                maxy = projs[x].y;
            }
        }

        /*
        float s = 10.f;
        spritebuffer::add(minx, miny, 1.4f, 1.f, 0.f, 1.f, 1.f, s, s, 0, 0.f);
        spritebuffer::add(maxx, maxy, 1.4f, 0.f, 1.f, 0.f, 1.f, s, s, 0, 0.f);
        */

        float ddd = do_static_update ? 8.f : 3.f;
        maxx += ddd; maxy += ddd;
        minx -= ddd; miny -= ddd;

        aabb.lowerBound.Set(minx, miny);
        aabb.upperBound.Set(maxx, maxy);

        W->b2->QueryAABB(&uncull_handler, aabb);

        if (this->caveview_size > 0.f && adventure::player) {
            this->caveview_pos = (tvec2){
                        roundf(adventure::player->get_position().x*2.f)/2.f - .25f,
                        roundf((adventure::player->get_position().y+.25f)*2.f)/2.f
                        //adventure::player->get_position().x*2.f/2.f,
                        //(adventure::player->get_position().y+.5f)*2.f/2.f
            };
            W->cwindow->recreate_caveview_texture(
                        roundf(adventure::player->get_position().x*2.f)/2.f,
                        roundf((adventure::player->get_position().y+.25f)*2.f)/2.f,
                        adventure::player->get_position().x,
                        (adventure::player->get_position().y+.25f)
                    );
        }
    }

#ifdef PROFILING
    tms_infof("culling: %d", SDL_GetTicks() - ss);
    ss = SDL_GetTicks();
#endif

    {
        int num_p = W->b2->GetParticleCount();
        b2Vec2 *p = W->b2->GetParticlePositionBuffer();
        float32 *a = W->b2->GetParticleAccumulationBuffer();
        void **u = W->b2->GetParticleUserDataBuffer();

#if 0
        b2ParticleContact *cc = W->b2->m_particleSystem.m_contactBuffer;
        int32 num_contacts = W->b2->m_particleSystem.m_contactCount;

        for (int x=0; x<num_contacts; x++) {
            int32 a = cc[x].indexA;
            int32 b = cc[x].indexB;
            float z = (float)(int)((uintptr_t)u[a]) * LAYER_DEPTH;

            linebuffer::add(p[a].x, p[a].y, z, p[b].x, p[b].y, z,
                    1.0f, 1.f, 1.f, cc[x].weight*2.f,
                    1.0f, 1.f, 1.f, cc[x].weight*2.f,
                    0.1f, 0.1f
                    );
        }
#endif

        for (int x=0; x<num_p; x++) {
            float pressure = fabsf((1.0f-a[x]*.55f) - .2f)*2.75f;
            float z = (float)(int)((uintptr_t)u[x]) * LAYER_DEPTH;

            //pressure = tclampf(1.f-pressure, .10f, .5f);
            pressure = .25f - pressure*.125f;
            pressure = tclampf(pressure, .05, .25f);
            float size = fmaxf(.05f+fminf(a[x]*.22f, .4f), .075f);
            /*fluidbuffer::add(p[x].x,p[x].y, z,
                    pressure*1.05f, pressure*1.02f, pressure, .33f, size, size);*/
            fluidbuffer::add(p[x].x,p[x].y, z,
                    pressure,
                    size, size);
        }
    }

#ifdef PROFILING
    tms_infof("particle sprite generation: %d", SDL_GetTicks() - ss);
    ss = SDL_GetTicks();
#endif

    this->update_entities();

#ifdef PROFILING
    tms_infof("update: %d", SDL_GetTicks() - ss);
    ss = SDL_GetTicks();
#endif

    for (std::set<entity*>::iterator i = this->u_effects.begin();
            i != this->u_effects.end(); i++) {
        if (enable_culling) {
            if ((*i)->cull_effects_method == CULL_EFFECTS_BY_POSITION) {
                b2Vec2 p = (*i)->get_position();

                if (!(p.x > aabb.lowerBound.x && p.x < aabb.upperBound.x
                            && p.y > aabb.lowerBound.y && p.y < aabb.upperBound.y)) {
                    continue;
                }
            } else if ((*i)->cull_effects_method == CULL_EFFECTS_DISABLE) {
            } else {
                if (tms_graph_is_entity_culled(this->graph, *i))
                    continue;
            }
        }

        (*i)->update_effects();
    }

    if (W->is_playing() && W->is_adventure()) {
        entity *last_host = 0;
        float y_offset = 0.f;
        tvec3 color;

        if (G->caveview_size >= 1.f) {
            color = (tvec3){1.f, 1.f, 1.f};
        } else {
            color = (tvec3){0.f, 0.f, 0.f};
        }

        for (std::multimap<entity*, struct loot>::iterator it = this->loots.begin();
                it != this->loots.end();) {
            entity *host = it->first;
            struct loot &l = it->second;

            if (!host) {
                continue;
            }

            if (host != last_host) {
                y_offset = 0.f;
            }

            l.scale += _tms.dt * 4.f;
            l.life -= _tms.dt * 2.f;

            if (l.scale >= 1.f) {
                l.scale = 1.f;
            }

            b2Vec2 pos = host->get_position();
            pos.y += y_offset + host->height + 0.25f;

            char tmp[64];
            snprintf(tmp, 63, "+%d %s", l.num, l.name);

            textbuffer::add_text(tmp, font::xlarge,
                    pos.x,
                    pos.y,
                    host->get_layer()*LAYER_DEPTH + LAYER_DEPTH/2.f+0.01,
                    color.r, color.g, color.b, fminf(l.scale, l.life),
                    0.0045 * powf(.0001f+fminf(l.scale, l.life), .125f)); // LOOT_SCALE

            last_host = host;

            y_offset += (0.3f * fminf(l.scale, l.life));

            if (l.life <= 0.f) {
                this->loots.erase(it++);
            } else {
                ++ it;
            }
        }
    }

    this->render_tt();

#ifdef PROFILING
    tms_infof("effects: %d", SDL_GetTicks() - ss);
    ss = SDL_GetTicks();
#endif

    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render before upload", ierr);
    display::upload();
    //tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render after display::upload", ierr);
    ledbuffer::upload();
    //tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render after ledbuffer::upload", ierr);
    spritebuffer::upload();
    fluidbuffer::upload();
    //tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render after spritebuffer::upload", ierr);
    linebuffer::upload();
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render after linebuffer::upload", ierr);
    textbuffer::upload();
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render after textbuffer::upload", ierr);
    rope::upload_buffers();
    //tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render after rope::upload", ierr);
    cable::upload_buffers();
    //tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render after cable::upload", ierr);
    pixel::upload_buffers();
    tpixel::upload_buffers();
    polygon::upload_buffers();
    plant::upload_buffers();
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render after plant::upload_buffers", ierr);

    GLenum err;
    do {
        err = glGetError();
    } while (err != GL_NO_ERROR);


#ifdef PROFILING
    tms_infof("upload: %d", SDL_GetTicks() - ss);
    ss = SDL_GetTicks();
#endif

#ifdef DEBUG
# ifdef TMS_BACKEND_MOBILE
    G->show_numfeed(_tms.fps_mean);
# else
    if (W->step_count % 120 == 0) {
        char fps[64];
        sprintf(fps, "Principia - FPS: %f (%f)", _tms.fps, _tms.fps_mean);
        SDL_SetWindowTitle((SDL_Window*)_tms._window, fps);
    }
# endif
#endif

    //glClear(GL_COLOR_BUFFER_BIT);
    //return T_OK;
    //tms_infof("RENDER");

    glViewport(0,0,_tms.opengl_width, _tms.opengl_height);


    //glFinish();
    //tms_infof("buffer shit %u", SDL_GetTicks() - start_time);

    //tms_assertf(glGetError() == 0, "error before gi render");
    /* create shadow map */
    glDisable(GL_BLEND);

    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render before shadow render", ierr);
    if (settings["enable_shadows"]->v.b) {
        if (!this->state.abo_architect_mode || !W->is_paused()) {
            if (settings["shadow_quality"]->v.i == 2) {
                P.best_variable_in_the_world2 = 0;
                this->gi_graph->render(this->gi_cam, this);

#define JKL .04f

                glColorMask(1,0,0,0);
                P.best_variable_in_the_world2 = 1337;
                tmat4_translate(this->gi_cam->view, JKL, JKL, 0.f);
                this->gi_graph->render(this->gi_cam, this);

                glColorMask(0,0,1,0);
                tmat4_translate(this->gi_cam->view, -JKL*2.f, -JKL*2.f, 0.f);
                this->gi_graph->render(this->gi_cam, this);

#undef JKL
            } else {
                P.best_variable_in_the_world2 = 0;
                this->gi_graph->render(this->gi_cam, this);
            }
        } else {
            tms_fb_bind(tms_pipeline_get_framebuffer(1));
            glClearColor(1.f, 1.f, 1.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);
            tms_fb_unbind(tms_pipeline_get_framebuffer(1));
        }

        /*
        if (settings["blur_shadow_map"]->v.b) {
            tms_fb_swap_blur3x3(tms_pipeline_get_framebuffer(1));
        }
        */
    }

    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render after shadow render", ierr);

    //tms_debugf("cam before render ao %f", this->cam->_position.z);
    if (settings["enable_ao"]->v.b) {
        glDisable(GL_BLEND);
        this->ao_graph->render(this->ao_cam, this);

        if (tms_pipeline_get_framebuffer(3)->width == 512) {
            tms_fb_swap_blur5x5(tms_pipeline_get_framebuffer(3));
        } else {
            tms_fb_swap_blur3x3(tms_pipeline_get_framebuffer(3));
        }
    }
#ifndef TMS_USE_GLES
    if (settings["postprocess"]->v.b) {
        //tms_assertf(glGetError() == 0, "error before main fb bind");
        tms_fb_bind(this->main_fb);
        //tms_assertf(glGetError() == 0, "error after main fb bind");
    }
#endif
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render after shadow/ao", ierr);
    glDisable(GL_BLEND);

    //ss = SDL_GetTicks();

    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render before bg", ierr);

#ifndef TMS_USE_GLES
    if (settings["gamma_correct"]->v.b && !settings["postprocess"]->v.b) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
#endif

    if (this->state.abo_architect_mode) {
        glClearColor(.25f, .25f, .25f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        if (material_factory::background_id == BG_COLORED_SPACE) {
            glClearColor(this->state.bg_color.r, this->state.bg_color.g, this->state.bg_color.b, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        } else if (material_factory::background_id == BG_SPACE || material_factory::background_id == BG_OUTDOOR) {
            tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render before space bg", ierr);
            glClearColor(4.f/255.f, 11.f/255.f, 19/255.f, 1.f);
            glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);

            if (material_factory::background_id == BG_OUTDOOR) {
                /* responsible for rendering the horizon background */
                struct tms_fb fb;
                fb.num_textures = 1;
                fb.toggle = 0;
                fb.fb_texture[0][0] = tex_bg->gl_texture;

                float pp = 0.f;

                float ff = this->cam->far;
                float nn = this->cam->near;

#define HORIZON_DIST 2500.f

                this->cam->far = HORIZON_DIST+1.f;
                this->cam->near = -10.f;
                this->cam->calculate();

                tvec3 v1 = tms_camera_project(this->cam, this->cam->_position.x, 0.f, -HORIZON_DIST);
                pp = (v1.y / this->cam->height) * 2.f;

                /* reset camera */
                this->cam->far = ff;
                this->cam->near = nn;
                this->cam->calculate();

                tms_program_bind(trans_program);
                glUniform2f(trans_program_shift_loc, 0.f,0* (this->cam->_position.y > 0? .01f : .075f) * this->cam->_position.y);
                glUniform2f(trans_program_pos_loc, 0.f, pp);
                glUniform2f(trans_program_poslower_loc, 0.f, pp);
                tms_fb_render(&fb, trans_program);

                //fb.fb_texture[0][0] = tex_bedrock->gl_texture;

                v1 = tms_camera_project(this->cam, this->cam->_position.x, -1.f, -.5f);
                float pp2 = (v1.y / this->cam->height) * 2.f;
                tms_program_bind(trans_program);
                //glUniform2f(trans_program_shift_loc, 0.f, 0.f);
                glUniform2f(trans_program_shift_loc, 0.f,0* (this->cam->_position.y > 0? .01f : .075f) * this->cam->_position.y);
                glUniform2f(trans_program_pos_loc, 0.f, pp2-2.f);
                glUniform2f(trans_program_poslower_loc, 0.f, pp);
                tms_fb_render(&fb, trans_program);
            } else {
                tms_texture_render(tex_bg);
            }
        } else {
            glClearColor(.05f, .05f, .05f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
    }

    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render after bg", ierr);

    {
        /* render primary scene */
        P.best_variable_in_the_world = 0;
        tms_graph_set_sort_callback(this->graph, TMS_SORT_PRIO, render_next_prio);
        this->graph->post_fn = post_fn;

        //tms_debugf("cam before render %f", this->cam->_position.z);
        this->graph->render(this->cam, this);
    }

    glDepthMask(true);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    if (W->is_paused() || W->level.type == LCAT_ADVENTURE) {
        this->render_selected_entity();
    }

    this->render_highlighted();
    this->render_trails();

    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!W->is_paused() && W->level.type == LCAT_ADVENTURE) {
        adventure::render();
    }

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);

    {
        /* render half-hidden layers */
        P.best_variable_in_the_world = 1337;
        tms_graph_set_sort_callback(this->graph, TMS_SORT_PRIO, render_hidden_prio);
        this->graph->post_fn = 0;
        this->graph->render(this->cam, this);
        tms_assertf((ierr = glGetError()) == 0, "gl error %d after graph render cam?", ierr);
    }

    {
        /* render foreground */
        P.best_variable_in_the_world = 0;
        P.best_variable_in_the_world3 = 1;
        tms_graph_set_sort_callback(this->graph, TMS_SORT_PRIO, render_foreground);
        this->graph->render(this->cam, this);
        P.best_variable_in_the_world = 1337;
        P.best_variable_in_the_world3 = 0;
        tms_assertf((ierr = glGetError()) == 0, "gl error %d after render foreground", ierr);
    }

#ifndef TMS_USE_GLES
    if (settings["gamma_correct"]->v.b && !settings["postprocess"]->v.b) {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }
#endif

    glDepthMask(true);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    tms_ddraw_set_matrices(this->dd, this->cam->view, this->cam->projection);
    //tms_ddraw_line3d(this->dd, 0, 0, 0, this->light.x*2.f, this->light.y*2.f, this->light.z*2.f);

#ifndef TMS_USE_GLES
    if (settings["postprocess"]->v.b) {
        tms_fb_unbind(this->main_fb);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

#if 0
        if (settings["gamma_correct"]->v.b) {
            tms_fb_render(this->main_fb, prg_output);
        } else {
            tms_fb_render(this->main_fb, _tms_fb_copy_program);
        }
#endif

        if (settings["gamma_correct"]->v.b) {
            glEnable(GL_FRAMEBUFFER_SRGB);
        }

        tms_fb_render(this->main_fb, _tms_fb_copy_program);

        if (settings["gamma_correct"]->v.b) {
            glDisable(GL_FRAMEBUFFER_SRGB);
        }

        glBindTexture(GL_TEXTURE_2D, this->main_fb->fb_texture[0][0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef TMS_USE_GLEW
        if (GLEW_VERSION_3_0) { /* XXX */
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            glGenerateMipmapEXT(GL_TEXTURE_2D);
        }
#else
        glGenerateMipmap(GL_TEXTURE_2D);
#endif

        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        tms_fb_render_to(this->main_fb, this->bloom_fb, prg_brightpass);
        tms_fb_swap_blur5x5(this->bloom_fb);
        glEnable(GL_BLEND);
        glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
        glDisable(GL_DEPTH_TEST);

        glBindTexture(GL_TEXTURE_2D, this->bloom_fb->fb_texture[this->bloom_fb->toggle][0]);
#ifdef TMS_USE_GLEW
        if (GLEW_VERSION_3_0) { /* XXX */
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            glGenerateMipmapEXT(GL_TEXTURE_2D);
        }
#else
        glGenerateMipmap(GL_TEXTURE_2D);
#endif

        for (int x=0; x<7; x++) {
            glBlendColor(1.f, 1.f, 1.f, .05f);
            glBindTexture(GL_TEXTURE_2D, this->bloom_fb->fb_texture[this->bloom_fb->toggle][0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, x);
            tms_fb_render(this->bloom_fb, _tms_fb_copy_program);
        }
        glBlendColor(1.f, 1.f, 1.f, 1.f);
        glBindTexture(GL_TEXTURE_2D, this->bloom_fb->fb_texture[this->bloom_fb->toggle][0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glDisable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        tms_assertf((ierr = glGetError()) == 0, "gl error %d after postprocess", ierr);
    }
#endif

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    this->render_controls_help();

    this->render_gui();

    tms_assertf((ierr = glGetError()) == 0, "gl error %d after gui", ierr);
    if (this->state.sandbox && W->is_paused()) {
        glScissor(0, 0, _tms.window_width - this->get_menu_width(), _tms.window_height);
        glEnable(GL_SCISSOR_TEST);
    }

    glEnable(GL_BLEND);
    tms_ddraw_set_matrices(this->dd, this->cam->view, this->cam->projection);

    if (W->is_paused() || W->level.type == LCAT_ADVENTURE) {
        if (this->get_mode() == GAME_MODE_SELECT_SOCKET) {
            // FIXME
            this->render_socksel();
        }
    }

    glBindTexture(GL_TEXTURE_2D, gui_spritesheet::atlas->texture.gl_texture);

    glDisable(GL_DEPTH_TEST);

    if (W->is_paused() || W->level.type == LCAT_ADVENTURE) {
        if (this->get_mode() == GAME_MODE_SELECT_CONN_TYPE) {
            this->render_conn_types();
        } else {
            this->render_connections();
        }

    }

    if (W->is_paused()) {
        if (this->get_mode() == GAME_MODE_CONN_EDIT) {
            this->render_existing_connections();
        }

        this->render_selected_connection();
    }

    if (W->is_playing() && W->is_adventure()) {
        this->pending_activators.clear();

        if (adventure::player) {
            for (std::set<activator*>::iterator it = W->activators.begin();
                    it != W->activators.end(); it++) {
                activator *act = *it;
                if (act->active
                        && adventure::player->activators.find(act) != adventure::player->activators.end()
                        && adventure::player->cur_activator != act) {
                    this->pending_activators.push_back(act);
                }
            }

            std::sort(this->pending_activators.begin(), this->pending_activators.end(), game_sorter::distance_to_creature(adventure::player));
        }

        this->render_activators();
    }

    if (this->get_mode() != GAME_MODE_CONN_EDIT && W->is_paused() && this->state.sandbox) {
        tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.0f, 1.0f);
        for (std::set<entity*>::iterator it = this->locked.begin();
                it != this->locked.end(); ++it) {
            b2Vec2 p = (*it)->get_position();

            tms_ddraw_sprite_r(this->dd, gui_spritesheet::get_sprite(S_LOCK),
                    p.x, p.y,
                    .375f, .375f,
                    cos((double)_tms.last_time/100000.) * 8.f
                    );
        }
    }

    if (this->get_mode() == GAME_MODE_DEFAULT && W->is_adventure() && adventure::player) {
        if (!(settings["tutorial"]->v.u32 & TUTORIAL_REPAIR_STATION)) {
            this->render_help_icon(W->repair_stations, OFFS_REPAIR_STATION);
        }
    }

#ifndef SCREENSHOT_BUILD
    if (this->get_mode() != GAME_MODE_CONN_EDIT && ((W->is_puzzle() && W->is_paused()) || (!W->is_puzzle() && !W->is_paused()))) {
        this->render_starred();
    }
#endif

    if (!W->is_paused() && this->state.sandbox && !this->state.test_playing) {
        for (std::set<er*>::iterator i = this->errors.begin();
                i != this->errors.end(); i++) {
            er *error = static_cast<er*>(*i);

            if (error->e && error->alpha > 0.1f) {
                tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.0f, error->alpha);

                error->alpha -= _tms.dt * this->get_time_mul();
                entity *e = error->e;

                b2Vec2 p = e->get_position();

                tms_ddraw_sprite_r(this->dd, gui_spritesheet::get_sprite(S_ERROR),
                        p.x,
                        p.y,
                        .35f, .35f, cos((double)_tms.last_time/100000.) * 4.f * this->get_time_mul());
            }
        }
    }

    for (int x=0; x<NUM_CA; x++) {
        if (this->ca[x].life >= 0.f && this->ca[x].life <= 1.f) {
            tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.f, 1.f);
            tms_ddraw_lcircle(this->dd, this->ca[x].p.x, this->ca[x].p.y, .75f*this->ca[x].life, .75f*this->ca[x].life);
            tms_ddraw_set_color(this->dd, 0.0f, 0.0f, 0.0f, 1.f);
            tms_ddraw_lcircle(this->dd, this->ca[x].p.x, this->ca[x].p.y, .72f*this->ca[x].life, .72f*this->ca[x].life);
            this->ca[x].life += _tms.dt*6.f * this->ca[x].dir;
        }
    }
    tms_assertf((ierr = glGetError()) == 0, "gl error %d after something3", ierr);

    if (this->get_mode() == GAME_MODE_QUICK_PLUG) {
        if (this->selection.e != 0) {
            if (this->selection.e->flag_active(ENTITY_IS_EDEVICE)) {
                tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.0f, 1.f);
                b2Vec2 p1 = this->selection.e->get_position();
                b2Vec2 p2 = this->sel_p_ent ? this->sel_p_ent->get_position() : b2Vec2(touch_quickplug_pos.x, touch_quickplug_pos.y);

                float z = this->sel_p_ent ? this->sel_p_ent->get_layer() : this->selection.e->get_layer();

                tms_ddraw_line3d(this->dd, p1.x, p1.y, this->selection.e->get_layer()*LAYER_DEPTH,
                        p2.x, p2.y, z*LAYER_DEPTH);
            }
        }
    }

    pscreen::render();

    if (W->is_paused()) {
        if (this->get_mode() == GAME_MODE_MULTISEL) {
            if (this->multi.box_select == 0 && !this->selection.m) {
                if (this->multi.import) {
                    tms_ddraw_set_color(this->dd, 0.2f, 0.2f, 0.2f, 0.5f);
                    tms_ddraw_square(this->dd,
                            this->multi.cursor.x,
                            this->multi.cursor.y,
                            this->multi.cursor_size.x, this->multi.cursor_size.y);

                    tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.0f, 0.5f);
                    tms_ddraw_lsquare(this->dd,
                            this->multi.cursor.x,
                            this->multi.cursor.y,
                            this->multi.cursor_size.x, this->multi.cursor_size.y);
                }
            } else if (this->multi.box_select >= 3) {
                tms_ddraw_set_color(this->dd, 0.0f, 1.0f, 1.0f, 1.0f);

                tms_ddraw_line(this->dd,
                        begin_box_select.x, begin_box_select.y,
                        begin_box_select.x, end_box_select.y);
                tms_ddraw_line(this->dd,
                        begin_box_select.x, begin_box_select.y,
                        end_box_select.x, begin_box_select.y);
                tms_ddraw_line(this->dd,
                        end_box_select.x, end_box_select.y,
                        begin_box_select.x, end_box_select.y);
                tms_ddraw_line(this->dd,
                        end_box_select.x, end_box_select.y,
                        end_box_select.x, begin_box_select.y);

                tms_ddraw_circle(this->dd, begin_box_select.x, begin_box_select.y, .1f, .1f);
                tms_ddraw_circle(this->dd, end_box_select.x, end_box_select.y, .1f, .1f);
            }
        }

        if (this->selection.e != 0) {
            if (this->selection.e->flag_active(ENTITY_IS_EDEVICE)) {
                edevice *ed = this->selection.e->get_edevice();

                if (this->get_mode() == GAME_MODE_QUICK_PLUG) {
                    tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.0f, 1.f);
                    b2Vec2 p1 = this->selection.e->get_position();
                    b2Vec2 p2 = this->sel_p_ent ? this->sel_p_ent->get_position() : b2Vec2(touch_quickplug_pos.x, touch_quickplug_pos.y);

                    float z = this->sel_p_ent ? this->sel_p_ent->get_layer() : this->selection.e->get_layer();

                    tms_ddraw_line3d(this->dd, p1.x, p1.y, this->selection.e->get_layer()*LAYER_DEPTH,
                            p2.x, p2.y, z*LAYER_DEPTH);
                }

                for (int x=0; x<ed->num_s_in; ++x) {
                    if (ed->s_in[x].p) {
                        plug_base *p = ed->s_in[x].p->get_other();

                        if (p) {
                            tms_ddraw_set_color(this->dd, 1.0f, 0.7f, 0.7f, 0.95f);
                            b2Vec2 p1 = ed->s_in[x].p->get_position();
                            b2Vec2 p2 = p->get_position();
                            tms_ddraw_line3d(this->dd, p1.x, p1.y, CABLE_Z + this->selection.e->get_layer()*LAYER_DEPTH, p2.x, p2.y, CABLE_Z + this->selection.e->get_layer()*LAYER_DEPTH);
                        }
                    }
                }
                for (int x=0; x<ed->num_s_out; ++x) {
                    if (ed->s_out[x].p) {
                        plug_base *p = ed->s_out[x].p->get_other();

                        if (p) {
                            tms_ddraw_set_color(this->dd, 0.7f, 1.0f, 0.7f, 0.95f);
                            b2Vec2 p1 = ed->s_out[x].p->get_position();
                            b2Vec2 p2 = p->get_position();
                            tms_ddraw_line3d(this->dd, p1.x, p1.y, CABLE_Z + this->selection.e->get_layer()*LAYER_DEPTH, p2.x, p2.y, CABLE_Z + this->selection.e->get_layer()*LAYER_DEPTH);
                        }
                    }
                }
            }

#ifdef DEBUG
            if (settings["debug"]->v.b && this->selection.e->gr) {
                /* draw the group centroid */
                tms_ddraw_set_color(this->dd, 1.0f, 0.0f, 0.0f, 1.0f);
                tms_ddraw_circle(this->dd, this->selection.e->gr->get_position().x, this->selection.e->gr->get_position().y, .25f, .25f);
            }
#endif

            switch (this->selection.e->g_id) {
                case O_CURSOR_FIELD: {
                    // Draw click area for Cursor field object

                    cursorfield *g = static_cast<cursorfield*>(this->selection.e);
                    tms_ddraw_set_color(this->dd, 0.0f, 0.0f, 1.0f, 1.0f);

                    b2PolygonShape sh;

                    b2Vec2 vertices[4] = {
                        g->local_to_world(b2Vec2(g->properties[0].v.f, g->properties[1].v.f), 0),
                        g->local_to_world(b2Vec2((g->properties[2].v.f), g->properties[1].v.f), 0),
                        g->local_to_world(b2Vec2((g->properties[2].v.f), (g->properties[3].v.f)), 0),
                        g->local_to_world(b2Vec2(g->properties[0].v.f, (g->properties[3].v.f)), 0)
                    };
                    for (int x=0; x<4; ++x) {
                        tms_ddraw_line3d(this->dd,
                                vertices[x].x, vertices[x].y, this->selection.e->get_layer()*LAYER_DEPTH,
                                vertices[(x+1)%4].x, vertices[(x+1)%4].y, this->selection.e->get_layer()*LAYER_DEPTH
                            );
                    }
                } break;
                case O_FLUID: {
                    // Draw bounding box for fluid particles to spawn

                    fluid *g = static_cast<fluid*>(this->selection.e);
                    tms_ddraw_set_color(this->dd, 0.0f, 0.0f, 1.0f, 1.0f);
                    b2Vec2 vertices[4] = {
                        g->local_to_world(b2Vec2(g->properties[0].v.f, g->properties[1].v.f), 0),
                        g->local_to_world(b2Vec2(-(g->properties[0].v.f), g->properties[1].v.f), 0),
                        g->local_to_world(b2Vec2(-(g->properties[0].v.f), -(g->properties[1].v.f)), 0),
                        g->local_to_world(b2Vec2(g->properties[0].v.f, -(g->properties[1].v.f)), 0)
                    };
                    for (int x=0; x<4; ++x) {
                        tms_ddraw_line3d(this->dd,
                                vertices[x].x, vertices[x].y, this->selection.e->get_layer()*LAYER_DEPTH,
                                vertices[(x+1)%4].x, vertices[(x+1)%4].y, this->selection.e->get_layer()*LAYER_DEPTH
                            );
                    }
                } break;
                case O_SHAPE_EXTRUDER: {
                    // Draw bounding box of Shape extruder

                    ghost *g = static_cast<ghost*>(this->selection.e);
                    if (g->conn_ll) {
                        tms_ddraw_set_color(this->dd, 0.0f, 0.0f, 1.0f, 1.0f);
                        composable *other = static_cast<composable*>(g->c.o);

                        if (other) {
                            float w = other->get_width();
                            float h = other->height;

                            b2PolygonShape sh;

                            b2Vec2 vertices[4] = {
                                other->local_to_world(b2Vec2(w+g->properties[0].v.f, h+g->properties[1].v.f), 0),
                                other->local_to_world(b2Vec2(-(w+g->properties[2].v.f), h+g->properties[1].v.f), 0),
                                other->local_to_world(b2Vec2(-(w+g->properties[2].v.f), -(h+g->properties[3].v.f)), 0),
                                other->local_to_world(b2Vec2(w+g->properties[0].v.f, -(h+g->properties[3].v.f)), 0)
                            };
                            for (int x=0; x<4; ++x) {
                                tms_ddraw_line3d(this->dd,
                                        vertices[x].x, vertices[x].y, this->selection.e->get_layer()*LAYER_DEPTH,
                                        vertices[(x+1)%4].x, vertices[(x+1)%4].y, this->selection.e->get_layer()*LAYER_DEPTH
                                    );
                            }
                        }
                    }
                } break;
                case O_PROXIMITY_SENSOR: {
                    // Draw range of proximity sensor

                    proximitysensor *sensor = static_cast<proximitysensor*>(this->selection.e);
                    static const int32 num_v = 4;
                    tms_ddraw_set_color(this->dd, 1.0f, 0.0f, 0.0f, 0.8f);
                    b2Vec2 vertices[num_v];

                    for (int x=0; x<num_v; ++x) {
                        vertices[x] = this->selection.e->local_to_world(sensor->get_sensor_shape().GetVertex(x), 0);
                    }

                    for (int x=0; x<num_v; ++x) {
                        tms_ddraw_line3d(this->dd,
                                vertices[x].x, vertices[x].y, this->selection.e->get_layer()*LAYER_DEPTH,
                                vertices[(x+1)%num_v].x, vertices[(x+1)%num_v].y, this->selection.e->get_layer()*LAYER_DEPTH
                            );
                    }
                } break;
                case O_ID_FIELD:
                case O_OBJECT_FIELD:
                case O_TARGET_SETTER: {
                    objectfield *of = static_cast<objectfield*>(this->selection.e);

                    tms_ddraw_set_color(this->dd, 1.0f, 0.0f, 0.0f, 0.8f);
                    static const int32 num_v = 4;
                    b2Vec2 vertices[num_v];

                    for (int x=0; x<num_v; ++x) {
                        vertices[x] = this->selection.e->local_to_world(of->sensor_shape.GetVertex(x), 0);
                    }

                    for (int x=0; x<num_v; ++x) {
                        tms_ddraw_line3d(this->dd,
                                vertices[x].x, vertices[x].y, this->selection.e->get_layer()*LAYER_DEPTH,
                                vertices[(x+1)%num_v].x, vertices[(x+1)%num_v].y, this->selection.e->get_layer()*LAYER_DEPTH
                            );
                    }
                } break;

                case O_DRAGFIELD: {
                    // Draw

                    dragfield *df = static_cast<dragfield*>(this->selection.e);

                    tms_ddraw_set_color(this->dd, 1.0f, 0.0f, 0.0f, 0.8f);

                    tms_ddraw_lcircle(this->dd,
                            df->get_position().x, df->get_position().y,
                            df->sensor_shape.m_radius, df->sensor_shape.m_radius);
                } break;
                case O_FAN: { // "Oh fan!"
                    // Draw lines showing the exhaust of selected Fan object

                    fan *f = static_cast<fan*>(this->selection.e);

                    #define NUM_RAYS    5
                    #define RAY_LENGTH  10.f
                    #define FAN_WIDTH   1.f
                    #define FAN_LINE_OFFSET (((x / ((float)NUM_RAYS - 1.f)) * FAN_WIDTH) - (FAN_WIDTH / 2.f))

                    tms_ddraw_set_color(this->dd, 1.0f, 0.0f, 0.0f, 0.4f);
                    for (int x=0; x<NUM_RAYS; x++) {
                        float a = f->get_angle();
                        b2Vec2 angle;
                        tmath_sincos(a, &angle.y, &angle.x);

                        b2Vec2 r = f->get_position();
                        b2Vec2 dir = f->local_to_world(b2Vec2(0.f,  RAY_LENGTH), 0);

                        r.x   += angle.x * FAN_LINE_OFFSET;
                        r.y   += angle.y * FAN_LINE_OFFSET;
                        dir.x += angle.x * FAN_LINE_OFFSET;
                        dir.y += angle.y * FAN_LINE_OFFSET;

                        float l = f->get_layer()*LAYER_DEPTH;

                        tms_ddraw_line3d(this->dd, r.x, r.y, l, dir.x, dir.y, l);
                    }
                } break;
            }
        }
    }
    tms_assertf((ierr = glGetError()) == 0, "gl error %d after something2", ierr);

    if (this->selection.e != 0) {
        entity *e = this->selection.e;
        if (e->flag_active(ENTITY_IS_RESIZABLE)) {
            this->render_shape_resize();
        }
    }

    if (this->state.sandbox && this->state.edev_labels && settings["render_edev_labels"]->v.b) {
        this->render_edev_labels();
        tms_assertf((ierr = glGetError()) == 0, "gl error %d after edev labels", ierr);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    std::vector<entity*> hp_occurences;

    for (int x=0; x<NUM_HP; x++) {
        struct hp *h = &this->hps[x];
        if (h->time <= 0.f || !h->e) {
            continue;
        } else if (h->time <= 0.25f && h->regen && h->percent < 1.f) {
            h->time = 0.25f;
            h->e->entity_health += 0.25f;
            h->percent = tclampf(h->e->entity_health / ENTITY_MAX_HEALTH, 0.f, 1.f);
        }

        h->time -= _tms.dt*.2f;

        float y_offset = h->e->height*2.f + (BAR_Y_OFFSET * std::count(hp_occurences.begin(), hp_occurences.end(), h->e));
        float alpha = tclampf(h->time / .25f, 0.f, 1.f) * .75f;

        this->draw_entity_bar(h->e, h->percent, y_offset, h->color, alpha);

        hp_occurences.push_back(h->e);

        if (h->e->flag_active(ENTITY_IS_CREATURE)) {
            if (static_cast<creature*>(h->e)->creature_flag_active(CREATURE_IS_ZOMBIE)) {
                h->color = (tvec3){0.4f, 0.4f, .4f};
            }
        }
    }

//#define SHOW_MOOD_DATA
#define SHOW_RAYCASTS

#if defined(SHOW_RAYCASTS) && defined(DEBUG)
    if (settings["debug"]->v.b) {
        std::set<struct game_debug_line*>::iterator it = this->debug_lines.begin();
        while (it != this->debug_lines.end()) {
            struct game_debug_line *gdl = *it;

            gdl->life -= G->timemul(8);
            float a = tclampf((gdl->life/1000.f)*.9f, 0.f, 1.f);

            tms_ddraw_set_color(this->dd, gdl->r, gdl->g, gdl->b, a);
            tms_ddraw_line(this->dd, gdl->x1, gdl->y1, gdl->x2, gdl->y2);

            if (gdl->life < 0) {
                this->debug_lines.erase(it++);
            } else {
                ++it;
            }
        }
    } else {
        this->debug_lines.clear();
    }
#endif

#ifdef SHOW_MOOD_DATA
    if (!W->is_paused()) {
        for (std::map<uint32_t, entity*>::iterator it = W->all_entities.begin();
                it != W->all_entities.end(); ++it) {
            if (it->second->flag_active(ENTITY_IS_ROBOT)) {
                robot_base *rob = static_cast<robot_base*>(it->second);

                float y = 1.65f;
                tvec3 color;
                for (int x=0; x<NUM_MOODS; ++x) {
                    switch (x) {
                        case MOOD_ANGER:
                            color = tvec3f(.95f, .33f, .33f);
                            break;

                        case MOOD_BRAVERY:
                            color = tvec3f(.33f, .33f, .95f);
                            break;

                        case MOOD_FEAR:
                            color = tvec3f(.95f, .95f, .33f);
                            break;

                        default:
                            color = tvec3f(.3f, .3f, .3f);
                            break;
                    }

                    this->draw_entity_bar(rob, rob->mood.get(x), y, color);
                    y += BAR_Y_OFFSET;
                }

                {
                    if (rob->roam) {
                        float v = tclampf((float)rob->logic_timer/rob->logic_timer_max, 0.f, 1.f);
                        color = tvec3f(.33f, .95f, .95f);
                        this->draw_entity_bar(rob, v, y, color);
                        y += BAR_Y_OFFSET;
                    }
                }
            }
        }
    }
#endif

    tms_ddraw_set_matrices(this->dd, this->cam->view, this->cam->projection);
    tms_assertf((ierr = glGetError()) == 0, "gl error %d after hp rendering", ierr);

#if 0
    if (this->state.waiting && !this->state.finished) {
        glEnable(GL_BLEND);
        tms_ddraw_set_color(this->get_surface()->ddraw, 0.f, 0.f, 0.f, .25f);
        tms_ddraw_square(this->get_surface()->ddraw, _tms.window_width/2.f, _tms.window_height/2.f, _tms.window_width, _tms.window_height);
    }
#endif

    if (!this->state.ending) {
        if (this->state.fade > 0.f) {
            glEnable(GL_BLEND);
            tms_ddraw_set_color(this->get_surface()->ddraw, 0.f, 0.f, 0.f, this->state.fade);
            tms_ddraw_square(this->get_surface()->ddraw, _tms.window_width/2.f, _tms.window_height/2.f, _tms.window_width, _tms.window_height);
            this->state.fade -= _tms.dt * GAME_FADE_SPEED/2.f;
        }
    } else {
        if (this->state.fade >= 1.f) {
            switch (this->state.end_action) {
                default: case GAME_END_PROCEED:
                    this->proceed();
                    break;

                case GAME_END_WARP:
                    P.add_action(ACTION_WARP, (void*)(uintptr_t)this->state.end_warp);
                    break;
            }
        }
        glEnable(GL_BLEND);
        tms_ddraw_set_color(this->get_surface()->ddraw, 0.f, 0.f, 0.f, this->state.fade);
        tms_ddraw_square(this->get_surface()->ddraw, _tms.window_width/2.f, _tms.window_height/2.f, _tms.window_width, _tms.window_height);
        this->state.fade += _tms.dt * GAME_FADE_SPEED;
    }
    tms_assertf((ierr = glGetError()) == 0, "gl error %d after fade stuff", ierr);

    glDisable(GL_BLEND);

    if (this->state.sandbox && W->is_paused()) {
        glDisable(GL_SCISSOR_TEST);
        glScissor(0, 0, _tms.window_width, _tms.window_height);
    }
    tms_assertf((ierr = glGetError()) == 0, "gl error %d after something", ierr);

#ifdef DEBUG
    if (settings["debug"]->v.b) {
        W->draw_debug(this->cam);
    }
#endif

    if (settings["enable_shadows"]->v.b && settings["swap_shadow_map"]->v.b
            && tms_pipeline_get_framebuffer(1)) {
        tms_fb_swap(tms_pipeline_get_framebuffer(1), 0);
        tms_assertf((ierr = glGetError()) == 0, "gl error %d after fb swap shadows", ierr);
    }

    /*
    if (settings["enable_ao"]->v.b && settings["swap_ao_map"]->v.b
            && tms_pipeline_get_framebuffer(3))
        tms_fb_swap(tms_pipeline_get_framebuffer(3), 0);
        */

    if (this->main_fb) {
        tms_fb_swap(this->main_fb, 0);
        tms_assertf((ierr = glGetError()) == 0, "gl error %d after fb swap main", ierr);
    }

#ifdef PROFILING
    tms_infof("render: %d", SDL_GetTicks() - ss);
#endif

    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render end", ierr);

    this->unset_caveview_zoom_limits();

    return T_OK;
}

void
game::add_error(entity *e, uint8_t error_type/*=ERROR_NONE*/, const char *message/*=0*/)
{
    std::set<er*>::iterator it = this->errors.begin();
    for (; it != this->errors.end(); ++it) {
        er *error = static_cast<er*>(*it);
        if (error->e == e) {
            error->alpha = 0.9f;
            return;
        }
    }

    er *error = new er();
    error->e = e;
    error->type = error_type;
    if (message) error->message = strdup(message);
    this->errors.insert(error);

    if (!this->wdg_error->surface && settings["render_gui"]->is_true()) {
        this->wdg_error->add();

        this->wm->rearrange();
    }
}

void
game::clear_errors()
{
    std::set<er*>::iterator it = this->errors.begin();
    for (; it != this->errors.end(); ++it) {
        delete *it;
    }

    this->errors.clear();
}

void
game::reselect()
{
    this->selection.select(this->selection.e, this->selection.b, this->selection.offs, this->selection.frame, true);
}

void
game::render_tt()
{
    for (int x=0; x<MAX_TUTORIAL_TEXTS; x++) {
        if (this->tt[x].life > 0.f) {
            b2Vec2 p;
            if (this->tt[x].e) {
                p = this->tt[x].e->get_position() + this->tt[x].pos;
            } else {
                p = b2Vec2(this->cam->_position.x, this->cam->_position.y) + this->tt[x].pos;
            }
            textbuffer::add_text(tutorial_texts[this->tt[x].what], font::medium,
                    p.x,p.y,
                    2.9f,
                    1.f, 1.f, 1.f, this->tt[x].life < .25f ? (this->tt[x].life/.25f) : 1.f,
                    .015,
                    ALIGN_CENTER,
                    ALIGN_CENTER,
                    true
                    );
            this->tt[x].life -= _tms.dt;
        }
    }
}

/**
 * Render any pending activators.
 * The pending activators will be cleared just before the pending activators
 * are refreshed.
 *
 * This currently happens in game::render()
 **/
void
game::render_activators(void)
{
    if (!this->pending_activators.empty()) {
        float mv[16];
        float p[16];
        tmat4_copy(p, this->cam->projection);

        int layer = 0;
        if (adventure::player) {
            layer = adventure::player->get_layer();
        }

        int x = 0;

        for (std::deque<activator*>::iterator it = this->pending_activators.begin();
                it != this->pending_activators.end(); ++it) {
            activator* act = *it;

            tmat4_copy(mv, this->cam->view);
            tmat4_translate(mv, 0, 0, layer*LAYER_DEPTH+((LAYER_DEPTH/2.f)));
            tms_ddraw_set_matrices(this->dd, mv, p);

            b2Vec2 activator_pos = act->get_activator_pos();
            float radius = act->get_activator_radius();

            tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.0f, .5f + .35f * cos((double)_tms.last_time/200000.));
            tms_ddraw_circle(this->dd,
                    activator_pos.x, activator_pos.y,
                    radius * 0.2f, radius * 0.2f);

            tms_ddraw_set_color(this->dd, 0.2f, 0.2f, 0.2f, .5f + .35f * cos((double)_tms.last_time/200000.));
            tms_ddraw_circle(this->dd,
                    activator_pos.x, activator_pos.y,
                    radius * 0.15f, radius * 0.15f);

#ifdef TMS_BACKEND_PC
            if (x < NUM_ACTIVATOR_BINDINGS && !adventure::player->cur_activator) {
                tvec3 proj;
                proj = tms_camera_project(this->cam, activator_pos.x, activator_pos.y, layer*LAYER_DEPTH+LAYER_DEPTH/2.f);
                this->add_text(activator_texts[x], proj.x, proj.y);
            }

            x++;
#endif
        }
    }
}

void
game::render_starred(void)
{
    if (settings["render_gui"]->is_false()) {
        return;
    }

    tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.0f, .75f+cos((double)_tms.last_time/100000.) * .25f);
    for (std::set<entity*>::iterator i = this->starred.begin();
    i != this->starred.end(); i++) {

        b2Vec2 p = (*i)->get_position();

        if ((*i)->type == ENTITY_CABLE) {
            p = ((cable*)(*i))->p[0]->get_position() + ((cable*)(*i))->p[1]->get_position();
            p *= .5f;
        } else if ((*i)->g_id == O_COMMAND_PAD) {
            p.x += .25f;
        }

        float s = _tms.xppcm * 0.01f;

        float max_z = 60.f;

        if (!W->level.flag_active(LVL_DISABLE_ADVENTURE_MAX_ZOOM) && !W->is_paused() && W->is_adventure() && this->follow_object == adventure::player) {
            max_z = 20.f;
        }

        s *= tclampf((this->cam->_position.z*2.f) / max_z, 1.f, 2.f);

        tms_ddraw_sprite_r(this->dd, gui_spritesheet::get_sprite(S_STAR),
        p.x,
        p.y,
        s, s,
        cos((double)_tms.last_time/100000.) * 16.f);
    }
}

void
game::render_controls_help()
{
#ifdef TMS_BACKEND_PC
    if (!this->render_controls) {
        if (this->tex_controls) {
            delete this->tex_controls;
            this->tex_controls = 0;
        }

        return;
    }

    if (!this->tex_controls) {
        this->tex_controls = new tms::texture();
        this->tex_controls->format = GL_RGBA;
        this->tex_controls->load("data/textures/controls.png");
        this->tex_controls->upload();
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    struct tms_sprite tmp;
    tmp.bl = tvec2f(0.f, 0.f);
    tmp.tr = tvec2f(1.f, 1.f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->tex_controls->gl_texture);

    int width = this->tex_controls->width;
    int height = this->tex_controls->height;

    if (this->tex_controls->width*1.25f >= _tms.window_width) {
        float mod = _tms.window_width / (this->tex_controls->width*1.25f);
        width *= mod;
        height *= mod;
    }

    tms_ddraw_sprite(this->get_surface()->ddraw, &tmp,
            _tms.window_width/2.f, _tms.window_height/2.f,
            width, height);

    this->add_text("Press <ESCAPE> or click anywhere on the screen to close this window.",
            font::xmedium,
            _tms.window_width/2.f, _tms.window_height/2.f + height/2.f + _tms.xppcm*0.1f,
            TV_WHITE,
            true,
            ALIGN_CENTER, ALIGN_BOTTOM);
#endif
}

void
game::render_existing_connections(void)
{
    float mv[16];
    float p[16];
    tmat4_copy(p, this->cam->projection);

    tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.0f, .95f);

    for (std::set<connection*>::iterator i = W->connections.begin();
            i != W->connections.end(); i++) {
        connection *c = *i;
        if (this->state.sandbox || (c->e->is_moveable() || c->o->is_moveable())) {
            b2Vec2 base = c->e->local_to_world(b2Vec2(c->p.x, c->p.y), c->f[0]);

            tmat4_copy(mv, this->cam->view);
            tmat4_translate(mv, base.x, base.y, c->layer*LAYER_DEPTH+.5f+((LAYER_DEPTH/2.f)*c->multilayer));
            tms_ddraw_set_matrices(this->dd, mv, p);
            tms_ddraw_sprite(this->dd, c->type == CONN_PIVOT ? gui_spritesheet::get_sprite(S_ATTACH_ROTARY): gui_spritesheet::get_sprite(S_ATTACH_RIGID), 0, 0, .75f, .75f);
        }
    }
}

void
game::render_connections(void)
{
    c_map::iterator i = this->pairs.begin();

    if (this->get_mode() == GAME_MODE_SELECT_SOCKET
        || this->get_mode() == GAME_MODE_QUICK_PLUG) {
        return;
    }

    float mv[16];
    float p[16];
    tmat4_copy(p, this->cam->projection);

    tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.0f, .75f + .15f * cos((double)_tms.last_time/50000.));
    int n=0;
    for (;i != this->pairs.end(); i++, n++) {
        connection *c = i->second;

        tvec3 dd = tms_camera_project(this->cam, this->cam->_position.x, this->cam->_position.y,c->layer*LAYER_DEPTH+((LAYER_DEPTH/2.f)*c->multilayer));
        tvec3 v1 = tms_camera_unproject(this->cam, 0.f, 0.f, dd.z);
        tvec3 v2 = tms_camera_unproject(this->cam, _tms.xppcm*.5f, 0.f, dd.z);

        float w = v2.x-v1.x;

        tmat4_copy(mv, this->cam->view);
        tmat4_translate(mv, 0, 0, c->layer*LAYER_DEPTH+((LAYER_DEPTH/2.f)*c->multilayer));
        tms_ddraw_set_matrices(this->dd, mv, p);

        //tms_ddraw_circle(this->dd, c->p.x, c->p.y, .25f, .25f);
        tms_ddraw_sprite_r(this->dd, gui_spritesheet::get_sprite(S_ATTACH), c->p.x, c->p.y, w, w, cos((double)_tms.last_time/100000.) * 16.f);

#ifdef TMS_BACKEND_PC
        if (n < 6) {
            tvec3 proj;
            proj = tms_camera_project(this->cam, c->p.x, c->p.y, c->layer*LAYER_DEPTH+((LAYER_DEPTH/2.f)*c->multilayer));
            this->add_glyph(font::medium->get_glyph('F'+n), proj.x, proj.y);
        }
#endif
    }
}

/**
 * Render the connection type selection menu
 **/
void
game::render_conn_types()
{
    float mv[16];
    float p[16];

    if (this->cs_timer < 1.5f) {
        this->cs_timer += _tms.dt*15.f;
        if (this->cs_timer > 1.5f) this->cs_timer = 1.5f;
    }

    tmat4_copy(p, this->cam->projection);
    tmat4_copy(mv, this->cam->view);
    tmat4_translate(mv, 0, 0, this->cs_conn->layer*LAYER_DEPTH+((LAYER_DEPTH/2.f)*this->cs_conn->multilayer));
    tms_ddraw_set_matrices(this->dd, mv, p);

    tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.0f, 1.f*this->cs_timer);
    //tms_ddraw_set_color(this->dd, 0.0f, 0.0f, 0.0f, 1.f*this->cs_timer);
    //

    tvec3 dd = tms_camera_project(this->cam, this->cam->_position.x, this->cam->_position.y,this->cs_conn->layer*LAYER_DEPTH+((LAYER_DEPTH/2.f)*this->cs_conn->multilayer));
    tvec3 v1 = tms_camera_unproject(this->cam, 0.f, 0.f, dd.z);
    tvec3 v2 = tms_camera_unproject(this->cam, _tms.xppcm*.5f, 0.f, dd.z);

    float w = v2.x-v1.x;

    tms_ddraw_line(this->dd,
            this->cs_conn->p.x, this->cs_conn->p.y,
            this->cs_conn->p.x - CSCONN_OFFSX*w * this->cs_timer, this->cs_conn->p.y + CSCONN_OFFSY*w * this->cs_timer
            );
    tms_ddraw_line(this->dd,
            this->cs_conn->p.x, this->cs_conn->p.y,
            this->cs_conn->p.x + CSCONN_OFFSX*w * this->cs_timer, this->cs_conn->p.y + CSCONN_OFFSY*w * this->cs_timer
            );

    tms_ddraw_sprite(this->dd, gui_spritesheet::get_sprite(S_ATTACH_RIGID),
            this->cs_conn->p.x - CSCONN_OFFSX*w * this->cs_timer, this->cs_conn->p.y + CSCONN_OFFSY*w * this->cs_timer,
            w * this->cs_timer, w * this->cs_timer
            );
    tms_ddraw_sprite(this->dd, gui_spritesheet::get_sprite(S_ATTACH_ROTARY),
            this->cs_conn->p.x + CSCONN_OFFSX*w * this->cs_timer, this->cs_conn->p.y + CSCONN_OFFSY*w * this->cs_timer,
            w * this->cs_timer, w * this->cs_timer
            );

    float ss = (1.f - cs_timer);
    if (ss < .25f) ss = .25f;
    tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.0f, ss);
    tms_ddraw_sprite(this->dd, gui_spritesheet::get_sprite(S_ATTACH),
            this->cs_conn->p.x, this->cs_conn->p.y,
            w * ss, w * ss
            );
    tms_ddraw_set_color(this->dd, 1.0f, 1.0f, 1.0f, 1.f);
}

void
game::select_socksel(int x)
{
    if (!this->ss_plug && !this->ss_asker) {
        /* ss_plug is 0 if we're unplugging */
        this->perform_socket_action(x);

    } else {
        cable *cc = 0;
        int ctype = this->ss_socks[x]->ctype;

        if (this->ss_asker) {
            /* quickplug */
            cc = (cable*)of::create(ctype == CABLE_RED ? 34 : (ctype == CABLE_BLACK ? 33 : 35));
            //W->add(cc);
            cc->construct();
            cc->on_pause();
            cc->p[0]->entity::set_layer(this->ss_edev->get_entity()->get_layer());

            /* find the first socket of this type in the asker and connect one of the plugs to it */
#if 0
            isocket *found = 0;
            if (this->ss_edev->get_socket_dir(this->ss_socks[x]) == CABLE_IN) {
                for (int x=0; x<this->ss_asker->num_s_out; x++) {
                    if (this->ss_asker->s_out[x].p == 0 && this->ss_asker->s_out[x].ctype == ctype) {
                        found = &this->ss_asker->s_out[x];
                        break;
                    }
                }
            } else {
                for (int x=0; x<this->ss_asker->num_s_in; x++) {
                    if (this->ss_asker->s_in[x].p == 0 && this->ss_asker->s_in[x].ctype == ctype) {
                        found = &this->ss_asker->s_in[x];
                        break;
                    }
                }
            }
#endif

            this->ss_plug = cc->p[1];

            /*
            if (found) {
                cc->p[1]->entity::set_layer(this->ss_asker->get_entity()->get_layer());
                //cc->p[0]->connect(this->ss_asker, found);
            } else {
                tms_infof("wtf");
                this->remove_entity(cc);
                W->remove(cc);
                delete cc;
                return;
            }
            */
        }

        bool success = false;

        if (this->ss_quickplug_step2) {
            W->add(this->ss_plug->c);
            this->add_entity(this->ss_plug->c);
        }

        int status;
        if (cc && this->ss_asker) {
            cc->connect(static_cast<plug*>(this->ss_plug), this->ss_edev, this->ss_edev->get_socket_index(this->ss_socks[x]));
            status = T_OK;
        } else {
            status = this->ss_plug->connect(this->ss_edev, this->ss_socks[x]);
        }

        switch (status) {
            case T_OK:
                success = true;
                this->add_ca(-1, this->ss_edev->get_entity()->local_to_world(this->ss_socks[x]->lpos, 0));
                this->selection.disable();
                /*if (W->is_paused()) {
                    this->selection.select(this->ss_edev->get_entity(), 0, (tvec2){0,0}, 0, true);
                }*/
                if (this->ss_asker) {
                    this->open_socket_selector(cc->p[0], this->ss_asker);
                    this->ss_quickplug_step2 = true;
                    return;
                }
                break;

            case 1:
                ui::message("The plugs of a cable cannot be more than one layer apart.");
                break;

            case 2:
                ui::message("Unable to connect the plug to the object.");
                break;

            case 3:
                ui::message("Incompatible cable types.");
                break;
        }

        if (!success && this->ss_asker) {
            /* TODO: remove created cable */
            tms_infof("ASKER MUST BE REMOVED");
        }
    }
    this->ss_quickplug_step2 = false;
    this->set_mode(GAME_MODE_DEFAULT);
    this->state.modified = true;
}

/**
 * Render the socket selection menu, and
 * store the sockets in the game object so we can identify
 * clicks on them later.
 **/
void
game::render_socksel()
{
    if (!this->ss_edev) {
        tms_errorf("object we're trying to render sockets for isn't there anymore, abort");
        this->set_mode(GAME_MODE_DEFAULT);
        return;
    } else if (!this->ss_edev->get_entity()) {
        tms_errorf("entity for the object we're trying to render sockets for isn't there anymore, abort!!");
        return;
    }

    float p[16], mv[16];
    tmat4_copy(p, this->cam->projection);
    tmat4_copy(mv, this->cam->view);
    tmat4_translate(mv, 0, 0, this->ss_edev->get_entity()->get_layer()*LAYER_DEPTH);
    tms_ddraw_set_matrices(this->dd, mv, p);

    glEnable(GL_BLEND);
    this->ss_anim += _tms.dt*10.f;
    if (this->ss_anim > 1.f) this->ss_anim = 1.f;
    this->ss_num_socks = 0;

    if (this->ss_asker) {
        /* quickplug */
        int asker_masks[3];
        for (int t=0; t<3; t++) {
            asker_masks[t] = this->ss_asker->get_inout_mask(t);
        }

        for (int x=0; x<this->ss_edev->num_s_in; x++) {
            //if ((asker_masks[this->ss_edev->s_in[x].ctype] & target_masks[this->ss_edev->s_in[x].ctype])
            if ((asker_masks[this->ss_edev->s_in[x].ctype] & CABLE_OUT)
                    && this->ss_edev->s_in[x].p == 0) {
                this->ss_socks[this->ss_num_socks] = &this->ss_edev->s_in[x];
                this->ss_num_socks ++;
            }
        }

        for (int x=0; x<this->ss_edev->num_s_out; x++) {
            //if ((asker_masks[this->ss_edev->s_out[x].ctype] & target_masks[this->ss_edev->s_out[x].ctype])
            if ((asker_masks[this->ss_edev->s_out[x].ctype] & CABLE_IN)
                    && this->ss_edev->s_out[x].p == 0) {
                this->ss_socks[this->ss_num_socks] = &this->ss_edev->s_out[x];
                this->ss_num_socks ++;
            }
        }
    } else if (this->ss_plug) {
        switch (this->ss_plug->plug_type) {
            case PLUG_PLUG:
                {
                    plug *p = static_cast<plug*>(this->ss_plug);
                    int mask = p->c->get_inout_mask(p->c->ctype);

                    if (mask & CABLE_IN) {
                        for (int x=0; x<this->ss_edev->num_s_in; x++) {
                            /* only add the socket if it's available */
                            if (this->ss_edev->s_in[x].p == 0 && this->ss_edev->s_in[x].ctype == p->c->ctype) {
                                this->ss_socks[this->ss_num_socks] = &this->ss_edev->s_in[x];
                                this->ss_num_socks ++;
                            }
                        }
                    }
                    if (mask & CABLE_OUT) {
                        for (int x=0; x<this->ss_edev->num_s_out; x++) {
                            if (this->ss_edev->s_out[x].p == 0 && this->ss_edev->s_out[x].ctype == p->c->ctype) {
                                this->ss_socks[this->ss_num_socks] = &this->ss_edev->s_out[x];
                                this->ss_num_socks ++;
                            }
                        }
                    }
                }
                break;

            case PLUG_JUMPER:
            case PLUG_RECEIVER:
                for (int x=0; x<this->ss_edev->num_s_in; x++) {
                    /* only add the socket if it's available */
                    if (this->ss_edev->s_in[x].p == 0 && this->ss_edev->s_in[x].ctype == CABLE_RED) {
                        this->ss_socks[this->ss_num_socks] = &this->ss_edev->s_in[x];
                        this->ss_num_socks ++;
                    }
                }
                break;

            case PLUG_MINI_TRANSMITTER:
                for (int x=0; x<this->ss_edev->num_s_out; x++) {
                    if (this->ss_edev->s_out[x].p == 0 && this->ss_edev->s_out[x].ctype == CABLE_RED) {
                        this->ss_socks[this->ss_num_socks] = &this->ss_edev->s_out[x];
                        this->ss_num_socks ++;
                    }
                }
                break;

            default:
                tms_errorf("Unknown plug type: %d", this->ss_plug->plug_type);
                break;
        }
    } else {
        /* disconnect */
        for (int x=0; x<this->ss_edev->num_s_in; x++) {
            if (this->ss_edev->s_in[x].p != 0 && (this->state.sandbox || (W->level.type == LCAT_ADVENTURE && !W->is_paused()) || this->ss_edev->s_in[x].p->is_moveable() || (this->ss_edev->s_in[x].p->c && this->ss_edev->s_in[x].p->c->is_moveable()))) {
                this->ss_socks[this->ss_num_socks] = &this->ss_edev->s_in[x];
                this->ss_num_socks ++;
            }
        }
        for (int x=0; x<this->ss_edev->num_s_out; x++) {
            if (this->ss_edev->s_out[x].p != 0 && (this->state.sandbox || (W->level.type == LCAT_ADVENTURE && !W->is_paused()) || this->ss_edev->s_out[x].p->is_moveable() || (this->ss_edev->s_out[x].p->c && this->ss_edev->s_out[x].p->c->is_moveable()))) {
                this->ss_socks[this->ss_num_socks] = &this->ss_edev->s_out[x];
                this->ss_num_socks ++;
            }
        }
    }

    if (!this->ss_num_socks) {
        tms_infof("no sockets, disabling socksel");
        this->ss_quickplug_step2 = false;
        this->set_mode(GAME_MODE_DEFAULT);
        this->ss_num_socks = 0;
        return;
    } else if (this->ss_num_socks == 1 && (!this->ss_plug || this->ss_quickplug_step2)) {
        /* automatically select the only socket */

        this->state.modified = true;
        if (!this->ss_asker && !this->ss_quickplug_step2) {
            this->perform_socket_action(0);

            this->ss_quickplug_step2 = false;
            this->set_mode(GAME_MODE_DEFAULT);
            this->ss_num_socks = 0;
            this->ss_edev = 0;
        } else {
            this->select_socksel(0);
        }
        return;
    }

    bool call_opengl_stuff = true;
    const float base_scale = 0.005f;

    for (int x=0; x<this->ss_num_socks; ++x) {
        tms_ddraw_set_color(this->dd, MENU_WHITE_FI, 1.f);
        b2Vec2 pos = this->ss_socks[x]->lpos;
        b2Vec2 ipos = pos;

        int i = this->ss_edev->get_socket_index(this->ss_socks[x]);

        //ipos *= 1.f/ipos.Length();
        //
        if (!this->ss_edev->scaleselect) {
            float ia = atan2f(ipos.y, ipos.x);
            ia += this->ss_socks[x]->abias;

            tmath_sincos(ia, &ipos.y, &ipos.x);

            pos = this->ss_edev->get_entity()->local_to_world(pos, 0);

            ipos *= 1.5f * this->ss_anim;
        } else
            ipos *= this->ss_edev->scalemodifier * this->ss_anim;

        ipos = this->ss_edev->get_entity()->local_to_world(ipos, 0);

        tms_ddraw_set_color(this->dd, MENU_WHITE_FI, 1.f);
        tms_ddraw_lcircle(this->dd, ipos.x, ipos.y, .375f * this->ss_anim, .375f * this->ss_anim);

        if (i < 0x80) {
        } else {
            //tms_ddraw_set_color(this->dd, 0.15f, 0.15f, 0.15f, 1.f);
        }

        if (!this->ss_edev->scaleselect) {
            tms_ddraw_line(this->dd, pos.x, pos.y, ipos.x, ipos.y);
        }

        tms_ddraw_circle(this->dd, ipos.x, ipos.y, .375f * this->ss_anim, .375f * this->ss_anim);

        p_text *tx_tag;
        p_text *tx_sid;
        if (this->ss_socks[x]->tag != SOCK_TAG_NONE) {
            tx_tag = gui_spritesheet::tx_sock_tag[this->ss_socks[x]->tag];
        } else {
            tx_tag = gui_spritesheet::tx_sock_tag[1];
        }

        float wh = 2.5;
        tvec3 bgc;

        switch (this->ss_socks[x]->ctype) {
            default: bgc = MENU_WHITE_F; break;
            case CABLE_RED: bgc = (tvec3){.3f, .3f, .3f}; break;
            case CABLE_BLUE: bgc = (tvec3){0.7f, .7f, 1.f}; break;
        }

        if (this->ss_socks[x]->tag != SOCK_TAG_NONE) {
            tms_ddraw_set_color(this->dd, TVEC3_INLINE(bgc), 1.f);
            tms_ddraw_square(this->dd,
                    ipos.x,
                    ipos.y-.275f,
                    .375f * this->ss_anim * 0.55f * wh,
                    .375f * this->ss_anim * .55f);

            tx_tag->set_scale(0.5f * base_scale * this->ss_anim);
            tx_tag->render_at_pos(this->dd, ipos.x, ipos.y-.275f, false, call_opengl_stuff);

            call_opengl_stuff = false;
        }

        if (i >= 0x80) {
            tx_sid = gui_spritesheet::tx_out[i-0x80];
        } else {
            tx_sid = gui_spritesheet::tx_in[i];
        }

        tx_sid->set_scale(base_scale * this->ss_anim);

        tx_sid->render_at_pos(this->dd, ipos.x, ipos.y, false, call_opengl_stuff);

        call_opengl_stuff = false;
    }
}

/* highlight the currently selected entity */
void
game::render_selected_entity()
{
    this->state.edev_labels = false;

    if (!this->selection.enabled() || !this->selection.e)
        return;

    this->state.edev_labels = (this->selection.e->flag_active(ENTITY_IS_EDEVICE) || this->selection.e->type == ENTITY_PLUG || this->selection.e->type == ENTITY_EDEVICE);

    if (this->selection.e->curr_update_method == ENTITY_UPDATE_GROUPED) {
        composable *ee = static_cast<composable*>(this->selection.e);

        b2Vec2 p = ee->gr->body->GetWorldPoint(ee->_pos);
        float a = ee->gr->body->GetAngle()+ee->_angle;

        float c,s;
        tmath_sincos(a, &s, &c);

        ee->M[0] = c;
        ee->M[1] = s;
        ee->M[4] = -s;
        ee->M[5] = c;
        ee->M[12] = p.x;
        ee->M[13] = p.y;
        ee->M[14] = ee->prio * LAYER_DEPTH;

        tmat3_copy_mat4_sub3x3(ee->N, ee->M);
    }

    /* XXX */
    tms_graph_add_entity_with_children(this->outline_graph, this->selection.e);

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    //glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);
    glDepthFunc(GL_EQUAL);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glColorMask(0,0,1,1);

    tms_graph_render(this->outline_graph, this->cam, this);

    glBlendEquation(GL_FUNC_ADD);
    //glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);

    tms_graph_remove_entity_with_children(this->outline_graph, this->selection.e);

    glColorMask(1,1,1,1);

    /* get the location of the rotation icon */

    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, gui_spritesheet::atlas->texture.gl_texture);

    if ((W->is_paused() || !this->selection.e->conn_ll) && this->selection.e->flag_active(ENTITY_ALLOW_ROTATION) && !this->selection.e->flag_active(ENTITY_CONNECTED_TO_BREADBOARD)) {
        const float width = selection.e->get_width();
        b2Vec2 p = this->selection.e->local_to_world(b2Vec2(width, 0.f), this->selection.frame);
        b2Vec2 r = this->selection.e->local_to_world(b2Vec2(width+.9f, 0.f), this->selection.frame);
        b2Vec2 r2 = this->selection.e->local_to_world(b2Vec2(width+1.f, 0.f), this->selection.frame);
        glDisable(GL_DEPTH_TEST);
        tms_ddraw_set_color(this->dd, 0.f, 0.f, 0.f, 1.f);
        float mv[16];
        tmat4_copy(mv, this->cam->view);
        tmat4_translate(mv, 0, 0, this->selection.e->get_layer()*LAYER_DEPTH);
        tms_ddraw_set_matrices(this->dd, mv, this->cam->projection);
        tms_ddraw_line(this->dd, p.x, p.y, r.x, r.y);
        tms_ddraw_set_color(this->dd, 1.f, 1.f, 1.f, 1.f);
        tms_ddraw_sprite(this->dd, gui_spritesheet::get_sprite(S_ROT), r2.x, r2.y, .5f, .5f);
    }

    if (this->selection.e->g_id == O_SERVO_MOTOR || this->selection.e->g_id == O_DC_MOTOR) {
        motor *s = (motor*)this->selection.e;
        b2Vec2 p = this->selection.e->local_to_world(b2Vec2(0.f, 0.f), this->selection.frame);
        b2Vec2 r = this->selection.e->local_to_world(b2Vec2(cosf(s->properties[1].v.f) *3.f, sinf(s->properties[1].v.f)*3.f), this->selection.frame);

        tms_ddraw_set_color(this->dd, 1.f, 0.f, 1.f, 1.f);
        float mv[16];
        tmat4_copy(mv, this->cam->view);
        tmat4_translate(mv, 0, 0, this->selection.e->get_layer()*LAYER_DEPTH);
        tms_ddraw_set_matrices(this->dd, mv, this->cam->projection);
        tms_ddraw_line(this->dd, p.x, p.y, r.x, r.y);
        tms_ddraw_square(this->dd, r.x, r.y, .25f, .25f);
    }

    if (this->selection.e->g_id == O_GRAVITY_MANAGER) {
        gravityman *g = static_cast<gravityman*>(this->selection.e);

        b2Vec2 p = g->local_to_world(b2Vec2(0.f, 0.f), this->selection.frame);
        b2Vec2 r = g->local_to_world(b2Vec2(cosf(g->properties[0].v.f) *2.f, sinf(g->properties[0].v.f)*2.f), this->selection.frame);

        tms_ddraw_set_color(this->dd, 1.f, 0.f, 1.f, 1.f);
        float mv[16];
        tmat4_copy(mv, this->cam->view);
        tmat4_translate(mv, 0, 0, g->get_layer()*LAYER_DEPTH);
        tms_ddraw_set_matrices(this->dd, mv, this->cam->projection);
        tms_ddraw_line(this->dd, p.x, p.y, r.x, r.y);
        tms_ddraw_square(this->dd, r.x, r.y, .25f, .25f);
    }

    glDisable(GL_BLEND);
}

void
game::render_selected_connection()
{
    if (this->selection.enabled() && this->selection.c) {
        glDisable(GL_DEPTH_TEST);
        connection *c = this->selection.c;
        b2Vec2 base = c->e->local_to_world(b2Vec2(c->p.x, c->p.y), c->f[0]);
        float mv[16];
        float p[16];

        tmat4_copy(p, this->cam->projection);
        tmat4_copy(mv, this->cam->view);

        tmat4_translate(mv, base.x, base.y, c->layer*LAYER_DEPTH+.5f+((LAYER_DEPTH/2.f)*c->multilayer));
        tms_ddraw_set_color(this->dd, 2.f, 2.f, 5.f, 1.f);
        tms_ddraw_set_matrices(this->dd, mv, p);
        tms_ddraw_sprite(this->dd, c->type == CONN_PIVOT ? gui_spritesheet::get_sprite(S_ATTACH_ROTARY): gui_spritesheet::get_sprite(S_ATTACH_RIGID), 0, 0, .75f, .75f);
        glEnable(GL_DEPTH_TEST);
    }

}

void
game::drop_interacting(void)
{
    if (W->level.type == LCAT_ADVENTURE)
        this->selection.disable();

    for (int x=0; x<MAX_INTERACTING; x++) {
        if (interacting[x]) {
            this->destroy_mover(x);
        }
    }

    this->set_mode(GAME_MODE_DEFAULT);
}

void
game::drop_if_interacting(entity *e)
{
    for (int x=0; x<MAX_INTERACTING; x++) {
        if (interacting[x] == e) {
            this->destroy_mover(x);
            break;
        }
    }
}

int
game::interacting_with(entity *e)
{
    for (int x=0; x<MAX_INTERACTING; x++) {
        if (interacting[x] == e) {
            return current_interacting != -1 && dragging[current_interacting] ? 2 : 1;
        }
    }

    return false;
}

int
game::is_mover_joint(b2Joint *j)
{
    for (int x=0; x<MAX_INTERACTING; x++) {
        if (mover_joint[x] == j) {
            return x;
        }
    }

    return -1;
}

void
game::render_trails()
{
    for (int x=0; x<MAX_INTERACTING; x++) {
        if (interacting[x]) {
            int num = interacting_p[x];
            if (num > INTERACT_TRAIL_LEN)
                num = INTERACT_TRAIL_LEN;

            tms_graph_add_entity(this->outline_graph, interacting[x]);

            float M_saved[16];
            float N_saved[9];
            tmat4_copy(M_saved, interacting[x]->M);
            tmat3_copy(N_saved, interacting[x]->N);

            glEnable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(0);
            //glDepthFunc(GL_EQUAL);

            glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
            //glBlendFunc(GL_ONE, GL_ONE_MINUS_CONSTANT_ALPHA);
            //glBlendFunc(GL_ONE_MINUS_CONSTANT_ALPHA, GL_CONSTANT_ALPHA);
            glBlendEquation(GL_FUNC_ADD);

            for (int y=0; y<num; y++) {
                int p = (interacting_p[x]-num-1+y)%INTERACT_TRAIL_LEN;
                if (p<0)p=0;
                glBlendColor(1.f, 1.f, 1.f, .05f+(float)y/(float)INTERACT_TRAIL_LEN * .15f);

                tmat4_copy(interacting[x]->M, interacting_M[x][p]);
                tmat3_copy(interacting[x]->N, interacting_N[x][p]);

                tms_graph_render(this->outline_graph, this->cam, this);
            }

            tmat4_copy(interacting[x]->M, M_saved);
            tmat3_copy(interacting[x]->N, N_saved);
            glBlendColor(1.f, 1.f, 1.f, 1.f);
            glDepthFunc(GL_EQUAL);
            tms_graph_render(this->outline_graph, this->cam, this);

            glDepthMask(0xff);
            glBlendEquation(GL_FUNC_ADD);
            glDisable(GL_BLEND);
            glDepthFunc(GL_LESS);

            tms_graph_remove_entity(this->outline_graph, interacting[x]);

            tmat4_copy(interacting[x]->M, M_saved);
            tmat3_copy(interacting[x]->N, N_saved);
        }
    }
}

static void
fadeout_update_matrices(struct tms_entity *e, b2Vec2 velocity, bool scale=false)
{
    for (int c=0; c<e->num_children; c++) {
        fadeout_update_matrices(e->children[c], velocity);
    }

    e->M[12] += velocity.x*_tms.dt;
    e->M[13] += velocity.y*_tms.dt;

    if (scale) {
        float damping = powf(.025f, _tms.dt);
        e->M[0] *= damping;
        e->M[5] *= damping;
        e->M[10] *= damping;
    }
}

void
game::render_highlighted()
{
    for (int x=0; x<NUM_HL; x++) {
        struct hl *hl = &this->hls[x];

        if (hl->type & HL_TYPE_ERROR) {
            if (hl->time <= -1.f) {
                hl->time = 1.f;
            }
        } else {
            if (hl->time <= 0.f) {
                this->clear_hl(hl);

                continue;
            }

        }

        if (!(hl->type & HL_TYPE_PERSISTENT)) {
            hl->time -= _tms.dt * 3.f;
        }

        if (hl->type & HL_TYPE_MULTI) {
            for (entity_set::iterator i = hl->entities->begin();
                    i != hl->entities->end(); i++) {
                (*i)->prepare_fadeout();
                tms_graph_add_entity_with_children(this->outline_graph, *i);
            }
        } else if (hl->e) {
            hl->e->prepare_fadeout();
            tms_graph_add_entity_with_children(this->outline_graph, hl->e);
        } else {
            continue;
        }

        float amod = 1.f;

        glEnable(GL_DEPTH_TEST);

        if (hl->type & HL_TYPE_ERROR) {
            amod = 0.5f;
            glDisable(GL_DEPTH_TEST);
        } else if (hl->type & HL_TYPE_TINT) {
            amod = 0.5f;
        }

        glDepthFunc(GL_LEQUAL);
        glCullFace(GL_BACK);
        glEnable(GL_BLEND);
        glEnable(GL_CULL_FACE);

        glBlendColor(1.f, 1.f, 1.0f, fabsf(hl->time * amod));

        if (hl->type & HL_TYPE_ERROR) {
            glColorMask(0,1,1,1);
            glBlendFunc(GL_ONE, GL_CONSTANT_ALPHA);
            glBlendEquation(GL_FUNC_SUBTRACT);
        } else if (hl->type & HL_TYPE_TINT) {
            glColorMask(1,1,1,1);
            glBlendFunc(GL_ONE, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);
        } else {
            glColorMask(1,1,1,1);
            glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);
        }

        tms_graph_render(this->outline_graph, this->cam, this);

        glBlendEquation(GL_FUNC_ADD);
        glDisable(GL_BLEND);
        glDepthFunc(GL_LESS);

        if (hl->type & HL_TYPE_MULTI) {
            for (entity_set::iterator i = hl->entities->begin();
                    i != hl->entities->end(); i++) {
                tms_graph_remove_entity_with_children(this->outline_graph, *i);
            }
        } else {
            tms_graph_remove_entity_with_children(this->outline_graph, hl->e);
        }

        glColorMask(1,1,1,1);
    }

    for (std::set<fadeout_event*>::iterator i = this->fadeouts.begin(); i != this->fadeouts.end(); ) {
        fadeout_event *ev = *i;

        //ev->time -= 100;
        ev->time -= _tms.dt*2.f;

        if (ev->time <= 0.f) {
            this->free_fadeout(ev);
            this->fadeouts.erase(i++);
        } else {
            glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);
            glDepthFunc(GL_LESS);
            glCullFace(GL_BACK);
            glEnable(GL_CULL_FACE);
            glEnable(GL_BLEND);

            glBlendColor(1.f, 1.f, 1.f, ev->time);

            for (std::vector<fadeout_entity>::iterator it = ev->entities.begin();
                    it != ev->entities.end(); it++) {
                //(*i).e->M[0] *= ev->time;
                //
                //float m[16];
                //tmat4_load_identity(m);
                //tmat4_scale(m, ev->time, ev->time, 1.f);
                //tmat4_multiply((*it).e->M, m);
                //
                //

                if (ev->absorber) {
                    b2Vec2 velocity = ev->absorber->local_to_world(ev->absorber_point, ev->absorber_frame) - b2Vec2((*it).e->M[12], (*it).e->M[13]);
                    velocity *= 10.f;
                    fadeout_update_matrices((*it).e, velocity, true);
                } else {
                    fadeout_update_matrices((*it).e, (*it).velocity);
                }
                tms_graph_add_entity_with_children(this->outline_graph, (*it).e);
            }

            tms_graph_render(this->outline_graph, this->cam, this);

            for (std::vector<fadeout_entity>::iterator it = ev->entities.begin();
                    it != ev->entities.end(); it++)
                tms_graph_remove_entity_with_children(this->outline_graph, (*it).e);

            glBlendEquation(GL_FUNC_ADD);
            glDepthFunc(GL_LESS);
            glDisable(GL_BLEND);
            i++;
        }
    }

    if (W->level.type == LCAT_ADVENTURE && W->is_paused()) {
        entity *e = W->get_entity_by_id(this->state.adventure_id);
        if (e && e->flag_active(ENTITY_IS_ROBOT)) {
            tms_graph_add_entity_with_children(this->outline_graph, e);

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_EQUAL);
            glCullFace(GL_BACK);
            glEnable(GL_BLEND);
            glEnable(GL_CULL_FACE);

            //glBlendColor(1.f, 1.f, 1.f, fabsf(this->hls[x].time * (this->hls[x].error ? .5f : 1.f)));
            glBlendColor(0.5f, 1.f, 0.5f, 0.2f);
            glColorMask(0,1,0,1);
            glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);

            tms_graph_render(this->outline_graph, this->cam, this);

            glBlendEquation(GL_FUNC_ADD);
            glDisable(GL_BLEND);
            glDepthFunc(GL_LESS);

            tms_graph_remove_entity_with_children(this->outline_graph, e);

            glColorMask(1,1,1,1);
        }
    }
}

void
game::free_fadeout(fadeout_event *ev)
{
    /* TODO: cleanup.
     *       ALSO masure we clean up if the game is paused before
     *       a fadeout is completed !!!!! */

    for (std::vector<fadeout_entity>::iterator i = ev->entities.begin();
            i != ev->entities.end(); i++) {
        if ((*i).do_free) {
            delete (*i).e;
        }
    }

    delete ev;
}

/**
 * Open a menu for choosing which socket to attach the
 * given plug to.
 *
 * If p is 0, we open the socket unplug menu for the given edevice
 **/
void
game::open_socket_selector(entity *e, edevice *edev, int action/*=0*/)
{
    tms_infof("open socket selector");
    this->ss_quickplug_step2 = false;
    this->ss_action = action;

    if (G->state.sandbox || !e || e->get_property_entity()->is_moveable() || W->level.type == LCAT_ADVENTURE) {
        this->set_mode(GAME_MODE_SELECT_SOCKET);
        this->ss_edev = edev;

        this->ss_plug = 0;
        if (e && !e->flag_active(ENTITY_IS_PLUG)) {
            this->ss_asker = e->get_edevice();
        } else {
            if (e && e->flag_active(ENTITY_IS_PLUG)) {
                this->ss_plug = static_cast<plug_base*>(e);
            }

            this->ss_asker = 0;
        }

        this->ss_num_socks = 0;
        this->ss_anim = 0.f;

        tms_infof("OK");
    }
}

void
game::set_follow_object(entity *e, bool snap, bool preserve_pos/*=false*/)
{
    if (this->follow_object == e && e != 0)
        return;

    if (e) {
        this->follow_object = e;
    } else {
        if (W->is_adventure()) {
            this->follow_object = adventure::player;
        } else {
            this->follow_object = 0;
        }
    }

    this->cam_vel.x = 0.f;
    this->cam_vel.y = 0.f;

    if (this->follow_object) {
        if (preserve_pos) {
            this->cam_rel_pos = b2Vec2(this->cam->_position.x, this->cam->_position.y) - this->follow_object->get_position();
        } else {
            if (this->follow_object == adventure::player) {
                this->cam_rel_pos = b2Vec2(0.f,0.f);
                this->adv_rel_pos = b2Vec2(0.f,0.f);
            } else {
                this->cam_rel_pos = b2Vec2(0.f,0.f);
            }
        }

        if (snap) {
            this->cam->_position.x = this->follow_object->get_position().x;
            this->cam->_position.y = this->follow_object->get_position().y;
        }
    }
}

void
game::setup_panel(panel *p)
{
    adventure::clear_widgets();
    this->current_panel = p;

    for (int x=0; x<p->num_widgets; x++) {
        if (p->widgets[x].used) {
            switch (p->widgets[x].wtype) {
                case PANEL_LEFT:  if (wdg_left_i < MAX_DIR_BTN) {
                                      wdg_left[wdg_left_i++] = &p->widgets[x];
                                  }
                                  break;
                case PANEL_RIGHT: if (wdg_right_i < MAX_DIR_BTN) {
                                      wdg_right[wdg_right_i++] = &p->widgets[x];
                                  }
                                  break;
                case PANEL_UP:    if (wdg_up_i < MAX_DIR_BTN) {
                                      wdg_up[wdg_up_i++] = &p->widgets[x];
                                  }
                                  break;
                case PANEL_DOWN:  if (wdg_down_i < MAX_DIR_BTN) {
                                      wdg_down[wdg_down_i++] = &p->widgets[x];
                                  }
                                  break;
                case PANEL_BTN:   if (wdg_btn_i < MAX_BTN) {
                                      wdg_btn[wdg_btn_i++] = &p->widgets[x];
                                  }
                                  break;

                case PANEL_RADIAL:
                case PANEL_BIGRADIAL:
                case PANEL_FIELD:
                case PANEL_BIGFIELD:
                    if (wdg_misc_i < MAX_MISC_WIDGETS) {
                        wdg_misc[wdg_misc_i++] = &p->widgets[x];
                    }
                    break;

                case PANEL_SLIDER:
                case PANEL_BIGSLIDER:
                    if (wdg_misc_i < MAX_MISC_WIDGETS) {
                        wdg_misc[wdg_misc_i++] = &p->widgets[x];
                    }
                    break;

                case PANEL_VSLIDER:
                case PANEL_VBIGSLIDER:
                    if (wdg_misc_i < MAX_MISC_WIDGETS) {
                        wdg_misc[wdg_misc_i++] = &p->widgets[x];
                    }
                    break;
            }
            this->get_surface()->add_widget(&p->widgets[x]);
        }
    }
}

/**
 * TODO: fix spaghetti
 **/
void
game::set_control_panel(entity *e)
{
    if (e) {
        /* If the entity is in the list of starred objects, remove it from that list. */
        this->starred.erase(e);
    }
    if (e && e == this->current_panel) {
        /* If the entity sent is the same panel that we're already controlling, stop here. */
        return;
    }

    /* We've recieved an unique RC.
     * Begin by resetting all labels and special events for any previous RCs. */
    for (int x=0; x<MAX_DIR_BTN; ++x) {
        wdg_up[x] = 0;
        wdg_down[x] = 0;
        wdg_left[x] = 0;
        wdg_right[x] = 0;
    }
    for (int x=0; x<MAX_BTN; ++x) {
        wdg_btn[x] = 0;
    }
    for (int x=0; x<MAX_MISC_WIDGETS; ++x) {
        wdg_misc[x] = 0;
    }
    wdg_up_i = 0;
    wdg_down_i = 0;
    wdg_left_i = 0;
    wdg_right_i = 0;
    wdg_btn_i = 0;
    wdg_misc_i = 0;

    if (this->current_panel) {
        if (this->current_panel->is_rc()) {
            /* If we are already connected to a panel and it's an RC,
             * call the panel_disconnected for the RC.
             *
             * That function resets any buttons that were pressed to their
             * default values, assuming the RC is an RC Basic or RC Micro.
             *
             * With any other RC we cannot rely on the default values
             * because of the ghost inputs. */
            ((panel*)this->current_panel)->panel_disconnected();

            /* We decativate any "specially activated" widgets */
            deactive_misc_wdg(&G->active_hori_wdg);
            deactive_misc_wdg(&G->active_vert_wdg);
        } else if (this->current_panel->is_creature()) {
            /* If the previous panel was a creature, we will assume it
             * was the adventure robot, and clear any such widgets from
             * the screen. */
            adventure::clear_widgets();
        } else {
            /* If the panel was neither an RC or a creature,
             * something strange must have happened. */
            return;
        }

        this->current_panel = 0;
    }

    this->active_hori_wdg = 0;
    this->active_vert_wdg = 0;

    /* If the input to this function is a null pointer, we stop any further
     * execution. */
    if (!e) {
        //this->set_follow_object(0, false);
        return;
    }

    panel *p = (e && e->is_rc() ? static_cast<panel*>(e) : 0);
    creature *c = (e && e->is_creature() ? static_cast<creature*>(e) : 0);

    this->current_panel = e;

    if (p) {
        if (!W->is_adventure()) {
            /* If the level flag "Disable RC camera-snap" is enabled,
             * we will not snap to the given RC. */
            this->set_follow_object(e, false, W->level.flag_active(LVL_DISABLE_RC_CAMERA_SNAP));
        }

        /* If the new panel is an RC, we set up any panel widget labels
         * for that RC. */
        this->setup_panel(static_cast<panel*>(e));
    } else if (c && W->is_adventure() && c->is_player()) {
        /* If the new panel is the adventure robot, we initialize the on-screen
         * adventure widgets. */
        adventure::init_widgets();
    }
}

void
game::reset()
{
    disable_menu = false;
    sm::stop_all();
    this->selection.disable();
    this->reset_touch(true);
    W->cwindow->reset();
    this->clear_entities();
    W->cwindow->preloader.clear_chunks();

    this->state.is_main_puzzle = false;

    this->tmp_ambientdiffuse.x = P.default_ambient;
    this->tmp_ambientdiffuse.y = P.default_diffuse;

    this->multi.box_select = 0;

    this->follow_options.linear = false;
    this->follow_options.offset_mode = 0;
    this->follow_options.offset.x = 0.f;
    this->follow_options.offset.y = 0.f;

    this->_restart_level = false;
    this->_submit_score = false;

    for (std::set<fadeout_event*>::iterator i = this->fadeouts.begin(); i != this->fadeouts.end(); i++) {
        fadeout_event *ev = *i;
        this->free_fadeout(ev);
    }
    this->fadeouts.clear();
    this->clear_errors();
    this->starred.clear();
    adventure::reset();

    _tms.emulating_portrait = false;
    this->cam->up = (tvec3){0.f, 1.f, 0.f};

    this->set_control_panel(0);
    this->follow_object = 0;

    for (int x=0; x<NUM_HL; x++) {
        this->hls[x].e = 0;
        this->hls[x].time = 0.f;
    }

    for (int x=0; x<NUM_HP; x++) {
        this->hps[x].e = 0;
        this->hps[x].time = 0.f;
    }

    for (int x=0; x<MAX_TUTORIAL_TEXTS; x++) {
        this->tt[x].e = 0;
        this->tt[x].what = 0;
        this->tt[x].life = 0.f;
    }

    this->loots.clear();

    this->dropping = -1;
    this->drop_step = 0;
    this->drop_amount = 0;
    this->drop_speed = 1.f;

    for (int x=0; x<MAX_COPY_ENTITIES; ++x) {
        copy_entity[x] = 0;
    }
    for (int x=0; x<NUM_PROMPT_SLOTS; ++x) {
        prompt_slot[x] = 0;
    }

    this->force_static_update = 1;
    this->do_static_update = false;
    this->last_static_update = (tvec3){0.f,0.f,0.f};

    this->current_prompt = 0;
    this->state.time_mul = 0.f;
    this->state.last_autosave_try = _tms.last_time;
    this->state.modified = false;
    this->state.edev_labels = false;
    this->state.test_playing = false;
    this->state.fade = 0.f;
    this->state.waiting = false;
    this->state.ending = false;
    this->state.end_action = GAME_END_PROCEED;
    this->state.finished = false;
    this->state.success = false;
    this->state.edit_layer = 0;
    this->set_score(0);
    this->state.submitted_score = false;
    this->state.new_adventure = false;
    this->brush = 0;
    this->brush_layer_inclusion = true;
    this->brush_material = TERRAIN_GRASS;
    this->get_scene()->add_entity(W->cwindow);
    this->get_scene()->add_entity(display::get_full_entity());
    this->get_scene()->add_entity(ledbuffer::get_entity());
    this->get_scene()->add_entity(linebuffer::get_entity());
    this->get_scene()->add_entity(linebuffer::get_entity2());
    this->get_scene()->add_entity(textbuffer::get_entity());
    this->get_scene()->add_entity(textbuffer::get_entity2());
    this->get_scene()->add_entity(fluidbuffer::get_entity());
    this->get_scene()->add_entity(spritebuffer::get_entity());
    this->get_scene()->add_entity(spritebuffer::get_entity2());
    tms_scene_add_entity(this->super.scene, cable::get_entity());
    tms_scene_add_entity(this->super.scene, rope::get_entity());

    this->get_scene()->add_entity(static_cast<entity*>(pixel::get_entity(0)));
    this->get_scene()->add_entity(static_cast<entity*>(pixel::get_entity(1)));
    this->get_scene()->add_entity(static_cast<entity*>(pixel::get_entity(2)));

    this->get_scene()->add_entity(static_cast<entity*>(tpixel::get_entity(0)));
    this->get_scene()->add_entity(static_cast<entity*>(tpixel::get_entity(1)));
    this->get_scene()->add_entity(static_cast<entity*>(tpixel::get_entity(2)));

    this->selection.reset();
    this->sel_p_body = 0;
    this->sel_p_frame = 0;
    this->sel_p_ent = 0;
    this->current_panel = 0;

    this->cam_vel = (tvec3){0.f, 0.f, 0.f};

#ifdef DEBUG
    this->debug_lines.clear();
#endif

#ifdef TMS_BACKEND_PC
    this->hov_text->active = false;
#endif

    _tms.emulating_portrait = false;
    this->cam->up = (tvec3){0.f, 1.f, 0.f};

    G->caveview_size = 0.f;
    G->caveview_zoom = 0.f;
}

/**
 * Apply state directly from world by reading
 **/
void
game::load_state()
{
    tms_debugf("loading game/world state");

    /* copy information about the worlds buffer and set the pointer to point at
     * the location of the state data */
    lvlbuf lb = W->lb;
    lb.rp = W->state_ptr;

    this->state.time_mul = lb.r_float();
    this->state.adventure_id = lb.r_uint32();
    this->set_score(lb.r_uint32());
    this->state.finished = lb.r_uint8();
    this->state.success = lb.r_uint8();
    W->gravity_x = lb.r_float();
    W->gravity_y = lb.r_float();
    W->step_count = lb.r_uint32();
    this->state.finish_step = lb.r_uint32();
    this->cam->_position.x = lb.r_float();
    this->cam->_position.y = lb.r_float();
    this->cam->_position.z = lb.r_float();
    this->cam_vel.x = lb.r_float();
    this->cam_vel.y = lb.r_float();
    this->cam_vel.z = lb.r_float();
    this->cam_rel_pos.x = lb.r_float();
    this->cam_rel_pos.y = lb.r_float();
    this->adv_rel_pos.x = lb.r_float();
    this->adv_rel_pos.y = lb.r_float();
    uint32_t follow = lb.r_uint32();
    uint32_t cp = lb.r_uint32();
    this->follow_options.offset.x = lb.r_float();
    this->follow_options.offset.y = lb.r_float();
    this->follow_options.linear = lb.r_bool();
    this->follow_options.offset_mode = lb.r_uint8();
    this->last_cursor_pos_x = lb.r_int32();
    this->last_cursor_pos_y = lb.r_int32();
    W->electronics_accum = lb.r_uint64();
    this->state.new_adventure = lb.r_bool();

    uint32_t num_events = lb.r_uint32();
    for (int x=0; x<num_events; ++x) {
        W->events[x] = lb.r_int32();
    }

    uint32_t num_timed_absorbs = lb.r_uint32();
    for (int x=0; x<num_timed_absorbs; ++x) {
        uint32_t entity_id = lb.r_uint32();
        int64_t itime = lb.r_uint64();

        W->timed_absorb.insert(std::pair<uint32_t, int64_t>(entity_id, itime));
    }

    /* further apply the read data */
    W->b2->SetGravity(b2Vec2(W->gravity_x, W->gravity_y));
    W->last_gravity = b2Vec2(W->gravity_x, W->gravity_y);

    if (follow) {
        entity *e = W->get_entity_by_id(follow);
        if (e) {
            this->follow_object = e;
        }
    }
    if (cp) {
        entity *e = W->get_entity_by_id(cp);
        if (e) {
            this->set_control_panel(e);
        }
    }
}

void
game::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    lb->ensure(this->get_state_size());

    lb->w_float(this->state.time_mul);
    lb->w_uint32(this->state.adventure_id);
    lb->w_uint32(this->get_real_score());
    lb->w_uint8(this->state.finished);
    lb->w_uint8(this->state.success);
    lb->w_float(W->gravity_x);
    lb->w_float(W->gravity_y);
    lb->w_uint32(W->step_count);
    lb->w_uint32(this->state.finish_step);
    lb->w_float(this->cam->_position.x);
    lb->w_float(this->cam->_position.y);
    lb->w_float(this->cam->_position.z);
    lb->w_float(this->cam_vel.x);
    lb->w_float(this->cam_vel.y);
    lb->w_float(this->cam_vel.z);
    lb->w_float(this->cam_rel_pos.x);
    lb->w_float(this->cam_rel_pos.y);
    lb->w_float(this->adv_rel_pos.x);
    lb->w_float(this->adv_rel_pos.y);
    lb->w_uint32(this->follow_object ? this->follow_object->id : 0);
    lb->w_uint32(this->current_panel ? this->current_panel->id : 0);
    lb->w_float(this->follow_options.offset.x);
    lb->w_float(this->follow_options.offset.y);
    lb->w_bool(this->follow_options.linear);
    lb->w_uint8(this->follow_options.offset_mode);
    lb->w_int32(this->last_cursor_pos_x);
    lb->w_int32(this->last_cursor_pos_y);
    lb->w_uint64(W->electronics_accum);
    lb->w_bool(this->state.new_adventure);

    uint32_t num_events = WORLD_EVENT__NUM;
    lb->w_uint32(num_events);
    for (int x=0; x<num_events; ++x) {
        lb->w_int32(W->events[x]);
    }

    uint32_t num_timed_absorbs = W->timed_absorb.size();
    lb->w_uint32(num_timed_absorbs);

    for (std::map<uint32_t, int64_t>::iterator it = W->timed_absorb.begin();
            it != W->timed_absorb.end(); ++it) {
        lb->w_uint32(it->first);
        lb->w_int64(it->second);
    }
}

size_t
game::get_state_size()
{
    return
        sizeof(float)    /* timemul */
        + sizeof(uint32_t) /* adventure_id */
        + sizeof(uint32_t) /* score */
        + sizeof(uint8_t) /* finished */
        + sizeof(uint8_t) /* success */
        + sizeof(float) /* gravity_x */
        + sizeof(float) /* gravity_y */
        + sizeof(uint32_t) /* step_count */
        + sizeof(uint32_t) /* finish_step */
        + sizeof(float) /* cam x */
        + sizeof(float) /* cam y */
        + sizeof(float) /* cam z */
        + sizeof(float) /* cam vel x */
        + sizeof(float) /* cam vel y */
        + sizeof(float) /* cam vel z */
        + sizeof(float) /* cam rel x */
        + sizeof(float) /* cam rel y */
        + sizeof(float) /* adv rel x */
        + sizeof(float) /* adv rel y */
        + sizeof(uint32_t) /* follow object */
        + sizeof(uint32_t) /* current panel */
        + sizeof(float) /* follow offset x */
        + sizeof(float) /* follow offset y */
        + sizeof(uint8_t) /* follow linear */
        + sizeof(uint8_t) /* follow offset_mode */
        + sizeof(int32_t) /* last cursor pos x */
        + sizeof(int32_t) /* last cursor pos y */
        + sizeof(uint64_t) /* electronics accum */
        + sizeof(uint8_t) /* is new adventure */
        + sizeof(uint32_t) /* num events */
        + (sizeof(int32_t) * WORLD_EVENT__NUM) /* W->events */
        + sizeof(uint32_t) /* num timed absorbs */
        + W->timed_absorb.size() * (sizeof(uint32_t)+sizeof(int64_t))
        ;
}

void
game::apply_level_properties()
{
    W->cwindow->set_seed(W->level.seed);

    this->state.adventure_id = W->level.get_adventure_id();

    if (!adventure::player) {
        entity *player = W->get_entity_by_id(this->state.adventure_id);
        if (player) {
            adventure::player = static_cast<creature*>(player);
        }
    }

    if (!this->caveview->scene) {
        this->get_scene()->add_entity(static_cast<tms::entity*>(this->caveview));
    }

    if (this->state.abo_architect_mode) {
        this->set_architect_mode(false);
    }

#ifdef TMS_BACKEND_MOBILE
    if (W->level.flag_active(LVL_PORTRAIT_MODE)) {
#else
    if (false) {
#endif
        _tms.emulating_portrait = true;
        this->cam->up = (tvec3){1.f, 0.f, 0.f};
    } else  {
        _tms.emulating_portrait = false;
        this->cam->up = (tvec3){0.f, 1.f, 0.f};
    }

    if (W->level.type == LCAT_ADVENTURE) {
        this->cam->_position.z = 12.f;
    }

    this->cam->_position.x = W->level.sandbox_cam_x;
    this->cam->_position.y = W->level.sandbox_cam_y;
    this->cam->_position.z = W->level.sandbox_cam_zoom;

    this->init_background();

    this->check_all_entities();
}

void
game::init_background()
{
    if (this->bgent->scene) {
        this->get_scene()->remove_entity(this->bgent);
    }

    material_factory::background_id = W->level.bg;
    tms_infof("setting bg to %d", material_factory::background_id);
    material_factory::load_bg_texture(true);

    tms_debugf("SETTING BOTTOM ONLY TO %d", (material_factory::background_id == BG_OUTDOOR));
    ((simplebg*)this->bgent)->bottom_only = (material_factory::background_id == BG_OUTDOOR);
    ((simplebg*)this->bgent)->set_color(W->level.bg_color);

    bool valid = ((simplebg*)this->bgent)->set_level_size(
        W->level.size_x[0],
        W->level.size_x[1],
        W->level.size_y[0],
        W->level.size_y[1]);

    if (!valid) {
        tms_infof("Border sizes invalid, falling back to space background.");
        material_factory::background_id = BG_SPACE;
    }

    switch (material_factory::background_id) {
        case BG_SPACE:
            /* TODO: cool space rendering */
        case BG_COLORED_SPACE:
            break;

        default:
            tms_infof("Background ID %d, adding entity.", material_factory::background_id);
            this->get_scene()->add_entity(this->bgent);
            break;
    }

    float r,g,b,a;
    unpack_rgba(W->level.bg_color, &r, &g, &b, &a);

    this->state.bg_color.r = r;
    this->state.bg_color.g = g;
    this->state.bg_color.b = b;
    this->state.bg_color.a = a;
}

void
game::on_play()
{

}

void
game::on_pause()
{

}

static bool just_paused = false;

void
game::do_pause()
{
    W->save_cache(W->level_id_type, W->level.local_id);

#ifdef TMS_BACKEND_PC
    SDL_SetWindowGrab((SDL_Window*)_tms._window, SDL_FALSE);
#endif
    if (this->state.test_playing) {
        tms_infof("returning to sandbox");
        /* When returning from the sandbox, i.e. when we've finished testing our level,
         * we will open the autosave. */
        this->open_sandbox(LEVEL_LOCAL, 0);
    } else {
        if (this->screen_back != 0 && !this->state.is_main_puzzle) {
            sm::stop_all();
            tms::set_screen(this->screen_back);
        } else {
            tms_infof("Returning to half-paused state.");
            just_paused = true;
            this->open_play(W->level_id_type, W->level.local_id, this->state.pkg, false, this->state.is_main_puzzle?1:0);
        }
    }

    this->layer_vis = layer_vis_saved;
    this->refresh_widgets();
}

void
game::do_play()
{
    tms_infof("Playing");

    if (!this->state.sandbox && W->level.type == LCAT_PUZZLE) {
        /* loop through all highlights, make sure there are no placement errors */
        bool cancel = false;
        for (int x=0; x<NUM_HL; x++) {
            if (this->hls[x].type & HL_TYPE_ERROR && this->hls[x].e) {
                cancel = true;
                break;
            }
        }

        if (cancel) {
            ui::message("Please ensure no object is blinking red (error) before playing.", false);
            return;
        }
    }

    uint32_t level_id_type = W->level_id_type;
    uint32_t local_id = W->level.local_id;

    if (this->state.sandbox && !W->is_puzzle()) {
        level_id_type = LEVEL_LOCAL;
        local_id = 0;
        this->autosave();
        this->wdg_help->remove();
    } else {
        this->wdg_help->add();
    }

    this->refresh_widgets();

    this->open_play(level_id_type, local_id, this->state.pkg, this->state.sandbox, 2);
}

void
game::clear_entities()
{
    tms_scene_clear_entities(this->get_scene());
    this->u_static.clear();
    this->u_static_custom.clear();
    this->u_ghost.clear();
    this->u_effects.clear();
    this->u_fastbody.clear();
    this->u_grouped.clear();
    this->u_custom.clear();
    this->u_joint.clear();
    this->u_joint_pivot.clear();
}

void
game::add_entity(entity *e, bool soft)
{
    this->get_scene()->add_entity(e);

    switch (e->curr_update_method) {
        case ENTITY_UPDATE_GHOST:
            this->u_ghost.insert(e);
            break;

        case ENTITY_UPDATE_FASTBODY:
            this->u_fastbody.insert(e);
            break;

        case ENTITY_UPDATE_GROUPED:
            this->u_grouped.insert(e);
            break;

        case ENTITY_UPDATE_JOINT_PIVOT:
            this->u_joint_pivot.insert(e);
            break;

        case ENTITY_UPDATE_JOINT:
            this->u_joint.insert(e);
            break;

        case ENTITY_UPDATE_STATIC:
            this->u_static.insert(e);
            if (!soft && !W->is_paused()) e->update();
            break;

        case ENTITY_UPDATE_STATIC_CUSTOM:
            this->u_static_custom.insert(e);
            if (!soft && !W->is_paused()) e->update();
            break;

        case ENTITY_UPDATE_NULL:
            break;

        default:
            this->u_custom.insert(e);
            break;
    }

    if (e->flag_active(ENTITY_DO_UPDATE_EFFECTS)) {
        this->u_effects.insert(e);
    }

    if (e->flag_active(ENTITY_IS_LOCKED)) {
        this->locked.insert(e);
    }
}

void
game::destroy_mover(uint8_t x, bool do_not_deselect)
{
    this->lock();

    if (mover_joint[x]) {
        b2Body *b = mover_joint[x]->GetBodyB();

        b->ResetMassData();
        b->SetAngularDamping(0.f);
        b->SetLinearDamping(0.f);
        b->SetGravityScale(1.f);

        if (adventure::player) {
            b->SetLinearVelocity(adventure::player->get_body(0)
                    ? adventure::player->get_body(0)->GetLinearVelocity() : b2Vec2(0.f, 0.f)
                    );
            b->SetAngularVelocity(0.f);
        }

        W->b2->DestroyJoint(mover_joint[x]);
        mover_joint[x] = 0;
    }

    if (interacting_discharge[x]) {
        this->remove_entity(interacting_discharge[x]);
        delete interacting_discharge[x];
        interacting_discharge[x] = 0;
    }

    if (interacting[x]) {
        if (interacting[x] == this->selection.e && !do_not_deselect) {
            this->selection.disable();
        }
        interacting[x]->on_release_playing();
        edevice *ed = interacting[x]->get_edevice();
        interacting[x] = 0;

        if (ed)
            ed->recreate_all_cable_joints();
    }

    interacting[x] = 0;

    this->unlock();
}

void
game::remove_entity(entity *e)
{
#ifdef TMS_BACKEND_PC
    if (this->hov_ent == e) {
        this->hov_ent = 0;
        this->hov_text->active = false;
    }
#endif

    if (W->is_paused()) {
        for (int x=0; x<MAX_COPY_ENTITIES; ++x) {
            if (e == copy_entity[x]) {
                copy_entity[x] = 0;
            }
        }

        if (e->flag_active(ENTITY_IS_EDEVICE)) {
            edevice *ed = e->get_edevice();
            if (ed) {
                if (ed == this->ss_asker) {
                    this->ss_asker = 0;
                }

                if (ed == this->ss_edev) {
                    this->ss_edev = 0;
                }
            }
        }

        if (this->selection.m) {
            this->selection.m->erase(e);
        }
    } else {
        for (int x=0; x<MAX_P; ++x) {
            if (drag_cursorfield[x] == e) {
                drag_cursorfield[x] = 0;
            }
            if (in_cursorfield[x] == e) {
                in_cursorfield[x] = 0;
            }
        }

        if (hover_cursorfield == e) {
            hover_cursorfield = 0;
        }
    }

    /* if this object is highlighted, disable its highlight */
    for (int x=0; x<NUM_HL; x++) {
        if (this->hls[x].type & HL_TYPE_MULTI && this->hls[x].entities) {
            this->hls[x].entities->erase(e);
        }
        if (this->hls[x].e == e) {
            this->hls[x].e = 0;
            this->hls[x].time = 0.f;
        }
    }

    for (int x=0; x<MAX_TUTORIAL_TEXTS; x++) {
        if (this->tt[x].e == e) {
            this->tt[x].e = 0;
            this->tt[x].life = 0.f;
        }
    }

    for (std::set<fadeout_event*>::iterator i = this->fadeouts.begin(); i != this->fadeouts.end();
            i ++ ) {
        fadeout_event *ev = *i;

        if (ev->absorber == e) {
            ev->absorber = 0;
        }
    }

    /* if the object is a robot, remove possible hp stuff */
    for (int x=0; x<NUM_HP; x++) {
        if (this->hps[x].e == e) {
            this->hps[x].time = 0.f;
            this->hps[x].e = 0;
        }
    }

    this->loots.erase(e);

    if (this->selection.e == e) {
        this->selection.disable();
    }
    if (this->sel_p_ent == e) {
        this->sel_p_ent = 0;
    }

    this->u_static.erase(e);
    this->u_static_custom.erase(e);
    this->u_ghost.erase(e);
    this->u_fastbody.erase(e);
    this->u_grouped.erase(e);
    this->u_custom.erase(e);
    this->u_joint_pivot.erase(e);
    this->u_joint.erase(e);

    this->u_effects.erase(e);

    this->locked.erase(e);

    this->get_scene()->remove_entity(e);
}

void
game::set_copy_entity(uint8_t slot, entity *e)
{
    if (slot >= MAX_COPY_ENTITIES) return;

    copy_entity[slot] = e;

    if (e) {
        this->add_highlight(e, HL_PRESET_DEFAULT);
        ui::messagef("Copy entity #%d set to %s[%d]", slot+1, e->get_name(), e->id);
    }
}

void
game::copy_properties(entity *destination, entity *source, bool hl/*=false*/)
{
    if (destination && source && source->compatible_with(destination)) {
        for (int x=0; x<destination->num_properties; ++x) {
            switch (destination->properties[x].type) {
                case P_INT8:
                case P_INT:
                case P_ID:
                case P_FLT:
                    memcpy(&destination->properties[x], &source->properties[x], sizeof(property));
                    break;

                case P_STR:
                    destination->set_property(x, source->properties[x].v.s.buf);
                    break;

                default:
                    tms_warnf("Unhandled property type %d in copy_properties", destination->properties[x].type);
                    break;
            }
        }

        if (destination->g_id == O_PIXEL) {
            ((pixel*)destination)->update_appearance();
        } else if (destination->g_id == O_BOX) {
            destination->on_load(false, false);
        }

        if (hl) {
            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    } else {
        tms_infof("Unable to copy properties. d: %p. s: %p. g_id %d == %d. np %d == %d",
                destination, source, (destination?destination->g_id:0), (source?source->g_id:0),
                (destination?destination->num_properties:0), (source?source->num_properties:0));
    }
}

void
game::add_entities(
        std::map<uint32_t, entity*> *entities,
        std::map<uint32_t, group*> *groups,
        std::set<connection*> *connections,
        std::set<cable*> *cables
        )
{
    for (std::map<uint32_t, entity*>::iterator i = entities->begin();
            i != entities->end(); i++) {
        this->add_entity(i->second);
    }

    for (std::map<uint32_t, group*>::iterator i = groups->begin();
            i != groups->end(); i++) {
        i->second->create_mesh();
        i->second->finalize();
        this->add_entity(i->second);
    }

    for (std::set<connection *>::iterator i = connections->begin();
            i != connections->end(); i++) {
        if ((*i)->self_ent)
            this->add_entity((*i)->self_ent);
    }

    for (std::set<cable *>::iterator i = cables->begin();
            i != cables->end(); i++) {
        this->add_entity(*i);
    }
}

void
game::check_all_entities()
{
    this->starred.clear();
    this->locked.clear();

    for (std::map<uint32_t, entity*>::iterator i = W->all_entities.begin();
            i != W->all_entities.end(); i++) {
        if (!this->state.sandbox && W->is_puzzle()) {
            if (i->second->is_moveable()) this->starred.insert(i->second);
        } else if (this->state.sandbox && W->is_paused()) {
            if (i->second->is_locked()) this->locked.insert(i->second);
        }

        if (!W->is_adventure() && i->second->is_control_panel() && i->second != this->current_panel) {
            if (((panel*)i->second)->widgets_in_use) this->starred.insert(i->second);
        }
    }

    for (std::set<cable*>::iterator i = W->cables.begin();
            i != W->cables.end(); i++) {
        if (!this->state.sandbox && W->level.type == LCAT_PUZZLE) {
            if ((*i)->is_moveable() && ((*i)->p[0]->s == 0 && (*i)->p[1]->s == 0)) this->starred.insert(*i);
        }
    }
}

void
game::apply_pending_connection(int n)
{
    entity *saved = this->selection.e;
    for (c_map::iterator it = this->pairs.begin(); it != this->pairs.end(); ++it) {
        connection *c = it->second;

        if (n == 0) {
            this->pairs.erase(it);
            this->apply_connection(c, 0);
            break;
        }

        n --;
    }

    this->selection.select(saved);
}

/**
 * Available keybindings.
 *
 * F5     Quicksave
 * F9     Quickload
 * C      Restore camera position
 **/
int
game::handle_input_playing(tms::event *ev, int action)
{
    if (ev->type == TMS_EV_POINTER_DOWN) {
        ui::open_dialog(CLOSE_ALL_DIALOGS);

        this->render_controls = false;
    }
    if (this->state.waiting) {
        if (this->state.finished) {
            if ((ev->type == TMS_EV_KEY_PRESS && ev->data.key.keycode == TMS_KEY_ENTER)
                    || ev->type == TMS_EV_POINTER_UP) {
                if (this->state.sandbox || this->state.test_playing) {
                    this->proceed();
                } else {
                    if (this->state.success && this->state.pkg != 0) {
                        if (this->state.is_main_puzzle) {
                            // XXX: causes segfaults on android

                            char filename[1024];
                            uint32_t next = this->state.pkg->get_next_level(W->level.local_id);
                            snprintf(filename, 1023, "%s/7.%d.psol", pkgman::get_level_path(LEVEL_LOCAL), next);

                            open_play_data *opd = new open_play_data(LEVEL_LOCAL, next, this->state.pkg, false, 1);

#ifndef TMS_BACKEND_ANDROID
                            tms_infof("does %s exist?", filename);
                            if (file_exists(filename)) {
                                tms_infof("yep! send ui confirm thing");
                                ui::confirm("Do you want to load your last saved solution?",
                                        "Yes",    principia_action(ACTION_OPEN_MAIN_PUZZLE_SOLUTION, opd),
                                        "No",     principia_action(ACTION_CREATE_MAIN_PUZZLE_SOLUTION, opd),
                                        "Back",   ACTION_BACK);
                            } else
#endif
                            {
                                tms_infof("file %s does not exist!", filename);
                                P.add_action(ACTION_CREATE_MAIN_PUZZLE_SOLUTION, opd);
                            }

                            return EVENT_DONE;
                        } else {
                            this->state.end_action = GAME_END_PROCEED;
                            this->state.ending = true;
                        }
                        } else {
                            tms_infof("restarting level");
                            this->do_pause();
                        }
                }
            } else if (ev->type == TMS_EV_KEY_PRESS && (ev->data.key.keycode == TMS_KEY_B ||
                        ev->data.key.keycode == SDL_SCANCODE_AC_BACK)) {
                this->do_pause();
            }

            return T_OK;
        } else {
            if (ev->type == TMS_EV_POINTER_UP
                || ev->type == TMS_EV_KEY_PRESS) {
                if (ev->type == TMS_EV_KEY_PRESS) {
                    switch (ev->data.key.keycode) {
#ifdef TMS_BACKEND_MOBILE
                        case SDL_SCANCODE_AC_BACK:
#endif
                        case TMS_KEY_B:
                            this->back();
                            return EVENT_DONE;
                    }
                }

                if (ev->type == TMS_EV_KEY_PRESS) {
                    if (ev->data.key.keycode != TMS_KEY_W
                            && ev->data.key.keycode != TMS_KEY_S
                            && ev->data.key.keycode != TMS_KEY_A
                            && ev->data.key.keycode != TMS_KEY_D
                            && ev->data.key.keycode != TMS_KEY_E
                            && ev->data.key.keycode != TMS_KEY_UP
                            && ev->data.key.keycode != TMS_KEY_DOWN
                            && ev->data.key.keycode != TMS_KEY_LEFT
                            && ev->data.key.keycode != TMS_KEY_RIGHT
                            && ev->data.key.keycode != TMS_KEY_ENTER
                            && ev->data.key.keycode != TMS_KEY_SPACE
                            )
                        return T_OK;
                }

                this->state.waiting = false;
#ifdef TMS_BACKEND_PC
                if (settings["jail_cursor"]->v.b == true) {
                    SDL_SetWindowGrab((SDL_Window*)_tms._window, SDL_TRUE);
                } else {
                    SDL_SetWindowGrab((SDL_Window*)_tms._window, SDL_FALSE);
                }
#endif

                if (ev->type == TMS_EV_KEY_PRESS && ev->data.key.keycode == TMS_KEY_SPACE) {
                    return T_OK;
                }
            } else {
                return T_OK;
            }
        }
    }

    if (ev->type == TMS_EV_KEY_PRESS ||
        ev->type == TMS_EV_KEY_UP ||
        ev->type == TMS_EV_POINTER_DOWN ||
        ev->type == TMS_EV_POINTER_UP) {
        this->passthru_input(ev);
    }

    if (ev->type == TMS_EV_KEY_PRESS) {
        if (this->menu_handle_event(ev) == EVENT_DONE) {
            return EVENT_DONE;
        }

        switch (ev->data.key.keycode) {
            case TMS_KEY_W: if (wdg_up[0]) { tms_wdg_set_active(wdg_up[0], 1); return T_OK; } break;
            case TMS_KEY_S: if (wdg_down[0]) { tms_wdg_set_active(wdg_down[0], 1); return T_OK; } break;
            case TMS_KEY_A: if (wdg_left[0]) { tms_wdg_set_active(wdg_left[0], 1); return T_OK; } break;
            case TMS_KEY_D: if (wdg_right[0]) { tms_wdg_set_active(wdg_right[0], 1); return T_OK; } break;

            case TMS_KEY_F: if (wdg_btn[0]) { tms_wdg_set_active(wdg_btn[0], 1); return T_OK; } else { apply_pending_connection(0); } break;
            case TMS_KEY_G: if (wdg_btn[1]) { tms_wdg_set_active(wdg_btn[1], 1); return T_OK; } else { apply_pending_connection(1); } break;
            case TMS_KEY_H: if (wdg_btn[2]) { tms_wdg_set_active(wdg_btn[2], 1); return T_OK; } else { apply_pending_connection(2); } break;
            case TMS_KEY_J: if (wdg_btn[3]) { tms_wdg_set_active(wdg_btn[3], 1); return T_OK; } else { apply_pending_connection(3); } break;
            case TMS_KEY_K: if (wdg_btn[4]) { tms_wdg_set_active(wdg_btn[4], 1); return T_OK; } else { apply_pending_connection(4); } break;
            case TMS_KEY_L: apply_pending_connection(5); break;

            case TMS_KEY_Z: if (try_activate_slider(0)) { return T_OK; } break;
            case TMS_KEY_X: if (try_activate_slider(1)) { return T_OK; } break;
            case TMS_KEY_C: if (try_activate_slider(2)) { return T_OK; } break;
            case TMS_KEY_V: if (try_activate_slider(3)) { return T_OK; } break;

            case TMS_KEY_T:
                if (this->selection.e) {
                    entity *e = this->selection.e;

                    e->disconnect_all();

                    this->post_interact_select(e);
                    this->refresh_widgets();
                }
                break;

            case TMS_KEY_ESC:
                if (this->render_controls) {
                    this->render_controls = false;
                    return EVENT_DONE;
                } else if (G->active_hori_wdg || G->active_vert_wdg) {
                    deactive_misc_wdg(&G->active_hori_wdg);
                    deactive_misc_wdg(&G->active_vert_wdg);
                    return EVENT_DONE;
                }
                break;
        }

        if (W->is_adventure()) {
            if (adventure::handle_input_playing(ev, action) == EVENT_DONE)
                return EVENT_DONE;

            switch (ev->data.key.keycode) {
                case TMS_KEY_ESC:
                    if (G->current_panel && G->current_panel != adventure::player) {
                        if (adventure::player) {
                            adventure::player->detach();
                        }
                    }
                    break;
            }
        }

        switch (ev->data.key.keycode) {
            case TMS_KEY_F5:
                if (W->level.flag_active(LVL_ALLOW_QUICKSAVING)) {
                    this->save_state();
                    ui::message("Saved!");
                } else {
                    ui::message("This level does not support quick saving.");
                }
                break;

            case TMS_KEY_F9:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    disable_menu = true;

                    this->open_latest_state(true);
                }
                break;

            case TMS_KEY_O:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    disable_menu = true;

                    ui::open_dialog(DIALOG_OPEN_STATE);
                }
                break;

#ifdef TMS_BACKEND_MOBILE
            case SDL_SCANCODE_AC_BACK:
#endif
            case TMS_KEY_B:
            case TMS_KEY_P:
                if (W->is_adventure()) {
                    if (this->state.sandbox || this->state.test_playing) {
                        if (settings["dna_sandbox_back"]->v.b) {
                            this->do_pause();
                        } else {
                            ui::confirm("Are you sure you want to quit this level?",
                                    "Yes",  ACTION_WORLD_PAUSE,
                                    "No",   ACTION_IGNORE,
                                    0,      ACTION_IGNORE,
                                    confirm_data(CONFIRM_TYPE_BACK_SANDBOX));
                        }
                    } else {
                        ui::confirm("Are you sure you want to quit this level?",
                                "Yes",  ACTION_WORLD_PAUSE,
                                "No",   ACTION_IGNORE);
                    }
                } else {
                    this->do_pause();
                }
                break;

            case TMS_KEY_C:
                this->cam_rel_pos.x = 0.f;
                this->cam_rel_pos.y = 0.f;
                this->adv_rel_pos.x = 0.f;
                this->adv_rel_pos.y = 0.f;
                break;

            case TMS_KEY_Q:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    ui::open_dialog(DIALOG_CONFIRM_QUIT);
                } else {
#ifdef DEBUG
                    this->print_stats();
#endif
                }
                break;

#ifdef TMS_BACKEND_MOBILE
            case SDL_SCANCODE_MENU:
                ui::open_dialog(DIALOG_PLAY_MENU);
                break;
#endif
        }
    } else if (ev->type == TMS_EV_KEY_DOWN || ev->type == TMS_EV_KEY_REPEAT) {
        if (this->menu_handle_event(ev) == EVENT_DONE) {
            return EVENT_DONE;
        }
        if (W->is_adventure()) {
            if (adventure::handle_input_playing(ev, action) == EVENT_DONE) {
                return EVENT_DONE;
            }
        }
    } else if (ev->type == TMS_EV_KEY_UP) {
        if (this->menu_handle_event(ev)) {
            return T_OK;
        }
        switch (ev->data.key.keycode) {
            case TMS_KEY_W:
                if (wdg_up[0]) {
                    tms_wdg_set_active(wdg_up[0], 0);
                    return T_OK;
                }
                break;
            case TMS_KEY_S:
                if (wdg_down[0]) {
                    tms_wdg_set_active(wdg_down[0], 0);
                    return T_OK;
                }
                break;
            case TMS_KEY_A:
                if (wdg_left[0]) {
                    tms_wdg_set_active(wdg_left[0], 0);
                    return T_OK;
                }

                break;
            case TMS_KEY_D:
                if (wdg_right[0]) {
                    tms_wdg_set_active(wdg_right[0], 0);
                    return T_OK;
                }
                break;

            case TMS_KEY_F: if (wdg_btn[0]) { tms_wdg_set_active(wdg_btn[0], 0); return T_OK; } break;
            case TMS_KEY_G: if (wdg_btn[1]) { tms_wdg_set_active(wdg_btn[1], 0); return T_OK; } break;
            case TMS_KEY_H: if (wdg_btn[2]) { tms_wdg_set_active(wdg_btn[2], 0); return T_OK; } break;
            case TMS_KEY_J: if (wdg_btn[3]) { tms_wdg_set_active(wdg_btn[3], 0); return T_OK; } break;
            case TMS_KEY_K: if (wdg_btn[4]) { tms_wdg_set_active(wdg_btn[4], 0); return T_OK; } break;
        }

        if (W->is_adventure()) {
            if (adventure::handle_input_playing(ev, action) == EVENT_DONE) {
                return EVENT_DONE;
            }
        }
    } else if (ev->type == TMS_EV_POINTER_DOWN) {
        int pid = ev->data.motion.pointer_id;
        if (pid == 0) current_interacting = -1;

        if (this->menu_handle_event(ev)) {
            down[pid] = false;
            return T_OK;
        }
        if (W->is_adventure()) {
            if (adventure::handle_input_playing(ev, action) == EVENT_DONE)
                return EVENT_DONE;
        }

        tvec2 click_pt = tvec2f(ev->data.motion.x, ev->data.motion.y);
        int64_t diff = (int64_t)_tms.last_time - (int64_t)touch_time[pid];
        float dist = std::abs(tvec2_dist(touch_pos[pid], click_pt));

        this->mining[pid] = false;
        down[pid] = true;
        touch_pos[pid] = click_pt;
        touch_time[pid] = _tms.last_time;
        dragging[pid] = false;
        rotating[pid] = 0;
        resizing[pid] = false;

        tvec3 tproj;
        W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, 0, &tproj);

        if (this->menu_handle_event(ev)) {
            down[pid] = false;
            sm::play(&sm::click, sm::position.x, sm::position.y, rand(), 1.f, false, 0, true);
            return EVENT_DONE;
        }

        if (this->get_mode() == GAME_MODE_DEFAULT && W->is_adventure() && adventure::player
         && !(settings["tutorial"]->v.u32 & TUTORIAL_REPAIR_STATION)) {

            /* Detect clicks on help icons! */
            bool r = false;
            r = this->check_click_help_icon(
                    W->repair_stations,
                    OFFS_REPAIR_STATION,
                    b2Vec2(tproj.x, tproj.y),
                    principia_action(
                        ACTION_OPEN_URL,
                        (void*)strdup("https://principia-web.se/wiki/Repair_Station"))
                    );

            if (r) {
                return EVENT_DONE;
            }
        }

        entity *e = 0;
        b2Body *b;
        tvec2 offs;
        uint8_t frame;

        W->query(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, &e, &b, &offs, &frame, this->layer_vis);

        if (e && e->handle_event(ev->type, pid, tvec2f(ev->data.motion.x, ev->data.motion.y)) == EVENT_DONE)
            return EVENT_DONE;

#ifdef TMS_BACKEND_PC
        if (pid == 0) {
            this->update_last_cursor_pos(ev->data.motion.x, ev->data.motion.y);
            W->events[WORLD_EVENT_CLICK_DOWN] ++;
        }
#else
        if (pid == 0) {
            this->update_last_cursor_pos(ev->data.motion.x, ev->data.motion.y);
        }
        W->events[WORLD_EVENT_CLICK_DOWN] ++;
#endif

        if (pid == 0 || pid == 1) {
            if (down[0] && down[1] && !W->level.flag_active(LVL_DISABLE_ZOOM)) {
                zoom_dist = tvec2_dist(touch_pos[0], touch_pos[1]);
                zooming = true;
                zoom_stopped = false;
            }
        }

        if (this->selection.e && this->check_click_shape_resize(ev->data.motion.x, ev->data.motion.y)) {
            tms_debugf("enablding shape resize");
            resizing[pid] = 1;
            return T_OK;
        }

        if (this->selection.e && !this->selection.e->conn_ll && this->check_click_rotate(ev->data.motion.x, ev->data.motion.y)) {
            tms_debugf("enabling rotation");
            rotating[pid] = 1;
            return T_OK;
        }
        if (W->level.type == LCAT_ADVENTURE && adventure::player && adventure::is_player_alive()) {
            robot_parts::tool *t = adventure::player->get_tool();
            if (t
#ifdef TMS_BACKEND_PC
                    && pid == 0
#endif
               ) {
                if (t->action(ev->type, pid, tvec2f(ev->data.motion.x, ev->data.motion.y)) == EVENT_DONE)
                    return EVENT_DONE;
            }
        }

        int lvis = this->layer_vis;
        if (W->is_adventure() && this->caveview_size > 0.f) {
            lvis &= ~4;
        }

        W->query(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, &this->sel_p_ent, &this->sel_p_body, &this->sel_p_offs, &this->sel_p_frame, lvis);

        if (this->sel_p_ent) {
            if (this->player_can_build()) {
                if (this->check_quick_plug(diff, ev->data.motion.x, ev->data.motion.y)) {
                    if (this->interact_select(this->selection.e) == -1) {
                        tms_infof("idsable");
                        this->selection.disable();
                        this->set_mode(GAME_MODE_DEFAULT);
                    } else {
                        tms_debugf("aaaaaaa");
                    }
                }
            }

            if (this->sel_p_ent->g_id == O_CURSOR_FIELD) {
                ((cursorfield*)this->sel_p_ent)->pressed ++;
                ((cursorfield*)this->sel_p_ent)->dragged ++;
                in_cursorfield[pid] = (cursorfield*)this->sel_p_ent;
                drag_cursorfield[pid] = (cursorfield*)this->sel_p_ent;
            }
        }
    } else if (ev->type == TMS_EV_POINTER_DRAG) {
        if (this->menu_handle_event(ev))
            return T_OK;

        if (W->is_adventure()) {
            if (adventure::handle_input_playing(ev, action) == EVENT_DONE) {
                return EVENT_DONE;
            }
        }

        int pid = ev->data.motion.pointer_id;

        if (pid == 0)
            this->update_last_cursor_pos(ev->data.motion.x, ev->data.motion.y);

        tvec2 tdown = (tvec2){ev->data.motion.x, ev->data.motion.y};
        tvec2 td = (tvec2){tdown.x-touch_pos[pid].x, tdown.y-touch_pos[pid].y};

        if (!down[pid]) return T_OK;

        if (W->level.type == LCAT_ADVENTURE && adventure::player && adventure::is_player_alive()) {
            robot_parts::tool *t = adventure::player->get_tool();
            if (t
#ifdef TMS_BACKEND_PC
                    && pid == 0
#endif
               ) {
                if (t->action(ev->type, pid, tvec2f(ev->data.motion.x, ev->data.motion.y)) == EVENT_DONE)
                    return EVENT_DONE;
            }
        }

        tvec3 tproj;
        W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, 0, &tproj);

        float td_mag = tvec2_magnitude(&td);

        if (zooming && (pid == 0 || pid == 1)) {
            if (!zoom_stopped) {
                touch_pos[pid] = tdown;
                touch_time[pid] = _tms.last_time;
                touch_proj[pid] = (tvec2){tproj.x, tproj.y};

                float dist = tvec2_dist(touch_pos[0], touch_pos[1]);
                float offs = dist - zoom_dist;
                zoom_dist = dist;
                if (settings["smooth_zoom"]->v.b) {
                    this->cam_vel.z -= offs * .25f * settings["zoom_speed"]->v.f;
                } else {
                    this->cam_vel.z -= offs * 2.5f * settings["zoom_speed"]->v.f;
                }
            }
        } else {
            if (in_cursorfield[pid]) {
                W->query(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, &this->sel_p_ent, &this->sel_p_body, &this->sel_p_offs, &this->sel_p_frame, this->layer_vis);

                if (this->sel_p_ent != drag_cursorfield[pid]) {

                    if (drag_cursorfield[pid] != 0)
                        drag_cursorfield[pid]->dragged --;

                    if (this->sel_p_ent != 0 && this->sel_p_ent->g_id == O_CURSOR_FIELD) {
                        drag_cursorfield[pid] = (cursorfield*)this->sel_p_ent;
                        drag_cursorfield[pid]->dragged ++;
                    } else
                        drag_cursorfield[pid] = 0;
                }

                return T_OK;
            }
            if (this->get_mode() == GAME_MODE_QUICK_PLUG) {
                if (this->selection.e) {
                    touch_proj[pid] = (tvec2){tproj.x, tproj.y};
                    touch_pos[pid] = tdown;
                    W->query(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, &this->sel_p_ent, &this->sel_p_body, &this->sel_p_offs, &this->sel_p_frame, this->layer_vis);

                    if (this->sel_p_ent && (!this->sel_p_ent->flag_active(ENTITY_IS_EDEVICE) || this->sel_p_ent == this->selection.e)) {
                        this->sel_p_ent = 0;
                    }
                    W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, this->selection.e->get_layer(), &touch_quickplug_pos);
                    //touch_time[pid] = _tms.last_time;
                    return T_OK;
                } else {
                    tms_debugf("quickplug, but we have no selection! canceling");
                    this->set_mode(GAME_MODE_DEFAULT);
                }
            }

            if (!dragging[pid]
#ifdef TMS_BACKEND_MOBILE
                    && td_mag > DRAG_DIST_MIN_EPS
#endif
                    && (_tms.last_time - touch_time[pid] > DRAG_TIME_EPS
                        || td_mag > DRAG_DIST_EPS)) {

                if (!rotating[pid]
#ifdef TMS_BACKEND_PC
                        && pid == 0
#endif
                        && this->sel_p_ent && (this->sel_p_ent->flag_active(ENTITY_IS_INTERACTIVE) || (W->level.type == LCAT_ADVENTURE && !this->sel_p_ent->flag_active(ENTITY_IS_STATIC)))) {
                    if (this->player_can_build() || (W->level.type != LCAT_ADVENTURE && (this->sel_p_ent->in_dragfield || W->level.flag_active(LVL_DO_NOT_REQUIRE_DRAGFIELD)))) {
                        tms_infof("SELECTED interactive object[%d] with pid %d", this->sel_p_ent->in_dragfield, pid);

                        if (!W->level.flag_active(LVL_DISABLE_INTERACTIVE)
                            && (!W->is_adventure() || adventure::is_player_alive())
                            ) {

                            int f;
                            if ((f = this->interact_select(this->sel_p_ent)) != -1) {
                                this->selection.select(this->sel_p_ent, this->sel_p_body, this->sel_p_offs, this->sel_p_frame, true);
                                this->sel_p_ent->on_grab_playing();

                                moving[pid] = true;
                                if (pid == 0) {
                                    current_interacting = f;
                                }
                            } else {
                            }
                        } else {
                            moving[pid] = true;
                            //down[pid] = false;
                            //moving[pid] = false;
                        }
                    } else {
                        moving[pid] = true;
                        down[pid] = false;
                        tms_debugf("object NOT in dragfield");
                    }
                } else if (rotating[pid] || resizing[pid]) {
                    moving[pid] = true;
                } else {
                    //tms_infof("entity NOT interactive");
                    moving[pid] = false;
                }

                dragging[pid] = true;
                //touch_time[pid] = _tms.last_time;
            }

            if (dragging[pid]) {
                if (!moving[pid]) {
                    tvec3 lastproj;
                    W->get_layer_point(this->cam, touch_pos[pid].x, touch_pos[pid].y, 0, &lastproj);

                    tvec2 diff = (tvec2){tproj.x, tproj.y};
                    diff = tvec2_sub(diff, (tvec2){lastproj.x, lastproj.y});

                    if (settings["smooth_cam"]->v.b) {
                        this->cam_vel.x -= diff.x * settings["cam_speed_modifier"]->v.f * 10.f;
                        this->cam_vel.y -= diff.y * settings["cam_speed_modifier"]->v.f * 10.f;
                    } else {
                        this->cam_move(
                                diff.x * settings["cam_speed_modifier"]->v.f,
                                diff.y * settings["cam_speed_modifier"]->v.f,
                                0);
                    }

                    touch_proj[pid] = (tvec2){tproj.x, tproj.y};
                    touch_pos[pid] = tdown;
                    //touch_time[pid] = _tms.last_time;
                } else {
                    /*
                    tvec3 pt;
                    W->get_layer_point(this->cam,
                            (int)ev->data.motion.x,
                            (int)ev->data.motion.y,
                            this->selection.e->get_layer(), &pt);
                            */
                    if (resizing[pid] && this->selection.e) {
                        tvec3 pos;
                        W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, this->selection.e->get_layer(), &pos);
                        this->handle_shape_resize(pos.x, pos.y);
                    } else if (rotating[pid] && this->selection.e) {
                        tvec3 pos;
                        W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, this->selection.e->get_layer(), &pos);
                        entity *re = this->selection.e;
                        float a = re->get_body(0)->GetAngle();
                        b2Vec2 p = re->get_body(0)->GetPosition();

                        b2Vec2 cs = b2Vec2(pos.x - p.x, pos.y - p.y);
                        cs *= 1.f/cs.Length();

                        float na = atan2f(cs.y, cs.x) + this->rot_offs;

                        /* Architect mode emulates the shift functionality,
                         * holding down shift when architect mode is active neutralizes the effect. */
                        bool shift_down = (this->shift_down() && !this->state.abo_architect_mode) || (!this->shift_down() && this->state.abo_architect_mode);
                        bool ctrl_down = this->ctrl_down();

#ifdef TMS_BACKEND_MOBILE
                        /* On Android and iOS we include alternate snap-methods (holding a second finger down on the screen) */
                        shift_down = shift_down || snap[0] || snap[1];
#endif

                        /**
                         * Holding down shift and ctrl produces 64-angle snapping
                         * Holding down shift produces 16-angle snapping
                         * Holding down ctrl produces 4-angle snapping
                         **/

                        if (shift_down && ctrl_down) {
                            na = na/(M_PI/32.f);
                            na = roundf(na);
                            na = na*(M_PI/32.f);
                        } else if (shift_down) {
                            na = na/(M_PI/8.f);
                            na = roundf(na);
                            na = na*(M_PI/8.f);
                        } else if (ctrl_down) {
                            na = na/(M_PI/2.f);
                            na = roundf(na);
                            na = na*(M_PI/2.f);
                        }

                        float da = tmath_adist(a, na);

                        for (int x=0; x<MAX_INTERACTING; x++) {
                            if (mover_joint[x] && mover_joint[x]->GetBodyB() == re->get_body(0)) {
                                mover_joint[x]->SetAngularOffset(re->get_body(0)->GetAngle()+da);
                                break;
                            }
                        }
                    } else if (this->selection.e) {
                        touch_pos[pid] = (tvec2){ev->data.motion.x, ev->data.motion.y};

                        if (pid == 0 && current_interacting != -1) {
                            tvec3 tproj;
                            W->get_layer_point(this->cam, touch_pos[pid].x, touch_pos[pid].y, 0, &tproj);

                            if (mover_joint[current_interacting]) {
                                if (interacting[current_interacting]->g_id == O_LADDER_STEP) {
                                    sel_p_offs.x = 0;
                                    sel_p_offs.y = 0;
                                    tproj.x *= 2.f;
                                    tproj.y *= 2.f;
                                    tproj.x = roundf(tproj.x+.5f);
                                    tproj.y = roundf(tproj.y);
                                    tproj.x /= 2.f;
                                    tproj.y /= 2.f;
                                    tproj.x -= .25f;
                                }
                                mover_joint[current_interacting]->SetLinearOffset(b2Vec2(tproj.x-sel_p_offs.x, tproj.y-sel_p_offs.y));
                            }
                        }
                    }
                }
            } else {
                //tms_infof("not dragging");
            }
        }

    } else if (ev->type == TMS_EV_POINTER_UP) {
        int pid = ev->data.motion.pointer_id;
        int curr_was = current_interacting;
        if (pid == 0) {
            current_interacting = -1;

            if (curr_was != -1 && interacting[curr_was] && interacting[curr_was]->g_id == O_LADDER_STEP) {
                ladder_step *ls = static_cast<ladder_step*>(interacting[curr_was]);

                if (ls->emitted_by && adventure::player && ls->emitted_by == adventure::player->id) {
                    ls->find_pairs();
                    if (ls->has_pair) {
                        tms_debugf("has_pair");
                        G->apply_connection(&ls->c_back, -1);
                    } else {
                        tms_debugf("does not have pair ");
                    }

                    ls->emitted_by = 0;
                    G->drop_interacting();
                }
            }
        }

        if (this->menu_handle_event(ev)) {
            return T_OK;
        }

        if (W->is_adventure()) {
            if (adventure::handle_input_playing(ev, action) == EVENT_DONE)
                return EVENT_DONE;
        }

#ifdef TMS_BACKEND_PC
        if (pid == 0) {
            this->update_last_cursor_pos(ev->data.motion.x, ev->data.motion.y);
            W->events[WORLD_EVENT_CLICK_UP] ++;
        }
#else
        if (pid == 0) {
            this->update_last_cursor_pos(ev->data.motion.x, ev->data.motion.y);
        }
        W->events[WORLD_EVENT_CLICK_UP] ++;
#endif

        if (!down[pid]) return T_OK;

        down[pid] = false;
        //interacting[pid] = 0;
        //

        if (W->level.type == LCAT_ADVENTURE && adventure::player && adventure::is_player_alive()) {
            robot_parts::tool *t = adventure::player->get_tool();
            if (t
#ifdef TMS_BACKEND_PC
                    && pid == 0
#endif
               ) {
                if (t->action(ev->type, pid, tvec2f(ev->data.motion.x, ev->data.motion.y)) == EVENT_DONE)
                    return EVENT_DONE;
            }
        }

        if (in_cursorfield[pid]) {
            if (drag_cursorfield[pid]) {
                drag_cursorfield[pid]->dragged --;
                drag_cursorfield[pid] = 0;
            }
            in_cursorfield[pid]->pressed --;
            in_cursorfield[pid] = 0;
            return T_OK;
        }

        entity *e = 0;
        b2Body *b;
        tvec2 offs;
        uint8_t frame;

        W->query(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, &e, &b, &offs, &frame, this->layer_vis);

        if (e && e->handle_event(ev->type, pid, tvec2f(ev->data.motion.x, ev->data.motion.y)) == EVENT_DONE)
            return EVENT_DONE;

        if (zooming && (pid == 0 || pid == 1)) {
            zoom_stopped = true;
            if (!down[0] && !down[1])
                zooming = false;
        } else {
            if (this->get_mode() == GAME_MODE_QUICK_PLUG) {
                this->sel_p_ent = e;
                this->sel_p_body = b;
                this->sel_p_offs = offs;
                this->sel_p_frame = frame;

                this->set_mode(GAME_MODE_DEFAULT);

                if (this->sel_p_ent && this->sel_p_ent->flag_active(ENTITY_IS_EDEVICE) && this->sel_p_ent != this->selection.e && this->selection.e) {
                    tms_infof("quickplug detected: %s", this->sel_p_ent->get_name());

                    for (int t=0; t<3; t++) {
                        int m1 = this->selection.e->get_edevice()->get_inout_mask(t);
                        int m2 = this->sel_p_ent->get_edevice()->get_outin_mask(t);

                        if ((m1 & m2)) {
                            this->open_socket_selector(this->selection.e, this->sel_p_ent->get_edevice());
                            tms_infof("compatible");
                            break;
                        }
                    }
                } else
                    tms_infof("nothing detected");

                return T_OK;
            }

            if (!dragging[pid]) {
                W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, 0, &pt[0]);
                W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, 1, &pt[1]);
                W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, 2, &pt[2]);
                W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y,
                         + .5f,
                        &half_pt[0]);
                W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y,
                        1.f * LAYER_DEPTH + .5f,
                        &half_pt[1]);
                W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y,
                        2.f * LAYER_DEPTH + .5f,
                        &half_pt[2]);

                if (W->is_adventure()) {
                    if (adventure::is_player_alive()) {
                        std::set<activator*>::iterator it = adventure::player->activators.begin();
                        b2Vec2 click = b2Vec2(pt[adventure::player->get_layer()].x, pt[adventure::player->get_layer()].y);

                        activator *nearest = 0;
                        float nearest_dist = INFINITY;
                        for (;it != adventure::player->activators.end(); ++it) {
                            activator *act = *it;
                            b2Vec2 ipos = act->get_activator_pos();

                            float dist = (ipos - click).Length();

                            if (dist < .575f) {
                                if (dist < nearest_dist) {
                                    nearest = act;
                                    nearest_dist = dist;
                                }
                            }
                        }

                        if (nearest) {
                            adventure::player->activate_activator(nearest);
                            return T_OK;
                        }
                    }

                    if (this->player_can_build()) {
                        if (this->get_mode() == GAME_MODE_SELECT_SOCKET) {
                            if (this->check_click_socksel()) {
                                return T_OK;
                            }
                        }
                        if (this->get_mode() == GAME_MODE_SELECT_OBJECT) {
                            this->check_select_object(ev->data.motion.x, ev->data.motion.y, pid);
                            return T_OK;
                        }

                        tms_infof("checking");

                        if (this->get_mode() == GAME_MODE_SELECT_CONN_TYPE) {
                            if (this->check_click_conntype(ev->data.motion.x, ev->data.motion.y)) {
                                return T_OK;
                            }
                        }

                        if (this->check_click_conn(ev->data.motion.x, ev->data.motion.y)) {
                            return T_OK;
                        }

                        if (this->interact_select(this->sel_p_ent) == -1) {
                            this->drop_interacting();
                        } else {
                            this->selection.select(this->sel_p_ent, this->sel_p_body, this->sel_p_offs, this->sel_p_frame, false);
                        }
                    }

                } else {
                    this->selection.select(this->sel_p_ent, this->sel_p_body, this->sel_p_offs, this->sel_p_frame, false);
                    if (this->selection.e) {
                        if (this->selection.e->flag_active(ENTITY_IS_CONTROL_PANEL))
                            this->set_control_panel(this->selection.e);
                    }
                }
            } else {
                if (curr_was != -1 && interacting[curr_was]) {
                    if (interacting[curr_was]->get_body(0)->GetLinearVelocity().LengthSquared() > 12.f && (b2Vec2(pt[interacting[curr_was]->get_layer()].x, pt[interacting[curr_was]->get_layer()].y) - interacting[curr_was]->get_position()).Length()>1.5f) {
                        if (interacting[curr_was] == this->selection.e) {
                            this->selection.disable();
                        }
                        this->destroy_mover(curr_was);
                    }
                }
            }
        }
    } else if (ev->type == TMS_EV_POINTER_MOVE) {
        if (this->menu_handle_event(ev) == EVENT_DONE)
            return EVENT_DONE;

        if (W->is_adventure()) {
            if (adventure::handle_input_playing(ev, action) == EVENT_DONE) {
                return EVENT_DONE;
            }
        }

        tvec2 pt = tvec2f(ev->data.motion.x, ev->data.motion.y);

        if (this->active_hori_wdg || this->active_vert_wdg) {
            float x_diff = pt.x - this->wdg_base_x;
            float y_diff = pt.y - this->wdg_base_y;

            float x_value_diff = (x_diff / (float)_tms.opengl_width * 2.f) * settings["widget_control_sensitivity"]->v.f;
            float y_value_diff = (y_diff / (float)_tms.opengl_height * 2.f) * settings["widget_control_sensitivity"]->v.f;

            if (this->active_hori_wdg) {
                panel::widget *wdg = this->active_hori_wdg;
                switch (wdg->type) {
                    case TMS_WDG_SLIDER:
                        //this->wdg_base_x = pt.x;
                        wdg->value[0] = tclampf(wdg->value[0] + x_value_diff, 0.f, 1.f);
                        SDL_WarpMouseInWindow((SDL_Window*)_tms._window, G->wdg_base_x, G->wdg_base_y);
                        break;
                }
            }

            if (this->active_vert_wdg) {
                panel::widget *wdg = this->active_vert_wdg;

                switch (wdg->type) {
                    case TMS_WDG_VSLIDER:
                        //this->wdg_base_y = pt.y;
                        wdg->value[0] = tclampf(wdg->value[0] + y_value_diff, 0.f, 1.f);
                        SDL_WarpMouseInWindow((SDL_Window*)_tms._window, G->wdg_base_x, G->wdg_base_y);
                        break;

                    case TMS_WDG_FIELD:
                        wdg->value[0] = tclampf(wdg->value[0] + x_value_diff, 0.f, 1.f);
                        wdg->value[1] = tclampf(wdg->value[1] + y_value_diff, 0.f, 1.f);

                        SDL_WarpMouseInWindow((SDL_Window*)_tms._window, G->wdg_base_x, G->wdg_base_y);
                        break;

                    case TMS_WDG_RADIAL:
                        {
#define REQUIRE_RADIAL_THRESHOLD

                            pt.x -= this->wdg_base_x;
                            pt.y -= this->wdg_base_y;
#ifdef REQUIRE_RADIAL_THRESHOLD
                            const float ax = std::abs(pt.x);
                            const float ay = std::abs(pt.y);
                            const float len = tmath_sqrt(ax*ax+ay*ay);
                            static const float radial_threshold = 0.200f * _tms.xppcm;
                            if (len > radial_threshold) {
#endif
                                float a = atan2(pt.y, pt.x) + (M_PI*2.f);

                                const bool shift_down = this->shift_down();
                                const bool ctrl_down = this->ctrl_down();

                                if (shift_down && ctrl_down) {
                                    a = a/(M_PI/32.f);
                                    a = roundf(a);
                                    a = a*(M_PI/32.f);
                                } else if (shift_down) {
                                    a = a/(M_PI/8.f);
                                    a = roundf(a);
                                    a = a*(M_PI/8.f);
                                } else if (ctrl_down) {
                                    a = a/(M_PI/2.f);
                                    a = roundf(a);
                                    a = a*(M_PI/2.f);
                                }

                                a = fmodf(a, M_PI*2.);
                                a = fabs(a) / (M_PI*2);

                                wdg->value[0] = a;
#ifdef REQUIRE_RADIAL_THRESHOLD
                            }
#endif
                        }
                        break;
                }
            }
        }
    } else if (ev->type == TMS_EV_POINTER_SCROLL) {
        if (this->menu_handle_event(ev))
            return T_OK;

        float z = ev->data.scroll.y < 0 ? -2.f : 2.f;

        if (settings["smooth_zoom"]->v.b) {
            this->cam_vel.z -= (z*2.f) * settings["zoom_speed"]->v.f;
        } else {
            this->cam_move(
                    0,
                    0,
                    z * settings["zoom_speed"]->v.f);
        }
    }

    return T_OK;
}

void
game::proceed()
{
    tms_infof("Proceeding from %d", W->level.local_id);
    if (this->state.pkg != 0) {
        tms_infof("curr pkg %p", this->state.pkg);
        uint32_t next = this->state.pkg->get_next_level(W->level.local_id);

        this->previous_level = this->state.pkg->get_level_index(W->level.local_id);
        if (next == 0) {
            tms_infof("completed all levels");

            tms::set_screen(P.s_menu_pkg);
        } else  {
            if (this->state.pkg->first_is_menu) {
                /* TODO: the level selector should prevent to main menu,
                 * and if you're in a level you should return to the level selector. */
                //this->screen_back = P.s_menu_pkg;
            }

            this->open_play(this->state.pkg->type, next, this->state.pkg, false);
        }

        if (this->state.pkg->first_is_menu) {
            this->state.waiting = false;
        }

        return;
    } else {
        this->do_pause();
    }
}

void
game::finish(bool success)
{
    if (!this->state.finished) {
        W->save_cache(W->level_id_type, W->level.local_id);

        this->state.finish_step = W->step_count;
        this->state.finished = true;
        this->state.success = success;

        uint8_t level_id_type = W->level_id_type;

        if (level_id_type >= LEVEL_LOCAL_STATE) {
            level_id_type -= LEVEL_LOCAL_STATE;
        }

        if (G->state.is_main_puzzle) {
            level_id_type = LEVEL_MAIN;
        }

        lvl_progress *p = 0;

        if (level_id_type < LEVEL_LOCAL_STATE) {
            p = progress::get_level_progress(level_id_type, W->level.local_id);
        }

        if (success) {
            if (p) {
                p->completed = 1;
            }

            W->events[WORLD_EVENT_LEVEL_COMPLETED] ++;
        } else {
            W->events[WORLD_EVENT_GAME_OVER] ++;
        }

        if (p && (success || W->level.flag_active(LVL_STORE_SCORE_ON_GAME_OVER))) {
            p->last_score = this->get_real_score();
            tms_debugf("Last score set to %d", p->last_score);

            if (W->level.flag_active(LVL_LOWER_SCORE_IS_BETTER)) {
                if (p->last_score < p->top_score || p->top_score == 0) {
                    tms_debugf("%d < %d || %d == 0, top score set", p->last_score, p->top_score, p->top_score);
                    p->top_score = p->last_score;
                }
            } else if (p->last_score > p->top_score) {
                tms_debugf("%d > %d, top score set", p->last_score, p->top_score);
                p->top_score = p->last_score;
            }
        }

        if ((W->level.pause_on_finish || !success) && !W->level.flag_active(LVL_DISABLE_ENDSCREENS)) {
            sm::stop_all();
            this->state.waiting = true;
        } else {
            //this->proceed();
        }

        if (p) {
            progress::commit();
        }

        if (!W->level.flag_active(LVL_DISABLE_ENDSCREENS)) {
            if (success) {
                sm::play(&sm::win, 0.f, 0.f, 0, 1.f, false, 0, true);
            } else {
                sm::play(&sm::lose, 0.f, 0.f, 0, 1.f, false, 0, true);
            }

        }

        if (!this->state.submitted_score && W->level.flag_active(LVL_ALLOW_HIGH_SCORE_SUBMISSIONS)
                && W->level_id_type == LEVEL_DB
                && p
                && (settings["score_automatically_submit"]->v.b || W->level.flag_active(LVL_AUTOMATICALLY_SUBMIT_SCORE))) {
            this->submit_score();
        }

        tms_infof("game FINISH");
    }

    this->refresh_widgets();
}

void
game::open_play(int id_type, uint32_t id, pkginfo *pkg, bool test_playing/*=false*/, int is_main_puzzle/*=0*/)
{
    tms_infof("playing level %d, type %d", id, id_type);

    this->layer_vis_saved = this->layer_vis;
    this->layer_vis = 7;

    this->reset();

    if (this->state.pkg != pkg) {
        this->previous_level = 0;
    }
    this->state.pkg = pkg;
    this->state.sandbox = false;
    this->state.test_playing = test_playing;
    this->opened_special_level = 0;

    bool paused = false;

    if (is_main_puzzle == 1) {
        paused = true;
    }

    G->state.is_main_puzzle = (is_main_puzzle > 0);

    if (W->open(id_type, id, paused, false)) {
        lvl_progress *p = progress::get_level_progress(id_type, id);

        if (p) {
            tms_infof("completion score: %u",p->top_score);
            tms_infof("last score: %u",p->last_score);
            tms_infof("num plays: %u",p->num_plays);
            p->num_plays ++;
        }
    }

#ifndef SCREENSHOT_BUILD
    if (!W->is_puzzle()) {
        if (just_paused || (!this->state.sandbox && !W->level.flag_active(LVL_DISABLE_INITIAL_WAIT) && !this->state.test_playing)) {
            this->state.waiting = true;
        }
    }
#endif

    just_paused = false;

    this->apply_level_properties();
    this->add_entities(&W->all_entities, &W->groups, &W->connections, &W->cables);

    W->begin();
    this->begin_play();

#ifndef SCREENSHOT_BUILD
    if (test_playing) {
        this->state.fade = 0.2f;
    } else {
        this->state.fade = 1.0f;
    }
#endif

    this->refresh_widgets();
}

/**
 * Prepare playing the world, this is always called after a level
 * has been opened for playing
 **/
void
game::begin_play(bool has_state)
{
    if (W->is_adventure()) {
        if (this->state.adventure_id != 0) {
            entity *e = W->get_entity_by_id(this->state.adventure_id);

            if (e && e->flag_active(ENTITY_IS_CREATURE)) {
                creature *c = static_cast<creature*>(e);

                /* if we have a state, the cam pos and everything else is set
                 * from that, otherwise we just call adventure::set_player and it will
                 * set everything up for us */
                if (has_state) {
                    adventure::player = c;
                } else {
                    adventure::set_player(c, true);
                }
            }
        }

        adventure::setup(); /* XXX TODO state handling of resources */
    }

    this->refresh_widgets();
}

void
game::create_level(int type, bool empty, bool play)
{
    switch (type) {
        case LCAT_PUZZLE:     tms_infof("Creating a Puzzle level"); break;
        case LCAT_ADVENTURE:  tms_infof("Creating an Adventure level"); break;
        case LCAT_CUSTOM:     tms_infof("Creating a Custom level"); break;
    }

    this->reset();
    this->state.sandbox = !play;
    this->state.test_playing = false;

    uint64_t seed = 0;

    if (!empty) {
        init_genrand(clock());
        seed = (((uint64_t)genrand_int32()) << 32ull) | ((uint64_t)genrand_int32());
    }

    W->create(type, seed, play);

    this->apply_level_properties();
    this->add_entities(&W->all_entities, &W->groups, &W->connections, &W->cables);
    W->begin();

    if (!empty && play) {
        sprintf(W->level.name, "Random Adventure");
        W->level.name_len = strlen("Random Adventure");
    }
}

void
game::open_state(int id_type, uint32_t id, uint32_t save_id)
{
    if (id_type < LEVEL_LOCAL_STATE) {
        id_type += LEVEL_LOCAL_STATE;
    }

    tms_infof("opening state %u of %d level %u", save_id, id_type, id);

    bool test = this->state.test_playing;

    this->reset();
    W->open(id_type, id, false, false, save_id);
    this->apply_level_properties();
    this->load_state();
    this->add_entities(&W->all_entities, &W->groups, &W->connections, &W->cables);
    W->begin();
    this->begin_play(true);

    this->state.waiting = false;
    this->state.test_playing = test;
}

void
game::open_sandbox(int id_type, uint32_t id)
{
    this->reset();
    this->state.sandbox = true;

    if (id == 0 && id_type == LEVEL_LOCAL) {
        /* open autosave */
        W->open_autosave();
    } else {
        W->open(id_type, id, true, true);
    }

    this->apply_level_properties();
    this->add_entities(&W->all_entities, &W->groups, &W->connections, &W->cables);
    W->begin();

    this->refresh_widgets();
}

bool
game::delete_level(int id_type, uint32_t id, uint32_t save_id)
{
    char path[1024];
    pkgman::get_level_full_path(id_type, id, save_id, path);

    return (unlink(path) == 0);
}

bool
game::delete_partial(uint32_t id)
{
    char path[1024];
    snprintf(path, 1023, "%s/%d.pobj", pkgman::get_level_path(LEVEL_LOCAL), id);
    tms_debugf("Deleting partial %u at %s", id, path);

    return (unlink(path) == 0);
}

void
game::save_state()
{
    if (W->level.version < LEVEL_VERSION_1_5) {
        ui::message("State saving not supported in levels created with Principia<1.5");
        return;
    }
    tms_debugf("saving state");
    W->level.sandbox_cam_x = this->cam->_position.x;
    W->level.sandbox_cam_y = this->cam->_position.y;
    W->level.sandbox_cam_zoom = this->cam->_position.z;
    W->save(SAVE_TYPE_STATE);
}

bool
game::autosave()
{
    if (!this->state.sandbox) {
        tms_debugf("Can't save while outside the sandbox.");
        return false;
    }

    this->state.modified = false;

    W->level.sandbox_cam_x = this->cam->_position.x;
    W->level.sandbox_cam_y = this->cam->_position.y;
    W->level.sandbox_cam_zoom = this->cam->_position.z;

    return W->save(SAVE_TYPE_AUTOSAVE);
}

bool
game::save(bool create_icon/*=true*/, bool force/*=false*/)
{
    if (!this->state.sandbox && !force) {
        tms_debugf("Can't save while outside the sandbox.");
        return false;
    }

    this->state.modified = false;

    W->level.sandbox_cam_x = this->cam->_position.x;
    W->level.sandbox_cam_y = this->cam->_position.y;
    W->level.sandbox_cam_zoom = this->cam->_position.z;

    if (create_icon) {
        tms_infof("Creating level icon...");
        this->create_icon();
    }

    return W->save();
}

bool
game::save_copy()
{
    W->level.local_id = 0;
    W->level.parent_id = W->level.community_id;
    W->level.community_id = 0;

    return this->save();
}

void
game::select_random_entity()
{
    entity *e = 0;
    size_t sz = W->all_entities.size();

    do {
        if (sz > 0) {
            std::map<uint32_t, entity*>::iterator i = W->all_entities.begin();
            std::advance(i, rand()%sz);
            e = static_cast<entity*>(i->second);
            this->cam->_position.x = e->get_position().x;
            this->cam->_position.y = e->get_position().y;

            this->selection.select(e, NULL, (tvec2){0,0}, 0, false);
        } else {
            return;
        }
    } while (!e || e->g_id != O_MINI_TRANSMITTER);
}

void
game::snap_to_camera(screenshot_marker *sm)
{
    if (sm->is_hidden()) {
        this->cam->_position.x = sm->saved_position.x;
        this->cam->_position.y = sm->saved_position.y;
        this->cam->_position.z = sm->properties[0].v.f;
    } else {
        this->cam->_position.x = sm->get_position().x;
        this->cam->_position.y = sm->get_position().y;
        this->cam->_position.z = sm->properties[0].v.f;
    }

    /* Reset current camera movement */
    this->cam_vel.x = 0.f;
    this->cam_vel.y = 0.f;
    this->cam_vel.z = 0.f;
}

bool
game::player_can_build()
{
    return W->level.type == LCAT_ADVENTURE && adventure::player && adventure::is_player_alive()
            && adventure::player->get_tool() && adventure::player->get_tool_type() == TOOL_BUILDER;
}

/**
 * Create an icon for the current level
 **/
void
game::create_icon()
{
    GLuint err;
    float cam_width = (float)_tms.window_height;
    float cam_height = (float)_tms.window_height;

    this->cam->enable(TMS_CAMERA_PERSPECTIVE);
    this->cam->width = cam_width;
    this->cam->height = cam_height;
    this->cam->calculate();

    tvec3 dd = tms_camera_project(this->cam, this->cam->_position.x, this->cam->_position.y, LAYER_DEPTH*1.f);

    tvec3 top = tms_camera_unproject(this->cam, 0.f, cam_height, dd.z);

    this->cam->disable(TMS_CAMERA_PERSPECTIVE);

    this->cam->width = fabsf(top.x - this->cam->_position.x)*2.f;
    this->cam->height = fabsf(top.y - this->cam->_position.y)*2.f;
    this->cam->owidth = cam_width;
    this->cam->oheight = cam_height;

    this->cam->calculate();

    settings["render_gui"]->set(false);

    tms_fb_bind(this->icon_fb);
    this->render();

    if ((err = glGetError()) != 0) tms_infof("opengl error in icon creation rendering: %u", err);

    glViewport(0,0,_tms.opengl_width, _tms.opengl_height);

    SDL_Surface *srf = SDL_CreateRGBSurface(SDL_SWSURFACE, 512, 512, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    glReadPixels(0, 0, 512, 512, GL_RGBA, GL_UNSIGNED_BYTE, srf->pixels);

    if ((err = glGetError()) != 0) tms_infof("glReadPixels: %u", err);

    SDL_Surface *n = zoomSurface(srf, .25, .25);

    for (int y=0; y<128; y++) {
        for (int x=0; x<128; x++) {
            int C = 60;

            float _r = ((float)((unsigned char*)n->pixels)[y * n->pitch + x*4]) * .299f;
            float _g = ((float)((unsigned char*)n->pixels)[y * n->pitch + x*4+1]) * .587f;
            float _b = ((float)((unsigned char*)n->pixels)[y * n->pitch + x*4+2]) * .114f;

            float lum = (_r+_g+_b);

            int c = (int) (lum)/ 2;
            int f = (259*(C+255))/(255*(259-C));
            int new_c = f*(c-128)+128;
            if (new_c < 0) new_c = 0;
            if (new_c > 255) new_c = 255;
            W->level.icon[y*128+x] = (uint8_t)new_c&0xff;
        }
    }
    SDL_FreeSurface(n);

    SDL_FreeSurface(srf);

    tms_fb_unbind(this->icon_fb);

#ifndef NO_UI
    settings["render_gui"]->set(true);
#endif

    if ((err = glGetError()) != 0) tms_infof("icon creation done: %u", err);
}

/* resize the current level to fit the borders around the content */
void
game::fit_level_borders()
{
    float min_x = -100, max_x = 100, min_y = 100, max_y = 100;

    /* calculate bounds wants a std set, we have a map... bad luck */
    std::set<entity*> entities;

    for (std::map<uint32_t, entity*>::iterator i = W->all_entities.begin();
            i != W->all_entities.end(); i++) {
        entities.insert(i->second);
    }

    W->calculate_bounds(&entities, &min_x, &max_x, &min_y, &max_y);

    min_x = -roundf(fminf(0.f, min_x))+3;
    max_x = roundf(fmaxf(0.f, max_x))+3;
    min_y = -roundf(fminf(0.f, min_y))+3;
    max_y = roundf(fmaxf(0.f, max_y))+3;

    W->level.size_x[0] = (uint16_t)min_x;
    W->level.size_x[1] = (uint16_t)max_x;
    W->level.size_y[0] = (uint16_t)min_y;
    W->level.size_y[1] = (uint16_t)max_y;

    this->apply_level_properties();

    tms_infof("borders: %f %f %f %f", min_x, max_x, min_y, max_y);

    ui::emit_signal(SIGNAL_REFRESH_BORDERS);
}

void
game::select_import_object(uint32_t id)
{
    if (this->multi.import) {
        delete this->multi.import;
        this->multi.import = 0;
    }

    this->multi.import = new lvledit;

    if (this->multi.import->open(LEVEL_PARTIAL, id)) {
        if (this->multi.import->lvl.type != LCAT_PARTIAL) {
            tms_errorf("Could not load partial (game::select_import_object)");
            delete this->multi.import;
            this->multi.import = 0;

            return;
        }

        this->multi.cursor_size.x = this->multi.import->lvl.max_x - this->multi.import->lvl.min_x + .5f;
        this->multi.cursor_size.y = this->multi.import->lvl.max_y - this->multi.import->lvl.min_y + .5f;

        this->refresh_info_label();
    } else {
        delete this->multi.import;
        this->multi.import = 0;
    }

    this->refresh_widgets();
}

void
game::import_object(uint32_t id)
{
    std::map<uint32_t, entity*> entities;
    std::map<uint32_t, group*> groups;
    std::set<connection*> connections;
    std::set<cable*> cables;

    W->load_partial(id, this->multi.cursor, &entities, &groups, &connections, &cables);
    this->add_entities(&entities, &groups, &connections, &cables);
}

void
game::export_object(const char *name)
{
    if (this->selection.m) {
        W->save_partial(this->selection.m, name, pkgman::get_next_object_id());
    }
}

void
game::handle_draw(int pid, int mx, int my)
{
    tvec3 tproj;
    W->get_layer_point(this->cam, mx, my, 0, &tproj);
    int depth = this->state.edit_layer;
    int z = 0;

    if (!this->brush_layer_inclusion) {
        z = depth;
    }

    for (; z<depth+1; z++) {

        int cx = (int)floor(tproj.x*2);
        int cy = (int)floor(tproj.y*2);

        int size = (this->brush+1)*6;

        std::set<level_chunk *> affected;

        for (int h=-size/2; h<=size/2; h++) {
            for (int w=-size/2; w<=size/2; w++) {
                affected.insert(W->cwindow->set_pixel(cx+w, cy+h, z, this->brush_material));
            }
        }

        for (std::set<level_chunk*>::iterator i = affected.begin(); i != affected.end(); i++) {
            (*i)->remerge();
            W->to_be_reloaded.insert((*i));
        }
    }
}

void
game::numkey_pressed(uint8_t key)
{
    if (this->get_mode() == GAME_MODE_MULTISEL) {
        principia_wdg *wdg = this->wm->get_widget(AREA_BOTTOM_LEFT, key);

        if (wdg && wdg->_type == TMS_WDG_BUTTON) {
            wdg->click();
        }
    } else {
        if (key > MAX_COPY_ENTITIES) {
            tms_errorf("handle_slot called with slot %u, even though the max number of copy entities is %d", key, MAX_COPY_ENTITIES);
            return;
        }

        if (this->shift_down()) {
            this->copy_properties(this->selection.e, copy_entity[key], true);
        } else {
            this->set_copy_entity(key, this->selection.e);
        }
    }
}

/**
 * Available keybindings.
 *
 * I            Show help about selected object.
 *
 * CTRL+S       Save level
 * U            Save level
 * CTRL+O       Open "open level"-dialog
 * CTRL+N       New level
 * CTRL+Q       Quit
 * Space        Quickadd, or confirm socket selection (choosing first available)
 * J            Publish level
 * R            Toggle rotate entity mode
 * G            Toggle grab entity mode
 * P            Play level
 *
 * WASD         Move camera
 * M/N          Zoom camera
 * Shift+M      Toggle multiselect mode
 * B            Back to main menu
 *
 * Del          Delete selected object
 * Shift+Del    Delete selected multiobject
 *
 * Page Down    Move selected entity one layer forward (looping)
 * X            Move selected entity one layer forward (looping)
 * Z            Move selected entity one layer backward (looping)
 *
 * SHIFT+Q      Disconnect all
 * SHIFT+E      Connect all
 * T            Emulate socket disconnect button click
 * Y            Emulate config button click
 * H            "Connection selection"
 * Comma        Duplicate selected entity
 * 1-5          Copy selected entity properties
 * Shift+1-5    Paste copied properties on to selected entity
 * F5           Restore camera position
 * CTRL+F7      Select random entity
 *
 * Debug bindings:
 * F1           Enable debug draw
 * F2           Toggle robot roaming
 * CTRL+F8      Save copy
 * CTRL+F9      Clamp IDs of current level
 * CTRL+F10     Open next UI dialog
 * CTRL+F11     Reload graphics
 * CTRL+F12     Delete all loose cables and plugs
 * O            Open latest saved level
 **/
static const float CAM_NORMAL_JUMP  = 0.25f;
static const float CAM_BIG_JUMP     = 1.f;

int
game::handle_input_paused(tms::event *ev, int action)
{
    if (ev->type == TMS_EV_KEY_DOWN) {
        if (this->menu_handle_event(ev)) {
            return EVENT_DONE;
        }

        switch (ev->data.key.keycode) {
            case TMS_KEY_A:
                this->cam->_position.x -= (this->shift_down() ? CAM_BIG_JUMP : CAM_NORMAL_JUMP);
                break;
            case TMS_KEY_D:
                this->cam->_position.x += (this->shift_down() ? CAM_BIG_JUMP : CAM_NORMAL_JUMP);
                break;
            case TMS_KEY_W:
                this->cam->_position.y += (this->shift_down() ? CAM_BIG_JUMP : CAM_NORMAL_JUMP);
                break;
            case TMS_KEY_S:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    // Prevent camera from moving when saving with Ctrl+S
                } else {
                    this->cam->_position.y -= (this->shift_down() ? CAM_BIG_JUMP : CAM_NORMAL_JUMP);
                }

                break;
            case TMS_KEY_V:
#ifdef DEBUG
#ifdef SINGLE_STEP_WORLD
                tms_debugf("STEPPING WORLD");
                W->b2->Step(.001f, 20, 20);
#endif
#endif
                break;

            case TMS_KEY_MINUS: this->cam->_position.z += 1.f; break;
            case TMS_KEY_EQUALS:this->cam->_position.z -= 1.f; break;

            case TMS_KEY_DELETE:
                if (this->get_mode() == GAME_MODE_DEFAULT) {
                    if (this->selection.e && !this->selection.e->requires_delete_confirmation()) {
                        this->delete_selected_entity();
                    }
                }
                break;
        }
    } else if (ev->type == TMS_EV_KEY_PRESS) {
        if (this->menu_handle_event(ev) == EVENT_DONE) {
            return EVENT_DONE;
        }

        switch (ev->data.key.keycode) {
            case TMS_KEY_LEFT_SHIFT:
            case TMS_KEY_RIGHT_SHIFT:
                if (this->get_mode() == GAME_MODE_MULTISEL) {
                    this->multi.additive_selection = !this->multi.additive_selection;
                    this->wdg_additive->faded = !this->multi.additive_selection;
                }
                break;

            case TMS_KEY_F7:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    disable_menu = true;
                    this->select_random_entity();
                }
                break;

#ifdef DEBUG
            case TMS_KEY_F6:
                if (ev->data.key.mod & TMS_MOD_SHIFT) {
                    G->create_level(LCAT_ADVENTURE, true, false);
                } else {
                    tms_infof("Seed: 0x%016" PRIx64, W->level.seed);
                }
                break;

            case TMS_KEY_F8:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    disable_menu = true;
                    this->save_copy();
                }
                break;

            case TMS_KEY_F9:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    disable_menu = true;
                    this->clamp_entities();
                }
                break;

            case TMS_KEY_F10:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    disable_menu = true;
                    static int cur_dialog = 115;
                    tms_debugf("Opening dialog %d", cur_dialog);
                    ui::open_dialog(cur_dialog ++);
                }
                break;

            case TMS_KEY_F11:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    disable_menu = true;
                    P.can_reload_graphics = true;
                    P.can_set_settings = true;
                    P.add_action(ACTION_RELOAD_GRAPHICS, 0);
                }
                break;

            case TMS_KEY_F12:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    disable_menu = true;

                    std::set<entity*> entities_to_remove;

                    for (std::set<cable*>::iterator it = W->cables.begin(); it != W->cables.end(); ++it) {
                        cable *c = static_cast<cable*>(*it);
                        entity *e = static_cast<entity*>(c);
                        if (!c->p[0]->s && !c->p[1]->s) {
                            entities_to_remove.insert(e);
                        }
                    }

                    for (std::map<uint32_t, entity*>::iterator it = W->all_entities.begin(); it != W->all_entities.end(); ++it) {
                        entity *e = it->second;
                        if (e->g_id == O_JUMPER || e->g_id == O_RECEIVER || e->g_id == O_MINI_TRANSMITTER) {
                            plug_base *pb = static_cast<plug_base*>(e);
                            if (!pb->is_connected()) {
                                entities_to_remove.insert(e);
                            }
                        }
                    }

                    for (std::set<entity*>::iterator it = entities_to_remove.begin(); it != entities_to_remove.end(); ++it) {
                        entity *e = *it;
                        this->selection.select(e);
                        this->delete_selected_entity();
                    }
                }
                break;
#endif

            /* I: Show help about selected object */
            case TMS_KEY_I:
                if (this->selection.e) {
                    this->info_btn_pressed(this->selection.e);
                }
                break;

            case TMS_KEY_SPACE:
                /* confirm whichever task might be available */
                if (this->get_mode() == GAME_MODE_SELECT_SOCKET && this->ss_num_socks > 0) {
                    /* if there are more than one socket available, choose the "first" option */
                    this->select_socksel(0);
                    disable_menu = true;
                }
                break;

            case TMS_KEY_R:
#ifdef TMS_BACKEND_PC
                if (this->get_mode() == GAME_MODE_ROTATE)
                    this->set_mode(GAME_MODE_DEFAULT);
                else if (this->get_mode() == GAME_MODE_DEFAULT) {
                    if (this->selection.e != 0) {
                        this->set_mode(GAME_MODE_ROTATE);

                        int mx, my;
                        SDL_GetMouseState(&mx, &my);
                        this->rot_mouse_pos = tvec2f(mx, my);
                        this->rot_mouse_base = this->selection.e->gr ? this->selection.e->gr->get_angle() : this->selection.e->get_angle();
                    }
                }
#endif
                break;

#define SMALL_STEP  0.01f
#define MEDIUM_STEP 0.05f
#define LARGE_STEP  0.1f

            case TMS_KEY_LEFT:
                if (this->selection.e) {
                    b2Vec2 old_pos = this->selection.e->get_position();

                    if (ev->data.key.mod & TMS_MOD_SHIFT && ev->data.key.mod & TMS_MOD_CTRL) {
                        disable_menu = true;
                        old_pos.x -= LARGE_STEP;
                    } else if (ev->data.key.mod & TMS_MOD_SHIFT) {
                        old_pos.x -= MEDIUM_STEP;
                    } else {
                        old_pos.x -= SMALL_STEP;
                    }

                    this->selection.e->set_position(old_pos.x, old_pos.y);
                }
                break;

            case TMS_KEY_RIGHT:
                if (this->selection.e) {
                    b2Vec2 old_pos = this->selection.e->get_position();

                    if (ev->data.key.mod & TMS_MOD_SHIFT && ev->data.key.mod & TMS_MOD_CTRL) {
                        disable_menu = true;
                        old_pos.x += LARGE_STEP;
                    } else if (ev->data.key.mod & TMS_MOD_SHIFT) {
                        old_pos.x += MEDIUM_STEP;
                    } else {
                        old_pos.x += SMALL_STEP;
                    }

                    this->selection.e->set_position(old_pos.x, old_pos.y);
                }
                break;
            case TMS_KEY_UP:
                if (this->selection.e) {
                    b2Vec2 old_pos = this->selection.e->get_position();

                    if (ev->data.key.mod & TMS_MOD_SHIFT && ev->data.key.mod & TMS_MOD_CTRL) {
                        disable_menu = true;
                        old_pos.y += LARGE_STEP;
                    } else if (ev->data.key.mod & TMS_MOD_SHIFT) {
                        old_pos.y += MEDIUM_STEP;
                    } else {
                        old_pos.y += SMALL_STEP;
                    }

                    this->selection.e->set_position(old_pos.x, old_pos.y);
                }
                break;
            case TMS_KEY_DOWN:
                if (this->selection.e) {
                    b2Vec2 old_pos = this->selection.e->get_position();

                    if (ev->data.key.mod & TMS_MOD_SHIFT && ev->data.key.mod & TMS_MOD_CTRL) {
                        disable_menu = true;
                        old_pos.y -= LARGE_STEP;
                    } else if (ev->data.key.mod & TMS_MOD_SHIFT) {
                        old_pos.y -= MEDIUM_STEP;
                    } else {
                        old_pos.y -= SMALL_STEP;
                    }

                    this->selection.e->set_position(old_pos.x, old_pos.y);
                }
                break;

            /* Toggle multiselect */
            case TMS_KEY_M:
                if (this->get_mode() == GAME_MODE_MULTISEL) {
                    this->set_mode(GAME_MODE_DEFAULT);
                } else {
                    this->set_mode(GAME_MODE_MULTISEL);
                    this->multi.additive_selection = false;
                }
                break;

            case TMS_KEY_T:
                if (this->selection.e) {
                    this->selection.e->disconnect_all();
                    this->refresh_widgets();
                }
#if 0
                if (this->state.sandbox && this->selection.e && this->selection.e->flag_active(ENTITY_IS_EDEVICE)) {
                    this->open_socket_selector(0, this->selection.e->get_edevice());
                    this->state.modified = true;
                } else {
                    if (this->get_mode() == GAME_MODE_DRAW) {
                        this->set_mode(GAME_MODE_DEFAULT);
                    } else {
                        this->set_mode(GAME_MODE_DRAW);
                    }
                }
#endif
                break;

            case TMS_KEY_S:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    disable_menu = true;

                    bool ask_for_new_name = (W->level.name_len == 0);

                    if (ask_for_new_name)
                        ui::open_dialog(DIALOG_SAVE);
                    else
                        P.add_action(ACTION_SAVE, 0);
                }
                break;
            case TMS_KEY_N:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    disable_menu = true;

                    ui::open_dialog(DIALOG_NEW_LEVEL);
                } else {
                    G->toggle_entity_lock(G->selection.e);
                    this->refresh_widgets();
                }
                break;

#ifdef TMS_BACKEND_MOBILE
            case SDL_SCANCODE_AC_BACK:
#endif
            case TMS_KEY_B: this->back(); break;

            case TMS_KEY_DELETE:
                if (this->get_mode() == GAME_MODE_MULTISEL) {
                    if (this->selection.m && !this->selection.m->empty()) {
                        ui::confirm("Are you sure you want to delete these objects?",
                                "Yes",  ACTION_MULTI_DELETE,
                                "No",   ACTION_IGNORE);
                    }
                } else {
                    if (this->selection.e && this->selection.e->requires_delete_confirmation()) {
                        ui::confirm("Are you sure you want to delete this object?",
                                "Yes",  ACTION_DELETE_SELECTION,
                                "No",   ACTION_IGNORE);
                    } else
                        this->delete_selected_entity();
                }
                break;

            case TMS_KEY_F:
            case TMS_KEY_G:
                if (ev->data.key.mod & TMS_MOD_SHIFT) {
                    this->set_architect_mode(!this->state.abo_architect_mode);
                    this->refresh_widgets();
                    break;
                } else {
#if 0
                    if (this->get_mode() == GAME_MODE_DEFAULT) {
                        if (this->selection.e != 0) {
                            this->set_mode(GAME_MODE_GRAB);

                            int mx, my;
                            SDL_GetMouseState(&mx, &my);
                            this->rot_mouse_pos = (tvec2){mx,my};
                            this->grab_mouse_pos = this->selection.e->get_position();
                        }
                    } else {
                        this->set_mode(GAME_MODE_DEFAULT);
                    }
#endif
                }
            case TMS_KEY_H:
                if (ev->data.key.mod & TMS_MOD_SHIFT) {
                    if (this->state.sandbox && this->selection.e && this->selection.e->flag_active(ENTITY_IS_EDEVICE)) {
                        this->open_socket_selector(0, this->selection.e->get_edevice(), SS_ACTION_SELECT);
                    }
                    break;
                }

            case TMS_KEY_J:
                if (ev->data.key.mod & TMS_MOD_SHIFT) {
                    ui::open_dialog(DIALOG_PUBLISH);
                    break;
                }
            case TMS_KEY_K:
            case TMS_KEY_L:
                {
                    int key = ev->data.key.keycode - TMS_KEY_F;
                    this->apply_pending_connection(key);
                }
                break;

            case TMS_KEY_PAGEDOWN:
            case TMS_KEY_X:
                if (this->selection.e && this->state.sandbox) {
                    this->selection.e->set_layer((this->selection.e->get_layer()+1)%3);
                    this->animate_disconnect(this->selection.e);
                    this->selection.e->disconnect_all();
                }
                this->refresh_widgets();
                break;

            /* Shift+Q: Disconnect all connections */
            /* CTRL+Q: Open Quit dialog */
            case TMS_KEY_Q:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    disable_menu = true;
                    ui::open_dialog(DIALOG_CONFIRM_QUIT);
                } else if (ev->data.key.mod & TMS_MOD_SHIFT) {
                    if (this->state.sandbox && this->selection.e) {
                        this->selection.e->disconnect_all();
                        this->animate_disconnect(this->selection.e);
                        this->state.modified = true;
                    }
                } else {
#ifdef DEBUG
                    this->print_stats();
#endif
                }
                break;

            /* Shift+E: Connect all */
            case TMS_KEY_E:
                if (ev->data.key.mod & TMS_MOD_SHIFT
                        && this->state.sandbox && this->selection.e) {
                    entity *saved = this->selection.e;
                    for (c_map::iterator it = this->pairs.begin(); it != this->pairs.end(); ++it) {
                        connection *c = it->second;

                        if (!c->typeselect) {
                            this->apply_connection(c, 0);
                        }
                    }

                    this->pairs.clear();

                    this->selection.select(saved);
                }
                break;

            case TMS_KEY_Z:
                if (this->selection.e && this->state.sandbox) {
                    if (this->selection.e->get_layer() > 0) {
                        this->selection.e->set_layer((this->selection.e->get_layer()-1));
                    } else {
                        this->selection.e->set_layer(2);
                    }
                    this->animate_disconnect(this->selection.e);
                    this->selection.e->disconnect_all();

                    this->refresh_widgets();
                }
                break;

            case TMS_KEY_O:
                if (ev->data.key.mod & TMS_MOD_CTRL) {
                    disable_menu = true;

                    ui::open_dialog(DIALOG_OPEN);
                }
#ifdef DEBUG
                else {
                    uint32_t latest_id = pkgman::get_latest_level_id(LEVEL_LOCAL);

                    if (latest_id != 0) {
                        this->open_sandbox(LEVEL_LOCAL, latest_id); // open the last modified level
                    } else {
                        this->open_sandbox(LEVEL_LOCAL, pkgman::get_next_level_id() - 1);
                    }
                }
#endif
                break;

            case TMS_KEY_Y:
                if (this->selection.e) {
                    this->config_btn_pressed(this->selection.e);
                } else if (this->selection.m && this->get_mode() == GAME_MODE_MULTISEL) {
                    ui::open_dialog(DIALOG_MULTI_CONFIG);
                }
                break;

            /**
             * COMMA        = Duplicate selected entity and place it on the current cursor position.
             *
             * SHIFT+COMMA  = Duplicate selected entity twice, place it on the same position as the
             *                selected entity, and fill the two layers that the selected entity
             *                is not currently occupying.
             **/
            case TMS_KEY_COMMA:
                {
                    if (ev->data.key.mod & TMS_MOD_SHIFT) {
                        if (this->selection.e) {
                            int layer = this->selection.e->get_layer();
                            b2Vec2 pos = this->selection.e->get_position();

                            entity *e1 = this->editor_construct_entity(this->selection.e->g_id);
                            entity *e2 = this->editor_construct_entity(this->selection.e->g_id);

                            entity *cur_e = e1;

                            for (int x=0; x<NUM_LAYERS; ++x) {
                                if (x != layer) {
                                    cur_e->set_layer(x);
                                    cur_e->set_position(pos);

                                    if (cur_e == e1) {
                                        cur_e = e2;
                                    }
                                }
                            }
                        }
                    } else {
                        if (this->selection.e) {
                            this->editor_construct_entity(this->selection.e->g_id);
                        }
                    }
                }
                break;

            case TMS_KEY_1: this->numkey_pressed(0); break;
            case TMS_KEY_2: this->numkey_pressed(1); break;
            case TMS_KEY_3: this->numkey_pressed(2); break;
            case TMS_KEY_4: this->numkey_pressed(3); break;
            case TMS_KEY_5: this->numkey_pressed(4); break;
            case TMS_KEY_6: this->numkey_pressed(5); break;
            case TMS_KEY_7: this->numkey_pressed(6); break;
            case TMS_KEY_8: this->numkey_pressed(7); break;
            case TMS_KEY_9: this->numkey_pressed(8); break;
            case TMS_KEY_0: this->numkey_pressed(9); break;

            case TMS_KEY_U: /* Save. */
                if (W->level.name_len == 0) {
                    ui::open_dialog(DIALOG_SAVE);
                } else {
                    P.add_action(ACTION_SAVE, 0);
                }
                break;

            case TMS_KEY_P:
                if (this->get_mode() == GAME_MODE_DEFAULT || this->get_mode() == GAME_MODE_MULTISEL || this->get_mode() == GAME_MODE_DRAW) {
                    this->set_mode(GAME_MODE_DEFAULT);
                    if (this->state.is_main_puzzle) {
                        this->save(false, true);
                    }
                    this->do_play();
                }
                break;

#ifdef TMS_BACKEND_MOBILE
            case SDL_SCANCODE_MENU:
                if (this->state.sandbox) {
                    ui::open_dialog(DIALOG_SANDBOX_MENU);
                }
                break;
#endif
        }
    } else if (ev->type == TMS_EV_KEY_UP) {
        switch (ev->data.key.keycode) {
            case TMS_KEY_SPACE:
                if (!disable_menu) {
                    ui::open_dialog(DIALOG_QUICKADD);
                }

                disable_menu = false;
                break;

            case TMS_KEY_LEFT_SHIFT:
            case TMS_KEY_RIGHT_SHIFT:
                if (this->get_mode() == GAME_MODE_MULTISEL) {
                    this->multi.additive_selection = !this->multi.additive_selection;
                    this->wdg_additive->faded = !this->multi.additive_selection;
                }
                break;

            case TMS_KEY_LEFT_CTRL:
            case TMS_KEY_RIGHT_CTRL:
                if (!disable_menu)
                    ui::open_dialog(DIALOG_SANDBOX_MENU);

                disable_menu = false;
                break;

            case TMS_KEY_ESC:
                if (this->get_mode() != GAME_MODE_DEFAULT) {
                    this->set_mode(GAME_MODE_DEFAULT);
                } else {
                    if (!this->selection.enabled() && !disable_menu) {
                        ui::open_dialog(DIALOG_SANDBOX_MENU);
                        tms_debugf("open main menu");
                    }

                    if (dragging[0] && moving[0]) {
                        dragging[0] = false;
                        moving[0] = false;
                        rotating[0] = false;
                        down[0] = false;
                        cam_move_x[0] = 0.f;
                        cam_move_x[1] = 0.f;
                        cam_move_y[0] = 0.f;
                        cam_move_y[1] = 0.f;
                        this->selection.e->set_position(this->selection.e->old_pos);
                        this->recheck_all_placements();
                        this->cam->_position.x = old_cam_pos.x;
                        this->cam->_position.y = old_cam_pos.y;
                    } else {
                        this->selection.disable();
                    }
                }

                disable_menu = false;
                break;
        }
    } else if (ev->type == TMS_EV_POINTER_MOVE) {
        if (this->menu_handle_event(ev) == EVENT_DONE) {
            return EVENT_DONE;
        }

#ifdef TMS_BACKEND_PC
        if (this->get_mode() == GAME_MODE_ROTATE) {
            if (!this->selection.e) {
                this->set_mode(GAME_MODE_DEFAULT);
            } else {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                float dist = my - this->rot_mouse_pos.y;
                dist *= 1.f/100.f;

                /* TODO: add snap */
                if (this->selection.e->gr) {
                    entity *re = this->selection.e->gr;
                    b2Vec2 p = this->selection.e->get_position(this->selection.frame);
                    b2Vec2 lock = p;

                    re->set_angle(rot_mouse_base-dist);
                    lock -= this->selection.e->get_position(this->selection.frame);
                    re->set_position(re->get_position()+lock);
                } else {
                    entity *re = this->selection.e->gr ? this->selection.e->gr : this->selection.e;

                    re->set_angle(rot_mouse_base-dist);
                }
            }
        } else if (this->get_mode() == GAME_MODE_GRAB) {
            if (!this->selection.e) {
                this->set_mode(GAME_MODE_DEFAULT);
            } else {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                float dist_x = mx - this->rot_mouse_pos.x;
                float dist_y = my - this->rot_mouse_pos.y;
                dist_x *= 1.f/100.f;
                dist_y *= -1.f/100.f;

                b2Vec2 npos = b2Vec2(grab_mouse_pos.x + dist_x, grab_mouse_pos.y + dist_y);
                this->selection.e->set_position(npos.x, npos.y, this->selection.frame);
            }
        }
#endif
    } else if (ev->type == TMS_EV_POINTER_DOWN) {
        if (this->get_mode() == GAME_MODE_ROTATE || this->get_mode() == GAME_MODE_GRAB) {
            this->set_mode(GAME_MODE_DEFAULT);
        }
        /* close all menus */
        ui::open_dialog(CLOSE_ALL_DIALOGS);

        int pid = ev->data.motion.pointer_id;
        down[pid] = true;

        tvec3 tproj;
        W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, 0, &tproj);

        tvec2 click_pt = tvec2f(ev->data.motion.x, ev->data.motion.y);
        int64_t diff = (int64_t)_tms.last_time - (int64_t)touch_time[pid];
        float dist = std::abs(tvec2_dist(touch_pos[pid], click_pt));

        touch_proj[pid] = (tvec2){tproj.x, tproj.y};
        touch_pos[pid] = (tvec2){ev->data.motion.x, ev->data.motion.y};
        touch_time[pid] = _tms.last_time;
        //tms_infof("touch_time updated");
        dragging[pid] = false;

        //tms_infof("DOWN__ FPS %d", pid);

        int r = this->menu_handle_event(ev);
        rotating[pid] = 0;
        resizing[pid] = 0;

        if (r == 2) {
            down[pid] = false;
            return T_OK;
        } else if (r == 1) {
            return T_OK;
        }

        if (pid == 0 || pid == 1) {
            if (down[0] && down[1]) {
                tms_infof("Initiate zoom.");
                if (((dragging[0] && moving[0]) || (dragging[1] && moving[1])) && this->selection.e) {
                    snap[pid] = true;
                    return T_OK;
                } else {
                    zoom_dist = tvec2_dist(touch_pos[0], touch_pos[1]);
                    zooming = true;
                    zoom_stopped = false;
                }
            }
        }

        if (this->get_mode() == GAME_MODE_DRAW
                && pid == 0
            ) {
            //this->handle_draw(pid, ev->data.motion.x, ev->data.motion.y);
            /* this is now handled in step */
            drawing = 1;
        } else if (this->selection.e) {
            /* an object is selected, the player might be clicking the
             * rotation icon */

            if (this->check_click_shape_resize(ev->data.motion.x, ev->data.motion.y)) {
                resizing[pid] = 1;
                return T_OK;
            }

            if (this->check_click_rotate(ev->data.motion.x, ev->data.motion.y)) {
                rotating[pid] = 1;
                return T_OK;
            }

            if (diff < 200000 && this->selection.e->flag_active(ENTITY_HAS_CONFIG) && dist < .5f
                && !this->selection.e->is_edevice()) {
                G->config_btn_pressed(G->selection.e);
                return T_OK;
            }

            if (this->selection.e->g_id == O_SERVO_MOTOR || this->selection.e->g_id == O_DC_MOTOR) {
                motor *s = (motor*)this->selection.e;
                tvec3 pt;
                W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, this->selection.e->get_layer(), &pt);
                b2Vec2 r = this->selection.e->local_to_world(b2Vec2(cosf(s->properties[1].v.f) * 3.f, sinf(s->properties[1].v.f)*3.f), this->selection.frame);

                if ((r - b2Vec2(pt.x, pt.y)).Length() < .5f) {
                    rotating[pid] = 2;
                    b2Vec2 p = this->selection.e->get_position();

                    float a1 = atan2f(r.y - p.y, r.x - p.x);
                    float a2 = atan2f(pt.y - p.y, pt.x - p.x);

                    this->rot_offs = tmath_adist(a2, a1);
                    W->query(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, &this->sel_p_ent, &this->sel_p_body, &this->sel_p_offs, &this->sel_p_frame, this->layer_vis);
                    return T_OK;
                }
            }
        } else if (this->get_mode() == GAME_MODE_MULTISEL
#ifdef TMS_BACKEND_PC
                && pid == 0
#endif
                ) {
            if (this->multi.box_select == 1) {
                W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, 0, &begin_box_select);
                box_select_pid = pid;
                this->multi.box_select = 2;
                return T_OK;
            } else {
                W->query(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, &this->sel_p_ent, &this->sel_p_body, &this->sel_p_offs, &this->sel_p_frame, this->layer_vis);

                if (this->sel_p_ent) {
                    if (!this->multi.additive_selection && this->selection.m && this->selection.m->find(this->sel_p_ent) != this->selection.m->end()) {
                    } else {
                        this->apply_multiselection(this->sel_p_ent);
                    }
                } else {
                    this->selection.disable();
                    tvec3 pt;
                    W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, 0, &pt);

                    this->multi.cursor.x = pt.x;
                    this->multi.cursor.y = pt.y;
                }
            }
        }

        if (this->get_mode() == GAME_MODE_MULTISEL) {
            if (this->multi.import && diff < 300000 && dist < 50.f
#ifdef TMS_BACKEND_PC
                && pid == 0
#endif
                    ) {
                tms_debugf("IMPORT (%.2f)", dist);
                this->import_object(this->multi.import->lvl_id);
            }
        } else {
            /* TODO: Make sure we're selecting the object that's actually closest. */
            W->query(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, &this->sel_p_ent, &this->sel_p_body, &this->sel_p_offs, &this->sel_p_frame, this->layer_vis);
            if (this->sel_p_ent) {
                this->check_quick_plug(diff, ev->data.motion.x, ev->data.motion.y);
            }
        }
    } else if (ev->type == TMS_EV_POINTER_DRAG) {
        if (this->menu_handle_event(ev)) {
            return T_OK;
        }

        int pid = ev->data.motion.pointer_id;

        if (!down[pid]) {
            return T_OK;
        }

        if (snap[pid]) {
            return T_OK;
        }

        tvec2 tdown = (tvec2){ev->data.motion.x, ev->data.motion.y};
        tvec2 td = (tvec2){tdown.x-touch_pos[pid].x, tdown.y-touch_pos[pid].y};

        tvec3 tproj;
        W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, 0, &tproj);

        float td_mag = tvec2_magnitude(&td);

        if (zooming && (pid == 0 || pid == 1)) {
            if (!zoom_stopped) {
                touch_pos[pid] = tdown;
                touch_time[pid] = _tms.last_time;
                touch_proj[pid] = (tvec2){tproj.x, tproj.y};

                float dist = tvec2_dist(touch_pos[0], touch_pos[1]);
                float offs = dist - zoom_dist;
                zoom_dist = dist;
                if (settings["smooth_zoom"]->v.b) {
                    this->cam_vel.z -= offs * .25f * settings["zoom_speed"]->v.f;
                } else {
                    this->cam_vel.z -= offs * 2.5f * settings["zoom_speed"]->v.f;
                }
            }
        } else if (this->get_mode() == GAME_MODE_DRAW
                && pid == 0
            ) {
            touch_pos[pid] = tdown;
            //this->handle_draw(pid, ev->data.motion.x, ev->data.motion.y);
            /* this is now handled in step */
        } else {
            if (this->get_mode() == GAME_MODE_QUICK_PLUG) {
                if (this->selection.e) {
                    touch_proj[pid] = (tvec2){tproj.x, tproj.y};
                    touch_pos[pid] = tdown;
                    W->query(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, &this->sel_p_ent, &this->sel_p_body, &this->sel_p_offs, &this->sel_p_frame, this->layer_vis);

                    if (this->sel_p_ent && (!this->sel_p_ent->flag_active(ENTITY_IS_EDEVICE) || this->sel_p_ent == this->selection.e)) {
                        this->sel_p_ent = 0;
                    }
                    W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, this->selection.e->get_layer(), &touch_quickplug_pos);
                    //touch_time[pid] = _tms.last_time;
                    return T_OK;
                } else {
                    this->set_mode(GAME_MODE_DEFAULT);
                }
            }

            /*
            tms_infof("DRAG_DIST_EPS: %.2f", DRAG_DIST_EPS);
            tms_infof("td_mag: %.2f", td_mag);
            tms_infof("DRAG_TIME_EPS: %d", DRAG_TIME_EPS);
            tms_infof("tms.last_time: %llu", _tms.last_time);
            tms_infof("touch_time[%d]: %llu", pid, touch_time[pid]);
            */

            if (!dragging[pid]
#ifdef TMS_BACKEND_MOBILE
                    && td_mag > DRAG_DIST_MIN_EPS
#endif
                    && (_tms.last_time - touch_time[pid] > DRAG_TIME_EPS
                        || td_mag > DRAG_DIST_EPS)
#ifdef TMS_BACKEND_PC
                    && pid == 0
#endif
                    ) {

                if (rotating[pid] || resizing[pid]) {
                    moving[pid] = true;
                } else {
                    if (this->get_mode() == GAME_MODE_MULTISEL) {
                        if (this->multi.additive_selection == false && this->selection.m) {
                            if (this->sel_p_ent) {
                                entity *first = this->sel_p_ent;
                                tms_debugf("first: %s", first->get_name());
                                W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, first->get_layer(), &tproj);
                                b2Vec2 first_pos = first->get_position();
                                this->selection.offs = (tvec2){tproj.x-first_pos.x,tproj.y-first_pos.y};
                                moving[pid] = true;
                            } else {
                                moving[pid] = false;
                            }
                        } else {
                            moving[pid] = false;
                        }
                    } else if (this->sel_p_ent || this->sel_p_body) {
                        this->selection.select(this->sel_p_ent, this->sel_p_body, this->sel_p_offs, this->sel_p_frame, true);
                        if (this->selection.e) {
                            this->selection.e->old_pos = this->selection.e->get_position();
                            old_cam_pos = this->cam->_position;
                            this->selection.e->on_grab(this);
                        }

                        moving[pid] = true;
                    } else {
                        moving[pid] = false;
                    }
                }

                dragging[pid] = true;
                touch_time[pid] = _tms.last_time;
                //tms_infof("touch_time updated");
            }

            if (dragging[pid]
#ifdef TMS_BACKEND_PC
                    || pid == 2 /* middle mouse button */
#endif
                    ) {
                if (moving[pid]) {
                    this->state.modified = true;

                    tvec3 pos;
                    if (this->selection.m && this->sel_p_ent && this->selection.m->size()) {
                        W->step_count = 0;
                        entity *first = this->sel_p_ent;
                        W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, first->get_layer(), &tproj);

                        b2Vec2 p = b2Vec2(tproj.x, tproj.y);
                        p.x -= this->selection.offs.x;
                        p.y -= this->selection.offs.y;

                        b2Vec2 diff = p - first->get_position();

                        /* only allow one entity in each group to move, since
                         * each entity will force the whole group to move */
                        std::set<group*> cache;

                        for (std::set<entity*>::iterator i = this->selection.m->begin();
                                i != this->selection.m->end(); i++) {
                            (*i)->on_grab(this);
                        }

                        for (std::set<entity*>::iterator i = this->selection.m->begin();
                                i != this->selection.m->end(); i++) {

                            if ((*i)->gr) {
                                /* see comment aboout groups above */
                                if (cache.find((*i)->gr) != cache.end())
                                    continue;

                                cache.insert((*i)->gr);
                            }

                            p = (*i)->get_position();
                            p += diff;
                            (*i)->set_position(p);
                        }

                        for (std::set<entity*>::iterator i = this->selection.m->begin();
                                i != this->selection.m->end(); i++) {
                            (*i)->on_release(this);
                        }
                    } else if (this->selection.e) {
                        W->step_count = 0;
                        W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, this->selection.e->get_layer(), &pos);

                        /* Stop the entity from being rotated or moved if connected to a static entity */
                        if (this->selection.e->gr && this->selection.e->gr->is_locked() && !this->selection.e->is_locked()) {
                            return EVENT_DONE;
                        } else {
                            std::set<entity*> entities;
                            this->selection.e->gather_connected_entities(&entities, false, false, true);

                            bool found_static = false;

                            for (std::set<entity*>::iterator it = entities.begin();
                                    it != entities.end(); ++it) {
                                if ((*it) == this->selection.e) continue;

                                if ((*it)->flag_active(ENTITY_IS_STATIC)) {
                                    found_static = true;
                                    break;
                                }
                            }

                            if (found_static) {
                                return EVENT_DONE;
                            }
                        }

                        bool simple_snap = false;
                        /* Architect mode emulates the shift functionality */
                        if (this->state.abo_architect_mode) {
                            simple_snap = !simple_snap;
                        }

                        /* Holding down shift when architect mode is active neutralizes the effect */
                        if (this->shift_down()) {
                            simple_snap = !simple_snap;
                        }

#ifdef TMS_BACKEND_MOBILE
                        /* On Android and iOS we include alternate snap-methods (holding a second finger down on the screen) */
                        if (snap[0] || snap[1]) {
                            simple_snap = !simple_snap;
                        }
#endif

                        bool advanced_snap = false;

                        /* Currently only used with rotations to make finer rotation snaps. */
                        if (this->ctrl_down()) {
                            advanced_snap = !advanced_snap;
                        }

                        if (!rotating[pid] && !resizing[pid]) {
                            /* If the Snap by Default level-flag is enabled, toggle the snap mode again!
                             * This is only used for movements, not for rotations. */
                            if (!this->state.sandbox && W->level.flag_active(LVL_SNAP)) {
                                simple_snap = !simple_snap;
                            }

                            pos.x -= this->selection.offs.x;
                            pos.y -= this->selection.offs.y;

                            if (this->selection.b && this->selection.b != this->selection.e->get_body(this->selection.frame)) {
                                this->selection.b->SetTransform(b2Vec2(pos.x, pos.y), this->selection.b->GetAngle());
                            } else {

                                if (simple_snap) {
                                    /* Shift-dragging pixels remove them from their grid */
                                    if (this->selection.e->g_id == O_PIXEL || this->selection.e->g_id == O_TPIXEL) {
                                        this->selection.e->entity::set_position(
                                                roundf(pos.x/state.gridsize)*state.gridsize,
                                                roundf(pos.y/state.gridsize)*state.gridsize,
                                                this->selection.frame);
                                    } else {
                                        this->selection.e->set_position(
                                                roundf(pos.x/state.gridsize)*state.gridsize,
                                                roundf(pos.y/state.gridsize)*state.gridsize,
                                                this->selection.frame);
                                    }
                                } else {
#define BORDER_SCROLL_SIZE 80
                                    int border_x = BORDER_SCROLL_SIZE;
                                    int border_y = BORDER_SCROLL_SIZE;
                                    int ox = (int)ev->data.motion.x - _tms.window_width/2;
                                    int oy = (int)ev->data.motion.y - _tms.window_height/2;
                                    int dir_x = ox > 0;
                                    int dir_y = oy > 0;

                                    if (dir_x == 1 && this->state.sandbox) { /* right */
                                        ox += this->get_menu_width();
                                    }

                                    int str_x = (_tms.window_width / 2) - std::abs(ox);
                                    if (str_x < border_x) {
                                        cam_move_x[dir_x] = tclampf(1.f-(float)str_x/border_x, 0.f, 1.f);
                                    } else {
                                        cam_move_x[dir_x] = 0.f;
                                    }

                                    int str_y = (_tms.window_height / 2) - std::abs(oy);
                                    if (str_y < border_y) {
                                        cam_move_y[dir_y] = tclampf(1.f-(float)str_y/border_y, 0.f, 1.f);
                                    } else {
                                        cam_move_y[dir_y] = 0.f;
                                    }

                                    this->selection.e->set_position(pos.x, pos.y, this->selection.frame);
                                }
                            }

                            this->state.modified = true;
                        } else {
                            if (resizing[pid]) {
                                this->handle_shape_resize(pos.x, pos.y);

                                this->state.modified = true;
                            } else if (rotating[pid] == 1) {
                                if (this->selection.e->gr) {
                                    entity *re = this->selection.e->gr;
                                    float a = this->selection.e->get_angle(this->selection.frame);
                                    b2Vec2 p = this->selection.e->get_position(this->selection.frame);
                                    //float a = this->selection.e->get_angle();
                                    //b2Vec2 p = this->selection.e->get_position();
                                    b2Vec2 lock = p;

                                    b2Vec2 cs = b2Vec2(pos.x - p.x, pos.y - p.y);
                                    cs *= 1.f/cs.Length();

                                    float na = atan2f(cs.y, cs.x) + this->rot_offs;
                                    float da = tmath_adist(a, na);

                                    re->set_angle(re->get_angle()+da);
                                    lock -= this->selection.e->get_position(this->selection.frame);
                                    re->set_position(re->get_position()+lock);
                                } else {
                                    entity *re = this->selection.e->gr ? this->selection.e->gr : this->selection.e;
                                    float a = re->get_angle();
                                    b2Vec2 p = re->get_position();

                                    b2Vec2 cs = b2Vec2(pos.x - p.x, pos.y - p.y);
                                    cs *= 1.f/cs.Length();

                                    float na = atan2f(cs.y, cs.x) + this->rot_offs;

                                    /**
                                     * Holding down shift and ctrl produces 64-angle snapping
                                     * Holding down shift produces 16-angle snapping
                                     * Holding down ctrl produces 4-angle snapping
                                     **/

                                    if (simple_snap && advanced_snap) {
                                        na = na/(M_PI/32.f);
                                        na = roundf(na);
                                        na = na*(M_PI/32.f);
                                    } else if (simple_snap) {
                                        na = na/(M_PI/8.f);
                                        na = roundf(na);
                                        na = na*(M_PI/8.f);
                                    } else if (advanced_snap) {
                                        na = na/(M_PI/2.f);
                                        na = roundf(na);
                                        na = na*(M_PI/2.f);
                                    }

                                    float da = tmath_adist(a, na);
                                    re->set_angle(a+da);
                                }

                                this->state.modified = true;
                            } else if (rotating[pid] == 2) {
                                /*b2Vec2 p = this->selection.e->get_position();
                                b2Vec2 cs = b2Vec2(pos.x - p.x, pos.y - p.y);
                                cs *= 1.f/cs.Length();*/

                                b2Vec2 p = this->selection.e->world_to_local(b2Vec2(pos.x, pos.y), 0);
                                p *= 1.f/p.Length();
                                float angle = atan2f(p.y, p.x);

                                angle /= M_PI/90.f;
                                angle = roundf(angle);
                                angle *= M_PI/90.f;

                                if (angle < 0.f) angle += M_PI*2;

                                G->show_numfeed(angle * (180.f/M_PI));

                                motor *s = (motor*)this->selection.e;
                                s->properties[1].v.f = angle;

                                this->state.modified = true;
                            }
                        }
                    } else if (this->selection.m) {
                        tms_infof("dragging multiselect");
                    }
                } else {
                    if (this->get_mode() == GAME_MODE_MULTISEL && (this->multi.box_select == 2 || this->multi.box_select == 3) && pid == box_select_pid) {
                        W->get_layer_point(this->cam, touch_pos[pid].x, touch_pos[pid].y, 0, &end_box_select);
                        this->multi.box_select = 3;
                    } else {
                        tvec3 lastproj;
                        W->get_layer_point(this->cam, touch_pos[pid].x, touch_pos[pid].y, 0, &lastproj);
                        tvec2 diff = (tvec2){tproj.x, tproj.y};
                        diff = tvec2_sub(diff, (tvec2){lastproj.x, lastproj.y});

                        if (settings["smooth_cam"]->v.b) {
                            this->cam_vel.x -= diff.x * settings["cam_speed_modifier"]->v.f * 10.f;
                            this->cam_vel.y -= diff.y * settings["cam_speed_modifier"]->v.f * 10.f;
                        } else {
                            this->cam_move(
                                    diff.x * settings["cam_speed_modifier"]->v.f,
                                    diff.y * settings["cam_speed_modifier"]->v.f,
                                    0);
                        }
                    }
                }

                touch_proj[pid] = (tvec2){tproj.x, tproj.y};
                touch_pos[pid] = tdown;
                //tms_infof("touch_time set to 0");
                touch_time[pid] = 0;
                //touch_time[pid] = _tms.last_time;
            } else {
                //touch_time[pid] = _tms.last_time;
            }
        }
    } else if (ev->type == TMS_EV_POINTER_UP) {
        int pid = ev->data.motion.pointer_id;
        if (!down[pid]) return T_OK;
        cam_move_x[0] = 0.f;
        cam_move_x[1] = 0.f;
        cam_move_y[0] = 0.f;
        cam_move_y[1] = 0.f;
        down[pid] = false;
        snap[pid] = false;

        //touch_time[pid] = _tms.last_time;

        if (this->menu_handle_event(ev))
            return T_OK;

        if (zooming && (pid == 0 || pid == 1)) {
            zoom_stopped = true;
            if (!down[0] && !down[1])
                zooming = false;
        } else {
            if (this->get_mode() == GAME_MODE_DRAW
                    && pid == 0
                ) {
                drawing = 0;
            } else if (this->get_mode() == GAME_MODE_QUICK_PLUG) {
                W->query(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, &this->sel_p_ent, &this->sel_p_body, &this->sel_p_offs, &this->sel_p_frame, this->layer_vis);

                this->set_mode(GAME_MODE_DEFAULT);

                if (this->sel_p_ent && this->sel_p_ent->flag_active(ENTITY_IS_EDEVICE) && this->sel_p_ent != this->selection.e && this->selection.e) {
                    tms_infof("quickplug detected: %s", this->sel_p_ent->get_name());

                    for (int t=0; t<3; t++) {
                        int m1 = this->selection.e->get_edevice()->get_inout_mask(t);
                        int m2 = this->sel_p_ent->get_edevice()->get_outin_mask(t);

                        if ((m1 & m2)) {
                            this->open_socket_selector(this->selection.e, this->sel_p_ent->get_edevice());
                            tms_infof("compatible");
                            break;
                        }
                    }
                } else {
                    tms_infof("nothing detected");
                }

                return T_OK;
            } else if (this->get_mode() == GAME_MODE_MULTISEL && this->multi.box_select > 0) {
                if (this->multi.box_select == 3) {
                    this->multi.box_select = 1;

                    b2AABB aabb;

                    float lower_x = std::min(begin_box_select.x, end_box_select.x);
                    float lower_y = std::min(begin_box_select.y, end_box_select.y);

                    float upper_x = std::max(begin_box_select.x, end_box_select.x);
                    float upper_y = std::max(begin_box_select.y, end_box_select.y);

                    aabb.lowerBound.Set(lower_x, lower_y);
                    aabb.upperBound.Set(upper_x, upper_y);

                    W->b2->QueryAABB(&box_select_handler, aabb);

                    entity_set *loop = new entity_set();
                    for (entity_set::iterator it = box_select_entities.begin();
                            it != box_select_entities.end(); ++it) {
                        entity *e = *it;
                        loop->insert(e);

                        if (e->is_edevice()) {
                            /* Add all jumpers/mini-emitters/receivers */
                            edevice *edev = e->get_edevice();

                            if (edev) {
                                for (int x=0; x<edev->num_s_in; ++x) {
                                    const isocket &s = edev->s_in[x];

                                    if (s.p) {
                                        entity *p = static_cast<entity*>(s.p);

                                        switch (p->g_id) {
                                            case O_RECEIVER:
                                            case O_JUMPER:
                                                loop->insert(p);
                                                break;
                                        }
                                    }
                                }

                                for (int x=0; x<edev->num_s_out; ++x) {
                                    const isocket &s = edev->s_out[x];

                                    if (s.p) {
                                        entity *p = static_cast<entity*>(s.p);

                                        switch (p->g_id) {
                                            case O_MINI_TRANSMITTER:
                                                loop->insert(p);
                                                break;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (loop->size()) {
                        this->selection.select(loop);
                    } else {
                        delete loop;

                        if (!this->multi.additive_selection) {
                            this->selection.disable();
                        }
                    }

                    tms_debugf("got %d objects", (int)box_select_entities.size());

                    box_select_entities.clear();
                } else {
                    this->multi.box_select = 1;
                }
            }

            W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, 0, &pt[0]);
            W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, 1*LAYER_DEPTH, &pt[1]);
            W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y, 2*LAYER_DEPTH, &pt[2]);
            W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y,
                     + .5f,
                    &half_pt[0]);
            W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y,
                    1.f * LAYER_DEPTH + .5f,
                    &half_pt[1]);
            W->get_layer_point(this->cam, (int)ev->data.motion.x, (int)ev->data.motion.y,
                    2.f * LAYER_DEPTH + .5f,
                    &half_pt[2]);

            if (!dragging[pid]) {
                if (this->get_mode() == GAME_MODE_SELECT_SOCKET) {
                    if (this->check_click_socksel()) {
                        return T_OK;
                    }
                }

                if (this->get_mode() != GAME_MODE_SELECT_SOCKET) {
                    if (this->get_mode() == GAME_MODE_SELECT_CONN_TYPE) {
                        if (this->check_click_conntype(ev->data.motion.x, ev->data.motion.y)) {
                            W->step_count = 0;
                            return T_OK;
                        }
                    }

                    if (this->get_mode() == GAME_MODE_CONN_EDIT) {
                        for (std::set<connection*>::iterator i = W->connections.begin();
                                i != W->connections.end(); i++) {
                            connection *c = *i;

                            if (this->state.sandbox || (c->e->is_moveable() || c->o->is_moveable())) {
                                b2Vec2 pos = c->e->local_to_world(b2Vec2(c->p.x, c->p.y), c->f[0]);

                                int layer = c->layer;

                                tvec3 dd = tms_camera_project(this->cam, this->cam->_position.x, this->cam->_position.y,c->layer*LAYER_DEPTH+((LAYER_DEPTH/2.f)*c->multilayer));
                                tvec3 v1 = tms_camera_unproject(this->cam, 0.f, 0.f, dd.z);
                                tvec3 v2 = tms_camera_unproject(this->cam, _tms.xppcm*.5f, 0.f, dd.z);
                                float w = v2.x-v1.x;

                                if ((b2Vec2(pt[layer].x, pt[layer].y)-pos).Length() < w) {
                                    this->selection.select(c);
                                    //tms_infof("clicked existing connection");
                                    return T_OK;
                                }
                            }
                        }
                    }

                    /* see if any pending connection was clicked */

                    if (this->check_click_conn(ev->data.motion.x, ev->data.motion.y)) {
                        W->step_count = 0;
                        return T_OK;
                    }
                }

                if (this->get_mode() == GAME_MODE_SELECT_OBJECT) {
                    this->check_select_object(ev->data.motion.x, ev->data.motion.y, pid);
                } else {
                    /* if nothing was "clicked" then simply apply the new selection */
                    /* TODO: Make sure we actually select the object that's closest to the users click */
                    if (this->get_mode() != GAME_MODE_DRAW && this->get_mode() != GAME_MODE_MULTISEL) {
                        if (!dragging[0] || (!moving[0] && !rotating[0])) {
                            /* You can only use pid 1 to select entities, not to deselect entities.
                             * Pid 0 can be used freely for all purposes */
                            if ((this->sel_p_ent && pid <= 1) || pid == 0) {
                                this->selection.select(this->sel_p_ent, this->sel_p_body, this->sel_p_offs, this->sel_p_frame, true);
                            }
                            this->set_mode(GAME_MODE_DEFAULT);
                            if (this->selection.e) {
                                tms_infof("clicked %s", this->selection.e->get_name());
                            }
                        } else {
#ifdef DEBUG
                            if (moving[0]) {
                                tms_infof("moving 0");
                            }
                            if (moving[1]) {
                                tms_infof("moving 1");
                            }

                            if (rotating[0]) {
                                tms_infof("rotating 0");
                            }

                            if (rotating[1]) {
                                tms_infof("rotating 1");
                            }
#endif
                        }
                    }
                }
            } else {
                if (this->selection.e) {
                    this->recheck_all_placements();
                }
            }
        }

#ifdef TMS_BACKEND_PC
        if (pid == 1) {
            ui::open_dialog(DIALOG_SANDBOX_MENU);
            return T_OK;
        }
#endif

        moving[pid] = false;
        dragging[pid] = false;
        rotating[pid] = false;
        resizing[pid] = false;
    } else if (ev->type == TMS_EV_POINTER_SCROLL) {
        if (this->menu_handle_event(ev)) {
            return T_OK;
        }

        float z = ev->data.scroll.y < 0 ? -2.f : 2.f;

        if (settings["smooth_zoom"]->v.b) {
            this->cam_vel.z -= (z*2.f) * settings["zoom_speed"]->v.f;
        } else {
            this->cam_move(
                    0,
                    0,
                    z * settings["zoom_speed"]->v.f);
        }
    }
    return T_OK;
}

void
game::post_interact_select(entity *e)
{
    W->post_interact.insert(e);
}

int
game::interact_select(entity *e)
{
    int found = -1;

    if (!e) return -1;
    if (e->flag_active(ENTITY_IS_STATIC)) return -1;
    if (e->flag_active(ENTITY_IS_BULLET)) return -1;
    if (!e->flag_active(ENTITY_CAN_BE_GRABBED)) return -1;
    if (e != adventure::player && e->is_creature() && !static_cast<creature*>(e)->is_dead()) return -1;
    if (e->g_id == O_CHUNK) return -1;

    for (int ip=0; ip<MAX_INTERACTING; ip++) {
        found = ip;

        if (interacting[ip]) {
            destroy_mover(ip, (interacting[ip] == e));
        }
    }

    if (e == adventure::player) {
        tms_debugf("clicked player");
        if (adventure::player->inventory[RESOURCE_WOOD] >= 2) {
            adventure::player->inventory[RESOURCE_WOOD] -= 2;
            entity *l = of::create(O_LADDER_STEP);

            l->_pos = adventure::player->get_position();
            l->_angle = 0;
            l->set_layer(adventure::player->get_layer());

            G->emit(l, adventure::player, b2Vec2(0.f, 0.f), true);

            e = l;
            this->sel_p_ent = e;
            this->sel_p_offs = (tvec2){0.f, 0.f};
        } else {
            ui::message("Ladder step requires 2 wood to construct.");
            return -1;
        }
    }

    if (found != -1) {
        int ip = found;

        if (W->is_adventure() && adventure::player) {
            b2Vec2 p1 = adventure::player->get_position();
            b2Vec2 p2 = e->get_position();
            if ((p2-p1).LengthSquared() > INTERACT_REACH_SQUARED)
                return -1;

            if (e->is_protected(true)) {
                return -1;
            }

            interacting_discharge[ip] = new discharge_effect(
                    p1,
                    p2,
#if 0
                    b2Vec2(pt.x, pt.y),
#endif
                    layer[ip] * LAYER_DEPTH,
                    layer[ip] * LAYER_DEPTH,
                    8,
                    100.f
                    );
            interacting_discharge[ip]->line_width = .075f;
#if 0
            interacting_discharge_lp[ip] = interacting[ip]->world_to_body(b2Vec2(pt.x,pt.y), 0);
#endif
            interacting_discharge_lp[ip] = b2Vec2(0.f, 0.f);
            this->add_entity(interacting_discharge[ip]);

            if (e->is_creature() && W->level.flag_active(LVL_ABSORB_DEAD_ENEMIES)) {
                /* Reset the timer on our absorb if we were interacted with
                 * by the adventure robot! */
                this->timed_absorb(e->id, W->level.dead_enemy_absorb_time);
            }
        }

        interacting[ip] = e;
        interacting_p[ip] = 1;
        layer[ip] = interacting[ip]->get_layer();
        e->interacted_with = true;

#if 0
        tvec3 pt;
        W->get_layer_point(this->cam,
                (int)ev->data.motion.x,
                (int)ev->data.motion.y,
                layer[ip], &pt);
#endif

        //interacting[ip]->prepare_fadeout();
        for (int x=0; x<INTERACT_TRAIL_LEN; x++) {
            tmat4_copy(interacting_M[ip][x], interacting[ip]->M);
            tmat3_copy(interacting_N[ip][x], interacting[ip]->N);
        }

        if (mover_joint[ip]) {
            W->b2->DestroyJoint(mover_joint[ip]);
            mover_joint[ip] = 0;
        }

        b2BodyDef bd;
        bd.type = b2_kinematicBody;
        bd.position = interacting[ip]->get_position();

        b2MotorJointDef mjd;
        mjd.bodyA = W->ground;
        mjd.bodyB = interacting[ip]->get_body(0);
        mjd.linearOffset = mjd.bodyB->GetPosition();
        mjd.angularOffset = mjd.bodyB->GetAngle();
        mjd.maxForce = 3.5f;

        if (interacting[ip] && interacting[ip]->flag_active(ENTITY_IS_CRANE_PULLEY)) {
            mjd.maxTorque = 0.f;
        } else {
            mjd.maxTorque = 3.5f*M_PI;
        }

        mjd.correctionFactor = .35f;
        mjd.collideConnected = true;

        if (mjd.bodyB->GetType() != b2_staticBody && interacting[ip]->conn_ll == 0) {
            b2MassData m;
            mjd.bodyB->GetMassData(&m);

            m.mass = .0125f;
            m.I = .0125f;

            mjd.bodyB->SetMassData(&m);
            mjd.bodyB->SetAngularDamping(4.f);
            mjd.bodyB->SetLinearDamping(4.f);
            mjd.bodyB->SetGravityScale(0.f);

            mover_joint[ip] = static_cast<b2MotorJoint*>(W->b2->CreateJoint(&mjd));

            edevice *ed;
            if ((ed = e->get_edevice())) {
                ed->recreate_all_cable_joints();
            }
        }
    }

    return found;
}

#ifdef TMS_BACKEND_PC
#define BOLD_BEGIN "<b>"
#define BOLD_END "</b>"
#else
#define BOLD_BEGIN
#define BOLD_END
#endif

// used with ENTITY_HAS_TRACKER
void
game::check_select_object(int x, int y, int pid)
{
    if (this->selection.e_saved != 0) {
        entity *e = this->sel_p_ent;

        switch (this->selection.e_saved->g_id) {
            case O_ESCRIPT:
                {
                    down[pid] = false;
                    char msg[2048];

                    if (this->sel_p_ent) {
                        entity *e = this->sel_p_ent;
                        snprintf(msg, 2047,
                                      BOLD_BEGIN "Name:" BOLD_END " %s\n"
                                      BOLD_BEGIN "ID:" BOLD_END " %u\n"
                                      BOLD_BEGIN "Type ID (g_id):" BOLD_END " %u\n"
                                      BOLD_BEGIN "Position:" BOLD_END " %.2f/%.2f\n"
                                      BOLD_BEGIN "Angle:" BOLD_END " %.2f\n",
                                      e->get_name(),
                                      e->id,
                                      e->g_id,
                                      e->get_position().x, e->get_position().y,
                                      e->get_angle()
                                );
                    } else {
                        tvec3 p;
                        W->get_layer_point(this->cam, x, y, 0, &p);
                        snprintf(msg, 2047,
                                      BOLD_BEGIN "No entity selected." BOLD_END "\n"
                                      BOLD_BEGIN "Click position:" BOLD_END " %.2f/%.2f\n",
                                      p.x, p.y
                                );
                    }

                    ui::alert(msg);
                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);

                }
                break;

            case O_RC_ACTIVATOR:
                {
                    rcactivator *_of = static_cast<rcactivator*>(this->selection.e_saved);

                    down[pid] = false;
                    if (this->sel_p_ent && this->sel_p_ent->flag_active(ENTITY_IS_CONTROL_PANEL)) {
                        _of->set_property(0, (uint32_t)this->sel_p_ent->id);
                        this->add_highlight(this->sel_p_ent, false);

                        char msg[256];
                        snprintf(msg, 256, "RC Activator connected to RC#%d",  this->sel_p_ent->id);
                        ui::message(msg);
                    } else if (this->sel_p_ent && this->sel_p_ent->id == _of->id) {
                        _of->set_property(0, (uint32_t)this->sel_p_ent->id);
                        this->add_highlight(this->sel_p_ent, false);
                        ui::message("RC Activator will now deactivate any activated RC upon activation.");
                    } else {
                        ui::message("RC Activator disconnected.");
                        _of->set_property(0, (uint32_t)0);
                    }
                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);
                }
                break;

            case O_PLAYER_ACTIVATOR:
                {
                    player_activator *_of = static_cast<player_activator*>(this->selection.e_saved);

                    down[pid] = false;
                    if (this->sel_p_ent && this->sel_p_ent->flag_active(ENTITY_IS_CREATURE)) {
                        _of->set_property(0, (uint32_t)this->sel_p_ent->id);
                        this->add_highlight(this->sel_p_ent, false);

                        char msg[256];
                        snprintf(msg, 256, "Player Activator connected to %d",  this->sel_p_ent->id);
                        ui::message(msg);
                    } else {
                        ui::message("Player Activator disconnected.");
                        _of->set_property(0, (uint32_t)0);
                    }
                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);
                }
                break;

            case O_ID_FIELD:
                {
                    objectfield *_of = static_cast<objectfield*>(this->selection.e_saved);

                    down[pid] = false;
                    if (this->sel_p_ent && this->sel_p_ent != _of && this->sel_p_ent->id != 0) {
                        _of->set_property(2, (uint32_t)this->sel_p_ent->id);
                        this->add_highlight(this->sel_p_ent, false);

                        char msg[256];
                        snprintf(msg, 256, "Now tracking %s(Unique ID %d)!", this->sel_p_ent->get_name(), this->sel_p_ent->id);
                        ui::message(msg);
                    } else {
                        ui::message("Resetting ID field detection.");
                        _of->set_property(2, (uint32_t)0);
                    }
                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);
                }
                break;

            case O_VENDOR:
                {
                    vendor *v = static_cast<vendor*>(this->selection.e_saved);

                    down[pid] = false;
                    bool set = false;

                    if (e && e != v && e->id != 0) {
                        if (e->g_id == O_RESOURCE) {
                            v->properties[0].v.i = e->g_id;
                            v->properties[1].v.i = ((resource*)e)->get_resource_type();
                            ui::messagef("Vendor now accepts %s as its currency.", resource_data[v->properties[1].v.i].name);
                            set = true;
                        } else if (e->g_id == O_ITEM) {
                            v->properties[0].v.i = e->g_id;
                            v->properties[1].v.i = ((item*)e)->get_item_type();
                            ui::messagef("Vendor now accepts %s as its currency.", item_options[v->properties[1].v.i].name);
                            set = true;
                        }
                    }

                    if (!set) {
                        if (e) {
                            ui::messagef("Vendor cannot accept %s as a currency.", e->get_name());
                        } else {
                            ui::message("Vendor now accepts no currencies.");
                            v->properties[0].v.i = 0;
                            v->properties[1].v.i = 0;
                        }
                    }

                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);
                }
                break;

            case O_TARGET_SETTER:
                {
                    objectfield *_of = static_cast<objectfield*>(this->selection.e_saved);

                    down[pid] = false;
                    if (this->sel_p_ent && this->sel_p_ent != _of && this->sel_p_ent->id != 0) {
                        _of->set_property(2, (uint32_t)this->sel_p_ent->id);
                        this->add_highlight(this->sel_p_ent, false);

                        char msg[256];
                        snprintf(msg, 256, "Now setting target to %s(Unique ID %d)!", this->sel_p_ent->get_name(), this->sel_p_ent->id);
                        ui::message(msg);
                    } else {
                        ui::message("This target setter will now reset the robots target.");
                        _of->set_property(2, (uint32_t)0);
                    }
                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);
                }
                break;

            case O_CAM_TARGETER:
                {
                    camtargeter *_of = static_cast<camtargeter*>(this->selection.e_saved);

                    down[pid] = false;
                    if (this->sel_p_ent && this->sel_p_ent != _of && this->sel_p_ent->id != 0) {
                        _of->set_property(0, (uint32_t)this->sel_p_ent->id);
                        this->add_highlight(this->sel_p_ent, false);

                        char msg[256];
                        snprintf(msg, 256, "Cam targeter now following %s!", this->sel_p_ent->get_name());
                        ui::message(msg);
                    } else {
                        ui::message("Now targeting nothing.");
                        _of->set_property(0, (uint32_t)0);
                    }
                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);

                }
                break;

            case O_OBJECT_FIELD:
                {
                    objectfield *_of = static_cast<objectfield*>(this->selection.e_saved);

                    down[pid] = false;
                    if (this->sel_p_ent && this->sel_p_ent != _of && this->sel_p_ent->id != 0) {
                        _of->set_property(2, (uint32_t)this->sel_p_ent->g_id);
                        this->add_highlight(this->sel_p_ent, false);

                        char msg[256];
                        snprintf(msg, 256, "Now tracking %s!", this->sel_p_ent->get_name());
                        ui::message(msg);
                    } else {
                        ui::message("Resetting object field detection.");
                        _of->set_property(2, (uint32_t)0);
                    }
                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);
                }
                break;

            case O_MINI_EMITTER:
            case O_EMITTER:
                {
                    emitter *_e = static_cast<emitter*>(this->selection.e_saved);

                    down[pid] = false;
                    if (this->sel_p_ent && this->sel_p_ent != _e && this->sel_p_ent->id != 0) {
                        if (_e->can_handle(this->sel_p_ent)) {
                            _e->copy_properties(this->sel_p_ent);
                            _e->set_property(1, (uint32_t)this->sel_p_ent->g_id);
                            _e->set_property(2, (uint32_t)this->sel_p_ent->id);
                            this->add_highlight(this->sel_p_ent, false);

                            char msg[256];
                            snprintf(msg, 256, "Now emitting %s!", this->sel_p_ent->get_name());
                            ui::message(msg);
                        } else {
                            char msg[256];
                            snprintf(msg, 256, "Unable to emit %s.", this->sel_p_ent->get_name());
                            ui::message(msg);
                        }
                    } else {
                        ui::message("Now emitting nothing!");
                        _e->set_property(1, (uint32_t)0);
                    }
                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);
                }
                break;

            case O_MINI_ABSORBER:
            case O_ABSORBER:
                {
                    absorber *_a = static_cast<absorber*>(this->selection.e_saved);

                    down[pid] = false;
                    if (this->sel_p_ent && this->sel_p_ent != _a && this->sel_p_ent->id != 0) {
                        if (_a->can_handle(this->sel_p_ent)) {
                            _a->set_property(1, (uint32_t)this->sel_p_ent->g_id);
                            this->add_highlight(this->sel_p_ent, false);

                            char msg[256];
                            snprintf(msg, 256, "Now absorbing %s!", this->sel_p_ent->get_name());
                            ui::message(msg);
                        } else {
                            char msg[256];
                            snprintf(msg, 256, "Unable to absorb %s.", this->sel_p_ent->get_name());
                            ui::message(msg);
                        }
                    } else {
                        ui::message("Now absorbing all possible objects!");
                        _a->set_property(1, (uint32_t)0);
                    }
                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);
                }
                break;

            case O_OBJECT_FINDER:
                {
                    object_finder *_e = static_cast<object_finder*>(this->selection.e_saved);

                    down[pid] = false;
                    if (this->sel_p_ent && this->sel_p_ent != _e && this->sel_p_ent->id != 0) {
                        char msg[256];
                        snprintf(msg, 256, "Now tracking %s!", this->sel_p_ent->get_name());
                        ui::message(msg);
                        _e->set_property(0, (uint32_t)this->sel_p_ent->id);
                        this->add_highlight(this->sel_p_ent, false);
                    } else {
                        ui::message("Now tracking nothing!");
                        _e->set_property(0, (uint32_t)0);
                    }

                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);
                }
                break;

            case O_HP_CONTROL:
                {
                    hp_control *hc = static_cast<hp_control*>(this->selection.e_saved);

                    down[pid] = false;
                    if (this->sel_p_ent && this->sel_p_ent != hc && this->sel_p_ent->id != 0) {
                        if (this->sel_p_ent->g_id == O_CHECKPOINT || this->sel_p_ent->id == W->level.get_adventure_id()) {
                            hc->properties[0].v.i = W->level.get_adventure_id();
                            ui::message("Controlling HP for the adventure robot!");
                        } else {
                            char msg[256];
                            snprintf(msg, 256, "Controlling HP for %s!", this->sel_p_ent->get_name());
                            ui::message(msg);
                            hc->properties[0].v.i = this->sel_p_ent->id;
                        }
                        this->add_highlight(this->sel_p_ent, false);
                    } else {
                        ui::message("HP control reset.");
                        hc->properties[0].v.i = 0;
                    }

                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);
                }
                break;

            case O_ROBOTMAN:
                {
                    robotman *rm = static_cast<robotman*>(this->selection.e_saved);

                    down[pid] = false;
                    if (this->sel_p_ent && this->sel_p_ent != rm && this->sel_p_ent->id != 0) {
                        if (this->sel_p_ent->flag_active(ENTITY_IS_ROBOT)) {
                            ui::messagef("Managing robot with id %u.", this->sel_p_ent->id);
                            rm->properties[0].v.i = this->sel_p_ent->id;
                        }
                        this->add_highlight(this->sel_p_ent, false);
                    } else {
                        ui::message("Robot Manager target reset.");
                        rm->properties[0].v.i = 0;
                    }

                    this->selection.load();
                    this->set_mode(GAME_MODE_DEFAULT);
                }
                break;
        }
    } else {
        entity *e;
        b2Body *body_unused;
        tvec2 offs_unused;
        uint8_t frame_unused;
        W->query(this->cam, x, y, &e, &body_unused, &offs_unused, &frame_unused, this->layer_vis, true);

        this->info_btn_pressed(e);
        if (e) this->add_highlight(e, false);
        this->set_mode(GAME_MODE_DEFAULT);
    }
}

bool
game::check_click_socksel()
{
    b2Vec2 click = b2Vec2(pt[this->ss_edev->get_entity()->get_layer()].x, pt[this->ss_edev->get_entity()->get_layer()].y);

    /* loop through all socket icons and check distance */
    for (int x=0; x<this->ss_num_socks; x++) {
        b2Vec2 ipos = this->ss_socks[x]->lpos;
        //ipos *= 1.f/ipos.Length();

        if (!this->ss_edev->scaleselect) {
            float ia = atan2f(ipos.y, ipos.x);
            ia += this->ss_socks[x]->abias;
            ipos = b2Vec2(cosf(ia), sinf(ia));

            ipos *= 1.5f * this->ss_anim;
        } else {
            ipos *= this->ss_edev->scalemodifier * this->ss_anim;
        }
        ipos = this->ss_edev->get_entity()->local_to_world(ipos, 0);

        float dist = (ipos - click).Length();

        if (dist < .375f) {
            this->select_socksel(x);
            return true;
        }
    }

    return false;
}

bool
game::check_click_rotate(int x, int y)
{
    if (this->selection.e->flag_active(ENTITY_ALLOW_ROTATION) && !this->selection.e->flag_active(ENTITY_CONNECTED_TO_BREADBOARD) &&
            (W->is_paused() || this->player_can_build())) {
        tvec3 pt;
        W->get_layer_point(this->cam, x, y, this->selection.e->get_layer(), &pt);
        b2Vec2 r = selection.e->local_to_world(b2Vec2(selection.e->get_width()+1.f, 0.f), this->selection.frame);
        float length = (r - b2Vec2(pt.x, pt.y)).Length();
        //tms_infof("length: %.2f", length);
        if (length < .75f) {
            b2Vec2 p;

            if (this->selection.e->gr) {
                p = this->selection.e->gr->get_position();
            } else {
                p = this->selection.e->get_position();
            }

            float a1 = atan2f(r.y - p.y, r.x - p.x);
            float a2 = atan2f(pt.y - p.y, pt.x - p.x);

            this->rot_offs = tmath_adist(a2, a1);

            /* set the pending to the current selected to override the deselect */
            //this->sel_p_ent = this->selection.e;
            //this->sel_p_body = this->selection.b;
            //this->sel_p_offs = this->selection.offs;
            //this->sel_p_frame = this->selection.frame;
            W->query(this->cam, x, y, &this->sel_p_ent, &this->sel_p_body, &this->sel_p_offs, &this->sel_p_frame, this->layer_vis);
            return true;
        }
    }

    return false;
}

bool
game::check_click_conntype(int x, int y)
{
    tvec3 dd = tms_camera_project(this->cam, this->cam->_position.x, this->cam->_position.y,this->cs_conn->layer*LAYER_DEPTH+((LAYER_DEPTH/2.f)*this->cs_conn->multilayer));
    tvec3 v1 = tms_camera_unproject(this->cam, 0.f, 0.f, dd.z);
    tvec3 v2 = tms_camera_unproject(this->cam, _tms.xppcm*.5f, 0.f, dd.z);

    float w = v2.x-v1.x;
    b2Vec2 p1 = this->cs_conn->p + b2Vec2(-CSCONN_OFFSX*w, CSCONN_OFFSY*w);
    b2Vec2 p2 = this->cs_conn->p + b2Vec2(CSCONN_OFFSX*w, CSCONN_OFFSY*w);

    b2Vec2 point = this->cs_conn->multilayer
                    ? b2Vec2(half_pt[this->cs_conn->layer].x, half_pt[this->cs_conn->layer].y)
                    : b2Vec2(pt[this->cs_conn->layer].x, pt[this->cs_conn->layer].y);

    if ((p1 - point).Length() < w) {
        this->apply_connection(this->cs_conn, 0);
    } else if ((p2 - point).Length() < w) {
        this->apply_connection(this->cs_conn, 1);
    }

    this->cs_conn = 0;

    /* always reset the mode after the first click */
    this->set_mode(GAME_MODE_DEFAULT);
    return true;
}

bool
game::check_quick_plug(uint64_t diff, int x, int y)
{
    tms_infof("checking quickplug %" PRIu64, diff);
    if (((this->state.sandbox && W->is_paused()) || (!W->is_paused() && this->player_can_build())) && this->get_mode() == GAME_MODE_DEFAULT) {
        if (diff < 300000) {
            if (this->sel_p_ent && this->sel_p_ent->flag_active(ENTITY_IS_EDEVICE)) {
                edevice *ed = this->sel_p_ent->get_edevice();

                bool avail = false;
                for (int xx=0; xx<ed->num_s_in; xx++) {
                    if (!ed->s_in[xx].p) {
                        avail = true;
                        break;
                    }
                }
                if (!avail) {
                    for (int xx=0; xx<ed->num_s_out; xx++) {
                        if (!ed->s_out[xx].p) {
                            avail = true;
                            break;
                        }
                    }
                }

                if (avail) {
                    this->selection.select(this->sel_p_ent, this->sel_p_body, this->sel_p_offs, this->sel_p_frame, true);
                    this->set_mode(GAME_MODE_QUICK_PLUG);
                    W->get_layer_point(this->cam, x, y, this->selection.e->get_layer(), &touch_quickplug_pos);
                    tms_infof("quickplug succeed");
                    return true;
                }
            }
            tms_infof("double click");
        }
    } else {
        tms_infof("mode is %d", this->get_mode());
    }

    return false;
}

bool
game::check_click_shape_resize(int x, int y)
{
    tms_debugf("check click shape resize");

    if (this->selection.e && this->selection.e->flag_active(ENTITY_IS_RESIZABLE)) {
        tms_debugf("checking");
        entity *e = this->selection.e;
        b2PolygonShape *sh = e->get_resizable_shape();

        if (!sh) return false;

        int vertices[POLYGON_MAX_CORNERS];

        int num_verts = e->get_resizable_vertices(vertices);

        tvec3 pt;
        W->get_layer_point(this->cam, x, y, e->get_layer(), &pt);
        b2Vec2 pp = b2Vec2(pt.x, pt.y);

        for (int x=0; x<num_verts; x++) {
            b2Vec2 p = e->local_to_world(sh->m_vertices[vertices[x]], 0);

            if (b2Distance(pp, p) < .25f) {
                tms_debugf("clicked corner %u (%u), local coord %f %f", vertices[x], x, sh->m_vertices[vertices[x]].x, sh->m_vertices[vertices[x]].y);
                resize_type = 0;
                resize_index = vertices[x];
                return true;
            }
        }

        /* TODO: check edges */
    }

    resize_index = -1;

    return false;
}

void
game::handle_shape_resize(float x, float y)
{
    entity *e = this->selection.e;

    if (!e) return;

    b2Vec2 projected = e->world_to_local(b2Vec2(x,y), 0);

    if (resize_type == 0) {
        /* snap the placements */
        projected.x = roundf(projected.x * 8.f)/8.f;
        projected.y = roundf(projected.y * 8.f)/8.f;

        b2PolygonShape *sh = e->get_resizable_shape();

        if (sh && resize_index >= 0) {
            b2Vec2 saved = sh->m_vertices[resize_index];
            sh->m_vertices[resize_index] = projected;

            /* perform sanity checks, make sure no edge angle makes a concave polygon */
            /* also make sure the distance to the previous or next vertex is greater than 1./16. */
            bool valid = sh->Validate()
                        && sh->ValidateMinEdgeLength(1.f/16.f)
                        //&& sh->ValidateVertexOrder()
                        && sh->ValidateAreaMin(.125f)
                        ;

            if (!valid) {
                tms_debugf("invalid polygon");
                sh->m_vertices[resize_index] = saved;
            } else {
                if (e->on_resize_vertex(resize_index, projected)) {
                    sh->RecalculateCentroid();
                    sh->RecalculateNormals();
                    e->get_body(0)->ResetMassData();
                } else {
                    sh->m_vertices[resize_index] = saved;
                }
            }
        }
    } else {
        /* TODO: edges */
    }
}

int
game::get_selected_shape_corner()
{
    return resize_index;
}

void
game::render_shape_resize()
{
    entity *e = this->selection.e;

    b2PolygonShape *sh = e->get_resizable_shape();

    if (!sh) return;

    int vertices[POLYGON_MAX_CORNERS];

    int num_verts = e->get_resizable_vertices(vertices);

    for (int x=0; x<num_verts; x++) {
        b2Vec2 p = e->local_to_world(sh->m_vertices[vertices[x]], 0);

        float size = .125f + (vertices[x] == resize_index)*.125f;

        if (vertices[x] == resize_index) {
            tms_ddraw_set_color(this->dd, .7f, .7f, 1.3f, .9f);
        } else {
            tms_ddraw_set_color(this->dd, .5f, .5f, 1.f, .8f);
        }

        tms_ddraw_circle(this->dd,
                p.x, p.y,
                size, size);

        tms_ddraw_set_color(this->dd, 1.f, 1.f, 1.f, .8f);
        tms_ddraw_lcircle(this->dd,
                p.x, p.y,
                size, size);
    }
}

bool
game::check_click_conn(int x, int y)
{
    c_map::iterator i = this->pairs.begin();

    for (;i != this->pairs.end(); ) {
        tms_infof("checking pair");
        connection *c = i->second;

        b2Vec2 point = c->multilayer
                        ? b2Vec2(half_pt[c->layer].x, half_pt[c->layer].y)
                        : b2Vec2(pt[c->layer].x, pt[c->layer].y);

        tvec3 dd = tms_camera_project(this->cam, this->cam->_position.x, this->cam->_position.y,c->layer*LAYER_DEPTH+((LAYER_DEPTH/2.f)*c->multilayer));
        tvec3 v1 = tms_camera_unproject(this->cam, 0.f, 0.f, dd.z);
        tvec3 v2 = tms_camera_unproject(this->cam, _tms.xppcm*.5f, 0.f, dd.z);
        float w = v2.x-v1.x;

        if ((point - c->p).Length() < w) {
            tms_infof("yes");
            if (c->typeselect) {
                /* let the user choose between the available types, like
                 * a weld joint or a pivot joint */
                this->set_mode(GAME_MODE_SELECT_CONN_TYPE);
                this->cs_timer = 0.f;
                this->cs_conn = c;
                return true;
            }

            if (W->is_adventure() && W->is_playing()) {
                this->apply_connection(c, c->option);
            } else {
                this->apply_connection(c, 0);
            }

            this->pairs.erase(i++);
            G->refresh_widgets();

            return true;
        } else {
            i ++;
        }
    }
    return false;
}

void
game::recheck_all_placements()
{
    W->step_count = 0;
    if (this->check_placement_allowed(this->selection.e)) {
        this->selection.e->on_release(this);

        if (this->selection.e->get_body(0) && this->selection.e->get_body(0)->GetType() == b2_kinematicBody)
            this->selection.e->get_body(0)->SetType(b2_dynamicBody);

        this->remove_highlight(this->selection.e);
    } else {
        this->add_highlight(this->selection.e, true);
        this->selection.e->on_release(this);
        this->selection.e->body->SetType(b2_kinematicBody);
        //this->selection.e->body->GetFixtureList()->SetFilterData(world::get_filter_for_layer(this->selection.e->get_layer(), 0));
        tms_infof("ERROR");
    }

    /* re-check all erroneous objects */
    for (int x=0; x<NUM_HL; x++) {
        if (this->hls[x].type & HL_TYPE_ERROR) {
            if (this->hls[x].e && this->hls[x].e != this->selection.e) {
                if (this->check_placement_allowed(this->hls[x].e)) {
                    if (this->hls[x].e->get_body(0)) {
                        if (this->hls[x].e->get_body(0)->GetType() == b2_kinematicBody)
                            this->hls[x].e->get_body(0)->SetType(b2_dynamicBody);
                    }
                    this->remove_highlight(this->hls[x].e);
                }
            }
        }
    }
}

bool
overlap_query::ReportFixture(b2Fixture *fx)
{
    b2Body* body = fx->GetBody();
    b2Shape* shape = fx->GetShape();

    entity *e = static_cast<entity*>(fx->GetUserData());

    if (e && (body->GetType() == b2_staticBody || !W->is_paused()) && !fx->IsSensor()) {

        if (e->get_layer() - test_e->get_layer() == this->desired_layerdist && (e->layer_mask & test_e->layer_mask) != 0) {
            if (b2TestOverlap(shape, 0, this->test_sh, 0, body->GetTransform(), this->test_bd->GetTransform()))
            {
                this->overlap = true;
                return false;
            }
        }
    }

    return true;
}

bool
game::ingame_layerswitch_test(entity *e, int dir)
{
    if (e->body && e->conn_ll == 0) {
        b2AABB aabb;
        overlap_query oq;
        oq.desired_layerdist = dir;

        b2Shape *shh = e->body->GetFixtureList()->GetShape();

        b2CircleShape c_sh;
        b2PolygonShape p_sh;

        oq.test_sh = shh;

        oq.test_e = e;
        oq.test_bd = e->body;

        e->body->GetFixtureList()->GetShape()->ComputeAABB(&aabb, e->body->GetTransform(), 0);
        //W->b2->QueryAABB(&oq, aabb);
        W->query_aabb(&oq, aabb);

        return !oq.overlap;
    }

    return false;
}

bool
game::check_placement_allowed(entity *e)
{
    if (this->state.sandbox)
        return true;

    if (e->body && e->conn_ll == 0 && (e->body->GetType() == b2_dynamicBody || e->body->GetType() == b2_kinematicBody)) {
        b2AABB aabb;
        overlap_query oq;
        oq.desired_layerdist = 0;

        b2Shape *shh = e->body->GetFixtureList()->GetShape();

        b2CircleShape c_sh;
        b2PolygonShape p_sh;

        /* bias the size of the shape */
        if (shh->m_type == b2Shape::e_circle) {
            c_sh = *((b2CircleShape*)shh);
            c_sh.m_radius -= fminf(OVERLAP_THRESHOLD, c_sh.m_radius/2.f);
            oq.test_sh = &c_sh;
        } else if ((shh->m_type = b2Shape::e_polygon)) {
            p_sh = *((b2PolygonShape*)shh);

            for (int x=0; x<p_sh.m_count; x++) {
                if (p_sh.m_vertices[x].x > 0.f) p_sh.m_vertices[x].x -= fminf(OVERLAP_THRESHOLD, p_sh.m_vertices[x].x/2.f);
                if (p_sh.m_vertices[x].x < 0.f) p_sh.m_vertices[x].x += fminf(OVERLAP_THRESHOLD, -p_sh.m_vertices[x].x/2.f);
                if (p_sh.m_vertices[x].y > 0.f) p_sh.m_vertices[x].y -= fminf(OVERLAP_THRESHOLD, p_sh.m_vertices[x].y/2.f);
                if (p_sh.m_vertices[x].y < 0.f) p_sh.m_vertices[x].y += fminf(OVERLAP_THRESHOLD, -p_sh.m_vertices[x].y/2.f);
            }
            oq.test_sh = &p_sh;
        } else {
            return true;
        }

        oq.test_e = e;
        oq.test_bd = e->body;

        e->body->GetFixtureList()->GetShape()->ComputeAABB(&aabb, e->body->GetTransform(), 0);
        W->b2->QueryAABB(&oq, aabb);

        return !oq.overlap;
    }

    return true;
}

int
game::handle_input(tms::event *ev, int action)
{
    if (ev->type == TMS_EV_KEY_PRESS || ev->type == TMS_EV_KEY_UP || ev->type == TMS_EV_KEY_REPEAT) {
        // Whenever a keyevent is received, we store its keymod state
        this->previous_keymod = this->current_keymod;
        this->current_keymod = ev->data.key.mod;
    }

    if (pscreen::handle_input(ev, action) == EVENT_DONE) {
        return EVENT_DONE;
    }

    switch (ev->type) {
        case TMS_EV_POINTER_DOWN:
#ifdef TMS_BACKEND_PC
# ifdef DEBUG
            this->print_screen_point_info((int)ev->data.motion.x, (int)ev->data.motion.y);
# endif
# ifndef NO_UI
            if (prompt_is_open) return T_OK;
# endif

            this->hov_ent = 0;
            this->hov_text->active = false;
#endif

            P.focused = 1;
            break;

        case TMS_EV_POINTER_UP:

            move_time = _tms.last_time;
            move_pos = tvec2f(ev->data.motion.x, ev->data.motion.y);
            move_queried = false;
            break;

        case TMS_EV_POINTER_MOVE:
            move_time = _tms.last_time;
            move_pos = tvec2f(ev->data.motion.x, ev->data.motion.y);
            move_queried = false;
            break;

        case TMS_EV_POINTER_DRAG:
#ifdef TMS_BACKEND_PC
            this->hov_ent = 0;
            this->hov_text->active = false;
#endif
            break;
    }

    if (W->is_paused()) {
        return this->handle_input_paused(ev, action);
    }

    return this->handle_input_playing(ev, action);
}

void
game::animate_disconnect(entity *e)
{
    /* create disconnect animations */
    connection *c = e->conn_ll;
    while (c) {
        this->add_ca(1.f, c->get_position());
        c = c->get_next(e);
    }
}

bool
game::damage_entity(entity *e, b2Fixture *fx, float dmg, const b2Vec2 &world_point,
        damage_type dmg_type, uint8_t damage_source, uint32_t attacker_id,
        bool damage_creature/*=true*/,
        bool damage_block/*=true*/,
        bool damage_interactive/*=true*/,
        bool damage_plant/*=true*/
        )
{
    if (e->is_interactive()) {
        if (damage_interactive) {
            this->damage_interactive(e, fx, fx->GetUserData2(), dmg, world_point, dmg_type);

            return true;
        }

        return false;
    }

    if (e->is_creature()) {
        if (damage_creature) {
            creature *c = static_cast<creature*>(e);

            c->damage(dmg, fx, dmg_type, damage_source, attacker_id);

            return true;
        }

        return false;
    }

    if (e->g_id == O_PLANT) {
        if (damage_plant) {
            this->damage_interactive(e, fx, fx->GetUserData2(), dmg, world_point, dmg_type);

            return true;
        }

        return false;
    }

    if (e->g_id == O_CHUNK || e->g_id == O_TPIXEL) {
        if (damage_block) {
            this->damage_interactive(e, fx, fx->GetUserData2(), dmg, world_point, dmg_type);

            return true;
        }

        return false;
    }

    return false;
}

void
game::damage_interactive(entity *e, b2Fixture *f, void *udata2, float dmg, const b2Vec2 &world_point, damage_type dmg_type)
{
    if (e->g_id == O_TPIXEL || e->g_id == O_CHUNK) {
        this->damage_tpixel(e, f, udata2, dmg, world_point, dmg_type);
    } else if (e->g_id == O_PLANT) {
        static_cast<plant*>(e)->damage_section(static_cast<plant_section*>(udata2), dmg, dmg_type);
    } else {
        e->interactive_hp -= dmg;
        tms_infof("interactive hp: %.2f", e->interactive_hp);
        if (e->interactive_hp <= 0.f) {
            if (this->absorb(e)) {
                W->events[WORLD_EVENT_INTERACTIVE_DESTROY] ++;
            }
        } else {
            this->add_highlight(e, HL_PRESET_DEFAULT, .2f);
        }
    }
}

void
game::damage_tpixel(entity *e, b2Fixture *f, void *udata2, float dmg, const b2Vec2 &world_point, damage_type dmg_type)
{
    struct tpixel_desc *desc = (struct tpixel_desc *)udata2;

    bool do_fadeout = false;
    float fadeout_time = .25f;
    level_chunk *c;

    int real_x, real_y;

    if (!desc) {
        tms_errorf("tpixel %p does not have desc [%d]", e, e->g_id);
        return;
    }

    if (desc->hp < 0.f) {
        /* this pixel has already been destroyed */
        return;
    }

    switch (dmg_type) {
        case DAMAGE_TYPE_FORCE:
        case DAMAGE_TYPE_ELECTRICITY:
        case DAMAGE_TYPE_OTHER:
        case DAMAGE_TYPE_HEAVY_SHARP:
            break;

        default:
            /* ignore all other damage types */
            return;
    }

    int layer = 0;
    if (f) layer = world::fixture_get_layer(f);

    int size = desc->size;

    if (e->g_id == O_CHUNK) {
        c = static_cast<level_chunk*>(e);
        do_fadeout = true;
        real_x = desc->get_local_x();
        real_y = desc->get_local_y();

        /* if this is a bigger pixel, try to find the nearest "subpixel" that was damaged */
        int nearest_x=0, nearest_y=0;
        if (size > 0) {
            int usize = (1<<desc->size);
            float nearest = 10000.f;
            for (int y=0; y<usize; y++) {
                for (int x=0; x<usize; x++) {
                    float d = b2DistanceSquared(world_point,
                            b2Vec2(
                                c->pos_x*8.f + (float)(real_x+x)*.5f,
                                c->pos_y*8.f + (float)(real_y+y)*.5f
                                ));
                    if (d < nearest) {
                        nearest_x = x;
                        nearest_y = y;
                        nearest = d;
                    }
                }
            }
        }
        real_x += nearest_x;
        real_y += nearest_y;

        if (this->state.new_adventure) {
            if (layer == 0) {
                if (c->pixels[1][real_y][real_x]) {
                    if (dmg_type == DAMAGE_TYPE_ELECTRICITY) {
                        // We only notify the user of the zapping rules he's
                        // trying to mine with the Zapper.
                        ui::message("Please zap in the two outer layers!");
                    }
                    return;
                }
            }
        }
    }

    desc->hp -= dmg;

    if (desc->hp <= 0.f) {
        fadeout_time = .55f;
        this->force_static_update = 2;

        /*
        tpixel *p = static_cast<tpixel*>(e);
        float wsize = (float)(1 << size)*.25f;
        float whs = wsize/2.f;
        float angle = p->get_angle();
        b2Vec2 pos = p->get_position();
        int layer = p->get_layer();

        float cs, sn;
        tmath_sincos(p->get_angle(), &sn, &cs);

        float rest = -p->interactive_hp;
        int diff = 0;
        if (rest > 20) rest = 20;
        do {
            rest /= 4.f;
            size --;
            if (diff > 0)
                whs = whs+whs/2.f;
            diff ++;
        } while (rest >= 1.f && size>0);

        b2Vec2 base = pos;
        base.x -= (cs-sn)*whs;
        base.y -= (sn+cs)*whs;

        // TODO: set new pixels hp to rest

        if (size >= 0 && rest < 1.f) {
            int count = 2*diff;
            for (int x=0; x<count; x++) {
                for (int y=0; y<count; y++) {
                    tpixel *n = static_cast<tpixel*>(of::create(O_TPIXEL));

                    b2Vec2 pp = b2Vec2(
                            ((float)x/(float)count)*cs - ((float)y/(float)count)*sn,
                            ((float)x/(float)count)*sn + ((float)y/(float)count)*cs
                            );

                    n->properties[0].v.i8 = size;
                    n->properties[1].v.i8 = p->properties[1].v.i8;
                    n->_pos.x = base.x+pp.x*wsize*2;
                    n->_pos.y = base.y+pp.y*wsize*2;
                    n->_angle = angle;
                    n->set_layer(layer);
                    this->emit(n, 0, b2Vec2(0.f,0.f));
                }
            }
        } else {
            p->drop_loot(1+((size+1)*4));
        }
        */

        switch (e->g_id) {
            case O_TPIXEL:
                {
                    if (this->absorb(e)) {
                        static_cast<tpixel*>(e)->drop_loot(1+((size+1)*4));
                    }
                }
                break;

            case O_CHUNK:
                {
                    c->pixels[layer][real_y][real_x] = 0;

                    /* remerge the chunk to fill in the holes */
                    c->merge(desc->get_local_x(), desc->get_local_y(), layer, desc->get_local_x()+(1<<desc->size), desc->get_local_y()+(1<<desc->size), layer+1);
                    W->to_be_reloaded.insert(c);
                }
                break;
        }
    }


    if (do_fadeout) {
        fadeout_event *ev = new fadeout_event();
        fadeout_entity fe;

        /* create an arbitrary fadeout entity */
        tpixel *t = new tpixel();
        t->_pos = b2Vec2(c->pos_x*8.f + real_x*.5f, c->pos_y*8.f + real_y*.5f);
        t->_angle = 0.f;
        t->properties[0].v.i8 = 0;
        //t->set_block_type(desc->material);
        if (fadeout_time < .5f) {
            t->set_block_type(0);
        } else {
            t->set_block_type(4);
        }
        t->set_layer(layer);
        t->on_load(false, false);
        t->prepare_fadeout();
        t->M[14] += .05f;

        fe.velocity = b2Vec2(0.f, 0.f);
        fe.e = t;
        fe.do_free = true;
        ev->time = fadeout_time;
        ev->entities.push_back(fe);

        G->lock();
        G->fadeouts.insert(ev);
        G->unlock();
    }
}

void
game::emit_partial_from_buffer(const char *buf, uint16_t buf_len, b2Vec2 position)
{
    tms_infof("emit partial from buffer");
    tms_assertf(W->is_paused() == false, "emit (multi) called when world was paused");

    pending_emit ee(buf, buf_len, position);
    W->to_be_emitted.push_back(ee);
}

void
game::emit(entity *e, entity *emitter, b2Vec2 velocity, bool immediate)
{
    tms_assertf(W->is_paused() == false, "emit (single) called when world was paused");

    pending_emit ee(e, emitter, velocity);

    W->to_be_emitted.push_back(ee);

    if (immediate) {
        W->emit_all();
    }
}

/**
 * post_emit should be used whenever the emit needs to be placed in a "dangerous" place.
 * Dangerous places include any function that are called when an entity is loaded,
 * added to world, emitted. Such as:
 * init()
 * setup()
 * on_entity_play()
 * on_load()
 * add_to_world()
 **/
void
game::post_emit(entity *e, entity *emitter, b2Vec2 velocity)
{
    tms_assertf(W->is_paused() == false, "emit (single) called when world was paused");

    pending_emit ee(e, emitter, velocity);

    W->post_to_be_emitted.push_back(ee);
}

void
game::absorb(std::set<entity *> *loop)
{
    tms_assertf(W->is_paused() == false, "absorb (multi) called when world was paused");

    tms_infof("absorbing loop");

    for (std::set<entity*>::iterator i = loop->begin(); i != loop->end(); i++) {
        ((entity*)*i)->set_flag(ENTITY_IS_ABSORBED, true);
        W->to_be_absorbed.insert(pending_absorb(*i));
    }
}

bool
game::absorb(entity *e, bool include_connection/*=false*/, entity *absorber/*=0*/, b2Vec2 absorber_point/*=b2Vec2(0.f, 0.f)*/, uint8_t absorber_frame/*=0*/)
{
    tms_assertf(W->is_paused() == false, "absorb (simple) called when world was paused");

    // return false for any entities that have already been absorbed
    if (e->flag_active(ENTITY_IS_ABSORBED)) return false;

    if (!e->conn_ll || include_connection) { /* do not absorb connected objects */
        e->set_flag(ENTITY_IS_ABSORBED, true);

        W->to_be_absorbed.insert(pending_absorb(e, absorber, absorber_point, absorber_frame));

        if (this->selection.e == e) {
            this->selection.disable();
        }
        return true;
    }

    return false;
}

bool
game::timed_absorb(uint32_t id, double time)
{
    tms_assertf(W->is_paused() == false, "absorb (timed) called when world was paused");

    int64_t itime = (int64_t)(time * 1000000.0);
    std::pair<std::map<uint32_t, int64_t>::iterator, bool> ret;
    ret = W->timed_absorb.insert(std::pair<uint32_t, int64_t>(id, itime));

    if (!ret.second) {
        (ret.first)->second = itime;
    }

    return true;
}

bool
game::timed_absorb(entity *e, double time)
{
    if (e) {
        if (e->flag_active(ENTITY_IS_ABSORBED)) return false;

        if (!e->conn_ll) { /* do not absorb connected objects */
            return this->timed_absorb(e->id, time);
        }
    }

    return false;
}

/* construct an entity at the mouse position */
entity*
game::editor_construct_entity(uint32_t g_id, int pid/*=0*/, bool force_on_pid/*=false*/, b2Vec2 offs/*=b2Vec2(0.f,0.f)*/)
{
    if (!this->state.sandbox) {
        tms_errorf("can not create an entity if not sandbox");
        return 0;
    }

    // Override for partial
    if (g_id == O_DAMPER_2) {
        g_id = O_DAMPER;
    } else if (g_id == O_RUBBERBAND_2) {
        g_id = O_RUBBERBAND;
    } else if (g_id == O_OPEN_PIVOT_2) {
        g_id = O_OPEN_PIVOT;
    }

    tvec3 pos;
#ifdef TMS_BACKEND_PC
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    W->get_layer_point(this->cam, mx, _tms.window_height-my, 0.f, &pos);
#else
    if (force_on_pid) {
        W->get_layer_point(this->cam, touch_proj[pid].x, touch_proj[pid].y, 0.f, &pos);
    } else {
        pos = this->cam->_position;
    }
#endif

    pos.x += offs.x;
    pos.y += offs.y;

    entity *e = of::create(g_id);

    if (!e) {
        tms_errorf("Unable to create an object with g_id %u", g_id);
        return 0;
    }

    e->_angle = 0.f;
    e->_pos = b2Vec2(pos.x, pos.y);
    e->set_layer(this->state.edit_layer);

    e->ghost_update();

    if (this->selection.e && e->compatible_with(this->selection.e) && this->selection.e->g_id != O_ROPE && (this->selection.e->g_id != O_TPIXEL)) {
        /* copy properties from the selected object */
        this->copy_properties(e, this->selection.e);

        e->_angle = this->selection.e->get_angle();
        e->set_angle(this->selection.e->get_angle());
        e->set_layer(this->selection.e->get_layer());
        e->set_moveable(this->selection.e->is_moveable());

    }

    e->on_load(true, false);

    W->add(e);
    this->add_entity(e);

    e->construct();
    e->on_pause();

    if (this->get_mode() == GAME_MODE_DRAW) {
        basepixel *p = static_cast<basepixel*>(e);
        basepixel::radius = BASE_PIXEL_RADIUS; /* Set pixel radius back to its default value */

        if (p->got_pos && (p->get_position().y > -((float)W->level.size_y[0]) || W->ground_fx[3] == 0)) {
            //p->merge_neighbours();
            return e;
        } else {
            int mid = (p->search_width-1)/2;
            int index = mid + (mid * p->search_width); // we offset 'middle' a little bit
            if (p->found[index]) {
#if 0
                //if (p->found[index]->properties[1].v.i8 != this->brush_material) {
                    int yy;
                    for (yy=mid; yy<p->search_width; yy++) {
                        int ii = mid+(yy*p->search_width);
                        if (!p->found[ii]) break;
                    }
                    if (yy-mid >= p->search_width/2)
                        p->found[index]->properties[1].v.i8 = 1;
                    else
                        p->found[index]->properties[1].v.i8 = this->brush_material;
                    /*
                    this->remove_entity(p->found[index]);
                    W->remove(p->found[index]);
                    delete p->found[index];
                    */
                //}
#endif
            }
            this->remove_entity(e);
            W->remove(e);
            delete e;
            return 0;
        }
    }

    if (e->type == ENTITY_CABLE) {
        cable *c = static_cast<cable*>(e);
        this->selection.select(c->p[0], c->p[0]->get_body(0), (tvec2){0,0}, 0, false);
    } else {
        this->selection.select(e, NULL, (tvec2){0,0}, 0, false);
    }

    if ((int)e->g_id != this->recent[0]) {
        int p = -1;
        for (int x=0; x<MAX_RECENT; x++) {
            if (this->recent[x] == (int)e->g_id) {
                p = x;
                break;
            }
        }

        if (p != -1) {
            for (int x=p; x>0; x--)
                this->recent[x] = this->recent[x-1];
        }
        for (int x=MAX_RECENT-1; x>0; x--)
            this->recent[x] = this->recent[x-1];

        this->recent[0] = (int)e->g_id;
    }

    if (W->is_paused()) {
        ui::emit_signal(SIGNAL_ENTITY_CONSTRUCTED, UINT_TO_VOID(e->id));
    }

    this->state.modified = true;

    return e;
}

/* construct an item at the mouse position */
entity*
game::editor_construct_item(uint32_t item_id)
{
    uint32_t g_id = O_ITEM;
    if (!this->state.sandbox) {
        tms_errorf("can not create an entity if not sandbox");
        return 0;
    }

    tvec3 pos;
#ifdef TMS_BACKEND_PC
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    W->get_layer_point(this->cam, mx, _tms.window_height-my, 0.f, &pos);
#else
    pos = this->cam->_position;
#endif

    entity *e = of::create(g_id);

    if (!e) {
        tms_errorf("Unable to create an object with g_id %u", g_id);
        return 0;
    }

    e->_angle = 0.f;
    e->_pos = b2Vec2(pos.x, pos.y);
    e->set_layer(this->state.edit_layer);
    ((item*)e)->set_item_type(item_id);

    e->ghost_update();

    if (this->selection.e && e->g_id == this->selection.e->g_id && this->selection.e->g_id != 12 && (this->selection.e->g_id != O_TPIXEL || this->get_mode() != GAME_MODE_DRAW)) {
        e->_angle = this->selection.e->get_angle();
        e->set_angle(this->selection.e->get_angle());
        e->set_layer(this->selection.e->get_layer());
        e->set_moveable(this->selection.e->is_moveable());

    }

    e->on_load(true, false);

    W->add(e);
    this->add_entity(e);

    e->construct();
    e->on_pause();

    this->selection.select(e, NULL, (tvec2){0,0}, 0, false);

    return e;
}

/* construct an item at the mouse position */
entity*
game::editor_construct_decoration(uint32_t decoration_id)
{
    uint32_t g_id = O_DECORATION;
    if (!this->state.sandbox) {
        tms_errorf("can not create an entity if not sandbox");
        return 0;
    }

    tvec3 pos;
#ifdef TMS_BACKEND_PC
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    W->get_layer_point(this->cam, mx, _tms.window_height-my, 0.f, &pos);
#else
    pos = this->cam->_position;
#endif

    entity *e = of::create(g_id);

    if (!e) {
        tms_errorf("Unable to create an object with g_id %u", g_id);
        return 0;
    }

    e->_angle = 0.f;
    e->_pos = b2Vec2(pos.x, pos.y);
    e->set_layer(this->state.edit_layer);
    ((decoration*)e)->set_decoration_type(decoration_id);

    e->ghost_update();

    if (this->selection.e && e->g_id == this->selection.e->g_id && this->selection.e->g_id != 12 && (this->selection.e->g_id != O_TPIXEL || this->get_mode() != GAME_MODE_DRAW)) {
        e->_angle = this->selection.e->get_angle();
        e->set_angle(this->selection.e->get_angle());
        e->set_layer(this->selection.e->get_layer());
        e->set_moveable(this->selection.e->is_moveable());

    }

    e->on_load(true, false);

    W->add(e);
    this->add_entity(e);

    e->construct();
    e->on_pause();

    this->selection.select(e, NULL, (tvec2){0,0}, 0, false);

    return e;
}

void
game::update_last_cursor_pos(int x, int y)
{
    this->last_cursor_pos_x = x;
    this->last_cursor_pos_y = y;
}

void
game::refresh_last_cursor_pos()
{
#ifdef TMS_BACKEND_PC
    SDL_GetMouseState(&this->last_cursor_pos_x, &this->last_cursor_pos_y);
    this->last_cursor_pos_y = _tms.window_height - this->last_cursor_pos_y;
#endif
}

b2Vec2
game::get_last_cursor_pos(int layer)
{
    this->refresh_last_cursor_pos();

    tvec3 pt;
    W->get_layer_point(this->cam, this->last_cursor_pos_x, this->last_cursor_pos_y, layer, &pt);
    return b2Vec2(pt.x, pt.y);
}

void
game::puzzle_play(int type)
{
    if (type == PUZZLE_SIMULATE) {
        this->save(false, true);
        this->do_play();
    } else if (type == PUZZLE_TEST_PLAY) {
        this->save(false);

        this->open_play(LEVEL_LOCAL, W->level.local_id, NULL, true);
        ui::message("Now testplaying your level! Press B to return.");
    } else {
        tms_warnf("Invalid input for puzzle play");
    }
}

void
game::refresh_score()
{
    this->state.m_score = this->get_real_score();
}

void
game::add_score(int score)
{
    if (this->state.finished) {
        return;
    }

    if (score > 0) {
        this->score_highlight += score/250.f;
        this->score_highlight = fminf(this->score_highlight, 1.f);
    }

    int new_score = this->get_real_score() + score;

    if (new_score < 0) {
        new_score = 0;
    }

    W->score_helper = new_score ^ SCORE_XOR;

    this->refresh_score();

    if (W->is_playing()) {
        if (this->get_score() >= W->level.final_score && W->level.final_score != 0) {
            this->finish(true);
        }
    }
}

void
game::set_score(int new_score)
{
    if (this->state.finished) {
        return;
    }

    if (this->get_score() != new_score) {
        this->score_highlight += (new_score - this->get_score()) / 250.f;
        this->score_highlight = fminf(this->score_highlight, 1.f);
    }

    if (new_score < 0) {
        new_score = 0;
    }

    W->score_helper = new_score ^ SCORE_XOR;

    this->refresh_score();

    if (W->is_playing()) {
        if (this->get_score() >= W->level.final_score && W->level.final_score != 0) {
            this->finish(true);
        }
    }
}

void
game::destroy_joint(b2Joint *j)
{
    if (W->is_paused()) return;
    W->to_be_destroyed.insert(j);
}

void
game::add_destructable_joint(b2Joint *j, float max_force)
{
    if (W->is_paused()) return;
    W->destructable_joints.insert(std::make_pair(j, max_force));
}

void
selection_handler::select(entity_set *new_m)
{
    if (G->multi.additive_selection && this->m) {
        /* First we check if this will be an addition or a removal */
        bool remove = false;
        if (G->sel_p_ent) {
            if (this->m->find(G->sel_p_ent) != this->m->end()) {
                remove = true;
            }
        } else {
            /* we will loop through all new entities and if all of them are already added,
             * we will remove them! */
            remove = true;

            for (entity_set::iterator it = new_m->begin(); it != new_m->end(); ++it) {
                if (this->m->find(*it) == this->m->end()) {
                    remove = false;
                    break;
                }
            }
        }

        G->add_highlight_multi(new_m, HL_PRESET_DEFAULT_MULTI, 1.f);

        if (remove) {
            tms_infof("selection: remove multi (%d)", (int)new_m->size());
            for (entity_set::iterator it = new_m->begin(); it != new_m->end(); ++it) {
                this->m->erase(*it);
            }
        } else {
            this->m->insert(new_m->begin(), new_m->end());
            tms_infof("selection: add multi (%d)", (int)new_m->size());
        }
    } else {
        this->disable(false);
        this->m = new_m;

        if (this->m) {
            tms_infof("selection: new multi (%d)", (int)new_m->size());
            G->add_highlight_multi(new_m, HL_PRESET_NO_FREE_MULTI, 1.f);
        }
    }

    uint8_t type = HL_PRESET_NO_FREE_MULTI + HL_TYPE_PERSISTENT + HL_TYPE_TINT;
    G->add_highlight_multi(this->m, type, 0.01f);

    G->refresh_widgets();
}

void
selection_handler::select(connection *c)
{
    this->disable(false);
    this->c = c;

    if (c) {
        tms_infof("selection: connection (%s:%p) (g_id: <%d,%d>, id: <%d,%d>)",
                c->type == CONN_GROUP ? "CONN_GROUP" :
                c->type == CONN_WELD ? "CONN_WELD" :
                c->type == CONN_PLATE ? "CONN_PLATE" :
                c->type == CONN_PIVOT ? "CONN_PIVOT" :
                c->type == CONN_CUSTOM ? "CONN_CUSTOM"
                : "<invalid>",
                c,
                c->e->g_id, c->o->g_id, c->e->id, c->o->id);
    }

    G->refresh_widgets();
}

void
selection_handler::select(entity *e, b2Body *b, tvec2 offs, uint8_t frame, bool ui)
{
    G->set_mode(GAME_MODE_DEFAULT);

    this->disable(false);

    if (e) G->starred.erase(e->get_property_entity());

    this->e = e;
    this->b = b;
    this->offs = offs;
    this->frame = frame;

#ifdef DEBUG
    if (e) {
        tms_infof("selection: entity %s:%p (g_id: %d, id: %u, pos: %4.2f/%4.2f, grouped:%s(%u,%d,%d)). sensor:%s",
                e->get_name(), e, e->g_id, e->id, e->get_position().x, e->get_position().y, e->gr?"YES":"NO", e->gr?e->gr->id:0, e->gr?(int)e->gr->entities.size():0, e->gr?(int)e->gr->connections.size():0,
                (e->get_body(0) ? (e->get_body(0)->GetFixtureList() ? (e->get_body(0)->GetFixtureList()->IsSensor() ? "YES" : "NO") : "NO") : "NO"));

        if (e->g_id == O_GOAL && G->state.pkg && G->state.pkg->type == LEVEL_MAIN) {
            G->finish(true);
        }
    }
#endif

    if (e) {
        e->set_flag(ENTITY_CONNECTED_TO_BREADBOARD, false);

        connection *c = e->conn_ll;

        if (c) {
            do {
                connection **ccn = &c->next[(c->e == e) ? 0 : 1];
                entity *o = c->e == e ? c->o : c->e;
                c = *ccn;
                if (!o) continue;
                if (o->g_id == O_BREADBOARD) {
                    e->set_flag(ENTITY_CONNECTED_TO_BREADBOARD, true);
                    break;
                }
            } while (c);
        }
    }

    G->refresh_widgets();
}

void
selection_handler::disable(bool refresh_widgets/*=true*/)
{
    //tms_infof("Disabling selection");
    if (this->m) {
        for (int x=0; x<NUM_HL; x++) {
            if (G->hls[x].entities == this->m) {
                G->hls[x].entities = 0;
                G->hls[x].type = HL_PRESET_DEFAULT;
                G->hls[x].time = 0.f;
            }
        }

        delete this->m;
    }

    this->m = 0;
    this->e = 0;
    this->c = 0;

    if (G) {
        G->state.edev_labels = false;

        if (refresh_widgets) {
            G->refresh_widgets();
        }
    }
}

void
game::say_goodbye(b2Joint *j)
{
    if (W->is_adventure() && adventure::is_player_alive()) {
        if (j == adventure::player->activator_joint) {
            adventure::player->activator_joint = 0;
        }
    }

    W->destructable_joints.erase(j);

    /**
     * Removing the joint from the to_be_destroyed list here is very risky;
     * there's a high chance this function is called
     * right when world::destroy_joints is called and the set
     * is being iterated.
     **/
    // W->to_be_destroyed.erase(j);

    int x = this->is_mover_joint(j);
    if (x != -1) {
        mover_joint[x] = 0;
    } else {
        joint_info *ji;
        if ((ji = (joint_info*)j->GetUserData())) {
            switch (ji->type) {
                case JOINT_TYPE_CONN:
                    //tms_debugf("say goodbye conn joint");
                    break;

                case JOINT_TYPE_CABLE:
                    {
                        cable *c = (cable*)ji->data;
                        if (c) {
                            c->joint = 0;
                            c->ji = 0;
                            //c->create_joint();
                        }
                    }
                    break;

                case JOINT_TYPE_BACKPACK:
                    {
                        /* FIXME */
#if 0
                        adventure::joint_backpack = 0;
                        adventure::bpack = 0;
                        adventure::bpack_panel = 0;
                        if (adventure::player) {
                            adventure::player->unset_attached();
                        }
                        this->set_control_panel(adventure::player);
#endif

                        /* we return here to stop the joint_info from being destroyed */
                        return;
                    }
                    break;

                case JOINT_TYPE_SCUP:
                    {
                        scup *e = static_cast<scup*>(ji->data);
                        for (int n=0; n<SCUP_NUM_JOINTS; ++n) {
                            if (j == e->j[n]) {
                                e->j[n] = 0;
                            }
                        }

                        e->stuck = false;
                    }
                    break;

                case JOINT_TYPE_RAGDOLL:
                    {
                        ragdoll *r = static_cast<ragdoll*>(ji->data);
                        for (int x=0; x<9; x++) {
                            if (r->joints[x] == j) {
                                r->joints[x] = 0;
                            }
                        }
                    }
                    break;

                default:
                    tms_debugf("Unhandled say_goodbye joint type: %d", ji->type);
                    break;
            }

            ji->destroy();
        }
    }
}

void
game::window_size_changed()
{
    this->cam->width = _tms.window_width;
    this->cam->height = _tms.window_height;
    if (this->get_surface() && this->get_surface()->ddraw) {
        float projection[16];
        tmat4_set_ortho(projection, 0, _tms.window_width, 0, _tms.window_height, 1, -1);
        tms_ddraw_set_matrices(this->get_surface()->ddraw, 0, projection);
    }

    this->refresh_gui();
}

static entity *previous_panel = 0;

void
game::set_mode(int new_mode)
{
    int cur_mode = this->get_mode();

    /* Any 'exit mode code' can be placed here */
    switch (cur_mode) {
        case GAME_MODE_FACTORY:
        case GAME_MODE_REPAIR_STATION:
            adventure::init_widgets();
            break;

        case GAME_MODE_INVENTORY:
            adventure::show_left_widgets();
            this->hide_inventory_widgets();
            break;

        case GAME_MODE_DRAW:
            this->state.edit_layer = 0;
            this->wdg_mode->s[0] = gui_spritesheet::get_sprite(S_CONFIG);
            break;

        case GAME_MODE_MULTISEL:
            this->selection.disable();
            this->wdg_mode->s[0] = gui_spritesheet::get_sprite(S_CONFIG);
            break;

        case GAME_MODE_CONN_EDIT:
            this->wdg_mode->s[0] = gui_spritesheet::get_sprite(S_CONFIG);
            break;

        case GAME_MODE_EDIT_PANEL:
            if (!W->is_paused()) {
                this->set_control_panel(previous_panel);
            }
            break;

        case GAME_MODE_SELECT_SOCKET:
            if (this->ss_quickplug_step2) {
                this->ss_quickplug_step2 = false;
                this->ss_plug->c->freeze = true;
                this->ss_plug->c->disconnect((plug*)this->ss_plug->get_other());
                delete (cable*)this->ss_plug->c;
            }
            break;
    }

    if (cur_mode != new_mode) {
        tms_infof("New mode: %d. Previous mode: %d", new_mode, cur_mode);
        /* Any 'init new mode' code can be placed here */
        switch (new_mode) {
            case GAME_MODE_FACTORY:
            case GAME_MODE_REPAIR_STATION:
                adventure::clear_widgets();
                break;

            case GAME_MODE_DRAW:
                this->state.edit_layer = 2;
                this->selection.disable(false);
                this->wdg_mode->s[0] = gui_spritesheet::get_sprite(S_TPIXEL_MULTI);
                break;

            case GAME_MODE_MULTISEL:
                this->wdg_mode->s[0] = gui_spritesheet::get_sprite(S_MULTISEL);
                this->selection.disable(false);
                this->multi.reset();
                break;

            case GAME_MODE_CONN_EDIT:
                this->wdg_mode->s[0] = gui_spritesheet::get_sprite(S_CONNEDIT);
                this->selection.disable(false);
                break;

            case GAME_MODE_INVENTORY:
                this->inventory_scroll_offset = 0.f; // should we reset the offset when we begin showing the inventory?
                adventure::hide_left_widgets();
                this->show_inventory_widgets();
                break;

            case GAME_MODE_EDIT_PANEL:
                {
                    if (!W->is_paused()) {
                        previous_panel = this->current_panel;
                        this->set_control_panel(0);
                    }

                    this->panel_edit_refresh();
                }
                break;
        }

        this->_mode = new_mode;
        this->refresh_widgets();
    }
}

static struct tms_wdg *inventory_widgets[NUM_RESOURCES];
static bool inventory_widgets_initialized = false;

void
inventory_widget_on_change(struct tms_wdg *w, float values[2])
{
    float value = values[0];
    if (value == 1.f) {
        int resource_id = VOID_TO_INT(w->data);
        G->drop_speed = 1.f;
        G->dropping = resource_id;
        G->drop_step = W->step_count;
        G->drop_amount = 1;

        adventure::player->drop_resource(resource_id, G->drop_amount, b2Vec2(adventure::player->look_dir*1.25f, .75f));

        if (!adventure::player->get_num_resources(resource_id)) {
            G->refresh_inventory_widgets();
        }
    } else {
        G->dropping = -1;
    }
}

static void
init_inventory_widgets()
{
    if (inventory_widgets_initialized) return;

    int iw = _tms.xppcm*.375f;
    int ih = _tms.yppcm*.375f;
    for (int n=0; n<NUM_RESOURCES; ++n) {
        inventory_widgets[n] = tms_wdg_alloc(TMS_WDG_BUTTON, gui_spritesheet::get_sprite(S_INVENTORY_ICONS0+n), 0);
        inventory_widgets[n]->on_change = &inventory_widget_on_change;
        inventory_widgets[n]->size.x = iw;
        inventory_widgets[n]->size.y = ih;
        inventory_widgets[n]->extra_right = iw * 1.2f;
        inventory_widgets[n]->data = INT_TO_VOID(n);
    }

    inventory_widgets_initialized = true;
}

void
game::show_inventory_widgets()
{
    init_inventory_widgets();

    int ih = _tms.yppcm*.375f;
    float x = this->get_bmenu_x();
    float y = _tms.window_height - this->get_bmenu_y() - this->inventory_scroll_offset;
    for (int n=0; n<NUM_RESOURCES; ++n) {
        this->get_surface()->add_widget(inventory_widgets[n]);
        if (adventure::player->get_num_resources(n)) {
            inventory_widgets[n]->pos.x = x;
            inventory_widgets[n]->pos.y = y;
            y -= ih*1.5f;
        } else {
            inventory_widgets[n]->pos.x = -500.f;
            inventory_widgets[n]->pos.y = -500.f;
        }
    }

    this->inventory_highest_y = y;
}

void
game::hide_inventory_widgets()
{
    for (int n=0; n<NUM_RESOURCES; ++n) {
        this->get_surface()->remove_widget(inventory_widgets[n]);
    }
}

void
game::refresh_inventory_widgets()
{
    if (this->get_mode() != GAME_MODE_INVENTORY) return;

    int ih = _tms.yppcm*.375f;
    float x = this->get_bmenu_x();
    float y = _tms.window_height - this->get_bmenu_y() - this->inventory_scroll_offset;
    for (int n=0; n<NUM_RESOURCES; ++n) {
        if (adventure::player->get_num_resources(n)) {
            inventory_widgets[n]->pos.x = x;
            inventory_widgets[n]->pos.y = y;
            y -= ih*1.5f;
        } else {
            /* XXX: Hide widgets in another way maybe? */
            inventory_widgets[n]->pos.x = -500.f;
            inventory_widgets[n]->pos.y = -500.f;
        }
    }

    this->inventory_highest_y = y;
}

void
game::draw_entity_bar(entity *e, float v, float y_offset, const tvec3 &color, float alpha)
{
    b2Vec2 p = e->get_position() + b2Vec2(0.f, y_offset);
    float barw = v * (BAR_WIDTH-.05f);
    float mv[16];

    tmat4_copy(mv, this->cam->view);
    tmat4_translate(mv, 0, 0, e->get_layer()*LAYER_DEPTH);
    tms_ddraw_set_matrices(this->dd, mv, this->cam->projection);

    tms_ddraw_set_color(this->dd, 0.f, 0.f, 0.f, alpha);
    tms_ddraw_square(this->dd,
            p.x, p.y,
            BAR_WIDTH,
            BAR_HEIGHT
            );

    tms_ddraw_set_color(this->dd, TVEC3_INLINE(color), alpha);
    tms_ddraw_square(this->dd,
            p.x, p.y,
            barw,
            BAR_HEIGHT * 0.75f
            );
}

/**
 * Can be used to perform an immediate camera move,
 * regardless of dt and the likes, bypassing cam_vel.
 **/
void
game::cam_move(float x, float y, float z)
{
    if (!W->level.flag_active(LVL_DISABLE_CAM_MOVEMENT) || (this->state.sandbox && W->is_paused())) {
        this->cam->_position.x -= x;
        this->cam->_position.y -= y;

        this->cam_rel_pos.x -= x;
        this->cam_rel_pos.y -= y;
    }

    if (!W->level.flag_active(LVL_DISABLE_ZOOM) || this->state.sandbox) {
        this->cam->_position.z -= z;
    }
}

/**
 * Delete the current multiselection
 **/
void
game::_multidelete()
{
    if (this->get_mode() == GAME_MODE_MULTISEL && this->selection.m) {
        do {
            entity_set my_copy(*this->selection.m);

            tms_debugf("BEGIN: %d", (int)my_copy.size());

            for (entity_set::iterator it = my_copy.begin();
                    it != my_copy.end(); ++it) {
                entity *e = static_cast<entity*>(*it);
                if (e->get_property_entity()) e = e->get_property_entity();
                tms_infof("Multideleting %p", e);
                tms_infof("%s", e->get_name());
                int r = this->delete_entity(e);

                if (r == 3) {
                    tms_debugf("we need to start iterating from the beginning!");
                    break;
                }
            }
        } while (!this->selection.m->empty());

        this->selection.disable();
    }
}

void
game::passthru_input(tms::event *ev)
{
    if (ev->type == TMS_EV_KEY_PRESS || ev->type == TMS_EV_KEY_UP) {
        for (std::set<key_listener*>::iterator it = W->key_listeners.begin();
                it != W->key_listeners.end(); ++it) {
            key_listener *kl = static_cast<key_listener*>(*it);

            if (kl->properties[0].v.i == ev->data.key.keycode) {
                kl->active = (ev->type == TMS_EV_KEY_PRESS);
            }
        }
    }

    for (std::set<escript*>::iterator it = W->escripts.begin();
            it != W->escripts.end(); ++it) {
        escript *e = static_cast<escript*>(*it);

        if (e->listen_on_input) {
            tms::event *new_ev = (tms::event*)malloc(sizeof(tms::event));
            memcpy(new_ev, ev, sizeof(tms::event));
            e->input_events.insert(new_ev);
        }
    }
}

void
game::perform_socket_action(int x)
{
    if (W->is_adventure() && W->is_playing()) {
        if (this->ss_socks[x]->p) {
            if (this->ss_action == SS_ACTION_SELECT) {
                this->selection.select(this->ss_socks[x]->p);
            } else {
                if (this->ss_socks[x]->p->c) {
                    this->absorb(this->ss_socks[x]->p->c);
                } else {
                    plug_base *p = this->ss_socks[x]->p;
                    this->ss_socks[x]->unplug();
                    this->absorb(p);
                }
            }
        }
    } else {
        if (this->ss_action == SS_ACTION_SELECT) {
            if (this->ss_socks[x]->p) {
                this->selection.select(this->ss_socks[x]->p);
            }
        } else {
            this->ss_socks[x]->unplug();

            this->add_ca(1, this->ss_edev->get_entity()->local_to_world(this->ss_socks[x]->lpos, 0));
        }
    }
}

void
game::play_sound(uint32_t sound_id, float x, float y, uint8_t random, float volume, bool loop/*=false*/, void *indent/*=0*/, bool global/*=false*/)
{
    sm_sound *snd = static_cast<sm_sound*>(soundman::translate(sound_id));
    if (!snd) return;

    typedef std::multimap<uint32_t, soundman*>::iterator iterator;
    std::pair<iterator, iterator> range = W->soundmanagers.equal_range(sound_id);

    iterator it = range.first;
    for (; it != range.second; ++it) {
        soundman *sm = it->second;
        if (!sm->is_busy()) {
            sm->enable();
            volume = sm->get_volume();
            break;
        }
    }

    if (volume <= SM_MIN_VOLUME && indent) {
        sm::stop(snd, indent);
    } else {
        sm::play(snd, x, y, random, volume, loop, indent, global);
    }
}

/**
 * Safe function to call from play-mode to queue up a level restart. (Same behaviour as P on PC)
 **/
void
game::restart_level()
{
    if (!this->_restart_level) {
        this->_restart_level = true;
        P.add_action(ACTION_WORLD_PAUSE, 0);
    }
}

/**
 * Safe function to call from play-mode to queue up a score submission.
 **/
void
game::submit_score()
{
    if (!this->state.submitted_score) {
        if (!this->_submit_score) {
            this->_submit_score = true;
            if (settings["score_ask_before_submitting"]->is_true()) {
                ui::confirm("Do you want to submit your highscore?",
                        "Yes",    principia_action(ACTION_SUBMIT_SCORE, 0),
                        "No",     principia_action(ACTION_IGNORE, 0)
                        );
            } else {
                P.add_action(ACTION_SUBMIT_SCORE, 0);
            }
        }
    }
}

void
game::destroy_possible_mover(entity *e)
{
    for (int x=0; x<MAX_INTERACTING; x++) {
        if (interacting[x] == e) {
            this->destroy_mover(x);
        }
    }
}

void
game::set_architect_mode(bool val)
{
    if (val) {
        if (!this->grident->scene) {
            this->get_scene()->add_entity(this->grident);
        }
    } else {
        if (this->grident->scene) {
            this->get_scene()->remove_entity(this->grident);
        }
    }

    this->state.abo_architect_mode = val;
}

static void
fix_entity(entity *e, uint32_t old_id, uint32_t new_id)
{
    for (uint8_t np = 0; np < e->num_properties; ++np) {
        property *p = &e->properties[np];

        if (p->type == P_ID && p->v.i == old_id) {
            p->v.i = new_id;
        }
    }
}

static void
update_entity_id_changed(uint32_t old_id, uint32_t new_id, std::map<uint32_t, entity*> *c1, std::map<uint32_t, entity*> *c2)
{
    if (old_id == new_id) return;

    if (c1) {
        for (std::map<uint32_t, entity*>::iterator it = c1->begin();
                it != c1->end(); ++it) {
            fix_entity(it->second, old_id, new_id);
        }
    }

    if (c2) {
        for (std::map<uint32_t, entity*>::iterator it = c2->begin();
                it != c2->end(); ++it) {
            fix_entity(it->second, old_id, new_id);
        }
    }
}

#ifdef DEBUG

void
game::clamp_entities()
{
    Uint32 ss = SDL_GetTicks();
    tms_debugf("Clamping all entities...");

    uint32_t id = 1;
    uint32_t prev_biggest = 0;

    uint32_t num_entities = 0;
    uint32_t num_cables = 0;
    uint32_t num_groups = 0;

    std::map<uint32_t, entity*> new_entities;

    for (std::map<uint32_t, entity*>::iterator it = W->all_entities.begin();
            it != W->all_entities.end();) {
        entity *e = it->second;
        uint32_t old_id = it->first;
        uint32_t new_id = id ++;

        if (old_id > prev_biggest) prev_biggest = old_id;

        if (W->is_adventure() && e->id == W->level.get_adventure_id() && e->is_creature()) {
            tms_debugf("IGNORE ADVENTURE ROBOT");
            // do not change id of adventure robot
            new_entities.insert(std::pair<uint32_t, entity*>(e->id, e));
            W->all_entities.erase(it++);

            continue;
        }

        e->id = new_id;

        new_entities.insert(std::pair<uint32_t, entity*>(new_id, e));
        W->all_entities.erase(it++);

        update_entity_id_changed(old_id, new_id, &W->all_entities, &new_entities);
        ++ num_entities;
    }

    W->all_entities = new_entities;

    for (std::set<cable*>::iterator it = W->cables.begin();
            it != W->cables.end(); ++it) {
        entity *e = (*it);
        uint32_t old_id = e->id;
        uint32_t new_id = id ++;

        if (old_id > prev_biggest) prev_biggest = old_id;
        e->id = new_id;

        ++ num_cables;
    }

    std::map<uint32_t, group*> new_groups;

    for (std::map<uint32_t, group*>::iterator it = W->groups.begin();
            it != W->groups.end();) {
        group *g = it->second;
        uint32_t old_id = it->first;
        uint32_t new_id = id ++;

        if (old_id > prev_biggest) prev_biggest = old_id;
        g->id = new_id;

        new_groups.insert(std::pair<uint32_t, group*>(new_id, g));
        W->groups.erase(it++);

        ++ num_groups;
    }

    W->groups = new_groups;

    tms_debugf("Done in %u ticks", SDL_GetTicks()-ss);
    tms_debugf("Old biggest id: %u. New: %u", prev_biggest, id-1);
    tms_debugf("Entities: %u, Cables: %u, Groups: %u",
               num_entities, num_cables, num_groups);

    of::_id = id;
}

#endif

int
game::post_render()
{
    if (this->info_label && this->info_label->active) {
        this->info_label->render(this->get_surface()->ddraw, true);
    }

    pscreen::post_render();

#ifdef TMS_BACKEND_PC
    if (this->hov_text->active) {
        this->hov_text->render(this->get_surface()->ddraw, true);
    }
#endif

    return T_OK;
}

connection*
game::set_connection_strength(connection *c, float strength)
{
    tms_infof("Set connection strength: %.2f", strength);
    if (strength == 1.f) {
        c->max_force = INFINITY;
    } else {
        c->max_force = strength * CONN_MAX_FORCE;
    }

    tms_infof("p  0 %f %f %p %p", c->p.x, c->p.y, c->e->get_body(0), c->e->gr);
    tms_infof("p2 0 %f %f %p %p", c->p_s.x, c->p_s.y, c->o->get_body(0), c->o->gr);

    if (c->type == CONN_GROUP || c->type == CONN_PLATE) {
        /* readd the connection */
        connection copy = *c;
        copy.p = c->e->local_to_world(c->p, c->f[0]);
        tms_infof("destroy conn: %p", c);
        c->e->destroy_connection(c);

        if (copy.owned) {
            tms_infof("owned");
            *c = copy;
        } else {
            c = this->get_tmp_conn();
            *c = copy;
        }

        c->j = 0;
        c->self_ent = 0;

        if (strength == 1.f && c->type == CONN_PLATE) {
            c->type = CONN_GROUP;
            tms_infof("setting type to GROUP %p", c);
        } else if (strength < 1.f && c->type == CONN_GROUP) {
            tms_infof("setting type to PLATE %p", c);
            c->type = CONN_PLATE;
        }

        tms_infof("conn_ll: %p %p", c->e->conn_ll, c->o->conn_ll);

        c = this->apply_connection(c, -1);

        tms_infof("p  1 %f %f %p %p", c->p.x, c->p.y, c->e->get_body(0), c->e->gr);
        tms_infof("p2 1 %f %f %p %p", c->p_s.x, c->p_s.y, c->o->get_body(0), c->o->gr);
    }

    return c;
}

void
game::multiselect_perform(void (*cb)(entity*, void*), void *userdata)
{
    if (this->state.sandbox && W->is_paused() && !this->state.test_playing) {
        if (this->get_mode() == GAME_MODE_MULTISEL && this->selection.m) {
            // We need to copy the entity list, because we're likely going to modify it.
            std::set<entity*> cloned_ent(*this->selection.m);
            std::set<entity*>::iterator i = cloned_ent.begin();
            for (; i != cloned_ent.end(); i++) {
                cb(*i, userdata);
            }
            cloned_ent.clear();
        }
    }
}

bool
game::apply_multiselection(entity *e)
{
    bool include_custom_conns = true;
    bool include_static = false;

    entity_set *loop = new entity_set();
    if (this->multi.follow_connections) {
        if (this->sel_p_ent->g_id == O_PIXEL) {
            ((pixel*)this->sel_p_ent)->gather_connected_pixels(loop);
        } else {
            this->sel_p_ent->gather_connected_entities(
                    loop,
                    this->multi.follow_cables,
                    include_custom_conns,
                    include_static,
                    this->multi.select_through_layers);
        }
    } else {
        loop->insert(this->sel_p_ent);
    }

    if (loop->size()) {
        this->selection.select(loop);

        return true;
    } else {
        delete loop;

        return false;
    }
}

#ifdef DEBUG

void game::print_screen_point_info(int x, int y)
{
    tvec3 tproj[3];
    for (int l=0; l<3; l++) { W->get_layer_point(this->cam, x, y, l, &tproj[l]); }
    terrain_coord coord(tproj[0].x, tproj[0].y);

    level_chunk *c = W->cwindow->get_chunk(coord.chunk_x,coord.chunk_y);

    printf("--- CLICK AT %d %d ---\n", x, y);
    printf("pt layer 0: %f %f\n", tproj[0].x, tproj[0].y);
    printf("pt layer 1: %f %f\n", tproj[1].x, tproj[1].y);
    printf("pt layer 2: %f %f\n", tproj[2].x, tproj[2].y);
    printf("chunk x: %d\n", coord.chunk_x);
    printf("chunk y: %d\n", coord.chunk_y);
    printf("chunk in active?: %d\n", (W->cwindow->preloader.active_chunks.find(chunk_pos(coord.chunk_x, coord.chunk_y)) != W->cwindow->preloader.active_chunks.end()));
    printf("chunk num_fixtures: %d\n", c->num_fixtures);
    printf("chunk num_dyn_fixtures: %d\n", c->num_dyn_fixtures);
    printf("chunk load phase: %d\n", c->load_phase);
    printf("chunk generate phase: %d\n", c->generate_phase);
    printf("chunk in wastebin?: %d\n", (W->cwindow->preloader.wastebin.find(chunk_pos(coord.chunk_x, coord.chunk_y)) != W->cwindow->preloader.wastebin.end()));

    if (c) {
        printf("chunk num fixtures: %d\n", c->num_fixtures);
        printf("chunk garbage: %d\n", c->num_fixtures);

        for (int x=0; x<8; x++) {
            if (c->neighbours[x]) {
                tms_debugf("chunk neighbour %d num fixtures: %d", x, c->neighbours[x]->num_fixtures);
                tms_debugf("chunk neighbour %d garbage:      %d", x, c->neighbours[x]->garbage);
            }
        }
    }

}

void game::print_stats()
{
    printf("--- CHUNK PRELOADER ---\n");
    printf("heap size:\t\t%" PRIu64 "/%" PRIu64 "\n", W->cwindow->preloader.heap.size, W->cwindow->preloader.heap.cap);
    printf("level size:\t\t%" PRIu64 "/%" PRIu64 "\n", W->cwindow->preloader.w_lb.size, W->cwindow->preloader.w_lb.cap);
    printf("active chunks:\t\t%u\n", (int)W->cwindow->preloader.active_chunks.size());
    printf("wastebin:\t\t%u\n", (int)W->cwindow->preloader.wastebin.size());
    printf("gentypes:\t\t%u\n", (int)W->cwindow->preloader.gentypes.size());
    SDL_Delay(5000);
}

#endif

bool
game::autosave_exists()
{
    char autosave_path[1024];
    snprintf(autosave_path, 1023, "%s/.autosave", pkgman::get_level_path(LEVEL_LOCAL));

    return file_exists(autosave_path);
}

void
game::open_latest_state(bool require_equal_id, tms::screen *previous_screen/*=0*/)
{
    lvlfile *level = pkgman::get_levels(LEVEL_LOCAL_STATE);
    lvlfile *next;

    uint32_t latest_save_id = 0;
    uint32_t latest_level_id = 0;
    int latest_level_id_type = LEVEL_LOCAL;
    time_t latest_mtime = 0;

    while (level) {
        if (!require_equal_id || level->id == W->level.local_id) {
            if (level->mtime > latest_mtime) {
                latest_save_id = level->save_id;
                latest_level_id = level->id;
                latest_mtime = level->mtime;
                latest_level_id_type = level->id_type;
            }
        }

        next = level->next;
        delete level;
        level = next;
    }

    tms_infof("Latest save id: %d", latest_save_id);

    if (latest_save_id != 0) {
        this->open_state(latest_level_id_type, latest_level_id, latest_save_id);

        if (!require_equal_id) {
            this->state.test_playing = false;
            this->state.sandbox = false;
        }

        if (previous_screen) {
            G->screen_back = previous_screen;
        }

        this->resume_action = GAME_RESUME_OPEN;

        if (_tms.screen == &P.s_loading_screen->super) {
            P.s_loading_screen->set_next_screen(this);
        } else if (_tms.screen != &this->super){
            tms::set_screen(this);
        }
    } else {
        ui::message("Found no save we can continue from.");
    }
}

void
game::add_loot(entity *host, uint8_t resource_type, int num, float life/*=1.f*/)
{
    typedef std::multimap<entity*, struct loot>::iterator iterator;
    std::pair<iterator, iterator> ip = this->loots.equal_range(host);

    bool added = false;

    iterator it = ip.first;
    for (; it != ip.second; ++it) {
        struct loot &l = it->second;

        if (l.resource_type == resource_type) {
            l.num += num;
            l.life = life;
            added = true;
            break;
        }
    }

    if (!added) {
        this->loots.insert(std::pair<entity*, struct loot>(host, loot(resource_type, resource_data[resource_type].name, num, life)));
    }
}

void
game::close_tt(int what)
{
    for (int x=0; x<MAX_TUTORIAL_TEXTS; x++) {
        if (this->tt[x].what == what) {
            this->tt[x].life = 0.f;
        }
    }
}

void
game::add_tt(int what, entity *e, b2Vec2 pos, float life)
{
    if (!this->state.new_adventure) {
        return;
    }

    int x;
    for (x=0; x<MAX_TUTORIAL_TEXTS; x++) {
        if (this->tt[x].what == what) {
            break;
        }
    }

    if (x == MAX_TUTORIAL_TEXTS) {
        for (x=0; x<MAX_TUTORIAL_TEXTS; x++) {
            if (this->tt[x].life <= 0.f) {
                break;
            }
        }
    }

    if (x == MAX_TUTORIAL_TEXTS) {
        x = MAX_TUTORIAL_TEXTS-1;
    }

    this->tt[x].what = what;
    this->tt[x].e = e;
    this->tt[x].life = life;
    this->tt[x].pos = pos;
}

void
game::finished_tt(int what)
{
    if (!this->state.new_adventure) {
        return;
    }

    settings["tutorial"]->v.u32 |= what;
}

void
game::add_hp(entity *host, float percent, tvec3 &color/*=TV_HP_RED*/, float time/*=1.f*/, bool regen/*=false*/)
{
    int x;
    bool found = false;

    /* first look if it's already added */
    for (x=0; x<NUM_HP; x++) {
        if (hps[x].e == host) {
            found = true;
            break;
        }
    }

    if (!found) {
        for (x=0; x<NUM_HP; x++) {
            if (hps[x].time <= 0.f) {
                break;
            }
        }

    }

    x = x%NUM_HP;

    hps[x].time = time;
    hps[x].e = host;
    hps[x].color = color;
    hps[x].percent = percent;
    hps[x].regen = regen;
}

void
game::render_help_icon(const std::set<entity*> &set, float off_x, float off_y)
{
    struct tms_sprite *spr = gui_spritesheet::get_sprite(S_ROUNDED_HELP);

    const b2Vec2 player_pos = adventure::player->get_position();

    for (std::set<entity*>::const_iterator it = set.begin();
            it != set.end(); ++it) {
        entity *e = (*it);

        const b2Vec2 icon_pos   = e->local_to_world(b2Vec2(off_x, off_y), 0);

        const float dist        = b2Distance(player_pos, icon_pos);

        if (dist < RH_MIN_DIST || dist > RH_MAX_DIST) {
            continue;
        }

        float size = 0.005f * _tms.xppcm;
        float max_z = 60.f;
        if (!W->level.flag_active(LVL_DISABLE_ADVENTURE_MAX_ZOOM) && !W->is_paused() && W->is_adventure() && this->follow_object == adventure::player) {
            max_z = 20.f;
        }
        size *= tclampf((this->cam->_position.z*2.f) / max_z, 1.f, 1.5f);

        float alpha = 1.f;
        if (dist > RH_MAX_DIST_ALPHA) {
            alpha = 1.f-((dist-RH_MAX_DIST_ALPHA) / (RH_MAX_DIST-RH_MAX_DIST_ALPHA));
        }
        tms_ddraw_set_color(this->dd, 1.f, 1.f, 1.f, alpha);

        tms_ddraw_sprite_r(this->dd, spr,
                icon_pos.x,
                icon_pos.y,
                size,
                size,
                e->get_angle() + cos((double)_tms.last_time/100000.) * 8.f);
    }
}

bool
game::check_click_help_icon(const std::set<entity*> &set, float off_x, float off_y, b2Vec2 click_pos, struct principia_action click_action)
{
    const b2Vec2 player_pos = adventure::player->get_position();

    for (std::set<entity*>::const_iterator it = set.begin();
            it != set.end(); ++it) {
        entity *e = (*it);

        const b2Vec2 icon_pos = e->local_to_world(b2Vec2(off_x, off_y), 0);

        const float dist = b2Distance(player_pos, icon_pos);

        if (dist < RH_MIN_DIST || dist > RH_MAX_DIST) {
            continue;
        }

        const float click_dist = b2Distance(icon_pos, click_pos);

        float size = 0.005f * _tms.xppcm;
        float max_z = 60.f;
        if (!W->level.flag_active(LVL_DISABLE_ADVENTURE_MAX_ZOOM) && !W->is_paused() && W->is_adventure() && this->follow_object == adventure::player) {
            max_z = 20.f;
        }
        size *= tclampf((this->cam->_position.z*2.f) / max_z, 1.f, 1.5f);
        size *= 0.65f;

        tms_infof("Click dist: %.2f", click_dist);
        tms_infof("Size: %.2f", size);

        if (click_dist < size) {
            P.add_action(click_action.action_id, click_action.action_data);
            settings["tutorial"]->v.u32 |= TUTORIAL_REPAIR_STATION;
            return true;
        }
    }

    return false;
}
