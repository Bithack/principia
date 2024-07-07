#include "widget_manager.hh"
#include "text.hh"
#include "game.hh"
#include "gui.hh"
#include "ui.hh"
#include "misc.hh"
#include "menu_shared.hh"
#include "menu_main.hh"
#include "menu_create.hh"
#include "menu-play.hh"
#include "settings.hh"
#include "soundmanager.hh"

#include <tms/core/wdg.h>
#include <tms/bindings/cpp/cpp.hh>

#include <algorithm>

#define TOOLTIP_ACTIVATION_TIME 0.2f

//#define DRAW_AREA_BOUNDING_BOX

const static float BASE_WIDGET_WIDTH  = 64.f;
const static float BASE_WIDGET_HEIGHT = 64.f;

static int MARGIN_X = 0;
static int MARGIN_Y = 0;

static void
base_touch_up(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    principia_wdg *pwdg = static_cast<principia_wdg*>(w->data2);
    pwdg->moved_out = false;

    if (w->value[0] < 0.5f) return;

    if (w->type == TMS_WDG_BUTTON) {
        if (w->on_change) {
            float new_values[2] = { 0.f };
            w->on_change(w, new_values);
        }

        w->value[0] = 0.f;
    }

    bool hovered;

#ifdef TMS_BACKEND_PC
    if (false && settings["emulate_touch"]->is_true()) {
        float dist = sqrtf((float)(ox*ox + oy*oy));

        tms_infof("DIST: %.2f", dist);

        hovered = (dist < w->size.w);
    } else {
        hovered = w->hovered;
    }
#else
    float dist = sqrtf((float)(ox*ox + oy*oy));

    hovered = (dist < w->size.w);
#endif

    if (hovered) {
#ifndef TMS_BACKEND_PC
        pid = 0;
#endif

        uint8_t button_id = VOID_TO_UINT8(w->data);


        if (pid == 0 || pid == 1) {
            bool ret = pwdg->get_home()->widget_clicked(pwdg, button_id, pid);

            if (ret) {
                sm::play(&sm::click, sm::position.x, sm::position.y, rand(), 0.5f, false, 0, true);
            }
        }
    }
}

static void
draggable_touch_down(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    principia_wdg *pwdg = static_cast<principia_wdg*>(w->data2);
    w->value[0] = 1.f;

    pwdg->set_dragging(true);
}

static void
draggable_touch_up(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    principia_wdg *pwdg = static_cast<principia_wdg*>(w->data2);
    w->value[0] = 0.f;

    pwdg->set_dragging(false);
}

static void
draggable_touch_drag(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    principia_wdg *pwdg = static_cast<principia_wdg*>(w->data2);

    float value_x = 0.f;
    float value_y = 0.f;

    if (pwdg->can_drag_x()) {
        pwdg->pos.x += ox;

        if (pwdg->has_limit_x) {
            if (pwdg->pos.x < pwdg->lower_limit_x) {
                pwdg->pos.x = pwdg->lower_limit_x;
            } else if (pwdg->pos.x > pwdg->upper_limit_x) {
                pwdg->pos.x = pwdg->upper_limit_x;
            }

            value_x = (pwdg->pos.x - pwdg->lower_limit_x) / (pwdg->upper_limit_x - pwdg->lower_limit_x);
        }

    }

    if (pwdg->can_drag_y()) {
        pwdg->pos.y += oy;

        if (pwdg->has_limit_y) {
            if (pwdg->pos.y < pwdg->lower_limit_y) {
                pwdg->pos.y = pwdg->lower_limit_y;
            } else if (pwdg->pos.y > pwdg->upper_limit_y) {
                pwdg->pos.y = pwdg->upper_limit_y;
            }

            value_y = (pwdg->pos.y - pwdg->lower_limit_y) / (pwdg->upper_limit_y - pwdg->lower_limit_y);
        }
    }

    if (pwdg->on_dragged) {
        pwdg->on_dragged(pwdg, value_x, value_y);
    }
}

static void
down_label(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry)
{
    if (w->on_change) {
        float new_values[2] = { 1.f };
        w->on_change(w, new_values);
    }

    w->value[0] = 1.f;
}

static void
knob_render(struct tms_wdg *w, struct tms_surface *s)
{
    principia_wdg *pwdg = static_cast<principia_wdg*>(w->data2);

    pscreen *ps = pwdg->get_home();

    if (!ps) {
        return;
    }

    const float ow = 10.f;

    tvec4 color = tvec4f(1.f, 1.f, 1.f, 0.8f);

    if (pwdg->hovered || pwdg->value[0] > 0.5f) {
        color.a = 0.9f;
    }

    float width = w->size.w - ow*2;
    float height = w->size.h - ow*2;

    ps->add_rounded_square(w->pos.x, w->pos.y, width, height, TV_BLACK, ow);
    ps->add_rounded_square(w->pos.x, w->pos.y, width - 2.f, height - 2.f, color, ow);
}

static void
label_render(struct tms_wdg *w, struct tms_surface *s)
{
    principia_wdg *pwdg = static_cast<principia_wdg*>(w->data2);

    float color = 1.f;

    if (pwdg->hovered) {
        color += .2f;
    }

    if (pwdg->focused) {
        color += .2f;
    }

    if (pwdg->label) {
        pwdg->label->color = tvec4f(color, color, color, pwdg->label->color.a);
    }
}

static void
my_post_render(struct tms_wdg *w, struct tms_surface *s)
{
    principia_wdg *pwdg = static_cast<principia_wdg*>(w->data2);

    if (pwdg->marker) {
        float px = w->pos.x, py = w->pos.y;
        float r = 0.f;

        if (_tms.emulating_portrait) {
            int xx = (int)px, yy = (int)py;
            tms_convert_to_portrait(&xx, &yy);
            px = (float)xx;
            py = (float)yy;

            r = -90.f;
        }

        tms_ddraw_set_color(s->ddraw,
                pwdg->marker_color.r*.5f,
                pwdg->marker_color.g*.5f,
                pwdg->marker_color.b*.5f,
                1.f);

        tms_ddraw_sprite_r(s->ddraw, gui_spritesheet::get_sprite(S_MARKER), px, py, w->size.x, w->size.y, r);

        tms_ddraw_set_color(s->ddraw, 1.0, 1.0, 1.0, 1.0);
    }
}

static void
on_mouse_over(struct tms_wdg *w)
{
}

static void
on_mouse_out(struct tms_wdg *w)
{
    principia_wdg *pwdg = static_cast<principia_wdg*>(w->data2);

    pwdg->moved_out = true;
    pwdg->tooltip_time = 0.0;
}

