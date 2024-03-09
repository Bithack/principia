#include <stdarg.h>

#include "tms.h"
#include "surface.h"
#include "event.h"
#include "wdg.h"
#include "texture.h"
#include "atlas.h"
#include "util.h"

#include "../math/glob.h"

#include <tms/backend/opengl.h>

#include "ddraw.h"

//#define DRAW_WIDGET_OUTLINES

#ifdef TMS_BACKEND_PC
#define TOLERANCE_X (tms.xppcm*.125f)
#define TOLERANCE_Y (tms.yppcm*.125f)
#else
#define TOLERANCE_X (tms.xppcm*.25f)
#define TOLERANCE_Y (tms.yppcm*.25f)
#endif
#define HOVER_TOLERANCE_X (tms.xppcm*.035f)
#define HOVER_TOLERANCE_Y (tms.yppcm*.035f)

/**
 * Allocate a surface.
 *
 * Will be called automatically by a screen if the screen's
 * spec has TMS_SCREEN_ALLOC_SURFACE.
 *
 * @relates tms_surface
 **/
struct tms_surface *tms_surface_alloc(void)
{
    struct tms_surface *s;

    (s = malloc(sizeof(struct tms_surface))) || tms_fatalf("out of mem (s_a)");

    tms_surface_init(s);

    return s;
}

/**
 * Initialize a surface.
 *
 * @relates tms_surface
 **/
int
tms_surface_init(struct tms_surface *s)
{
    float projection[16];

    s->alpha = 1.f;
    s->widgets = 0;
    s->widget_count = 0;
    s->widget_cap = 0;

    memset(s->active_widgets, 0, sizeof(s->active_widgets));

    tmat4_set_ortho(projection, 0, tms.window_width, 0, tms.window_height, 1, -1);

    s->ddraw = tms_ddraw_alloc();
    tms_ddraw_set_matrices(s->ddraw, 0, projection);

    return T_OK;
}

#ifdef DEBUG
static void
tms_surface_draw_widget_outlines(struct tms_surface *s, struct tms_wdg *w, int state)
{
    switch (state) {
        case 0:   tms_ddraw_set_color(s->ddraw, 0.6f, 0.6f, 0.6f, 1.f); break;
        case 1:   tms_ddraw_set_color(s->ddraw, 1.0f, 0.6f, 0.6f, 1.f); break;
        case 2:   tms_ddraw_set_color(s->ddraw, 1.0f, 1.0f, 0.6f, 1.f); break;

        case 100: tms_ddraw_set_color(s->ddraw, 0.3f, 0.3f, 0.3f, 1.f); break;
        case 101: tms_ddraw_set_color(s->ddraw, 0.3f, 1.0f, 0.3f, 1.f); break;
        case 102: tms_ddraw_set_color(s->ddraw, 0.3f, 1.0f, 1.0f, 1.f); break;
    }

    tms_ddraw_lsquare(s->ddraw,
            w->pos.x, w->pos.y,
            w->size.x, w->size.y);

    tms_ddraw_set_color(s->ddraw, 1.0, 0.3, 1.0, 1.0);

    tms_ddraw_lsquare(s->ddraw,
            w->pos.x, w->pos.y,
            w->size.x+TOLERANCE_X*2, w->size.y+TOLERANCE_Y*2);
}
#endif

/**
 * Render the surface (all widgets)
 *
 * @relates tms_surface
 **/
int
tms_surface_render(struct tms_surface *s)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    tms_ddraw_set_color(s->ddraw, 1.f, 1.f, 1.f, s->alpha);

    int x;

    glActiveTexture(GL_TEXTURE0);
    tms_texture_bind(&s->atlas->texture);

    /**
     * states:
     * 0   = base = 1.0
     * 100 = base = 0.5, due to widget faded
     *
     * +1  = base + 0.1 for every state increase
     **/
    int last_state = -100;

