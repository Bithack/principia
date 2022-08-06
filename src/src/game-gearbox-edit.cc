#include "game.hh"
#include "gui.hh"
#include "gearbox.hh"

#define MAX_P 24

//static widget_decl btns[6];
static int num_buttons = 6;
static float base_x = 0;

static int dragging[MAX_P];
static tvec2 tdown[MAX_P];

#define PADDING (.1f*menu_xdim)

void
game::init_gearbox_edit()
{
    for (int x=0; x<MAX_P; x++) {
        dragging[x] = -1;
    }
#if 0
    btns[0].s = gui_spritesheet::s_slider_bg;
    btns[0].type = PANEL_SLIDER;
    btns[1].s = gui_spritesheet::s_left;
    btns[1].type = PANEL_LEFT;
    btns[2].s = gui_spritesheet::s_right;
    btns[2].type = PANEL_RIGHT;
    btns[3].s = gui_spritesheet::s_up;
    btns[3].type = PANEL_UP;
    btns[4].s = gui_spritesheet::s_down;
    btns[4].type = PANEL_DOWN;
    btns[5].s = gui_spritesheet::s_radial;
    btns[5].type = PANEL_RADIAL;

    float width = 0.f;
    for (int x=0; x<num_buttons;x++) {
        width += btns[x].s->width/128 * menu_xdim + PADDING;
        btns[x].sx = btns[x].s->width/64;
        btns[x].sy = btns[x].s->height/64;

        if (btns[x].sx > 2) btns[x].sx = 2;
        if (btns[x].sy > 2) btns[x].sy = 2;
    }

    base_x = _tms.window_width/2.f - width/2.f;

    for (int x=0; x<MAX_P; x++) {
        dragging[x] = -1;
    }
#endif
}