principia_wdg::principia_wdg(tms::surface *surface, int widget_type,
                             struct tms_sprite *s0, struct tms_sprite *s1,
                             float scale/*=1.f*/)
    : draggable(false)
    , draggable_x(false)
    , draggable_y(false)
    , dragging(false)
    , has_limit_x(false)
    , lower_limit_x(0.f)
    , upper_limit_x(0.f)
    , has_limit_y(false)
    , lower_limit_y(0.f)
    , upper_limit_y(0.f)
    , on_dragged(0)
    , parent(0)
    , render_background(false)
{
    tms_wdg_init((struct tms_wdg*)this, widget_type, s0, s1);

    this->_type = widget_type;
    this->data2 = this;
    this->marker = false;
    this->marker_color = tvec3f(1.f, 1.f, 1.f);
    this->lmodx = 0.f;
    this->lmody = 0.f;

    this->on_long_press = 0;
    this->moved_out = false;
    this->label = 0;
    this->tooltip = 0;
    this->tooltip_active = false;
    this->tooltip_time = 0.0;
    this->_surface = surface;
    this->priority = 1000;

    switch (widget_type) {
        case TMS_WDG_LABEL:
            this->render = label_render;
            this->touch_down = down_label;
            break;

        case TMS_WDG_KNOB:
            this->render = knob_render;
            break;
    }

    this->post_render = my_post_render;
    this->mouse_over = on_mouse_over;
    this->mouse_out  = on_mouse_out;

    if (s0) {
        this->size.x = (s0->width  / BASE_WIDGET_WIDTH)  * 0.5f * _tms.xppcm;
        this->size.y = (s0->height / BASE_WIDGET_HEIGHT) * 0.5f * _tms.yppcm;
    } else {
        this->size.x = 1.f;
        this->size.y = 1.f;
    }

    this->size.x *= scale;
    this->size.y *= scale;

    this->padding = tvec2f(_tms.xppcm * 0.1f, _tms.yppcm * 0.1f);
}

principia_wdg::~principia_wdg()
{
    if (this->tooltip) {
        delete this->tooltip;
    }

    if (this->label) {
        delete this->label;
    }
}

void
principia_wdg::step()
{
#ifdef TMS_BACKEND_PC
    if (this->hovered) {
        this->tooltip_time += _tms.dt;
    }

    if (this->value[0] > 0.5f && this->type == TMS_WDG_BUTTON) {
        this->tooltip_time = -0.5;
    }

    bool prev_tooltip_active = this->tooltip_active;
    this->tooltip_active = this->tooltip_time >= TOOLTIP_ACTIVATION_TIME;

    if (this->tooltip && this->tooltip_active && !prev_tooltip_active) {
        float x_offset = (this->size.x) * this->area->tmodx;
        float y_offset = (this->size.y + this->tooltip->get_height()) * this->area->tmody;

        x_offset -= (this->tooltip->get_width() * this->area->tsmodx);

        this->tooltip->set_position(this->pos.x + x_offset, this->pos.y + y_offset);
    }
#endif

    if (this->area->do_set_alpha) {
        this->alpha = this->area->alpha;
        if (this->label) {
            this->label->color.a = this->area->alpha;
            this->label->outline_color.a = this->area->alpha;
        }
    }

#ifdef TMS_BACKEND_MOBILE
    if (this->type == TMS_WDG_BUTTON) {
        if (this->moved_out) {
            this->down_time = 0.0;
        } else if (this->down_time >= 1.0) {
            if (this->on_long_press) {
                if (this->on_change) {
                    float new_values[2] = { 0.f };
                    this->on_change(this, new_values);
                }
                this->value[0] = 0.f;
                this->down_time = 0.0;

                this->on_long_press(this);
            } else {
                this->moved_out = true;
            }
        } else if (this->value[0] > 0.5f) {
            this->down_time += _tms.dt;
        } else {
            this->down_time = 0.0;
        }
    }
#endif
}

/* Emulate a click event. This will only reliably work on a button */
void
principia_wdg::click()
{
    this->value[0] = 1.f;
    this->hovered = true;

    if (this->touch_up) {
        this->touch_up(this, 0, SDL_BUTTON_LEFT, 0, 0, 0.f, 0.f);
    }

    this->value[0] = 0.f;
    this->hovered = false;
}

void
principia_wdg::add()
{
    if (this->surface) {
        return;
    }

    if (this->area->enabled) {
        this->_surface->add_widget(this);
    }
}

void
principia_wdg::remove()
{
    if (!this->surface) {
        return;
    }

    this->set_dragging(false);
    this->_surface->remove_widget(this);
}

void
principia_wdg::set_tooltip(const char *text, p_font *font/*=font::medium*/)
{
    if (!this->tooltip) {
        this->tooltip = new p_text(font);
    }

    if (!text) {
        this->tooltip->active = false;
        return;
    }

    this->tooltip->active = true;

    uint8_t horizontal_align = ALIGN_LEFT;

    if (this->area->tmodx < -0.5f) {
        horizontal_align = ALIGN_RIGHT;
    }

    this->tooltip->set_text(text, false);
    this->tooltip->calculate(horizontal_align, ALIGN_CENTER);
    this->tooltip->color = tvec4f(1.f, 1.f, 1.f, 1.f);
    this->tooltip->outline_color = tvec4f(0.f, 0.f, 0.f, 1.f);
}

void
principia_wdg::set_label(const char *text, p_font *font/*=font::medium*/)
{
    if (!this->label) {
        this->label = new p_text(font);
    }

    if (this->_type == TMS_WDG_LABEL) {
        //this->label->set_alignment(this->area->label_halign, ALIGN_CENTER);
    }

    this->label->set_vertical_align(ALIGN_CENTER);

    this->label->set_text(text);

    if (this->is_label()) {
        this->size.x = this->label->get_width();
        this->size.y = this->label->get_height();
    }
}

void
principia_wdg::resize_percentage(
        int base_width,  float max_width_percentage,
        int base_height, float max_height_percentage
        )
{
    if (this->is_label() && this->label) {
        float w = this->label->get_width();
        float h = this->label->get_height();

        float cur_width_percentage  = w / base_width;
        float cur_height_percentage = h / base_height;

        tms_infof("Current percentages: %.2f/%.2f",
                cur_width_percentage,
                cur_height_percentage);

        tms_infof("Desired percentages: %.2f/%.2f",
                max_width_percentage,
                max_height_percentage);

        /* Calculate what scale we need to have to reach one of the desired percentages,
         * but not go over the other */

        // try width
        float mod = 1.0+(cur_width_percentage/max_width_percentage);

        float new_w = base_width * max_width_percentage;
        float scale_ratio = new_w/w;
        float new_h = h * scale_ratio;

        w = new_w;
        h = new_h;

        float new_width_percentage  = w / base_width;
        float new_height_percentage = h / base_height;

        tms_infof("New percentages:     %.2f/%.2f",
                new_width_percentage,
                new_height_percentage);

        this->label->set_scale(scale_ratio);
    }
}

