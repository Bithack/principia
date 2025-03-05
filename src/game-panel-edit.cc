#include "game.hh"
#include "gui.hh"
#include "panel.hh"
#include "ui.hh"
#include "widget_manager.hh"
#include "text.hh"

#define MAX_P 24

struct edit_widget_decl : public widget_decl {
    int ex; int ey;
    int rx; int ry;
};

static edit_widget_decl btns[NUM_PANEL_WIDGET_TYPES];
static float base_y = 0;
static float scroll_x = 0.f;
static float max_x = 0.f;

static int dragging[MAX_P];
static int modifying[MAX_P];

#define PADDING (.1f*menu_xdim)

static const int MAX_X = 17;
static const int MAX_Y = 3;

static const float WIDGET_SCALE = 1.0f;
static const float WIDGET_PADDING = 1.05f;

static void
on_drag_scroll(principia_wdg *w, float value_x, float value_y)
{
    scroll_x = 0.f + (-max_x+_tms.window_width) * value_x;
}

void
game::init_panel_edit()
{
    btns[0].s = gui_spritesheet::get_sprite(S_SLIDER_2);
    btns[0].type = PANEL_SLIDER;
    btns[0].ex = 0; btns[0].ey = 1;
    btns[0].rx = 2; btns[0].ry = 1;

    btns[1].s = gui_spritesheet::get_sprite(S_LEFT);
    btns[1].type = PANEL_LEFT;
    btns[1].ex = 2; btns[1].ey = 2;
    btns[1].rx = 1; btns[1].ry = 1;

    btns[2].s = gui_spritesheet::get_sprite(S_RIGHT);
    btns[2].type = PANEL_RIGHT;
    btns[2].ex = 3; btns[2].ey = 2;
    btns[2].rx = 1; btns[2].ry = 1;

    btns[3].s = gui_spritesheet::get_sprite(S_UP);
    btns[3].type = PANEL_UP;
    btns[3].ex = 0; btns[3].ey = 2;
    btns[3].rx = 1; btns[3].ry = 1;

    btns[4].s = gui_spritesheet::get_sprite(S_DOWN);
    btns[4].type = PANEL_DOWN;
    btns[4].ex = 1; btns[4].ey = 2;
    btns[4].rx = 1; btns[4].ry = 1;

    btns[5].s = gui_spritesheet::get_sprite(S_RADIAL_2);
    btns[5].type = PANEL_RADIAL;
    btns[5].ex = 3; btns[5].ey = 0;
    btns[5].rx = 2; btns[5].ry = 2;

    btns[6].s = gui_spritesheet::get_sprite(S_EMPTY);
    btns[6].type = PANEL_BTN;
    btns[6].ex = 4; btns[6].ey = 2;
    btns[6].rx = 1; btns[6].ry = 1;

    btns[7].s = gui_spritesheet::get_sprite(S_SLIDER_3);
    btns[7].type = PANEL_BIGSLIDER;
    btns[7].ex = 0; btns[7].ey = 0;
    btns[7].rx = 3; btns[7].ry = 1;

    btns[8].s = gui_spritesheet::get_sprite(S_SLIDER_2);
    btns[8].type = PANEL_VSLIDER;
    btns[8].ex = 6; btns[8].ey = 0;
    btns[8].rx = 1; btns[8].ry = 2;

    btns[9].s = gui_spritesheet::get_sprite(S_SLIDER_3);
    btns[9].type = PANEL_VBIGSLIDER;
    btns[9].ex = 5; btns[9].ey = 0;
    btns[9].rx = 1; btns[9].ry = 3;

    btns[10].s = gui_spritesheet::get_sprite(S_RADIAL_3);
    btns[10].type = PANEL_BIGRADIAL;
    btns[10].ex = 7; btns[10].ey = 0;
    btns[10].rx = 3; btns[10].ry = 3;

    btns[11].s = gui_spritesheet::get_sprite(S_FIELD_2);
    btns[11].type = PANEL_FIELD;
    btns[11].ex = 13;
    btns[11].ey = 0;
    btns[11].rx = 2;
    btns[11].ry = 2;

    btns[12].s = gui_spritesheet::get_sprite(S_FIELD_3);
    btns[12].type = PANEL_BIGFIELD;
    btns[12].ex = 10;
    btns[12].ey = 0;
    btns[12].rx = 3;
    btns[12].ry = 3;

    btns[PANEL_BTN_RADIAL].type = PANEL_BTN_RADIAL;
    btns[PANEL_BTN_RADIAL].s  = gui_spritesheet::get_sprite(S_RADIAL_2);
    btns[PANEL_BTN_RADIAL].ex = 15;
    btns[PANEL_BTN_RADIAL].ey = 0;
    btns[PANEL_BTN_RADIAL].rx = 2;
    btns[PANEL_BTN_RADIAL].ry = 2;

    for (int x=0; x<NUM_PANEL_WIDGET_TYPES;x++) {
        btns[x].sx = btns[x].rx;
        btns[x].sy = btns[x].ry;
    }

    base_y = _tms.window_height;// - PANEL_WDG_OUTER_Y / 2.f;
    base_y = _tms.window_height - (MAX_Y * b_h * (1.f - WIDGET_SCALE))/2.f;

    for (int x=0; x<MAX_P; x++) {
        dragging[x] = -1;
        modifying[x] = -1;
    }

    const float WIDGET_SIZE_MOD_X = (0.10f * _tms.xppcm) * WIDGET_SCALE;
    const float WIDGET_SIZE_MOD_Y = (0.10f * _tms.yppcm) * WIDGET_SCALE;

    for (int n=0; n<NUM_PANEL_WIDGET_TYPES; ++n) {
        float w = (btns[n].rx * b_w * WIDGET_SCALE) - WIDGET_SIZE_MOD_X;
        float h = (btns[n].ry * b_h * WIDGET_SCALE) - WIDGET_SIZE_MOD_Y;

        float x = scroll_x + (btns[n].ex * b_w * WIDGET_SCALE) + (btns[n].rx * b_w * WIDGET_SCALE) / 2.f;
        float y = base_y - (btns[n].ey * b_h * WIDGET_SCALE) - (btns[n].ry * b_h * WIDGET_SCALE) / 2.f;

        if (btns[n].ry > btns[n].rx) {
            float temp = w;
            w = h;
            h = temp;
        }

        if (x+w/2.f > max_x) {
            max_x = x+w/2.f;
        }
    }

    this->panel_edit_need_scroll = (_tms.window_width < max_x);

    if (this->panel_edit_need_scroll) {
        // how much can we scroll?
        // this will decide the size of the knob
        const float ow = 10.f;
        float width = (_tms.window_width * (_tms.window_width / max_x));
        float height = (_tms.yppcm * .15f);
        this->wdg_panel_slider_knob->size.w = width;
        this->wdg_panel_slider_knob->size.h = height;
        this->wdg_panel_slider_knob->set_position(width/2.f, _tms.window_height - 1.5f*menu_ydim - (_tms.yppcm * .075f));
        this->wdg_panel_slider_knob->set_draggable_x(true, true, width/2.f, _tms.window_width-(width/2.f));
        this->wdg_panel_slider_knob->on_dragged = on_drag_scroll;
    } else {
        /* If we don't need any scrolling support, we can programmatically scroll
         * enough so the widgets are horizontally centered */
        scroll_x = (_tms.window_width - max_x)/2.f;
    }

    this->help_dragpanel = new p_text(font::small);
    this->help_dragpanel->set_text("Add widgets by dragging them down to open slots. (no/no)");
    this->help_dragpanel->set_position(_tms.window_width/2.f, _tms.window_height - 1.5f*menu_ydim / 2.f - (menu_ydim*0.95f));
}