#ifndef NO_UI
    for (int x=0; x<s->widget_count; x++) {
        if (s->widgets[x]->render) {
            float alpha = fmin(s->alpha, s->widgets[x]->alpha);
            if (s->widgets[x]->color) {
                tms_ddraw_set_color(s->ddraw, s->widgets[x]->color->r, s->widgets[x]->color->g, s->widgets[x]->color->b, alpha);
                last_state = -100;
            } else {
                int state = 0;

                if (s->widgets[x]->faded) {
                    state = 100;
                }

                if ((s->widgets[x]->focused || ((s->widgets[x]->ghost[0] == 1.f && s->widgets[x]->enable_ghost && s->widgets[x]->type == TMS_WDG_BUTTON)))) {
                    ++ state;
                }
                if (s->widgets[x]->hovered && !s->widgets[x]->faded) {
                    ++ state;
                }

                if (1 || last_state != state) {
                    last_state = state;

                    switch (state) {
                        case 0:   tms_ddraw_set_color(s->ddraw, 1.0f, 1.0f, 1.0f, alpha); break;
                        case 1:   tms_ddraw_set_color(s->ddraw, 1.1f, 1.1f, 1.1f, alpha); break;
                        case 2:   tms_ddraw_set_color(s->ddraw, 1.2f, 1.2f, 1.2f, alpha); break;

                        case 100: tms_ddraw_set_color(s->ddraw, 0.5f, 0.5f, 0.5f, alpha); break;
                        case 101: tms_ddraw_set_color(s->ddraw, 0.6f, 0.6f, 0.6f, alpha); break;
                        case 102: tms_ddraw_set_color(s->ddraw, 0.7f, 0.7f, 0.7f, alpha); break;
                    }
                }
            }

            s->widgets[x]->render(s->widgets[x], s);

            if (s->widgets[x]->post_render) {
                s->widgets[x]->post_render(s->widgets[x], s);
            }

#ifdef DEBUG
#ifdef DRAW_WIDGET_OUTLINES
            tms_surface_draw_widget_outlines(s, s->widgets[x], state);
#endif
#endif
        }
    }
#endif

    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_DEPTH_TEST);

    return T_OK;
}

/**
 * Add many widgets to the surface
 *
 * @relates tms_surface
 **/
int tms_surface_add_widgets(struct tms_surface *s, int num_widgets, ...)
{
    va_list ap;
    va_start(ap, num_widgets);

    for (int x=0; x<num_widgets; x++)
        if (tms_surface_add_widget(s, va_arg(ap, struct tms_wdg *)) != T_OK)
            goto err;

    va_end(ap);

    return T_OK;

err:
    va_end(ap);

    return T_OUT_OF_MEM;
}

static void
get_nearest_widget(struct tms_surface *s, int mx, int my, int *nearest_id, int *nearest_ox, int *nearest_oy)
{
    float nearest = 100000.f;
    *nearest_id = -1;
    *nearest_ox = 0;
    *nearest_oy = 0;

    for (int i = 0; i < s->widget_count; ++i) {
        int ox = mx - s->widgets[i]->pos.x;
        int oy = my - s->widgets[i]->pos.y;
        float dist = sqrtf((float)(ox*ox + oy*oy));

        if (s->widgets[i]->type == TMS_WDG_RADIAL) {
            if (dist < (s->widgets[i]->size.x/2.f + TOLERANCE_X*3.f)) {
                if (dist < nearest && s->widgets[i]->clickthrough == 0) {
                    *nearest_id = i;
                    *nearest_ox = ox;
                    *nearest_oy = oy;
                    nearest = dist;
                }
            }
        } else if (ox < s->widgets[i]->size.x/2.f + TOLERANCE_X + s->widgets[i]->extra_right
                && ox > -s->widgets[i]->size.x/2.f - TOLERANCE_X - s->widgets[i]->extra_left
                && oy < s->widgets[i]->size.y/2.f + TOLERANCE_Y + s->widgets[i]->extra_up
                && oy > -s->widgets[i]->size.y/2.f - TOLERANCE_Y - s->widgets[i]->extra_down
                && abs(oy) < s->widgets[i]->size.y/2.f + TOLERANCE_Y) {
            if (dist < nearest && s->widgets[i]->clickthrough == 0) {
                *nearest_id = i;
                *nearest_ox = ox;
                *nearest_oy = oy;
                nearest = dist;
            }
        }
    }

}