void
principia_wdg::set_draggable(bool val)
{
    this->draggable = val;

    this->touch_down = draggable_touch_down;
    this->touch_up = draggable_touch_up;
    this->touch_drag = draggable_touch_drag;
}

void
principia_wdg::set_dragging(bool val)
{
    bool prev = this->dragging;

    if (prev != val) {
        this->dragging = val;
    }
}

pscreen*
principia_wdg::get_home()
{
    if (this->parent) {
        return this->parent->get_home();
    }

    return 0;
}

widget_manager::widget_manager(pscreen *_home, bool _override_down, bool _override_up)
    : home(_home)
    , override_down(_override_down)
    , override_up(_override_up)
{
    MARGIN_X = _tms.xppcm * 0.1f;
    MARGIN_Y = _tms.yppcm * 0.1f;

    this->init_areas();
}

widget_manager::~widget_manager()
{
    for (std::deque<principia_wdg*>::iterator it = this->widgets.begin();
            it != this->widgets.end(); ++it) {
        delete *it;
    }
}

int
widget_manager::get_margin_x()
{
    return MARGIN_X;
}

int
widget_manager::get_margin_y()
{
    return MARGIN_Y;
}

void
widget_manager::init_areas()
{
    this->areas[AREA_TOP_LEFT].base_x = MARGIN_X + 0.f;
    this->areas[AREA_TOP_LEFT].base_y = _tms.window_height - MARGIN_Y;
    this->areas[AREA_TOP_LEFT].imodx  =  1.0f;
    this->areas[AREA_TOP_LEFT].imody  = -0.5f;
    this->areas[AREA_TOP_LEFT].modx   =  0.0f;
    this->areas[AREA_TOP_LEFT].mody   = -1.0f;
    this->areas[AREA_TOP_LEFT].tmodx  =  1.0f;
    this->areas[AREA_TOP_LEFT].tmody  =  0.0f;
    this->areas[AREA_TOP_LEFT].horizontal_align = ALIGN_LEFT;

    this->areas[AREA_TOP_LEFT2].base_x = MARGIN_X + 50.f;
    this->areas[AREA_TOP_LEFT2].base_y = _tms.window_height - MARGIN_Y;
    this->areas[AREA_TOP_LEFT2].imodx  =  1.0f;
    this->areas[AREA_TOP_LEFT2].imody  = -1.0f;
    this->areas[AREA_TOP_LEFT2].modx   =  0.0f;
    this->areas[AREA_TOP_LEFT2].mody   = -1.0f;
    this->areas[AREA_TOP_LEFT2].tmodx  =  1.0f;
    this->areas[AREA_TOP_LEFT2].tmody  =  0.0f;
    this->areas[AREA_TOP_LEFT2].checkarea = AREA_TOP_LEFT;
    this->areas[AREA_TOP_LEFT2].horizontal_align = ALIGN_LEFT;

    this->areas[AREA_BOTTOM_LEFT].base_x = MARGIN_X + 0.f;
    this->areas[AREA_BOTTOM_LEFT].base_y = MARGIN_Y;
    this->areas[AREA_BOTTOM_LEFT].imodx  =  1.0f;
    this->areas[AREA_BOTTOM_LEFT].imody  =  1.0f;
    this->areas[AREA_BOTTOM_LEFT].modx   =  1.0f;
    this->areas[AREA_BOTTOM_LEFT].mody   =  0.0f;
    this->areas[AREA_BOTTOM_LEFT].tmodx  = -0.4f;
    this->areas[AREA_BOTTOM_LEFT].tmody  =  1.0f;
    this->areas[AREA_BOTTOM_LEFT].horizontal_align = ALIGN_LEFT;

    this->areas[AREA_NOMARGIN_BOTTOM_LEFT].base_x = 0.f;
    this->areas[AREA_NOMARGIN_BOTTOM_LEFT].base_y = 0.f;
    this->areas[AREA_NOMARGIN_BOTTOM_LEFT].imodx  =  1.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_LEFT].imody  =  1.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_LEFT].modx   =  1.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_LEFT].mody   =  0.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_LEFT].tmodx  = -0.4f;
    this->areas[AREA_NOMARGIN_BOTTOM_LEFT].tmody  =  1.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_LEFT].horizontal_align = ALIGN_LEFT;

    this->areas[AREA_BOTTOM_RIGHT].base_x = _tms.window_width - MARGIN_X/2.f;
    this->areas[AREA_BOTTOM_RIGHT].base_y = MARGIN_Y/2.f;
    this->areas[AREA_BOTTOM_RIGHT].imodx  = -1.0f;
    this->areas[AREA_BOTTOM_RIGHT].imody  =  1.0f;
    this->areas[AREA_BOTTOM_RIGHT].modx   = -1.0f;
    this->areas[AREA_BOTTOM_RIGHT].mody   =  0.0f;
    this->areas[AREA_BOTTOM_RIGHT].tmodx  = -0.4f;
    this->areas[AREA_BOTTOM_RIGHT].tmody  =  1.0f;
    this->areas[AREA_BOTTOM_RIGHT].horizontal_align = ALIGN_RIGHT;

    this->areas[AREA_NOMARGIN_BOTTOM_RIGHT].base_x = _tms.window_width;
    this->areas[AREA_NOMARGIN_BOTTOM_RIGHT].base_y = 0.f;
    this->areas[AREA_NOMARGIN_BOTTOM_RIGHT].imodx  = -1.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_RIGHT].imody  =  1.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_RIGHT].modx   = -1.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_RIGHT].mody   =  0.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_RIGHT].tmodx  = -0.4f;
    this->areas[AREA_NOMARGIN_BOTTOM_RIGHT].tmody  =  1.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_RIGHT].horizontal_align = ALIGN_RIGHT;

    this->areas[AREA_BOTTOM_CENTER].base_x = _tms.window_width / 2.f;
    this->areas[AREA_BOTTOM_CENTER].base_y = MARGIN_Y;
    this->areas[AREA_BOTTOM_CENTER].imodx  =  1.0f;
    this->areas[AREA_BOTTOM_CENTER].imody  =  1.0f;
    this->areas[AREA_BOTTOM_CENTER].modx   =  1.0f;
    this->areas[AREA_BOTTOM_CENTER].mody   =  0.0f;
    this->areas[AREA_BOTTOM_CENTER].tmodx  = -0.5f;
    this->areas[AREA_BOTTOM_CENTER].tmody  =  1.0f;
    this->areas[AREA_BOTTOM_CENTER].horizontal_align = ALIGN_CENTER;

    this->areas[AREA_NOMARGIN_BOTTOM_CENTER].base_x = _tms.window_width/2.f;
    this->areas[AREA_NOMARGIN_BOTTOM_CENTER].base_y = 0.f;
    this->areas[AREA_NOMARGIN_BOTTOM_CENTER].imodx  =  1.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_CENTER].imody  =  1.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_CENTER].modx   =  1.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_CENTER].mody   =  0.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_CENTER].tmodx  = -0.5f;
    this->areas[AREA_NOMARGIN_BOTTOM_CENTER].tmody  =  1.0f;
    this->areas[AREA_NOMARGIN_BOTTOM_CENTER].horizontal_align = ALIGN_CENTER;

    this->areas[AREA_TOP_CENTER].base_x = _tms.window_width / 2.f;
    this->areas[AREA_TOP_CENTER].base_y = _tms.window_height - MARGIN_Y;
    this->areas[AREA_TOP_CENTER].imodx  =  1.0f;
    this->areas[AREA_TOP_CENTER].imody  = -1.0f;
    this->areas[AREA_TOP_CENTER].modx   =  1.0f;
    this->areas[AREA_TOP_CENTER].mody   =  0.0f;
    this->areas[AREA_TOP_CENTER].tmodx  = -0.5f;
    this->areas[AREA_TOP_CENTER].tmody  = -1.0f;
    this->areas[AREA_TOP_CENTER].horizontal_align = ALIGN_CENTER;

    this->areas[AREA_SUB_HEALTH].base_x = _tms.window_width / 2.f;
    this->areas[AREA_SUB_HEALTH].base_y =
          _tms.window_height
        - MARGIN_Y
        - ((_tms.yppcm  * .2f)/2.f)
        - (((_tms.yppcm * .2f)/2.f)*1.25f)
        - ((_tms.yppcm  * .1f)/2.f)
        - (((_tms.yppcm * .1f)/2.f)*1.0f)
        ;
    this->areas[AREA_SUB_HEALTH].imodx  =  1.0f;
    this->areas[AREA_SUB_HEALTH].imody  = -0.5f;
    this->areas[AREA_SUB_HEALTH].modx   =  1.0f;
    this->areas[AREA_SUB_HEALTH].mody   =  0.0f;
    this->areas[AREA_SUB_HEALTH].tmodx  = -0.5f;
    this->areas[AREA_SUB_HEALTH].tmody  = -1.0f;
    this->areas[AREA_SUB_HEALTH].horizontal_align = ALIGN_CENTER;

    this->areas[AREA_TOP_RIGHT].base_x = _tms.window_width - MARGIN_X;
    this->areas[AREA_TOP_RIGHT].base_y = _tms.window_height - MARGIN_Y;
    this->areas[AREA_TOP_RIGHT].imodx  = -1.0f;
    this->areas[AREA_TOP_RIGHT].imody  = -1.0f;
    this->areas[AREA_TOP_RIGHT].modx   =  0.0f;
    this->areas[AREA_TOP_RIGHT].mody   = -1.0f;
    this->areas[AREA_TOP_RIGHT].tmodx  = -1.0f;
    this->areas[AREA_TOP_RIGHT].tmody  =  0.0f;
    this->areas[AREA_TOP_RIGHT].horizontal_align = ALIGN_CENTER;

    this->areas[AREA_GAME_TOP_RIGHT].base_x = _tms.window_width - MARGIN_X - G->get_menu_width();
    this->areas[AREA_GAME_TOP_RIGHT].base_y = _tms.window_height - MARGIN_Y;
    this->areas[AREA_GAME_TOP_RIGHT].imodx  = -1.0f;
    this->areas[AREA_GAME_TOP_RIGHT].imody  = -1.0f;
    this->areas[AREA_GAME_TOP_RIGHT].modx   =  0.0f;
    this->areas[AREA_GAME_TOP_RIGHT].mody   = -1.0f;
    this->areas[AREA_GAME_TOP_RIGHT].tmodx  = -1.0f;
    this->areas[AREA_GAME_TOP_RIGHT].tmody  =  0.0f;
    this->areas[AREA_GAME_TOP_RIGHT].horizontal_align = ALIGN_CENTER;

    this->areas[AREA_MENU_TOP_RIGHT].base_x = _tms.window_width;
    this->areas[AREA_MENU_TOP_RIGHT].base_y = _tms.window_height;
    this->areas[AREA_MENU_TOP_RIGHT].imodx  = -1.0f;
    this->areas[AREA_MENU_TOP_RIGHT].imody  = -1.0f;
    this->areas[AREA_MENU_TOP_RIGHT].modx   =  0.0f;
    this->areas[AREA_MENU_TOP_RIGHT].mody   = -1.0f;
    this->areas[AREA_MENU_TOP_RIGHT].tmodx  = -1.0f;
    this->areas[AREA_MENU_TOP_RIGHT].tmody  =  0.0f;
    this->areas[AREA_MENU_TOP_RIGHT].horizontal_align = ALIGN_CENTER;

    this->areas[AREA_MENU_TOP_LEFT].base_x = 0.f;
    this->areas[AREA_MENU_TOP_LEFT].base_y = _tms.window_height;
    this->areas[AREA_MENU_TOP_LEFT].imodx  =  1.0f;
    this->areas[AREA_MENU_TOP_LEFT].imody  = -1.0f;
    this->areas[AREA_MENU_TOP_LEFT].modx   =  1.0f;
    this->areas[AREA_MENU_TOP_LEFT].mody   =  0.0f;
    this->areas[AREA_MENU_TOP_LEFT].tmodx  =  1.0f;
    this->areas[AREA_MENU_TOP_LEFT].tmody  =  0.0f;
    this->areas[AREA_MENU_TOP_LEFT].horizontal_align = ALIGN_LEFT;

    this->areas[AREA_MENU_TOP_CENTER].base_x = _tms.window_width/2.f;
    this->areas[AREA_MENU_TOP_CENTER].base_y = _tms.window_height - menu_shared::bar_height;
    this->areas[AREA_MENU_TOP_CENTER].imodx  =  1.0f;
    this->areas[AREA_MENU_TOP_CENTER].imody  = -1.0f;
    this->areas[AREA_MENU_TOP_CENTER].modx   =  1.0f;
    this->areas[AREA_MENU_TOP_CENTER].mody   =  0.0f;
    this->areas[AREA_MENU_TOP_CENTER].tmodx  = -0.5f;
    this->areas[AREA_MENU_TOP_CENTER].tmody  = -1.0f;
    this->areas[AREA_MENU_TOP_CENTER].horizontal_align = ALIGN_CENTER;
    this->areas[AREA_MENU_TOP_CENTER].last_height = 0.f;

    this->areas[AREA_MENU_CENTER].base_x = _tms.window_width/2.f;
    this->areas[AREA_MENU_CENTER].base_y = _tms.window_height/2.f;
    this->areas[AREA_MENU_CENTER].imodx  =  0.0f;
    this->areas[AREA_MENU_CENTER].imody  = -1.25f;
    this->areas[AREA_MENU_CENTER].modx   =  0.0f;
    this->areas[AREA_MENU_CENTER].mody   = -1.25f;
    this->areas[AREA_MENU_CENTER].tmodx  =  1.0f;
    this->areas[AREA_MENU_CENTER].tmody  =  0.0f;
    this->areas[AREA_MENU_CENTER].horizontal_align = ALIGN_CENTER;
    this->areas[AREA_MENU_CENTER].vertical_align = ALIGN_CENTER;
    this->areas[AREA_MENU_CENTER].last_height = 0.f;

    this->areas[AREA_MENU_LEVELS].base_x = _tms.window_width/2.f;
    this->areas[AREA_MENU_LEVELS].base_y = -100000000000.f;
    this->areas[AREA_MENU_LEVELS].imodx  =  1.0f;
    this->areas[AREA_MENU_LEVELS].imody  = -1.0f;
    this->areas[AREA_MENU_LEVELS].modx   =  1.0f;
    this->areas[AREA_MENU_LEVELS].mody   =  0.0f;
    this->areas[AREA_MENU_LEVELS].tmodx  =  0.0f;
    this->areas[AREA_MENU_LEVELS].tmody  =  0.6f;
    this->areas[AREA_MENU_LEVELS].tsmodx =  0.5f;
    this->areas[AREA_MENU_LEVELS].tsmody =  0.0f;
    this->areas[AREA_MENU_LEVELS].horizontal_align = ALIGN_CENTER;
    this->areas[AREA_MENU_LEVELS].last_height = 0.f;

    this->areas[AREA_MENU_SUB_LEVELS].base_x = _tms.window_width/2.f;
    this->areas[AREA_MENU_SUB_LEVELS].base_y = -100000000.f;
    this->areas[AREA_MENU_SUB_LEVELS].imodx  =  1.0f;
    this->areas[AREA_MENU_SUB_LEVELS].imody  = -0.0f;
    this->areas[AREA_MENU_SUB_LEVELS].modx   =  1.0f;
    this->areas[AREA_MENU_SUB_LEVELS].mody   =  0.0f;
    this->areas[AREA_MENU_SUB_LEVELS].tmodx  = -0.5f;
    this->areas[AREA_MENU_SUB_LEVELS].tmody  = -1.0f;
    this->areas[AREA_MENU_SUB_LEVELS].checkarea = AREA_MENU_SUB_LEVELS;
    this->areas[AREA_MENU_SUB_LEVELS].horizontal_align = ALIGN_CENTER;
    this->areas[AREA_MENU_SUB_LEVELS].last_height = 0.f;

    this->areas[AREA_MENU_LEFT_HCENTER].base_x = _tms.window_width/4.f;
    this->areas[AREA_MENU_LEFT_HCENTER].base_y = _tms.window_height - MARGIN_Y - menu_shared::bar_height;
    this->areas[AREA_MENU_LEFT_HCENTER].imodx  =  0.0f;
    this->areas[AREA_MENU_LEFT_HCENTER].imody  = -1.0f;
    this->areas[AREA_MENU_LEFT_HCENTER].modx   =  0.0f;
    this->areas[AREA_MENU_LEFT_HCENTER].mody   = -1.25f;
    this->areas[AREA_MENU_LEFT_HCENTER].tmodx  = -1.0f;
    this->areas[AREA_MENU_LEFT_HCENTER].tmody  =  0.0f;
    this->areas[AREA_MENU_LEFT_HCENTER].horizontal_align = ALIGN_CENTER;

    this->areas[AREA_MENU_RIGHT_HCENTER].base_x = _tms.window_width/4.f + _tms.window_width/2.f;
    this->areas[AREA_MENU_RIGHT_HCENTER].base_y = _tms.window_height - MARGIN_Y - menu_shared::bar_height;
    this->areas[AREA_MENU_RIGHT_HCENTER].imodx  =  0.0f;
    this->areas[AREA_MENU_RIGHT_HCENTER].imody  = -1.0f;
    this->areas[AREA_MENU_RIGHT_HCENTER].modx   =  0.0f;
    this->areas[AREA_MENU_RIGHT_HCENTER].mody   = -1.25f;
    this->areas[AREA_MENU_RIGHT_HCENTER].tmodx  = -1.0f;
    this->areas[AREA_MENU_RIGHT_HCENTER].tmody  =  0.0f;
    this->areas[AREA_MENU_RIGHT_HCENTER].horizontal_align = ALIGN_CENTER;

    this->areas[AREA_MENU_LEFT_HLEFT].base_x = _tms.window_width/4.f;
    this->areas[AREA_MENU_LEFT_HLEFT].base_y = _tms.window_height - MARGIN_Y - menu_shared::bar_height;
    this->areas[AREA_MENU_LEFT_HLEFT].imodx  =  0.0f;
    this->areas[AREA_MENU_LEFT_HLEFT].imody  = -1.0f;
    this->areas[AREA_MENU_LEFT_HLEFT].modx   =  0.0f;
    this->areas[AREA_MENU_LEFT_HLEFT].mody   = -1.0f;
    this->areas[AREA_MENU_LEFT_HLEFT].tmodx  = -1.0f;
    this->areas[AREA_MENU_LEFT_HLEFT].tmody  =  0.0f;
    this->areas[AREA_MENU_LEFT_HLEFT].label_halign = ALIGN_LEFT;
    this->areas[AREA_MENU_LEFT_HLEFT].horizontal_align = ALIGN_LEFT;

    this->areas[AREA_MENU_RIGHT_HRIGHT].base_x = _tms.window_width/4.f + _tms.window_width/2.f;
    this->areas[AREA_MENU_RIGHT_HRIGHT].base_y = _tms.window_height - MARGIN_Y - menu_shared::bar_height;
    this->areas[AREA_MENU_RIGHT_HRIGHT].imodx  =  0.0f;
    this->areas[AREA_MENU_RIGHT_HRIGHT].imody  = -1.0f;
    this->areas[AREA_MENU_RIGHT_HRIGHT].modx   =  0.0f;
    this->areas[AREA_MENU_RIGHT_HRIGHT].mody   = -1.0f;
    this->areas[AREA_MENU_RIGHT_HRIGHT].tmodx  = -1.0f;
    this->areas[AREA_MENU_RIGHT_HRIGHT].tmody  =  0.0f;
    this->areas[AREA_MENU_RIGHT_HRIGHT].label_halign = ALIGN_RIGHT;
    this->areas[AREA_MENU_RIGHT_HRIGHT].horizontal_align = ALIGN_RIGHT;

    this->areas[AREA_CREATE_LEFT_SUB].base_x = _tms.window_width/4.f;
    this->areas[AREA_CREATE_LEFT_SUB].base_y = _tms.window_height - MARGIN_Y - menu_shared::bar_height;
    this->areas[AREA_CREATE_LEFT_SUB].imodx  =  0.0f;
    this->areas[AREA_CREATE_LEFT_SUB].imody  = -1.25f;
    this->areas[AREA_CREATE_LEFT_SUB].modx   =  0.0f;
    this->areas[AREA_CREATE_LEFT_SUB].mody   = -1.25f;
    this->areas[AREA_CREATE_LEFT_SUB].tmodx  = -1.0f;
    this->areas[AREA_CREATE_LEFT_SUB].tmody  =  0.0f;
    this->areas[AREA_CREATE_LEFT_SUB].label_halign = ALIGN_CENTER;
    this->areas[AREA_CREATE_LEFT_SUB].horizontal_align = ALIGN_CENTER;

    this->areas[AREA_MENU_BOTTOM_LEFT].base_x = MARGIN_X;
    this->areas[AREA_MENU_BOTTOM_LEFT].base_y = MARGIN_Y + menu_shared::bar_height;
    this->areas[AREA_MENU_BOTTOM_LEFT].imodx  =  1.0f;
    this->areas[AREA_MENU_BOTTOM_LEFT].imody  =  1.0f;
    this->areas[AREA_MENU_BOTTOM_LEFT].modx   =  0.0f;
    this->areas[AREA_MENU_BOTTOM_LEFT].mody   = -1.0f;
    this->areas[AREA_MENU_BOTTOM_LEFT].tmodx  = -1.0f;
    this->areas[AREA_MENU_BOTTOM_LEFT].tmody  =  0.0f;
    this->areas[AREA_MENU_BOTTOM_LEFT].horizontal_align = ALIGN_LEFT;
    this->areas[AREA_MENU_BOTTOM_LEFT].last_width = 0.f;

    this->areas[AREA_MENU_BOTTOM_CENTER].base_x = _tms.window_width/2.f;
    this->areas[AREA_MENU_BOTTOM_CENTER].base_y = MARGIN_Y + menu_shared::bar_height;
    this->areas[AREA_MENU_BOTTOM_CENTER].imodx  =  0.0f;
    this->areas[AREA_MENU_BOTTOM_CENTER].imody  = -1.0f;
    this->areas[AREA_MENU_BOTTOM_CENTER].modx   =  0.0f;
    this->areas[AREA_MENU_BOTTOM_CENTER].mody   = -1.0f;
    this->areas[AREA_MENU_BOTTOM_CENTER].tmodx  = -1.0f;
    this->areas[AREA_MENU_BOTTOM_CENTER].tmody  =  0.0f;
    this->areas[AREA_MENU_BOTTOM_CENTER].horizontal_align = ALIGN_CENTER;
    this->areas[AREA_MENU_BOTTOM_CENTER].last_width = 0.f;

    this->areas[AREA_CREATE_CONTEST_TOP].base_x = MARGIN_X;
    this->areas[AREA_CREATE_CONTEST_TOP].base_y = _tms.window_height - MARGIN_Y;
    this->areas[AREA_CREATE_CONTEST_TOP].imodx  =  1.25f;
    this->areas[AREA_CREATE_CONTEST_TOP].imody  =  0.0f;
    this->areas[AREA_CREATE_CONTEST_TOP].modx   =  1.0f;
    this->areas[AREA_CREATE_CONTEST_TOP].mody   =  0.0f;

    this->areas[AREA_CREATE_CONTEST_BOTTOM].base_x = MARGIN_X;
    this->areas[AREA_CREATE_CONTEST_BOTTOM].base_y = MARGIN_Y + menu_shared::bar_height;
    this->areas[AREA_CREATE_CONTEST_BOTTOM].imodx  =  1.25f;
    this->areas[AREA_CREATE_CONTEST_BOTTOM].imody  =  1.0f;
    this->areas[AREA_CREATE_CONTEST_BOTTOM].modx   =  1.0f;
    this->areas[AREA_CREATE_CONTEST_BOTTOM].mody   =  0.0f;
    this->areas[AREA_CREATE_CONTEST_BOTTOM].tmodx  = -1.0f;
    this->areas[AREA_CREATE_CONTEST_BOTTOM].tmody  =  0.0f;
}

