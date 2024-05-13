#include "wdg.h"
#include <tms/core/glob.h>
#include <tms/math/misc.h>

static void
base_mouse_over(struct tms_wdg *w) { }

static void
base_mouse_out(struct tms_wdg *w) { }

static void render(struct tms_wdg *w, struct tms_surface *s);

static void
handler_field(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    float new_values[2];
    new_values[0] = (rx + 1.f) / 2.f;
    new_values[1] = (ry + 1.f) / 2.f;

    for (int x=0; x<2; ++x) {
        if (w->snap[x] != 0.f) {
            new_values[x] /= w->snap[x];
            new_values[x]  = roundf(new_values[x]);
            new_values[x] *= w->snap[x];
        }
    }

    if (w->on_change)
        w->on_change(w, new_values);

    w->value[0] = new_values[0];
    w->value[1] = new_values[1];
}

static void
handler_radial(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    double dx = (double)rx;
    double dy = (double)ry;

    /*
    double l = sqrt(dx*dx + dy*dy);
    dx *= 1./l;
    dy *= 1./l;
    */

    double value = atan2((double)dy, (double)dx);

    if (value < 0.)
        value += 2.*M_PI;

    value /= 2.*M_PI;

    /*
    if (w->snap != 0.f) {
        value /= w->snap;
        value = roundf(value);
        value *= w->snap;
    }
    */

    if (w->on_change && value != w->value[0]) {
        float new_values[2] = { value };
        w->on_change(w, new_values);
    }

    w->value[0] = (float)value;
}

#define BTN_RADIAL_MID_RADIUS .55f

static void
drag_handler_btn_radial(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    double dx = (double)rx;
    double dy = (double)ry;
    double adx = fabs(dx);
    double ady = fabs(dy);
    float len = tmath_sqrt(adx + adx * ady + ady);

    double value = atan2(dy, dx);
    float value2 = w->value[1];

    if (value2 > .5f && len < BTN_RADIAL_MID_RADIUS) {
        value = w->value[0];
    } else {
        value2 = 0.f;
        if (value < 0.) {
            value += 2.*M_PI;
        }

        value /= 2.*M_PI;
    }

    /*
    if (w->snap != 0.f) {
        value /= w->snap;
        value = roundf(value);
        value *= w->snap;
    }
    */

    if (w->on_change && value != w->value[0]) {
        float new_values[2] = { value, value2 };
        w->on_change(w, new_values);
    }

    w->value[1] = value2;
    w->value[0] = (float)value;
}

static void
down_handler_btn_radial(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    float arx = fabsf(rx);
    float ary = fabsf(ry);
    float len = tmath_sqrt(arx * arx + ary * ary);

    if (len < BTN_RADIAL_MID_RADIUS) {
        if (w->on_change) {
            float new_values[2] = { w->value[0], 1.f };
            w->on_change(w, new_values);
        }

        w->value[1] = 1.f;
    } else {
        if (w->touch_drag) {
            w->touch_drag(w, pid, bid, ox, oy, rx, ry);
        }
    }
}

static void
up_handler_btn_radial(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    if (w->on_change) {
        float new_values[2] = { w->value[0], 0.f };
        w->on_change(w, new_values);
    }

    w->value[1] = 0.f;
}

#undef BTN_RADIAL_MID_RADIUS

static void
handler_slider(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    float value = (rx + 1.f) / 2.f;

    if (w->snap[0] != 0.f) {
        value /= w->snap[0];
        value  = roundf(value);
        value *= w->snap[0];
    }

    if (w->on_change && value != w->value[0]) {
        float new_values[2] = { value };
        w->on_change(w, new_values);
    }

    w->value[0] = value;
}

static void
handler_vslider(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    float value = (ry + 1.f) / 2.f;

    if (w->snap[0] != 0.f) {
        value /= w->snap[0];
        value  = roundf(value);
        value *= w->snap[0];
    }

    if (w->on_change && value != w->value[0]) {
        float new_values[2] = { value };
        w->on_change(w, new_values);
    }

    w->value[0] = value;
}

static void
down_checkbox(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    float nv;
    if (w->value[0] == 0.f)
        nv = 1.f;
    else
        nv = 0.f;

    if (w->on_change && nv != w->value[0]) {
        float new_values[2] = { nv };
        w->on_change(w, new_values);
    }

    w->value[0] = nv;
}