int
game::panel_edit_handle_event(tms::event *ev)
{
    int pid = ev->data.motion.pointer_id;
    tvec2 sp = (tvec2){ev->data.motion.x, ev->data.motion.y};

    if (ev->type == TMS_EV_POINTER_MOVE || ev->type == TMS_EV_POINTER_DRAG) {
        move_pos = (tvec2){ev->data.motion.x, ev->data.motion.y};
    }

    switch (ev->type) {
        case TMS_EV_KEY_PRESS:
            switch (ev->data.key.keycode) {
                case TMS_KEY_ESC:
                case TMS_KEY_B:
                case SDL_SCANCODE_AC_BACK:
                    this->set_mode(GAME_MODE_DEFAULT);
                    return EVENT_DONE;

                default:
                    return EVENT_CONT;
            }
            break;

        case TMS_EV_POINTER_SCROLL:
            if (this->panel_edit_need_scroll) {
                if (fabsf(move_pos.y - (_tms.window_height - 1.5f*menu_ydim / 2.f)) < menu_ydim) {
                    float z = ev->data.scroll.y < 0 ? _tms.xppcm * -0.2f : _tms.xppcm * 0.2f;
                    scroll_x += z;

                    if (scroll_x > 0.f) {
                        scroll_x = 0.f;
                    } else if (scroll_x < -max_x+_tms.window_width) {
                        scroll_x = -max_x+_tms.window_width;
                    }
                }
            }
            break;

        case TMS_EV_POINTER_DOWN:
            if (fabsf(sp.y - (_tms.window_height - 1.5f*menu_ydim / 2.f)) < menu_ydim) {
                int sx = (int)floorf((roundf(sp.x - scroll_x) - 4.f) / b_w);
                int sy = (int)floorf(roundf(base_y - sp.y) / b_h);

                if (sx >= 0 && sy >= 0) {
                    for (int x=0; x<NUM_PANEL_WIDGET_TYPES; ++x) {
                        if (sx >= btns[x].ex && sx <= btns[x].ex+btns[x].rx-1 &&
                                sy >= btns[x].ey && sy <= btns[x].ey+btns[x].ry-1) {
                            tms_debugf("Clicked btn %d", btns[x].type);
                            dragging[pid] = x;
                            btns[x].dragging = true;
                            btns[x].dx = sp.x;
                            btns[x].dy = sp.y;
                            break;
                        }
                    }
                }
            } else {
                /* see if we clicked an already added widget */
                panel *p = (panel*)this->selection.e;
                for (int x=0; x<p->num_widgets; x++) {
                    panel::widget *w = &p->widgets[x];
                    if (w->used) {
                        float size_x = (w->size.x/2.f) * WIDGET_PADDING;
                        float size_y = (w->size.y/2.f) * WIDGET_PADDING;
                        if (sp.x > w->pos.x-size_x && sp.x < w->pos.x+size_x
                            && sp.y > w->pos.y-size_y && sp.y < w->pos.y+size_y) {
                            modifying[pid] = x;
                        }
                    }
                }
            }
            break;

        case TMS_EV_POINTER_DRAG:
            if (modifying[pid] != -1) {
                panel *p = (panel*)this->selection.e;
                panel::widget *w = &p->widgets[modifying[pid]];

                if (fabsf(sp.x - w->pos.x) > PANEL_WDG_OUTER_X*1.5f
                        ||fabsf(sp.y - w->pos.y) > PANEL_WDG_OUTER_Y*1.5f) {
                    if (!btns[w->wtype].dragging) {
                        p->remove_widget(modifying[pid]);
                        dragging[pid] = w->wtype;
                        btns[w->wtype].dragging = true;
                        btns[w->wtype].dx = sp.x;
                        btns[w->wtype].dy = sp.y;

                        this->panel_edit_refresh();
                        break;
                    }
                    modifying[pid] = -1;
                } else {
                    float rx = (float)(sp.x-w->pos.x) / ((float)w->size.x/2.f);
                    if (rx < -1.f) rx = -1.f;
                    if (rx > 1.f) rx = 1.f;
                    float ry = (float)(sp.y-w->pos.y) / ((float)w->size.y/2.f);
                    if (ry < -1.f) ry = -1.f;
                    if (ry > 1.f) ry = 1.f;
                    float value = 0.f;
                    float value2 = 0.f;

                    switch (w->wtype) {
                        case PANEL_SLIDER:
                        case PANEL_BIGSLIDER:
                            {
                                value = (rx + 1.f) / 2.f;
                                G->show_numfeed(value);
                            }
                            break;

                        case PANEL_VSLIDER:
                        case PANEL_VBIGSLIDER:
                            {
                                value = (ry + 1.f) / 2.f;
                                G->show_numfeed(value);
                            }
                            break;

                        case PANEL_FIELD:
                        case PANEL_BIGFIELD:
                            {
                                value = (rx + 1.f) / 2.f;
                                value2 = (ry + 1.f) / 2.f;
                            }
                            break;

                        case PANEL_RADIAL:
                        case PANEL_BIGRADIAL:
                            {
                                tvec2 d = (tvec2){rx, ry};
                                tvec2_normalize(&d);

                                value = atan2f(d.y, d.x);

                                if (value < 0.f)
                                    value += 2.f*M_PI;

                                value /= M_PI/90.f;
                                value = roundf(value);
                                value *= M_PI/90.f;

                                G->show_numfeed(value * (180.f/M_PI));

                                value /= 2.f*M_PI;
                            }
                            break;
                    }

                    w->default_value[0] = value;
                    w->default_value[1] = value2;
                }
            }
            if (dragging[pid] != -1) {
                btns[dragging[pid]].dx = sp.x;
                btns[dragging[pid]].dy = sp.y;
            }

            break;

        case TMS_EV_POINTER_UP:
            if (modifying[pid] != -1) {
                modifying[pid] = -1;
            }
            if (dragging[pid] != -1) {

                /* see if we placed the button over available slots */

                int id = dragging[pid];
                int z = 0;
                if (sp.x > _tms.window_width/2.f) {
                    z = 1;
                    sp.x -= _tms.window_width - 3*PANEL_WDG_OUTER_X;
                }

                float fx = .25f*PANEL_WDG_OUTER_X;
                float fy = .25f*PANEL_WDG_OUTER_Y;

                /*
                if (((btns[id].sx & 1) == 0)) {
                    tms_errorf("JDKSAL");
                    exit(0);
                }
                */

                if ((btns[id].sx & 1) == 0) fx -= .5f*PANEL_WDG_OUTER_X;
                if ((btns[id].sy & 1) == 0) fy -= .5f*PANEL_WDG_OUTER_X;

                int sx = (int)floorf((sp.x+fx) / (PANEL_WDG_OUTER_X));
                int sy = (int)floorf((sp.y+fy) / (PANEL_WDG_OUTER_Y));

                tms_infof("slot %d %d", sx, sy);

                if (id == PANEL_BIGSLIDER && sx > 0 && sx < 3) {
                    /* if we're handling a big sider (3x1), always default x grid coordinate
                     * to 0 to simplify placing it */
                    sx = 0;
                } else if (id == PANEL_VBIGSLIDER && sy > 0 && sy < 3) {
                    /* same as above comment, except y and vertical big slider */
                    sy = 0;
                } else if ((id == PANEL_BIGRADIAL || id == PANEL_BIGFIELD) && sy > 0 && sx > 0 && sy < 3 && sx < 3) {
                    /* same as above comment, except x AND y for big radial */
                    sx = 0;
                    sy = 0;
                }

                if (sx >= 0 && sy >= 0) {
                    if (sx + btns[id].sx < 4 && sy + btns[id].sy < 4) {
                        panel *p = (panel*)this->selection.e;

                        bool used = false;
                        int num_skipped = 0;
                        for (int y=0; y<btns[id].sy; y++) {
                            for (int x=0; x<btns[id].sx; x++) {
                                if (p->slot_used(sx+x, sy+y, z)) {

                                    if (num_skipped == 0 && id == PANEL_RADIAL) {
                                        /* special case for the radial, one of the slots are
                                         * allowed to be used if the other is a radial too */
                                        if (p->slot_owned_by_radial(sx+x, sy+y, z)) {
                                            num_skipped ++;
                                            continue;
                                        }
                                    }
                                    used = true;
                                    break;
                                }
                            }
                        }

                        if (!used) {
                            /* FITS HERE */
                            tms_debugf("Placing widget(%d) in slot %d/%d", id, sx, sy);

                            int r = p->add_widget(btns[id], sx,sy,z);

                            if (r == PANEL_NO_ROOM) {
                                ui::messagef("This RC has reached its maximum number of widgets (%d).", p->num_widgets);
                            }

                            this->panel_edit_refresh();
                        }
                    }
                }

                btns[dragging[pid]].dragging = false;
                dragging[pid] = -1;
            }
            break;
    }

    return EVENT_DONE;
}