void
widget_manager::refresh_areas()
{
    if (this->get_home() == G) {
        if (W->is_paused() && G->state.sandbox) {
            this->areas[AREA_GAME_TOP_RIGHT].base_x = _tms.window_width - MARGIN_X - G->get_menu_width();
        } else {
            this->areas[AREA_GAME_TOP_RIGHT].base_x = _tms.window_width - MARGIN_X;
        }
    } else if (this->get_home() == P.s_menu_main) {
        this->areas[AREA_MENU_TOP_CENTER].base_y =
              _tms.window_height
            - (.75f*menu_shared::tex_principia->height*P.s_menu_main->scale)
            - menu_shared::bar_height;

        this->areas[AREA_MENU_LEVELS].base_y = this->areas[AREA_MENU_TOP_CENTER].base_y - this->areas[AREA_MENU_TOP_CENTER].last_height;
#ifdef TMS_BACKEND_PC
        this->areas[AREA_MENU_LEVELS].base_y -= font::medium->get_height()*1.3f;
#else
        if (_tms.window_height > 500) {
            this->areas[AREA_MENU_LEVELS].base_y -= font::medium->get_height()*1.3f;
        }
#endif

        this->areas[AREA_MENU_SUB_LEVELS].base_y =
            this->areas[AREA_MENU_LEVELS].bot.y;
    } else if (this->get_home() == P.s_menu_create) {
        this->areas[AREA_CREATE_LEFT_SUB].base_y = this->areas[AREA_MENU_LEFT_HCENTER].bot.y;

        this->areas[AREA_CREATE_CONTEST_BOTTOM].base_x =
              this->areas[AREA_MENU_BOTTOM_LEFT].base_x
            + MARGIN_X
            + this->areas[AREA_MENU_BOTTOM_LEFT].last_width;

        this->areas[AREA_CREATE_CONTEST_TOP].base_x =
              this->areas[AREA_CREATE_CONTEST_BOTTOM].base_x;

        this->areas[AREA_CREATE_CONTEST_TOP].base_y =
              this->areas[AREA_CREATE_CONTEST_BOTTOM].first_pos.y
            + this->areas[AREA_CREATE_CONTEST_BOTTOM].last_height;

        //this->areas[AREA_CREATE_CONTEST_TOP].base_y = this->areas[AREA_MENU_BOTTOM_LEFT].top.y;
        this->areas[AREA_CREATE_CONTEST_TOP].base_y = this->areas[AREA_CREATE_CONTEST_BOTTOM].top.y;


        int32_t diff = this->areas[AREA_CREATE_LEFT_SUB].bot.y ;

        int32_t min_diff = 135 + MARGIN_Y + menu_shared::bar_height;

        tms_debugf("diff: %d < %d?", diff, min_diff);

        if (diff < min_diff) {
            this->areas[AREA_CREATE_CONTEST_TOP].enabled = false;
            this->areas[AREA_CREATE_CONTEST_BOTTOM].enabled = false;
            this->areas[AREA_MENU_BOTTOM_LEFT].enabled = false;
        } else {
            this->areas[AREA_CREATE_CONTEST_TOP].enabled = true;
            this->areas[AREA_CREATE_CONTEST_BOTTOM].enabled = true;
            this->areas[AREA_MENU_BOTTOM_LEFT].enabled = true;
        }
    } else if (this->get_home() == P.s_menu_play) {
        this->areas[AREA_MENU_LEFT_HLEFT].base_y = this->areas[AREA_MENU_TOP_CENTER].bot.y;

        this->areas[AREA_MENU_RIGHT_HRIGHT].base_y = this->areas[AREA_MENU_TOP_CENTER].bot.y;
    }
}