void
game::gearbox_edit_handle_event(tms::event *ev)
{
    int pid = ev->data.motion.pointer_id;
    tvec2 sp = (tvec2){ev->data.motion.x, ev->data.motion.y};
    tdown[pid] = sp;
    gearbox *gb = (gearbox*)this->selection.e;

    switch (ev->type) {
        case TMS_EV_POINTER_DOWN:
            //gb->properties[48].v.i8 ++;
            dragging[pid] = -1;
            if (sp.x > _tms.window_width - menu_xdim && sp.y > _tms.window_height - menu_ydim) {
                this->set_mode(GAME_MODE_DEFAULT);
                gb->active_conf = gb->properties[48].v.i8;
                tms_infof("close");
            } else if (fabsf(sp.y - (_tms.window_height / 2.f - 1.0f*menu_ydim)) < .5f*menu_ydim) {
                float x = sp.x - (_tms.window_width / 2.f - 2.25f * menu_xdim);
                x = roundf(x / (.5f*menu_xdim));
                int sx = (int)x;

                if (x >= 0 && x < gb->num_configs) {
                    gb->properties[48].v.i8 = x;
                }
            } else if (fabsf(sp.y - _tms.window_height/2.f) < menu_ydim/2.f) {
                if (sp.x > _tms.window_width/2.f - (3.f*.25f*menu_xdim)) {
                    tms_infof("clicked buttons");
                    float px = sp.x - (_tms.window_width/2.f - (2.5f*.25f*menu_xdim));
                    int found_n = (int)px / (int)(.25f*menu_xdim);

                    if (found_n <= 5) {
                        if (found_n == 5) found_n = 4;
                        if (found_n < 0) found_n = 0;
                        tms_infof("found n %d", found_n);

                        dragging[pid] = found_n;
                    }
                }
            } else {
                float a0_y = _tms.window_height / 2.f - 3.0f*menu_ydim;
                float a1_y = _tms.window_height / 2.f - 3.0f*menu_ydim + 3*.3f*menu_ydim;
                float a2_y = _tms.window_height / 2.f - 3.0f*menu_ydim - 3*.3f*menu_ydim;

                float a0_dist = fabsf(sp.y - a0_y);
                float a1_dist = fabsf(sp.y - a1_y);
                float a2_dist = fabsf(sp.y - a2_y);
                float dist;

                int axle;

                if (a0_dist < a1_dist) {
                    if (a2_dist < a0_dist) {
                        axle = 2; dist = a2_dist;
                    } else {
                        axle = 0; dist = a0_dist;
                    }
                } else {
                    if (a2_dist < a1_dist) {
                        axle = 2; dist = a2_dist;
                    } else {
                        axle = 1; dist = a1_dist;
                    }
                }

                if (dist > menu_ydim*.75f)
                    axle = -1;

                if (axle == 0) {
                    tms_infof("click axle 0");

                    int pos = get_gb_pos();
                    int slot = (int)roundf((sp.x - (_tms.window_width / 2.f + (16*.125f*menu_xdim) - pos * .125f*menu_xdim)) / (.125f*menu_xdim));
                    slot += 16;

                    int tr[3] = {0,-1, 1};
                    for (int t=0; t<3; t++) {
                        int sl = slot+tr[t];
                        if (sl >= 0 && sl < 16) {
                            if (gb->properties[sl].v.i8 != 0) {
                                dragging[pid] = gb->properties[sl].v.i8-1;
                                gb->properties[sl] .v.i8= 0;
                                gb->update_configurations();
                                break;
                            }
                        }
                    }
                } else if (axle == 1) {
                    int slot = (int)roundf((sp.x - (_tms.window_width / 2.f - (16*.125f*menu_xdim)/2.f)) / (.125f*menu_xdim));
                    slot += 7;

                    tms_infof("click axle 1, %d", slot);

                    int tr[3] = {0,-1, 1};
                    for (int t=0; t<3; t++) {
                        int sl = slot+tr[t];
                        if (sl >= 0 && sl < 16) {
                            if (gb->properties[16+sl] .v.i8!= 0) {
                                dragging[pid] = gb->properties[16+sl].v.i8-1;
                                gb->properties[16+sl].v.i8 = 0;
                                gb->update_configurations();
                                break;
                            }
                        }
                    }
                } else if (axle == 2) {
                    int slot = (int)roundf((sp.x - (_tms.window_width / 2.f - (16*.125f*menu_xdim)/2.f)) / (.125f*menu_xdim));
                    slot += 7;

                    tms_infof("click axle 2, %d", slot);

                    int tr[3] = {0,-1, 1};
                    for (int t=0; t<3; t++) {
                        int sl = slot+tr[t];
                        if (sl >= 0 && sl < 16) {
                            if (gb->properties[32+sl].v.i8 != 0) {
                                dragging[pid] = gb->properties[32+sl].v.i8-1;
                                gb->properties[32+sl].v.i8 = 0;
                                gb->update_configurations();
                                break;
                            }
                        }
                    }
                }
                /* see if we clicked an already added widget */
                /*
                panel *p = this->selection.e;
                for (int x=0; x<PANEL_MAX_WIDGETS; x++) {
                    panel::widget *w = &p->widgets[x];
                    if (w->used) {
                        if (sp.x > w->pos.x-w->size.x/2.f && sp.x < w->pos.x+w->size.x/2.f
                            && sp.y > w->pos.y-w->size.y/2.f && sp.y < w->pos.y+w->size.y/2.f) {
                            tms_infof("clicked widget");
                            int found_n = w->type;
                            if (!btns[w->type].dragging) {
                                p->remove_widget(x);
                                dragging[pid] = found_n;
                                btns[found_n].dragging = true;
                                btns[found_n].dx = sp.x;
                                btns[found_n].dy = sp.y;
                                break;
                            }
                        }
                    }
                }
                */
            }
            break;

        case TMS_EV_POINTER_DRAG:
            if (dragging[pid] != -1) {
                //btns[dragging[pid]].dx = sp.x;
                //btns[dragging[pid]].dy = sp.y;
            }
            break;

        case TMS_EV_POINTER_UP:
            if (dragging[pid] != -1) {

                float a0_y = _tms.window_height / 2.f - 3.0f*menu_ydim;
                float a1_y = _tms.window_height / 2.f - 3.0f*menu_ydim + 3*.3f*menu_ydim;
                float a2_y = _tms.window_height / 2.f - 3.0f*menu_ydim - 3*.3f*menu_ydim;

                float a0_dist = fabsf(sp.y - a0_y);
                float a1_dist = fabsf(sp.y - a1_y);
                float a2_dist = fabsf(sp.y - a2_y);
                float dist;

                int axle;

                if (a0_dist < a1_dist) {
                    if (a2_dist < a0_dist) {
                        axle = 2; dist = a2_dist;
                    } else {
                        axle = 0; dist = a0_dist;
                    }
                } else {
                    if (a2_dist < a1_dist) {
                        axle = 2; dist = a2_dist;
                    } else {
                        axle = 1; dist = a1_dist;
                    }
                }

                if (dist > menu_ydim*.75f)
                    axle = -1;

                if (axle == 0) {

                    int pos = get_gb_pos();
                    int slot = (int)roundf((sp.x - (_tms.window_width / 2.f + (16*.125f*menu_xdim) - pos * .125f*menu_xdim)) / (.125f*menu_xdim));
                    slot += 16;

                    if (slot >= 0 && slot < 16) {
                        gb->properties[slot].v.i8 = dragging[pid]+1;
                        gb->update_configurations();
                    }
                } else if (axle == 1) {
                    int slot = (int)roundf((sp.x - (_tms.window_width / 2.f - (16*.125f*menu_xdim)/2.f)) / (.125f*menu_xdim));
                    slot += 7;

                    if (slot >= 0 && slot < 16) {
                        gb->properties[16+slot].v.i8 = dragging[pid]+1;
                        gb->update_configurations();
                    }
                } else if (axle == 2) {
                    int slot = (int)roundf((sp.x - (_tms.window_width / 2.f - (16*.125f*menu_xdim)/2.f)) / (.125f*menu_xdim));
                    slot += 7;

                    if (slot >= 0 && slot < 16) {
                        gb->properties[32+slot].v.i8 = dragging[pid]+1;
                        gb->update_configurations();
                    }
                }

            }
            dragging[pid] = -1;
            break;
#if 0

        case TMS_EV_POINTER_UP:
            if (dragging[pid] != -1) {

                /* see if we placed the button over available slots */

                int id = dragging[pid];
                int z = 0;
                if (sp.x > _tms.window_width/2.f) {
                    z = 1;
                    sp.x -= _tms.window_width - 3*(menu_xdim);
                }

                float fx = .25f*menu_xdim;
                float fy = .25f*menu_ydim;

                /*
                if (((btns[id].sx & 1) == 0)) {
                    tms_errorf("JDKSAL");
                    exit(0);
                }
                */

                if ((btns[id].sx & 1) == 0) fx -= .5f*menu_xdim;
                if ((btns[id].sy & 1) == 0) fy -= .5f*menu_ydim;

                int sx = (int)floorf((sp.x+fx) / (menu_xdim));
                int sy = (int)floorf((sp.y+fy) / (menu_ydim));

                tms_infof("slot %d %d", sx, sy);

                if (sx >= 0 && sy >= 0) {
                    if (sx + btns[id].sx < 4 && sy + btns[id].sy < 4) {
                        panel *p = this->selection.e; 

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
                            tms_infof("OMG IT FITS HERE !!!!!!!!!!!!!!!!!!!!!!!");

                            p->add_widget(btns[id], sx,sy,z);
                        }
                    }
                }

                btns[dragging[pid]].dragging = false;
                dragging[pid] = -1;
            }
            break;
#endif
    }
}