void
game::render_panel_edit(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gui_spritesheet::atlas->texture.gl_texture);
    tms_ddraw_set_color(this->get_surface()->ddraw, .0f, 0.f, .0f, .75f);
    tms_ddraw_square(this->get_surface()->ddraw, _tms.window_width/2.f, _tms.window_height/2.f,
            _tms.window_width, _tms.window_height);

    tms_ddraw_set_color(this->get_surface()->ddraw, .9f, 1.f, .9f, .75f);

    /* draw the backgrounds middle area where all the buttons are */
    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width / 2.f,
            _tms.window_height - 1.5f*menu_ydim / 2.f,
            _tms.window_width,
            1.5f*menu_ydim);

    /* drag slider BG, if needed */
    if (this->panel_edit_need_scroll) {
        tms_ddraw_set_color(this->get_surface()->ddraw, .2f, .2f, .2f, .75f);
        tms_ddraw_square(this->get_surface()->ddraw,
                _tms.window_width / 2.f,
                _tms.window_height - 1.5f*menu_ydim - (_tms.yppcm * .075f),
                _tms.window_width,
                _tms.yppcm * 0.15f);

        tms_ddraw_set_color(this->get_surface()->ddraw, .9f, 1.f, .9f, .75f);
    }


    /* draw a square for each slot */
    for (int z=0; z<2; z++) {
        float px = z * (_tms.window_width - 3*(PANEL_WDG_OUTER_X));
        for (int y=0; y<3; y++) {
            for (int x=0; x<3; x++) {
                tms_ddraw_square(this->get_surface()->ddraw,
                        px+(PANEL_WDG_OUTER_X)/2.f + x*PANEL_WDG_OUTER_X,
                        (PANEL_WDG_OUTER_Y)/2.f + y*PANEL_WDG_OUTER_Y,
                        PANEL_WDG_OUTER_X*.9f, PANEL_WDG_OUTER_Y*.9f);
            }
        }
    }

    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.0f, 1.f);

    this->add_text(help_dragpanel);

    const float WIDGET_SIZE_MOD_X = (0.10f * _tms.xppcm) * WIDGET_SCALE;
    const float WIDGET_SIZE_MOD_Y = (0.10f * _tms.yppcm) * WIDGET_SCALE;

    /* draw all available buttons */
    for (int n=0; n<NUM_PANEL_WIDGET_TYPES; ++n) {
        float w = (btns[n].rx * b_w * WIDGET_SCALE) - WIDGET_SIZE_MOD_X;
        float h = (btns[n].ry * b_h * WIDGET_SCALE) - WIDGET_SIZE_MOD_Y;

        float x = scroll_x + (btns[n].ex * b_w * WIDGET_SCALE) + (btns[n].rx * b_w * WIDGET_SCALE) / 2.f;
        float y = base_y - (btns[n].ey * b_h * WIDGET_SCALE) - (btns[n].ry * b_h * WIDGET_SCALE) / 2.f;

        float r = 0.f;

        if (btns[n].ry > btns[n].rx) {
            float temp = w;
            w = h;
            h = temp;
            r = -90.f;
        }

        tms_ddraw_sprite_r(this->get_surface()->ddraw,
                btns[n].s,
                x, y,
                w, h,
                r);
    }

    /* render shit currently being dragged */
    for (int x=0; x<NUM_PANEL_WIDGET_TYPES; x++) {
        if (btns[x].dragging) {
            float w = btns[x].s->width/128.f*PANEL_WDG_OUTER_X;
            float h = btns[x].s->height/128.f*PANEL_WDG_OUTER_Y;
            float r = 0.f;

            if (btns[x].ry > btns[x].rx) {
                r = -90.f;
            }

            tms_ddraw_sprite_r(this->get_surface()->ddraw,
                    btns[x].s,
                    btns[x].dx,
                    btns[x].dy,
                    w, h,
                    r);
        }
    }

    /* render the widgets of the panel */
    panel *p = (panel*)this->selection.e;
    for (int x=0; x<p->num_widgets; x++) {
        if (p->widgets[x].used) {
            p->widgets[x].value[0] = p->widgets[x].default_value[0];
            p->widgets[x].value[1] = p->widgets[x].default_value[1];
            p->widgets[x].render((struct tms_wdg*)&p->widgets[x], this->get_surface());
        }
    }

    for (int x=0; x<p->num_widgets; ++x) {
        if (p->widgets[x].used) {
            if (p->widgets[x].num_outputs == 1) {
                this->add_text(gui_spritesheet::t_out[x],
                        p->widgets[x].pos.x,
                        p->widgets[x].pos.y+(_tms.yppcm/4.f),
                        true, false
                        );
            } else {
                int i=0;
                for (int n=0; n<PANEL_MAX_WIDGETS; ++n) {
                    if (p->widgets[x].outputs & (1ULL << (n+1))) {
                        this->add_text(gui_spritesheet::t_out[n],
                                p->widgets[x].pos.x,
                                p->widgets[x].pos.y+(_tms.yppcm/4.f) - ((_tms.yppcm/4.f) * i++),
                                true, false
                                );
                    }
                }
            }
        }
    }

    glDisable(GL_BLEND);
}

void
game::panel_edit_refresh()
{
    char tmp[256];
    int wdg_in_use = -1;
    int num_wdg = -1;

    if (this->selection.e && this->selection.e->is_control_panel()) {
        wdg_in_use = 0;
        panel *p = static_cast<panel*>(this->selection.e);
        for (int x=0; x<p->num_widgets; ++x) {
            if (p->widgets[x].used) {
                wdg_in_use += p->widgets[x].num_outputs;
            }
        }

        num_wdg = p->num_widgets;
    }

    snprintf(tmp, 255, "Add widgets by dragging them down to open slots. (%d/%d)", wdg_in_use, num_wdg);
    this->help_dragpanel->set_text(tmp);
}