principia_wdg*
widget_manager::create_widget(tms::surface *surface, int widget_type,
                              uint8_t id, WidgetArea area,
                              struct tms_sprite *s0, struct tms_sprite *s1,
                              float scale/*=1.f*/)
{
    principia_wdg *wdg = new principia_wdg(surface, widget_type, s0, s1, scale);
    wdg->data = UINT_TO_VOID(id);
    wdg->area = &this->areas[area];
    wdg->lmody = wdg->area->imody;
    wdg->parent = this;

    if (this->override_down) {
        tms_errorf("touch down is not implemented!");
        //wdg->touch_down = this->touch_down;
    }

    if (this->override_up) {
        wdg->touch_up = base_touch_up;
    }

    this->widgets.push_back(wdg);

    return wdg;
}

principia_wdg*
widget_manager::get_widget(enum WidgetArea area_id, uint8_t id)
{
    if (area_id >= NUM_AREAS) {
        tms_errorf("Invalid area %u", area_id);
        return 0;
    }

    struct widget_area *area = &this->areas[area_id];
    uint8_t counter = 0;

    for (std::deque<principia_wdg*>::iterator it = this->widgets.begin();
            it != this->widgets.end(); ++it) {
        principia_wdg *w = *it;

        if (w->surface && w->area == area) {
            if (id == counter) {
                return w;
            } else {
                ++ counter;
            }
        }
    }

    return 0;
}