static void
down_button(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    if (w->on_change) {
        float new_values[2] = { 1.f };
        w->on_change(w, new_values);
    }

    w->value[0] = 1.f;
}

static void
up_button(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    if (w->on_change) {
        float new_values[2] = { 0.f };
        w->on_change(w, new_values);
    }

    w->value[0] = 0.f;
}

void
tms_wdg_slider_render(struct tms_wdg *w, struct tms_surface *s)
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

    tms_ddraw_sprite_r(s->ddraw, w->s[0], px, py, w->size.x, w->size.y, r);
    if (w->s[1]) {
        tms_ddraw_sprite_r(s->ddraw, w->s[1],
                px + ((w->value[0]-.5f) * 2.f)*w->size.x/2.f*sx,
                py + ((w->value[0]-.5f) * 2.f)*w->size.x/2.f*sy,
                .6f/4.f * tms.xppcm, .6f * tms.xppcm, r);
    }
}

void
tms_wdg_vslider_render(struct tms_wdg *w, struct tms_surface *s)
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

    tms_ddraw_sprite_r(s->ddraw, w->s[0], px, py, w->size.y, w->size.x, r);
    if (w->s[1]) {
        tms_ddraw_sprite_r(s->ddraw, w->s[1],
                px + ((w->value[0]-.5f) * 2.f)*w->size.y/2.f*sx,
                py + ((w->value[0]-.5f) * 2.f)*w->size.y/2.f*sy,
                .6f/4.f * tms.xppcm, .6f * tms.xppcm, r);
    }
}

void
tms_wdg_field_render(struct tms_wdg *w, struct tms_surface *s)
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

    float knob_w = w->size.x/5.f;
    float knob_h = w->size.y/5.f;

    tms_ddraw_sprite_r(s->ddraw, w->s[0], px, py, w->size.x, w->size.y, r);
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