static void
forward_touch(struct tms_surface *s, struct tms_wdg *w, int pid, int bid, int ox, int oy, void (*handler)(struct tms_wdg*, int pid, int bid, int ox, int oy, float rx, float ry))
{
    float rx = (float)ox / ((float)w->size.x/2.f);
    float ry = (float)oy / ((float)w->size.y/2.f);

    if (w->type != TMS_WDG_RADIAL) {
        if (rx < -1.f) rx = -1.f;
        if (rx > 1.f) rx = 1.f;
        if (ry < -1.f) ry = -1.f;
        if (ry > 1.f) ry = 1.f;
    }

    if (handler) {
        handler(w, pid, bid, ox, oy, rx, ry);
    }
}

static void
handle_mouse_movement(struct tms_surface *s, int mx, int my)
{
    for (int i = 0; i < s->widget_count; ++i) {
        int ox = mx - s->widgets[i]->pos.x;
        int oy = my - s->widgets[i]->pos.y;

        int new_hover = 0;

        if (s->widgets[i]->type == TMS_WDG_RADIAL) {
            float dist = sqrtf((float)(ox*ox + oy*oy));
            if (dist < (s->widgets[i]->size.x/2.f + HOVER_TOLERANCE_X*3.f)) {
                new_hover = 1;
            }
        } else if (ox < s->widgets[i]->size.x/2.f + HOVER_TOLERANCE_X + s->widgets[i]->extra_right
                && ox > -s->widgets[i]->size.x/2.f - HOVER_TOLERANCE_X - s->widgets[i]->extra_left
                && oy < s->widgets[i]->size.y/2.f + HOVER_TOLERANCE_Y + s->widgets[i]->extra_up
                && oy > -s->widgets[i]->size.y/2.f - HOVER_TOLERANCE_Y - s->widgets[i]->extra_down
                && abs(oy) < s->widgets[i]->size.y/2.f + HOVER_TOLERANCE_Y) {
            new_hover = 1;
        }

        if (s->widgets[i]->clickthrough == 0 && new_hover != s->widgets[i]->hovered) {
            s->widgets[i]->hovered = new_hover;

            /* we always assume an mouse_over and mouse_out is set */
            if (new_hover) {
                s->widgets[i]->mouse_over(s->widgets[i]);
            } else {
                s->widgets[i]->mouse_out(s->widgets[i]);
            }
        }
    }
}