void
widget_manager::step()
{
    for (std::deque<principia_wdg*>::iterator it = this->widgets.begin();
            it != this->widgets.end(); ++it) {
        principia_wdg *w = *it;

        if (w->surface) {
            w->step();
        }
    }

    for (int i=0; i<NUM_AREAS; ++i) {
        this->areas[i].do_set_alpha = false;
    }
}

void
widget_manager::remove_all()
{
    for (std::deque<principia_wdg*>::iterator it = this->widgets.begin();
            it != this->widgets.end(); ++it) {
        principia_wdg *w = *it;

        w->remove();
    }
}

struct widget_sorter
{
    static bool order(principia_wdg* a, principia_wdg* b)
    {
        if (a->area == b->area) {
            return a->priority > b->priority;
        }

        return a->area < b->area;
    }
};

void
widget_manager::render()
{
#ifdef DRAW_AREA_BOUNDING_BOX
    {
        const float cr = 2.5f;
        for (int x=0; x<NUM_AREAS; ++x) {
            struct widget_area *area = &this->areas[x];

            if (!area->first_widget) {
                float x = (area->bot.x + area->top.x) / 2.f;
                float y = (area->bot.y + area->top.y) / 2.f;
                float w = area->top.x - area->bot.x;
                float h = area->top.y - area->bot.y;
                tms_ddraw_set_color(this->get_home()->get_surface()->ddraw, 1.f, 1.f, 1.f, 1.f);
                tms_ddraw_lsquare(this->get_home()->get_surface()->ddraw,
                        x,
                        y,
                        w,
                        h);

                tms_ddraw_circle(this->get_home()->get_surface()->ddraw,
                        area->top.x,
                        area->bot.y,
                        cr, cr);

                tms_ddraw_circle(this->get_home()->get_surface()->ddraw,
                        area->top.x,
                        area->top.y,
                        cr, cr);

                tms_ddraw_circle(this->get_home()->get_surface()->ddraw,
                        area->bot.x,
                        area->top.y,
                        cr, cr);

                tms_ddraw_circle(this->get_home()->get_surface()->ddraw,
                        area->bot.x,
                        area->bot.y,
                        cr, cr);
            }
        }
    }
#endif

    for (std::deque<principia_wdg*>::iterator it = this->widgets.begin();
            it != this->widgets.end(); ++it) {
        principia_wdg *w = *it;

        if (w->surface && w->label) {
            pending_text *pt = new pending_text(60, w->label);

            if (w->_type == TMS_WDG_LABEL) {
                pt->x = w->pos.x;
                pt->y = w->pos.y;

                int state = 0;

                if (w->faded) {
                    state = 100;
                }

                if (w->focused) {
                    ++ state;
                }
                if (w->hovered && !w->faded) {
                    ++ state;
                }

                pt->set_color = true;

                tvec4 bg_color;

                switch (state) {
                    case 0:
                        /* Active widget, not hovered nor focused */
                        pt->o_r = 0.0f;
                        pt->o_g = 0.0f;
                        pt->o_b = 0.0f;
                        bg_color.r = 0.7f;
                        bg_color.g = 0.7f;
                        bg_color.b = 0.7f;
                        bg_color.a = pt->a * .2f;
                        break;
                    case 1:
                        /* Active widget, hovered or focused */
                        pt->o_r = 0.4f;
                        pt->o_g = 0.4f;
                        pt->o_b = 0.4f;
                        bg_color.r = 0.7f;
                        bg_color.g = 0.7f;
                        bg_color.b = 0.7f;
                        bg_color.a = pt->a * .3f;
                        break;
                    case 2:
                        /* Active widget, hovered AND focused */
                        pt->o_r = 0.4f;
                        pt->o_g = 0.4f;
                        pt->o_b = 0.7f;
                        bg_color.r = 0.7f;
                        bg_color.g = 0.7f;
                        bg_color.b = 0.7f;
                        bg_color.a = pt->a * .4f;
                        break;

                    case 100:
                    case 101:
                    case 102:
                        /* Faded widget, disregarding further state */
                        pt->o_r = 0.0f;
                        pt->o_g = 0.0f;
                        pt->o_b = 0.0f;
                        bg_color.r = 0.7f;
                        bg_color.g = 0.7f;
                        bg_color.b = 0.7f;
                        bg_color.a = pt->a * .2f;
                        break;
                }

                if (w->render_background) {
                    //tms_infof("RENDER BG");
                    float width = w->label->get_width();
                    float height = w->label->get_height();
                    this->get_home()->add_rounded_square(pt->x, pt->y, width * 1.05f, height * 1.05f, bg_color, 4.f);
                }
            }

            this->get_home()->add_pending_text(pt);
        }
    }

    for (std::deque<principia_wdg*>::iterator it = this->widgets.begin();
            it != this->widgets.end(); ++it) {
        principia_wdg *w = *it;

        if (w->surface && w->tooltip && w->tooltip_active && w->tooltip->active) {
            pending_text *pt = new pending_text(60, w->tooltip);
            float alpha = tclampf((w->tooltip_time - TOOLTIP_ACTIVATION_TIME) * 5.0f, 0.f, 1.f);

            float width = w->tooltip->get_width();
            float height = w->tooltip->get_max_height();

            float x = pt->x;
            float y = pt->y;

            if (w->area->tmodx < -0.5f) {
                /* align right */
                x -= width/2.f;
            } else {
                /* align left */
                x += width/2.f;
            }

            w->tooltip->color.a = alpha;
            w->tooltip->outline_color.a = alpha;

            this->get_home()->add_rounded_square(x, y, width, height * 1.1f, tvec4f(.2f, .2f, .2f, alpha*0.85f), 4.f);

            this->get_home()->add_pending_text(pt);
        }
    }
}