int game::get_gb_pos()
{
    gearbox *gb = (gearbox*)this->selection.e;

    if (gb->num_configs) {
        if (gb->properties[48].v.i8 >= gb->num_configs)
            gb->properties[48].v.i8 = gb->num_configs-1;
        if (gb->properties[48].v.i8 < 0)
            gb->properties[48].v.i8 = 0;

        return gb->configs[gb->properties[48].v.i8].pos;
    } else
        return 0;
}

void
game::render_gearbox_edit(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gui_spritesheet::atlas->texture.gl_texture);
    tms_ddraw_set_color(this->get_surface()->ddraw, .9f, 1.f, .9f, .75f);

    gearbox *gb = (gearbox*)this->selection.e;

    /* draw the backgrounds middle area where alll the buttons are */
    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width / 2.f,
            _tms.window_height / 2.f,
            _tms.window_width,
            1.5f*menu_ydim);

    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, 1.f);
    for (int x=1; x<6; x++) {
        tms_ddraw_square(this->get_surface()->ddraw,
                _tms.window_width/2.f+x*.25f*menu_xdim - 3*.25f*menu_xdim,
                _tms.window_height / 2.f,
                .10f*menu_xdim,
                x * .3f*menu_ydim - .05f*menu_ydim
                );
    }

    /* render shit being dragged */
    for (int x=0; x<MAX_P; x++) {
        if (dragging[x] != -1) {
            tms_ddraw_square(this->get_surface()->ddraw,
                    tdown[x].x,
                    tdown[x].y,
                    .10f*menu_xdim,
                    (dragging[x]+1) * .3f*menu_ydim - .05f*menu_ydim
                    );

        }
    }

    tms_ddraw_set_color(this->get_surface()->ddraw, .0f, 0.f, .0f, .2f);

    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width / 2.f,
            _tms.window_height / 2.f - 3.0f*menu_ydim,
            4.2f * menu_xdim,
            2.8f * menu_ydim);
    
    tms_ddraw_set_color(this->get_surface()->ddraw, .0f, 0.f, .0f, .75f);

    /*
    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width / 2.f,
            _tms.window_height / 2.f - 1.5f*menu_ydim,
            4.f * menu_xdim,
            .5f * menu_ydim);

    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width / 2.f,
            _tms.window_height / 2.f - 4.5f*menu_ydim,
            4.f * menu_xdim,
            .5f * menu_ydim);
            */

    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width / 2.f - 2.25f * menu_xdim,
            _tms.window_height / 2.f - 3.0f*menu_ydim,
            .5f * menu_xdim,
            3.5f * menu_ydim);

    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width / 2.f + 2.25f * menu_xdim,
            _tms.window_height / 2.f - 3.0f*menu_ydim,
            .5f * menu_xdim,
            3.5f * menu_ydim);

    tms_ddraw_set_color(this->get_surface()->ddraw, 0.0f, 0.0f, 0.0f, 1.f);

    int pos = get_gb_pos();

    if (gb->num_configs) {
        if (gb->properties[48].v.i8 >= gb->num_configs)
            gb->properties[48].v.i8 = gb->num_configs-1;
        if (gb->properties[48].v.i8 < 0)
            gb->properties[48].v.i8 = 0;

        pos = gb->configs[gb->properties[48].v.i8].pos;
    } else
        pos = 0;

    /* g0 axle */
    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width / 2.f + (16*.125f*menu_xdim) - pos * .125f*menu_xdim,
            _tms.window_height / 2.f - 3.0f*menu_ydim,
            16*.125f*menu_xdim * 2.f,
            .125f*menu_ydim);

    /* g1 axle */
    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width / 2.f - (16*.125f*menu_xdim)/2.f,
            _tms.window_height / 2.f - 3.0f*menu_ydim + 3*.3f*menu_ydim,
            16*.125f*menu_xdim,
            .125f*menu_ydim);

    /* g2 axle */
    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width / 2.f - (16*.125f*menu_xdim)/2.f,
            _tms.window_height / 2.f - 3.0f*menu_ydim - 3*.3f*menu_ydim,
            16*.125f*menu_xdim,
            .125f*menu_ydim);

    /* draw END marker */
    tms_ddraw_set_color(this->get_surface()->ddraw, 0.0f, 0.0f, 0.0f, 1.f);
    {
        float xx = (16-pos)*.125f*menu_xdim;
        tms_ddraw_square(this->get_surface()->ddraw,
                _tms.window_width/2.f+xx,
                _tms.window_height / 2.f - 3.f*menu_ydim,
                .05f*menu_xdim,
                .25f*menu_ydim
                );
    }

    /* draw BEGIN marker */
    {
        float xx = (-pos-.25f)*.125f*menu_xdim;
        tms_ddraw_square(this->get_surface()->ddraw,
                _tms.window_width/2.f+xx,
                _tms.window_height / 2.f - 3.f*menu_ydim,
                .05f*menu_xdim,
                .25f*menu_ydim
                );
    }

    tms_ddraw_set_color(this->get_surface()->ddraw, 1.0f, 1.0f, 1.0f, .75f);

    for (int x=0; x<16; x++) {
        if (gb->properties[x].v.i8 > 0) {
            float xx = (x-pos)*.125f*menu_xdim;
            tms_ddraw_square(this->get_surface()->ddraw,
                    _tms.window_width/2.f+xx,
                    _tms.window_height / 2.f - 3.f*menu_ydim,
                    .10f*menu_xdim,
                    gb->properties[x].v.i8 * .3f*menu_ydim - .05f*menu_ydim
                    );
        }

        if (gb->properties[16+x].v.i8 > 0) {
            float xx = (x)*.125f*menu_xdim;
            tms_ddraw_square(this->get_surface()->ddraw,
                    _tms.window_width/2.f+xx - (15*.125f*menu_xdim),
                    _tms.window_height / 2.f - 3.f*menu_ydim + 3*.3f*menu_ydim,
                    .10f*menu_xdim,
                    gb->properties[16+x].v.i8 * .3f*menu_ydim - .05f*menu_ydim
                    );
        }

        if (gb->properties[32+x].v.i8 > 0) {
            float xx = (x)*.125f*menu_xdim;
            tms_ddraw_square(this->get_surface()->ddraw,
                    _tms.window_width/2.f+xx - (15*.125f*menu_xdim),
                    _tms.window_height / 2.f - 3.f*menu_ydim - 3*.3f*menu_ydim,
                    .10f*menu_xdim,
                    gb->properties[32+x].v.i8 * .3f*menu_ydim - .05f*menu_ydim
                    );
        }
    }

    /* draw error/block marker */
    tms_ddraw_set_color(this->get_surface()->ddraw, 1.0f, 0.0f, 0.0f, 1.f);
    if (gb->max_pos != 15) {
        float xx = ((-gb->max_pos))*.125f*menu_xdim;
        tms_ddraw_square(this->get_surface()->ddraw,
                _tms.window_width/2.f+xx,
                _tms.window_height / 2.f - 3.f*menu_ydim,
                .05f*menu_xdim,
                .25f*menu_ydim
                );

    }

    tms_ddraw_set_color(this->get_surface()->ddraw, 0.0f, 0.0f, 1.0f, 1.f);
    if (gb->min_pos != 0) {
        float xx = ((-gb->min_pos))*.125f*menu_xdim;
        tms_ddraw_square(this->get_surface()->ddraw,
                _tms.window_width/2.f+xx,
                _tms.window_height / 2.f - 3.f*menu_ydim,
                .05f*menu_xdim,
                .25f*menu_ydim
                );

    }

    /* draw available configs and mark currently selected one */
    for (int x=0; x<gb->num_configs; x++) {

        if (x == gb->properties[48].v.i8)
            tms_ddraw_set_color(this->get_surface()->ddraw, 1.0f, 1.0f, 1.0f, 1.f);
        else
            tms_ddraw_set_color(this->get_surface()->ddraw, 1.0f, 1.0f, 1.0f, 0.5f);

        tms_ddraw_sprite(this->get_surface()->ddraw,
                gui_spritesheet::get_sprite(S_CLOSE),
                _tms.window_width / 2.f - 2.25f * menu_xdim + x*.5f*menu_xdim,
                _tms.window_height / 2.f - 1.0f*menu_ydim,
                gui_spritesheet::get_sprite(S_CLOSE)->width/128.f*menu_xdim,
                gui_spritesheet::get_sprite(S_CLOSE)->height/128.f*menu_ydim);
    }

    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.0f, 1.f);
    /* close button */
    tms_ddraw_sprite(this->get_surface()->ddraw,
            gui_spritesheet::get_sprite(S_CLOSE),
            _tms.window_width - gui_spritesheet::get_sprite(S_CLOSE)->width/128.f*menu_xdim*.75f,
            _tms.window_height - gui_spritesheet::get_sprite(S_CLOSE)->height/128.f*menu_ydim*.75f,
            gui_spritesheet::get_sprite(S_CLOSE)->width/128.f*menu_xdim,
            gui_spritesheet::get_sprite(S_CLOSE)->height/128.f*menu_ydim);