void
tms_wdg_radial_render(struct tms_wdg *w, struct tms_surface *s)
{
    //tms_infof("draw col %f %f %f %f", s->ddraw->color.x,s->ddraw->color.y,s->ddraw->color.z,s->ddraw->color.w);
    //
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

    tms_ddraw_sprite_r(s->ddraw, w->s[0], px, py, w->size.x, w->size.y, r);

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
btn_radial_render(struct tms_wdg *w, struct tms_surface *s)
{
    tvec4 col = s->ddraw->color;

    float px = w->pos.x, py = w->pos.y;
    float r = 0.f;

    if (_tms.emulating_portrait) {
        int xx = (int)px, yy = (int)py;
        tms_convert_to_portrait(&xx, &yy);
        px = (float)xx;
        py = (float)yy;

        r = -90.f;
    }

    if (w->focused) {
        if (w->value[1] > .5f) {
            tms_ddraw_set_color(s->ddraw, 1.0f, 1.0f, 1.0f, 1.f);
        } else {
            tms_ddraw_set_color(s->ddraw, 1.2f, 1.2f, 1.2f, 1.f);
        }
    }

    /* XXX: Experimental rotation of the first sprite instead, which is used instead of the regular knob */
    float rotation_offset = (w->value[0]*(M_PI*2.f)) / (M_PI/180.);

    tms_ddraw_sprite_r(s->ddraw, w->s[0], px, py, w->size.x, w->size.y, r - rotation_offset);

    float a =  w->value[0] * 2.f * M_PI + (_tms.emulating_portrait ? M_PI/2.f : 0.f);

    float button_w = w->size.x/1.5f;
    float button_h = w->size.y/1.5f;

    if (w->s[1]) {
        if (w->value[1] > .5f) {
            tms_ddraw_set_color(s->ddraw, 1.2f, 1.2f, 1.2f, 1.0f);
        } else {
            tms_ddraw_set_color(s->ddraw, 1.0f, 1.0f, 1.0f, 0.7f);
        }

        tms_ddraw_sprite_r(s->ddraw, w->s[1],
                px,
                py,
                button_w,
                button_h, r);
    }

    tms_ddraw_set_color(s->ddraw, col.x, col.y, col.z, col.w);
}

void
tms_wdg_free(struct tms_wdg *wdg)
{
    if (wdg->surface) {
        tms_surface_remove_widget(wdg->surface, wdg);
    }

    if (wdg->color) {
        free(wdg->color);
    }

    free(wdg);
}

void
tms_wdg_init(struct tms_wdg *w, int type, struct tms_sprite *s0, struct tms_sprite *s1)
{
    w->type = type;
    w->value[0] = 0.f;
    w->value[1] = 0.f;
    w->surface = 0;
    w->snap[0] = 0.f;
    w->snap[1] = 0.f;
    w->ghost[0] = 0.f;
    w->ghost[1] = 0.f;
    w->alpha = 1.f;
    w->enable_ghost = 0;
    w->focused = 0;
    w->hovered = 0;
    w->faded = 0;
    w->extra_up = 0.f;
    w->extra_right = 0.f;
    w->extra_down = 0.f;
    w->extra_left = 0.f;
    w->clickthrough = 0;

    w->data = 0;
    w->data2 = 0;
    w->data3 = 0;
    w->color = 0;

    w->touch_drag = 0;
    w->on_change = 0;
    w->on_focus_change = 0;
    w->render = render;
    w->post_render = 0;

    w->s[0] = s0;
    w->s[1] = s1;

    w->mouse_over = base_mouse_over;
    w->mouse_out  = base_mouse_out;

    switch (type) {
        case TMS_WDG_BUTTON:
            w->touch_down = down_button;
            w->touch_up = up_button;
            w->size = (tvec2){20.f, 20.f};
            break;

        case TMS_WDG_CHECKBOX:
            w->touch_down = down_checkbox;
            w->touch_up = 0;
            w->size = (tvec2){20.f, 20.f};
            break;

        case TMS_WDG_SLIDER:
            w->touch_down = handler_slider;
            w->touch_drag = handler_slider;
            w->touch_up = handler_slider;
            w->render = tms_wdg_slider_render;
            w->size = (tvec2){.5f* 3.f * tms.xppcm, .5f * tms.yppcm};
            break;

        case TMS_WDG_VSLIDER:
            w->touch_down = handler_vslider;
            w->touch_drag = handler_vslider;
            w->touch_up   = handler_vslider;
            w->render     = tms_wdg_vslider_render;
            w->size       = (tvec2){.5f*tms.xppcm, .5f*3.f*tms.yppcm};
            break;

        case TMS_WDG_RADIAL:
            w->touch_down = handler_radial;
            w->touch_drag = handler_radial;
            w->touch_up = handler_radial;
            w->render = tms_wdg_radial_render;
            w->size = (tvec2){.5f* 2.f * tms.xppcm, .5f* 2.f * tms.yppcm};
            break;

        case TMS_WDG_FIELD:
            w->touch_down = handler_field;
            w->touch_drag = handler_field;
            w->touch_up = handler_field;
            w->render = tms_wdg_field_render;
            w->size = (tvec2){.5f* 2.f * tms.xppcm, .5f* 2.f * tms.yppcm};
            break;

        case TMS_WDG_BTN_RADIAL:
            w->touch_down = down_handler_btn_radial;
            w->touch_drag = drag_handler_btn_radial;
            w->touch_up = up_handler_btn_radial;
            w->render = btn_radial_render;
            w->size = (tvec2){.5f* 2.f * tms.xppcm, .5f* 2.f * tms.yppcm};
            break;
    }
}

void
tms_wdg_set_active(struct tms_wdg *w, int active)
{
    if (active) {
        if (w->type == TMS_WDG_BUTTON) {
            w->value[0] = 1.f;
            w->value[1] = 1.f;
        }
        w->focused = 1;
    } else {
        if (w->type == TMS_WDG_BUTTON) {
            w->value[0] = 0.f;
            w->value[1] = 0.f;
        }
        w->focused = 0;
    }

    if (w->on_focus_change) {
        w->on_focus_change(w);
    }
}

struct tms_wdg *tms_wdg_alloc(int type, struct tms_sprite *s0, struct tms_sprite *s1)
{
    struct tms_wdg *w = calloc(1, sizeof(struct tms_wdg));

    tms_wdg_init(w,type,s0,s1);
    return w;
}

static void
render(struct tms_wdg *w, struct tms_surface *s)
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

    tms_ddraw_sprite_r(s->ddraw, w->s[0], px, py, w->size.x, w->size.y, r);

    if (w->s[1]) {
        tms_ddraw_sprite_r(s->ddraw, w->s[1], px, py, w->size.x/2.f, w->size.y/2.f,r);
    }
}