/**
 * Loop through all active widgets and set their positions according to
 * their areas and priorities
 **/
void
widget_manager::rearrange()
{
    this->refresh_areas();

    /* Reset x/y for all areas */
    for (int i=0; i<NUM_AREAS; ++i) {
        this->areas[i].x = this->areas[i].base_x;
        this->areas[i].y = this->areas[i].base_y;
        this->areas[i].first_widget = true;
        this->areas[i].top = tvec2f(-5000, -5000);
        this->areas[i].bot = tvec2f( 5000,  5000);
    }

    std::sort(this->widgets.begin(), this->widgets.end(), &widget_sorter::order);

    for (std::deque<principia_wdg*>::iterator it = this->widgets.begin();
            it != this->widgets.end(); ++it) {
        principia_wdg *w = *it;
        if (!w->surface || w->draggable) continue;

        switch (w->area->horizontal_align) {
            case ALIGN_CENTER:
                w->area->x += ((w->size.x + w->padding.x) * -w->area->modx) / 2.f;
                break;

            default:
                /* Do nothing, default behaviour. */
                break;
        }

        switch (w->area->vertical_align) {
            case ALIGN_CENTER:
                w->area->y += ((w->size.y + w->padding.y) * -w->area->mody) / 2.f;
                break;

            default:
                /* Do nothing, default behavior. */
                break;
        }
    }

    for (std::deque<principia_wdg*>::iterator it = this->widgets.begin();
            it != this->widgets.end(); ++it) {
        principia_wdg *w = *it;
        if (!w->surface || w->draggable) continue;

        bool first = w->area->first_widget;

        w->area->used = true;
        w->area->first_widget = false;

        w->area->last_width = w->size.x;
        w->area->last_height = w->size.y;

        if (w->area->checkarea != NUM_AREAS) {
            struct widget_area *checkarea = &this->areas[w->area->checkarea];

            if (!checkarea->used) {
                w->area->x = checkarea->x;
                w->area->y = checkarea->y;
            }
        }

        float x = w->area->x;
        float y = w->area->y;

        if (w->_type == TMS_WDG_LABEL && w->label) {
            w->size.x = w->label->get_width();
            w->size.y = w->label->get_height();

            if (w->area->label_halign == ALIGN_LEFT) {
                x += w->size.x/2.f;
            }
        }

        x += w->area->imodx * (w->size.x / 2.f + w->padding.x / 2.f);
        y += w->area->imody * (w->size.y / 2.f + w->padding.y / 2.f);

        w->set_position(x, y);

        if (first) {
            w->area->first_off.x = (x - (w->size.x) - w->padding.x) - w->area->base_x;
            w->area->first_off.y = (y - (w->size.y) - w->padding.y) - w->area->base_y;

            w->area->first_pos = w->pos;
        }

        w->area->last_pos = w->pos;

        if (w->_type == TMS_WDG_LABEL && w->label) {
            //w->label->set_position(x, y);
            //x -= w->area->imodx * (w->size.x / 2.f + w->padding.x / 2.f);
            //y -= w->area->imody * (w->size.y / 2.f + w->padding.y / 2.f);


            //x += w->area->imodx * (w->size.x / 2.f + w->padding.x / 2.f);
            //y += w->area->imody * (w->size.y / 2.f + w->padding.y / 2.f);


            w->area->x += (w->size.x + w->padding.x) * w->area->modx;
            w->area->y += (w->size.y + w->padding.y) * w->area->mody;
        } else {
            if (w->label) {
                w->label->set_position(x + (w->lmodx * w->size.x), y + (w->lmody * w->size.y));
                w->area->last_height += w->label->get_height()*1.5f;
            }
            w->area->x += (w->size.x + w->padding.x) * w->area->modx;
            w->area->y += (w->size.y + w->padding.y) * w->area->mody;
        }

        if (x-w->size.x < w->area->bot.x) {
            w->area->bot.x = x-w->size.x;
        }
        if (x+w->size.x > w->area->top.x) {
            w->area->top.x = x+w->size.x;
        }

        if (y-w->size.y < w->area->bot.y) {
            w->area->bot.y = y-w->size.y;
        }
        if (y+w->size.y > w->area->top.y) {
            w->area->top.y = y+w->size.y;
        }
    }
}