#if 0

    /* draw a square for each slot */
    for (int z=0; z<2; z++) {
        float px = z * (_tms.window_width - 3*(menu_xdim));
        for (int y=0; y<3; y++) {
            for (int x=0; x<3; x++) {
                tms_ddraw_square(this->get_surface()->ddraw,
                        px+(menu_xdim)/2.f + x*menu_xdim,
                        (menu_ydim)/2.f + y*menu_ydim,
                        menu_xdim*.8f, menu_ydim*.8f);
            }
        }
    }

    tms_ddraw_set_color(this->get_surface()->ddraw, .0f, 0.f, .0f, 1.f);
    tms_ddraw_line(this->get_surface()->ddraw,
            0,
            _tms.window_height / 2.f - 0.75f*menu_ydim,
            _tms.window_width,
            _tms.window_height / 2.f - 0.75f*menu_ydim);

    tms_ddraw_line(this->get_surface()->ddraw,
            0,
            _tms.window_height / 2.f + 0.75f*menu_ydim,
            _tms.window_width,
            _tms.window_height / 2.f + 0.75f*menu_ydim);

    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.0f, 1.f);

    tms_ddraw_sprite(this->get_surface()->ddraw,
            gui_spritesheet::get_sprite(HELP_DRAGPANEL),
            _tms.window_width / 2.f,
            _tms.window_height / 2.f - menu_ydim,
            gui_spritesheet::get_sprite(HELP_DRAGPANEL)->width,
            gui_spritesheet::get_sprite(HELP_DRAGPANEL)->height
            );

    /* draw all available buttons */
    float xx = 0.f;
    for (int x=0; x<num_buttons; x++) {
        tms_ddraw_sprite(this->get_surface()->ddraw,
                btns[x].s,
                base_x + xx + (btns[x].s->width/128.f*menu_xdim)/2.f,
                _tms.window_height/2.f,
                btns[x].s->width/128.f*menu_xdim,
                btns[x].s->height/128.f*menu_ydim);

        xx+=btns[x].s->width/128.f*menu_xdim + PADDING;
    }

    /* render shit currently being dragged */
    for (int x=0; x<num_buttons; x++) {
        if (btns[x].dragging) {
            tms_ddraw_sprite(this->get_surface()->ddraw,
                    btns[x].s,
                    btns[x].dx,
                    btns[x].dy,
                    btns[x].s->width/128.f*menu_xdim,
                    btns[x].s->height/128.f*menu_ydim);
        }
    }

    /* render the widgets of the panel */
    panel *p = this->selection.e;
    for (int x=0; x<PANEL_MAX_WIDGETS; x++) {
        if (p->widgets[x].used) {
            tms_ddraw_sprite(this->get_surface()->ddraw,
                    btns[p->widgets[x].type].s,
                    p->widgets[x].pos.x,
                    p->widgets[x].pos.y,
                    btns[p->widgets[x].type].sx*menu_xdim*.75f,
                    btns[p->widgets[x].type].sy*menu_xdim*.75f);
//                    btns[p->widgets[x].type].s->width/128.f*menu_xdim,
                    //btns[p->widgets[x].type].s->height/128.f*menu_ydim);

            //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            tms_ddraw_sprite(this->get_surface()->ddraw,
                    gui_spritesheet::s_blk_out[x],
                    p->widgets[x].pos.x,
                    p->widgets[x].pos.y,
                    gui_spritesheet::s_blk_out[x]->width,
                    gui_spritesheet::s_blk_out[x]->height);
        }
    }
#endif
    glDisable(GL_BLEND);
}