int
tms_surface_handle_input(struct tms_surface *s,
                         const struct tms_event *ev, int action)
{
    int p_id = ev->data.motion.pointer_id;
    int b_id = ev->data.button.button;

    int mx = ev->data.motion.x;
    int my = ev->data.motion.y;

    if (tms.emulating_portrait) {
        //tms_convert_to_portrait(&mx, &my);

        /* convert back from portrait */

        int tmp_x = mx;

        if (my < _tms.window_height / 2) {
            mx = my;
        } else {
            mx = _tms.window_width - (_tms.window_height - my);
        }

        my = _tms.window_width - tmp_x;
    }

    switch (ev->type) {
        case TMS_EV_POINTER_DOWN:
            {
                if (p_id > TMS_MAX_POINTER_ID)
                    return T_CONT;
                /* only search through all widgets on pointer down */

                int nearest_id, nearest_ox, nearest_oy;
                get_nearest_widget(s, mx, my, &nearest_id, &nearest_ox, &nearest_oy);

                if (nearest_id != -1) {
                    s->widgets[nearest_id]->focused = 1;
                    s->active_widgets[p_id] = s->widgets[nearest_id];

                    if (s->widgets[nearest_id]->type == TMS_WDG_BTN_RADIAL) {
                        /* For btn radials, we need to forward touch BEFORE we call any
                         * focus change functions. */
                        forward_touch(s, s->widgets[nearest_id], p_id, b_id, nearest_ox, nearest_oy, s->widgets[nearest_id]->touch_down);

                        if (s->active_widgets[p_id]->on_focus_change) {
                            s->active_widgets[p_id]->on_focus_change(s->active_widgets[p_id]);
                        }
                    } else {
                        if (s->active_widgets[p_id]->on_focus_change) {
                            s->active_widgets[p_id]->on_focus_change(s->active_widgets[p_id]);
                        }

                        forward_touch(s, s->widgets[nearest_id], p_id, b_id, nearest_ox, nearest_oy, s->widgets[nearest_id]->touch_down);
                    }
                    return T_OK;
                }
            }

            break;

        case TMS_EV_POINTER_MOVE:
            {
                if (p_id > TMS_MAX_POINTER_ID) {
                    return T_CONT;
                }

                handle_mouse_movement(s, mx, my);
            }
            break;

        case TMS_EV_POINTER_DRAG:
            {
                if (p_id > TMS_MAX_POINTER_ID) {
                    return T_CONT;
                }

                handle_mouse_movement(s, mx, my);

                if (s->active_widgets[p_id]) {
                    double ox = (double)mx - (double)s->active_widgets[p_id]->pos.x;
                    double oy = (double)my - (double)s->active_widgets[p_id]->pos.y;

                    forward_touch(s, s->active_widgets[p_id], p_id, b_id, (float)ox, (float)oy, s->active_widgets[p_id]->touch_drag);
                    return T_OK;
                }
            }
            break;

        case TMS_EV_POINTER_UP:
            if (p_id > TMS_MAX_POINTER_ID)
                return T_CONT;

            if (s->active_widgets[p_id]) {
                int ox = mx - s->active_widgets[p_id]->pos.x;
                int oy = my - s->active_widgets[p_id]->pos.y;

                forward_touch(s, s->active_widgets[p_id], p_id, b_id, ox, oy, s->active_widgets[p_id]->touch_up);

                if (s->active_widgets[p_id]) { /* widget could have been destroyed during touch forward */
                    s->active_widgets[p_id]->focused = 0;

                    if (s->active_widgets[p_id]->on_focus_change)
                        s->active_widgets[p_id]->on_focus_change(s->active_widgets[p_id]);

                    s->active_widgets[p_id] = 0;
                }

                return T_OK;
            }
            break;
    }

    return T_CONT;
}

/**
 * Remove a widget from the surface
 *
 * @relates tms_surface
 **/
int
tms_surface_remove_widget(struct tms_surface *s, struct tms_wdg *w)
{
    tms_assertf(w->surface == s, "attempt to remove widget from surface it was not added to");

    /* XXX pretty slow */
    for (int x=0; x<s->widget_count; x++) {
        if (s->widgets[x] == w) {
            /* we found it, swap it with the last widget
             * in the list */
            if (x != s->widget_count - 1)
                s->widgets[x] = s->widgets[s->widget_count - 1];

            s->widget_count --;

            /* if this widget was an active widget (i.e., being pressed)
             * we must remove it there as well */
            for (int y=0; y<TMS_MAX_POINTER_ID; y++) {
                if (s->active_widgets[y] == w) {
                    s->active_widgets[y] = 0;
                }
            }

            w->surface = 0;
            return T_OK;
        }
    }

    /* XXX return error if not found? */
    tms_errorf("surface widget array has been tampered with :(");
    return T_OK;
}

/**
 * Add a widget to this surface
 *
 * @relates tms_surface
 **/
int
tms_surface_add_widget(struct tms_surface *s, struct tms_wdg *w)
{
    tms_assertf(w->surface == 0, "widget is already added to a surface");

    ARRAY_ENSURE_CAPACITY(s->widgets, (s->widget_count+1), s->widget_cap);

    if (!s->widgets)
        return T_OUT_OF_MEM;

    s->widgets[s->widget_count] = w;
    s->widget_count ++;

    w->surface = s;

    return T_OK;
}

