#include <vector>
#include <algorithm>

#include "game.hh"
#include "game-message.hh"
#include "settings.hh"
#include "robot.hh"
#include "object_factory.hh"
#include "i1o1gate.hh"
#include "ui.hh"
#include "adventure.hh"
#include "motor.hh"
#include "rubberband.hh"
#include "damper.hh"
#include "pivot.hh"
#include "screenshot_marker.hh"
#include "cable.hh"
#include "prompt.hh"
#include "factory.hh"
#include "item.hh"
#include "widget_manager.hh"
#include "font.hh"
#include "text.hh"
#include "gui.hh"
#if defined(TMS_BACKEND_LINUX) && defined(DEBUG)
#define CREATE_SANDBOX_TEXTURES
#include "savepng.h"
#endif

#ifndef UINT32_MAX
#define UINT32_MAX  (4294967295U)
#endif

#define MAX_P 24
#define CAM_Z 20
#define SPACING 2.f
#define CAM_DAMPING .02f

#define C_OFFS 10
#define C_PADDING 40

#define SIZE_PER_OBJECT_ITEM 64
#define SIZE_PER_MENU_ITEM   100

static void
forward_slider_on_change(struct tms_wdg *w, float values[2])
{
    entity *e = G->selection.e;
    int slider_id = VOID_TO_INT(w->data3);

    if (e) {
        e->get_property_entity()->on_slider_change(slider_id, values[0]);
    }
}

static float cooldown_time = 0.f;
static bool initialized = false;
static int _menu_width = 200;
static int menu_min_width;
static int menu_max_width;
static int real_menu_width = 200;
static float menu_scale = 1.f;
static int curr_category = 0;
static bool category_selector_open = false;
static float category_selector_alpha = 0.f;
static int category_selector_x = 0;
static float menu_highlight = 0.f;
static int menu_height = 0;
static float menu_cam_vel = 0.f;
static int menu_cam_target = -1;
static tms::graph *menu_graph = 0;
static tms::camera *menu_cam = 0;
static float menu_depth = 0.f;
static int menu_cat_width = 0;
static float s_size = 0;

int b_w = 0;
int b_h = 0;
int b_w_pad = 0;
int b_h_pad = 0;
int b_y = 0;
int b_margin_y = 0;
int b_margin_x = 0;

static entity *pending_ent[MAX_P];
static bool pending_a_scene[MAX_P];
static tvec2 tdown_p[MAX_P];
static int tdown[MAX_P];
static bool sliding_menu[MAX_P];
static bool resizing_menu[MAX_P];
static bool dragging[MAX_P];

float menu_xdim, menu_ydim;

static tms_sprite *betasprite;
static tms_sprite *devsprite;
tms_sprite *catsprites[of::num_categories+1];
tms_sprite *catsprite_hints[of::num_categories];
tms_sprite *filterlabel;
tms_sprite *factionlabel;
static int widest_catsprite = 0;
static int tallest_catsprite = 0;

float cat_pos[of::num_categories];

std::vector<struct menu_obj> menu_objects;
std::vector<int> menu_objects_cat[of::num_categories];

int gid_to_menu_pos[256];

float
game::get_bmenu_x()
{
    return b_w_pad / 2.f + b_margin_x;
}

float
game::get_bmenu_y()
{
    return b_y + b_margin_y;
}

float
game::get_bmenu_pad()
{
    return b_w_pad;
}

void
game::reset_touch_gui()
{
    for (int x=0; x<MAX_P; x++) {
        tdown[x] = 0;
        resizing_menu[x] = 0;
        sliding_menu[x] = 0;
        dragging[x] = 0;
    }
}

void
game::add_menu_item(int cat, entity *e)
{
    if (!e) {
        return;
    }

    static struct tms_sprite sss;
    sss.tr=(tvec2){0.f,0.f};
    sss.bl=(tvec2){0.f,0.f};
    sss.width=0.f;
    sss.height=0.f;

    int ierr;
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::add_menu_item (g_id: %d) 1", ierr, e->g_id);

    //cat = 0;

    struct menu_obj o;
    o.highlighted = false;
    o.category = cat;
    o.e = e;
    o.pos = menu_objects.size();

    gid_to_menu_pos[o.e->g_id] = o.pos;

    o.name = this->text_small->add_to_atlas(this->texts, e->get_name());

    if (!o.name) {
        o.name = &sss;
    } else {
        o.name->width *= gui_spritesheet::text_factor;
        o.name->height *= gui_spritesheet::text_factor;
    }

    menu_objects.push_back(o);
    menu_objects_cat[cat].push_back(o.pos);

    //e->_pos = b2Vec2(0.f, -o.pos * 2.f);
    e->_pos = e->menu_pos;

    if (e->g_id == O_SIGNAL_CABLE || e->g_id == O_POWER_CABLE || e->g_id == O_INTERFACE_CABLE) {
        e->set_position(e->menu_pos);
    }
    e->_angle = 0.0f;

    game::update_ghost_entity(e);
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::add_menu_item (g_id: %d) 5", ierr, e->g_id);
    //tms_entity_update_with_children(static_cast<tms_entity*>(e));
}

static void
conndamping_on_change(struct tms_wdg *w, float values[2])
{
    float value = values[0];
    connection *c = static_cast<connection*>(w->data3);
    c->damping = value * CONN_DAMPING_MAX;

    G->show_numfeed(c->damping);
}

static void
connstrength_on_change(struct tms_wdg *w, float values[2])
{
    float value = values[0];
    connection *c = static_cast<connection*>(w->data3);

    connection *nc = G->set_connection_strength(c, value);

    if (nc) {
        G->selection.c = nc;
        w->data3 = nc;
    } else {
        G->selection.disable();
        tms_errorf("could not apply connection");
    }
}

int
game::delete_entity(entity *e)
{
    do {
        if (e == adventure::player) {
            adventure::player = 0;
        }
        /**
         * For these special cases, we need to figure out
         * which part of the objects we're dealing with.
         * After that, we disconnect them from eachother and
         * remove the part of the group that the user does not
         * have selected. After that, we can move on to the regular
         * object deletion code.
         **/
        if (e->g_id == O_OPEN_PIVOT || e->g_id == O_OPEN_PIVOT_2) { /* Pivot */
            pivot_1 *p1 = 0;
            pivot_2 *p2 = 0;
            entity *other = 0;

            if (e->g_id == O_OPEN_PIVOT) {
                p1 = static_cast<pivot_1*>(e);
                other = p1->dconn.o;
                if (other && other != p1)
                    p2 = static_cast<pivot_2*>(other);
            } else {
                p2 = static_cast<pivot_2*>(e);
                other = p2->get_property_entity();
                if (other && other != p2)
                    p1 = static_cast<pivot_1*>(other);
            }

            if (!p1 || !p2) {
                tms_errorf("Unable to fetch all parts of the Pivot");
                break;
            }

            if (p1->dconn.o) {
                p1->dconn.destroy_joint();
            }
            W->erase_connection(&p1->dconn);

            p1->disconnect_all();
            p2->disconnect_all();

            W->remove(p1);
            this->remove_entity(p1);
            delete p1;

            W->remove(p2);
            this->remove_entity(p2);
            delete p2;

            return 3;
        } else if (e->g_id == O_DAMPER || e->g_id == O_DAMPER_2) { /* Damper */
            damper_1 *d1 = 0;
            damper_2 *d2 = 0;
            entity *other = 0;

            if (e->g_id == O_DAMPER) {
                d1 = static_cast<damper_1*>(e);
                other = d1->dconn.o;
                if (other && other != d1)
                    d2 = static_cast<damper_2*>(other);
            } else {
                d2 = static_cast<damper_2*>(e);
                other = d2->get_property_entity();
                if (other && other != d2)
                    d1 = static_cast<damper_1*>(other);
            }

            if (!d1 || !d2) {
                tms_errorf("Unable to fetch all parts of the Damper");
                break;
            }

            if (d1->dconn.o) {
                d1->dconn.destroy_joint();
            }
            W->erase_connection(&d1->dconn);

            d1->disconnect_all();
            d2->disconnect_all();

            W->remove(d1);
            this->remove_entity(d1);
            delete d1;

            W->remove(d2);
            this->remove_entity(d2);
            delete d2;

            return 3;
        } else if (e->g_id == O_RUBBERBAND || e->g_id == O_RUBBERBAND_2) { /* Rubberband */
            rubberband_1 *r1 = 0;
            rubberband_2 *r2 = 0;
            entity *other = 0;

            if (e->g_id == O_RUBBERBAND) {
                r1 = static_cast<rubberband_1*>(e);
                other = r1->dconn.o;
                if (other && other != r1)
                    r2 = static_cast<rubberband_2*>(other);
            } else {
                r2 = static_cast<rubberband_2*>(e);
                other = r2->get_property_entity();
                if (other && other != r2)
                    r1 = static_cast<rubberband_1*>(other);
            }

            if (!r1 || !r2) {
                tms_errorf("Unable to fetch all parts of the Rubberband");
                break;
            }

            if (r1->dconn.o)
                r1->dconn.destroy_joint();
            W->erase_connection(&r1->dconn);

            r1->disconnect_all();
            r2->disconnect_all();

            W->remove(r1);
            this->remove_entity(r1);
            delete r1;

            W->remove(r2);
            this->remove_entity(r2);
            delete r2;

            return 3;
        }
    } while (0);

    tms_infof("Disconnect all called on %s", e->get_name());
    e->disconnect_all();

    if (e->flag_active(ENTITY_IS_PLUG)) {
        ((plug_base*)e)->disconnect();
    }

    if (e->flag_active(ENTITY_IS_EDEVICE)) {
        edevice *ed = e->get_edevice();

        for (int x=0; x<ed->num_s_in; x++) {
            ed->s_in[x].unplug();
        }
        for (int x=0; x<ed->num_s_out; x++) {
            ed->s_out[x].unplug();
        }

        if (e->get_edevice() == G->ss_edev) {
            /* XXX: Does anything else need to be disabled/reset? */
            this->ss_edev = 0;
            this->set_mode(GAME_MODE_DEFAULT);
        }
    }

    if (e->type == ENTITY_CABLE) {
        cable *c = static_cast<cable*>(e);
        if (c->p[0] == this->ss_plug || c->p[1] == this->ss_plug) {
            tms_infof("Resetting ss_plug");
            this->ss_plug = 0;
            this->set_mode(GAME_MODE_DEFAULT);
        }
    } else {
        if (e == this->ss_plug) {
            tms_infof("Resetting ss_plug");
            this->ss_plug = 0;
            this->set_mode(GAME_MODE_DEFAULT);
        }
    }

    W->remove(e);
    this->remove_entity(e);
    delete e;

    return 1;
}

/**
 * This function should only be called due to user input.
 **/
int
game::delete_selected_entity(bool multi/*=false*/)
{
    if (this->get_mode() == GAME_MODE_EDIT_PANEL) return 0;

    if (this->selection.e && this->selection.e->get_property_entity()) {
        entity *e = this->selection.e;
        if (e->flag_active(ENTITY_IS_OWNED)) e = (entity*)e->parent;

        this->selection.disable();

        return this->delete_entity(e);

        this->selection.disable(); /* cool */

        this->sel_p_ent = 0;
        this->sel_p_body = 0;
    } else if (this->selection.c) {
        /* connection selected, remove the connection */
        connection *c = this->selection.c;

        this->selection.disable();
        c->e->destroy_connection(c);
    }

    return 1;
}

void
game::config_btn_pressed(entity *e)
{
    if (!e) return;

    if (e->flag_active(ENTITY_IS_CONTROL_PANEL)) {
        this->set_mode(GAME_MODE_EDIT_PANEL);
    } else if (e->is_gearbox()) {
        this->set_mode(GAME_MODE_EDIT_GEARBOX);
    }/* else if (e->flag_active(ENTITY_IS_TOOL)) {
        switch (((robot_parts::tool*)e)->tool_id) {
            case TOOL_PAINTER: ui::open_dialog(DIALOG_BEAM_COLOR); break;
            default: tms_debugf("Unhandled entity config button."); break;
        }
    }*/ else {
        switch (e->g_id) {
            case O_PROMPT:
                G->current_prompt = static_cast<prompt*>(e);
                break;

            case O_CAM_MARKER:
                {
                    /* snap to camera view */
                    screenshot_marker *sm = static_cast<screenshot_marker*>(e);
                    this->snap_to_camera(sm);
                    return;
                }
                break;

            case O_FACTORY:
            case O_ROBOT_FACTORY:
            case O_ARMORY:
            case O_OIL_MIXER:
                if (W->is_paused()) {
                    ui::open_dialog(DIALOG_FACTORY);
                } else {
                    tms_warnf("Unhandled play-config button for factory.");
                }
                break;
        }

        if (e->dialog_id != DIALOG_IGNORE) {
            if (e->dialog_id == DIALOG_POLYGON) {
                /* if a corner is selected, open a color configuration dialog for that corner */
                int corner = this->get_selected_shape_corner();

                if (corner != -1) {
                    ui::open_dialog(DIALOG_POLYGON_COLOR);
                    return;
                }
            }

            ui::open_dialog(e->dialog_id);
        } else {
            tms_warnf("Unhandled config button for '%s'[%d]", e->get_name(), e->g_id);
        }
    }
}

void
game::info_btn_pressed(entity *e)
{
    if (!e) return;

    e = e->get_property_entity();

    tms_debugf("Opening help dialog for %d:%s", e->g_id, e->get_name());

    char wikiurl[256];

    if (e->is_item()) {
        item *i = static_cast<item*>(e);
        snprintf(wikiurl, 255, "https://principia-web.se/wiki/Special:GotoItem?id=%d", i->get_item_type());
    } else
        snprintf(wikiurl, 255, "https://principia-web.se/wiki/Special:GotoObject?id=%d", e->g_id);

    ui::open_url(wikiurl);
}

void
game::toggle_entity_lock(entity *e)
{
    if (!e) return;

    e->set_locked(!e->is_locked());

    if (e->is_locked()) {
        this->locked.insert(e);
    } else {
        this->locked.erase(e);
    }
}

/**
 * return values:
 *   0 = event needs more handling
 *   1 = event was handled
 *   2 = event something ??
 **/
int
game::menu_handle_event(tms::event *ev)
{
    switch (this->get_mode()) {
        case GAME_MODE_EDIT_PANEL:
            this->state.modified = true;
            return this->panel_edit_handle_event(ev);

        case GAME_MODE_FACTORY:
            return this->factory_handle_event(ev);

        case GAME_MODE_REPAIR_STATION:
            return this->repair_station_handle_event(ev);

        case GAME_MODE_INVENTORY:
            return this->inventory_handle_event(ev);

        case GAME_MODE_EDIT_GEARBOX:
            this->state.modified = true;
            this->gearbox_edit_handle_event(ev);
            return EVENT_DONE;

        case GAME_MODE_SELECT_OBJECT:
            return EVENT_CONT;
    }

    int pid = ev->data.motion.pointer_id;
    tvec2 sp = (tvec2){ev->data.motion.x, ev->data.motion.y};

    switch (ev->type) {
        case TMS_EV_KEY_PRESS:
            return 0;

        case TMS_EV_POINTER_DOWN:{
                                     /*
                                      * FIXME
            if (this->state.finished && !W->level.pause_on_finish
                    && !W->level.flag_active(LVL_DISABLE_CONTINUE_BUTTON) && this->state.pkg) {
                if (ev->data.motion.x > _tms.window_width - (gui_spritesheet::get_sprite(S_CONT)->width+15)
                        && ev->data.motion.y < (gui_spritesheet::get_sprite(S_CONT)->height+15)) {
                    if (!this->state.sandbox && !this->state.test_playing
                         && this->state.pkg != 0) {
                        this->state.ending = true;
                        this->state.end_action = GAME_END_PROCEED;
                    } else {
                        this->do_pause();
                    }
                    return 1;
                }
            }
            */

            /* sandbox menu */
            resizing_menu[pid] = false;

            if (W->is_paused() && this->state.sandbox && ev->data.motion.x > (_tms.window_width - this->get_menu_width() - _tms.xppcm*.125f - (category_selector_open?widest_catsprite/2.f:0))) {

                if (category_selector_open) {
                    float ch = _tms.yppcm * .35f;
                    float h = ch;
                    float w = (h / tallest_catsprite) * widest_catsprite;
                    float x1 = std::max((float)category_selector_x, _tms.window_width - this->get_menu_width() + w/2.f);
                    if (x1 > _tms.window_width - w/2.f)
                        x1 = _tms.window_width - w/2.f;

                    int btn = floor((_tms.window_height - ev->data.motion.y)/ch);

                    if (btn < of::num_categories) {
                        curr_category = btn;
                    }

                    category_selector_open = false;
                    //menu_highlight = 1.f;
                    dragging[pid] = false;
                    tdown[pid] = 0;
                    return 1;
                }

                if (ev->data.motion.y > _tms.window_height-_tms.yppcm*.25f) {
                    category_selector_open = true;
                    category_selector_x = ev->data.motion.x;

                    tdown[pid] = 1;
                    dragging[pid] = false;
                    return 1;
                }

                bool resize = std::abs((ev->data.motion.x - (_tms.window_width-this->get_menu_width()))) < _tms.xppcm/5.f;
                pid = ev->data.motion.pointer_id;

                if (resize) {
                    resizing_menu[pid] = true;
                    dragging[pid] = false;
                    tdown[pid] = 1;
                    return 1;
                }

                struct menu_obj o;
                int found = 0;
                int n=0;

                /* --- */
                float x1 = _tms.window_width - this->get_menu_width();
                float btn_outer_x = _tms.xppcm * 1.1f;
                float btn_outer_y = _tms.yppcm * 1.1f;
                float top = -_tms.yppcm/4.f + (_tms.window_height - btn_outer_y/2.f);

                int xx = roundf((this->get_menu_width()) / btn_outer_x);
                if (xx < 1) xx = 1;
                int yy = floor((_tms.window_height-btn_outer_y) / btn_outer_y);

                float a_x = 0.f;
                float a_y = 0.f;
                int ww = 0;

                float pp = top;

                float cy = menu_cam->_position.y;
                if (cy > (btn_outer_y*yy)) {
                    //cy -= (btn_outer_y*(xx)*yy);
                }

                int div = ceil((float)menu_objects_cat[curr_category].size()/((float)xx));

                if (div < yy) div = yy;

                float max_y = 0.f;
                /* --- */

                if (ev->data.motion.y > _tms.yppcm/2.f + _tms.yppcm*.25f) {
                    for (int x=0; x<menu_objects_cat[curr_category].size(); x++) {
                        o = menu_objects[menu_objects_cat[curr_category][x]];

                        if (x != 0 && (x%div) == 0) {
                            a_x += btn_outer_x;
                            pp = top;
                            ww++;
                        }

                        float _x = x1 + a_x +btn_outer_x/2.f;
                        float _y = cy + pp;

                        if (std::abs(_x - ev->data.motion.x) < btn_outer_x/2.f
                            && std::abs(_y - ev->data.motion.y) < btn_outer_y/2.f) {
                            found = 1;
                            break;
                        }

                        pp -= btn_outer_y;
                    }
                } else {
                    float tja = ev->data.motion.x - (_tms.window_width - this->get_menu_width());
                    tja /= btn_outer_x/2.f;

                    tja = floor(tja);

                    int tjatja = (int)tja;

                    if (tjatja >= 0 && tjatja < MAX_RECENT && this->recent[tjatja] != -1) {
                        found = 1;
                        o = menu_objects[gid_to_menu_pos[this->recent[tjatja]]];
                    }
                }

                if (found) {
                    if (o.e) {
                        entity *e = of::create(o.e->g_id);
                        if (e) {
                            menu_objects[gid_to_menu_pos[e->g_id]].highlighted = true;
                            pending_ent[pid] = e;
                            pending_a_scene[pid] = false;

                            if (this->selection.e && e->g_id == this->selection.e->g_id) {
                                /* copy properties from the selected object */
                                this->copy_properties(e, this->selection.e);

                                e->_angle = this->selection.e->get_angle();
                                e->set_layer(this->selection.e->get_layer());
                                e->set_angle(this->selection.e->get_angle());
                                e->set_moveable(this->selection.e->is_moveable());
                            }

                            e->on_load(true, false);
                        } else {
                            sliding_menu[pid] = true;
                        }
                    }
                } else {
                    sliding_menu[pid] = true;
                }

                tdown[pid] = 1;
                tdown_p[pid] = (tvec2){ev->data.motion.x, ev->data.motion.y};

                return 1;
            }
            }break;

        case TMS_EV_POINTER_UP:
            this->dropping = -1;
            if (tdown[ev->data.motion.pointer_id]) {
                for (int x=0; x<menu_objects.size(); x++) {
                    menu_objects[x].highlighted = false;
                }

                if (dragging[pid]) {
                } else {
                    if (resizing_menu[pid]) {
                        tms_debugf("resizing menu");
                        if (this->get_menu_width() < _tms.xppcm/2.0f) {
                            this->set_menu_width(real_menu_width > _tms.xppcm ? real_menu_width : (real_menu_width = _tms.xppcm));
                        } else {
                            real_menu_width = this->get_menu_width();
                            this->set_menu_width(menu_min_width);
                        }
                    }
                }

                if (pending_ent[pid]) {
                    if (pending_a_scene[pid]) {
                        this->set_menu_width(real_menu_width);

                        W->add(pending_ent[pid]);

                        if ((int)pending_ent[pid]->g_id != this->recent[0]) {
                            int p = -1;
                            for (int x=0; x<MAX_RECENT; x++) {
                                if (this->recent[x] == (int)pending_ent[pid]->g_id) {
                                    p = x;
                                    break;
                                }
                            }

                            if (p != -1) {
                                for (int x=p; x>0; x--) {
                                    this->recent[x] = this->recent[x-1];
                                }
                            }
                            for (int x=MAX_RECENT-1; x>0; x--) {
                                this->recent[x] = this->recent[x-1];
                            }

                            this->recent[0] = (int)pending_ent[pid]->g_id;
                        }

                        /* readd the entity to the scene, since the update method
                            * has changed */
                        this->remove_entity(pending_ent[pid]);
                        this->add_entity(pending_ent[pid]);

                        pending_ent[pid]->construct();
                        pending_ent[pid]->on_pause();

                        if (pending_ent[pid]->type == ENTITY_CABLE) {
                            cable *c = static_cast<cable*>(pending_ent[pid]);
                            this->selection.select(c->p[0], c->p[0]->get_body(0), (tvec2){0,0}, 0, true);
                        } else {
                            this->selection.select(pending_ent[pid], pending_ent[pid]->get_body(0), (tvec2){0,0}, 0, true);
                        }

                        this->state.modified = true;

                        if (W->is_paused()) {
                            ui::emit_signal(SIGNAL_ENTITY_CONSTRUCTED, UINT_TO_VOID(pending_ent[pid]->id));
                        }
                    } else {
                        delete pending_ent[pid];
                    }

                    pending_ent[pid] = 0;
                }
                tdown[pid] = 0;
                sliding_menu[pid] = false;
                resizing_menu[pid] = false;
                dragging[pid] = false;
                return 1;
            }
            break;

        case TMS_EV_POINTER_DRAG:
            if (tdown[pid]) {

                if (resizing_menu[pid]) {
                    if (!dragging[pid]) {
                        if (std::abs(this->get_menu_width() - std::abs(ev->data.motion.x - _tms.window_width)) > _tms.xppcm*.125f) {
                            dragging[pid] = true;
                        }
                    }

                    if (dragging[pid]) {
                        this->set_menu_width(std::abs(ev->data.motion.x - _tms.window_width));
                        if (this->get_menu_width() < menu_min_width) this->set_menu_width(menu_min_width);
                        if (this->get_menu_width() > menu_max_width) this->set_menu_width(menu_max_width);
                        real_menu_width = this->get_menu_width();
                        dragging[pid] = true;
                    }

                    return 1;
                }

                if (pending_ent[pid] || sliding_menu[pid]) {
                    if (pending_ent[pid] && pending_a_scene[pid]) {
                        /* the entity has been added to the scene already */
                        tvec3 pos;
                        W->get_layer_point(this->cam, sp.x, sp.y, pending_ent[pid]->get_layer(), &pos);

                        bool do_snap = ((this->state.abo_architect_mode && !this->shift_down()) || (this->shift_down() && !this->state.abo_architect_mode));
                        if (do_snap) {
                            double gox = fmod(pending_ent[pid]->get_width(), state.gridsize);
                            double goy = fmod(pending_ent[pid]->height, state.gridsize);

                            gox = 0.f;
                            goy = 0.f;

                            b2Vec2 new_pos(
                                    roundf(pos.x/state.gridsize)*state.gridsize+gox,
                                    roundf(pos.y/state.gridsize)*state.gridsize+goy);

                            pending_ent[pid]->_pos = new_pos;
                            pending_ent[pid]->set_position(new_pos);
                        } else {
                            pending_ent[pid]->_pos = b2Vec2(pos.x, pos.y);
                            pending_ent[pid]->set_position(pos.x, pos.y);
                        }

                        pending_ent[pid]->ghost_update();
                    } else if (sliding_menu[pid]) {
                        if (settings["smooth_menu"]->v.b) {
                            menu_cam_vel -= (_tms.yppcm / 1000.f) * (tdown_p[pid].y - sp.y);
                        } else {
                            menu_cam_vel -= tdown_p[pid].y - sp.y;
                        }
                        tdown_p[pid].y = sp.y;
                    } else {
                        /* not added yet, determine whether we are sliding the menu or
                         * moving out an object */
                        if (std::abs(tdown_p[pid].x - sp.x) > _tms.xppcm*.4f) {
                            tvec3 pos;
                            W->get_layer_point(this->cam, sp.x, sp.y, 0, &pos);
                            pending_ent[pid]->set_position(pos.x, pos.y);
                            if (pending_ent[pid]->material) {
                                if (this->selection.e && pending_ent[pid]->g_id == this->selection.e->g_id) {
                                    pending_ent[pid]->set_layer(this->selection.e->get_layer());
                                } else {
                                    pending_ent[pid]->set_layer(this->state.edit_layer);
                                }
                            }
                            real_menu_width = this->get_menu_width();
                            this->set_menu_width(0);
                            this->add_entity(pending_ent[pid]);
                            pending_a_scene[pid] = true;
                        } else {
                            if (fabsf(sp.y - tdown_p[pid].y) > _tms.yppcm*.125f
                                && _tms.xppcm/10.f > fabsf(sp.x - tdown_p[pid].x)) {
                                tdown_p[pid].y = sp.y;
                                tms_infof("sliding");
                                sliding_menu[pid] = true;
                                menu_cam_target = -1;
                            }
                        }
                    }
                }
                return 1;
            }
            break;

        case TMS_EV_POINTER_SCROLL:
            if (W->is_paused() && this->state.sandbox && ev->data.scroll.mouse_x > (_tms.window_width-this->get_menu_width())) {
                if (settings["smooth_menu"]->v.b) {
                    menu_cam_vel -= (float)ev->data.scroll.y * (settings["menu_speed"]->v.f * 4.0f);
                } else {
                    menu_cam_vel -= (float)ev->data.scroll.y * (settings["menu_speed"]->v.f * 40.0f);
                }
                return EVENT_DONE;
            }
            break;
    }

    return EVENT_CONT;
}

entity *game::get_pending_ent()
{
    //for (int x=0; x<MAX_P; x++) {
    int x=0;
        if (pending_ent[x] && pending_a_scene[x]) return pending_ent[x];
    //}

    return 0;
}

static void
my_long_press(principia_wdg *w)
{
    tms_debugf("LONG PRESS, YEAH");
}

bool
game::widget_clicked(principia_wdg *w, uint8_t button_id, int pid)
{
    bool left = (pid == 0);
    bool right = (pid == 1);

    /* Do not accept widget clicks if the widget mouse controls are active. */

    if (settings["rc_lock_cursor"]->v.b) {
        if ((this->active_hori_wdg && !this->active_hori_wdg->is_radial())
                || (this->active_vert_wdg && !this->active_vert_wdg->is_radial())) {
            return false;
        }
    }

    /* GW_ALL */
    switch (button_id) {
        case GW_PLAYPAUSE:
            {
                if (W->is_paused()) {
                    /* PLAY */
                    if (W->is_puzzle() && G->state.sandbox) {
                        ui::open_dialog(DIALOG_PUZZLE_PLAY);
                    } else {
                        if (W->is_puzzle() && this->state.is_main_puzzle) {
                            this->save(false, true);
                        }
                        G->do_play();
                    }
                } else {
                    /* PAUSE */
                    if (W->is_adventure()) {
                        ui::confirm("Are you sure you want to quit this level?",
                                "Yes",  ACTION_WORLD_PAUSE,
                                "No",   ACTION_IGNORE);
                    } else {
                        G->do_pause();
                    }
                }
            }
            return true;

        case GW_ORTHOGRAPHIC:
            {
                G->set_architect_mode(!G->state.abo_architect_mode);

                if (G->state.abo_architect_mode) {
                    w->faded = 0;
                } else {
                    w->faded = 1;
                }
            }
            return true;

        case GW_LAYERVIS:
            {
                if (left) {
                    switch (G->layer_vis) {
                        case 0b001: G->layer_vis = 0b111; break;
                        case 0b011: G->layer_vis = 0b001; break;
                        default: case 0b111: G->layer_vis = 0b011; break;
                    }
                } else if (right) {
                    switch (G->layer_vis) {
                        case 0b001: G->layer_vis = 0b011; break;
                        case 0b011: G->layer_vis = 0b111; break;
                        default: case 0b111: G->layer_vis = 0b001; break;
                    }
                }

                switch (G->layer_vis) {
                    case 0b001: G->wdg_layervis->s[0] = gui_spritesheet::get_sprite(S_LAYERVIS_1); break;
                    case 0b011: G->wdg_layervis->s[0] = gui_spritesheet::get_sprite(S_LAYERVIS_2); break;
                    default: case 0b111: G->wdg_layervis->s[0] = gui_spritesheet::get_sprite(S_LAYERVIS_3); break;
                }
            }
            return true;

        case GW_MODE:
            {
                switch (G->get_mode()) {
                    default:
                        ui::open_dialog(DIALOG_SANDBOX_MODE);
                        break;

                    case GAME_MODE_DRAW:
                    case GAME_MODE_CONN_EDIT:
                    case GAME_MODE_MULTISEL:
                        G->set_mode(GAME_MODE_DEFAULT);
                        break;
                }
            }
            return true;

        case GW_ADVANCED:
            {
                G->state.advanced_mode = !G->state.advanced_mode;
                G->refresh_widgets();
            }
            return true;

        case GW_HELP:
            {
                if (!W->level.descr_len) {
                    if (W->is_puzzle()) {
                        ui::message("No help available for this level.");
                    } else {
                        ui::message("No description available for this level.");
                    }
                } else {
                    ui::open_help_dialog("Level description", W->level.descr);
                }
            }
            return true;

        case GW_ERROR:
            {
                uint8_t error_count[ERROR_COUNT];

                for (int x=0; x<ERROR_COUNT; ++x) error_count[x] = 0;

#define MAX_ERROR_LENGTH 4096

                char error_str[MAX_ERROR_LENGTH] = { 0 };
                int len = 0;

                std::set<er*>::iterator it = G->errors.begin();
                for (; it != G->errors.end(); ++it) {
                    er *error = static_cast<er*>(*it);

                    if (error->type == ERROR_SCRIPT_COMPILE) {
                        // For ERROR_SCRIPT_COMPILE, we will list all LuaScript-objects that fail to compile.
                        if (len > MAX_ERROR_LENGTH-1) break;
                        len += snprintf(error_str + len, MAX_ERROR_LENGTH - len - 1, "LuaScript with id %d compile error:\n%s\n", error->e->id, error->message);
                    } else {
                        error_count[error->type] ++;
                    }
                }

                for (int error_type=0; error_type<ERROR_COUNT; ++error_type) {
                    if (len > MAX_ERROR_LENGTH-1) break;
                    if (!error_count[error_type]) continue;
                    const char *plural = error_count[error_type] > 1 ? "s" : "";
                    switch (error_type) {
                        case ERROR_NONE: break;
                        case ERROR_SOLVE:
                                         len += snprintf(error_str + len, MAX_ERROR_LENGTH - len - 1, "%d Electronic device%s unable to complete their programming. This can be caused by unsolvable loops, or a chain of electronic devices that aren't fully connected.\n\n", error_count[error_type], plural);
                                         break;
                        case ERROR_RC_NO_WIDGETS:
                                         len += snprintf(error_str + len, MAX_ERROR_LENGTH - len - 1, "%d RC object%s missing widgets. You can solve this by sleecting the erroneous RC while paused, clicking the cogwheel-button in the bottom-left and dragging widgets to the desired location.\n\n", error_count[error_type], plural);
                                         break;
                        case ERROR_RC_ACTIVATOR_INVALID:
                                         len += snprintf(error_str + len, MAX_ERROR_LENGTH - len - 1, "%d RC Activator%s don't point to a valid RC object. While paused, select the erroneous RC Activator, click the crosshair-button in the bottom-left and select the RC you wish the RC activator to be linked to.\n\n", error_count[error_type], plural);
                                         break;
                    }
                }

                tms_debugf("Error str: '%s'[%d]", error_str, len);

                ui::open_error_dialog(error_str);
            }
            return true;

        case GW_DEFAULT_LAYER:
            if (left) {
                G->state.edit_layer = (G->state.edit_layer + 1) % 3;
            } else if (right) {
                G->state.edit_layer = (G->state.edit_layer - 1);
                if (G->state.edit_layer < 0) G->state.edit_layer = 2;
            }

            G->refresh_widgets();
            return true;

        case GW_MULTISEL_IMPORT_EXPORT:
            if (G->get_mode() == GAME_MODE_MULTISEL && G->selection.m) {
                ui::open_dialog(DIALOG_EXPORT);
            } else {
                if (G->multi.import) {
                    delete G->multi.import;
                    G->multi.import = 0;
                    G->refresh_widgets();
                } else {
                    ui::open_dialog(DIALOG_OPEN_OBJECT);
                }
            }
            return true;

        case GW_REMOVE:
            if (G->get_mode() == GAME_MODE_MULTISEL && G->selection.m) {
                ui::confirm("Are you sure you want to delete these objects?",
                        "Yes",  ACTION_MULTI_DELETE,
                        "No",   ACTION_IGNORE);
            } else if (G->selection.e && G->selection.e->requires_delete_confirmation()) {
                ui::confirm("Are you sure you want to delete this object?",
                        "Yes",  ACTION_DELETE_SELECTION,
                        "No",   ACTION_IGNORE);
            } else if (G->delete_selected_entity() == T_OK) {
                G->state.modified = true;
                G->refresh_widgets();
            }
            return true;

        case GW_BRUSH_LAYER_INCLUSION:
            G->brush_layer_inclusion = !G->brush_layer_inclusion;

            G->refresh_widgets();

            return true;

        case GW_TOGGLE_LOCK:
            G->toggle_entity_lock(G->selection.e);

            G->refresh_widgets();
            break;

        case GW_DETACH:
            if (G->selection.e && G->selection.e->conn_ll) {
                tms_infof("DISCONNECT ALL");
                entity *e = G->selection.e;
                G->animate_disconnect(e);
                e->disconnect_all();

                G->wdg_detach->faded = (e->conn_ll == 0);
                G->state.modified = true;

                if (W->is_adventure() && W->is_playing()) {
                    G->post_interact_select(e);
                }

                return true;
            }
            break;

        case GW_UNPLUG:
            if (G->selection.e) {
                G->open_socket_selector(0, G->selection.e->get_edevice());
                G->state.modified = true;

                return true;
            }
            break;

        case GW_TOGGLE_AXIS_ROT:
            if (G->selection.e) {
                G->selection.e->toggle_axis_rot();
                G->state.modified = true;

                G->refresh_axis_rot();

                return true;
            }
            break;

        case GW_LAYER_DOWN_CYCLE:
            if (G->selection.e && (left || right)) {
                if (!(!G->state.sandbox && W->is_puzzle() && W->level.flag_active(LVL_DISABLE_LAYER_SWITCH))) {
                    int num_available_layers = 3;
                    if (W->level.flag_active(LVL_DISABLE_3RD_LAYER)) {
                        num_available_layers = 2;
                    }

                    if (left) {
                        G->selection.e->set_layer((G->selection.e->get_layer()+1) % num_available_layers);
                    } else {
                        int new_layer = G->selection.e->get_layer() - 1;
                        if (new_layer < 0) new_layer = num_available_layers - 1;
                        G->selection.e->set_layer(new_layer);
                    }

                    G->animate_disconnect(G->selection.e);
                    G->selection.e->disconnect_all();

                    G->recheck_all_placements();

                    G->refresh_widgets();
                    G->state.modified = true;
                }

                return true;
            }
            break;

        case GW_LAYER_DOWN:
        case GW_LAYER_UP:
            if (W->is_adventure() && !W->is_paused()) {
                G->handle_ingame_object_button(button_id);

                return true;
            }
            break;

            /* FIXME: Damper, Open pivot and Rubberband can't be moveable right now. fix sometime */
        case GW_TOGGLE_MOVEABLE:
            if (G->selection.e) {
                if (G->selection.e->g_id == O_OPEN_PIVOT || G->selection.e->g_id == O_OPEN_PIVOT_2) {
                    ui::message("Can't set the Pivot to moveable.");
                } else if (G->selection.e->g_id == O_DAMPER || G->selection.e->g_id == O_DAMPER_2) {
                    ui::message("Can't set the Damper to moveable.");
                } else if (G->selection.e->g_id == O_RUBBERBAND || G->selection.e->g_id == O_RUBBERBAND_2) {
                    ui::message("Can't set the Rubberband to moveable.");
                } else {
                    G->selection.e->get_property_entity()->set_moveable(!G->selection.e->get_property_entity()->is_moveable());

                    if (G->selection.e->get_property_entity()->is_moveable()) {
                        ui::message("Object is now moveable when playing.");
                    } else {
                        ui::message("Object can no longer be moved when playing.");
                    }
                }
                G->state.modified = true;

                G->wdg_moveable->faded = !G->selection.e->get_property_entity()->is_moveable();

                return true;
            }
            break;

        case GW_CONFIG:
            if (W->is_paused()) {
                if (G->get_mode() == GAME_MODE_MULTISEL) {
                    ui::open_dialog(DIALOG_MULTI_CONFIG);
                } else {
                    G->config_btn_pressed(G->selection.e);
                }
            } else if (W->is_adventure()) {
                G->handle_ingame_object_button(button_id);
            }
            return true;

        case GW_TOGGLE_CLOCKWISE:
            if (G->selection.e && G->selection.e->is_motor()) {
                motor *m = static_cast<motor*>(G->selection.e);

                m->toggle_dir();
                G->wdg_cwccw->s[0] = m->get_dir() ? gui_spritesheet::get_sprite(S_CW) : gui_spritesheet::get_sprite(S_CCW);

                G->state.modified = true;

                return true;
            }
            break;

        case GW_FREQ_UP:
            if (G->selection.e && G->selection.e->is_wireless()) {
                int mod = 0;
                if (left) {
                    mod = 1;
                } else if (right) {
                    mod = 5;
                }

                if (mod) {
                    if (G->selection.e->properties[0].v.i > UINT32_MAX - mod) {
                        G->selection.e->properties[0].v.i = UINT32_MAX;
                        ui::message("reached max frequency");
                    } else {
                        G->selection.e->properties[0].v.i += mod;
                        ui::messagef("+%d", mod);
                    }

                    G->refresh_widgets();

                    return true;
                }
            }
            break;

        case GW_FREQ_DOWN:
            if (G->selection.e && G->selection.e->is_wireless()) {
                int mod = 0;
                if (left) {
                    mod = 1;
                } else if (right) {
                    mod = 5;
                }

                if (mod) {
                    if (G->selection.e->properties[0].v.i < mod) {
                        G->selection.e->properties[0].v.i = 0;
                    } else {
                        G->selection.e->properties[0].v.i -= mod;
                        ui::messagef("-%d", mod);
                    }

                    G->refresh_widgets();

                    return true;
                }
            }
            break;

        case GW_TRACKER:
            G->begin_tracker(G->selection.e);
            return true;

        case GW_INFO:
            G->info_btn_pressed(G->selection.e);
            return true;

        case GW_MENU:
            if (W->is_paused() && G->state.sandbox) {
                ui::open_dialog(DIALOG_SANDBOX_MENU);
            } else {
                ui::open_dialog(DIALOG_PLAY_MENU);
            }
            return true;

        case GW_QUICKADD:
            if (W->is_paused() && G->state.sandbox) {
                ui::open_dialog(DIALOG_QUICKADD);
            }
            return true;

        case GW_FOLLOW_CONNECTIONS:
            G->multi.follow_connections = !G->multi.follow_connections;
            w->faded = !G->multi.follow_connections;
            return true;

        case GW_FOLLOW_CABLES:
            G->multi.follow_cables = !G->multi.follow_cables;
            w->faded = !G->multi.follow_cables;
            return true;

        case GW_ADDITIVE:
            G->multi.additive_selection = !G->multi.additive_selection;
            w->faded = !G->multi.additive_selection;
            return true;

        case GW_SELECT_THROUGH_LAYERS:
            G->multi.select_through_layers = !G->multi.select_through_layers;
            w->faded = !G->multi.select_through_layers;
            return true;

        case GW_BOX_SELECT:
            if (G->multi.box_select > 0) {
                G->multi.box_select = 0;
            } else {
                G->multi.box_select = 1;
                /* XXX: Test this a little bit, make sure it's not annoying. */
                G->selection.disable();
            }

            G->refresh_widgets();
            return true;

        case GW_DISCONNECT_FROM_RC:
            if (adventure::is_player_alive()) {
                adventure::player->detach();
            }
            return true;

        case GW_CLOSE_PANEL_EDIT:
            G->set_mode(GAME_MODE_DEFAULT);
            return true;

        case GW_USERNAME:
            if (P.username) {
                P.add_action(ACTION_AUTOSAVE, 0);
                P.num_unread_messages = 0;
                COMMUNITY_URL("user/%s", P.username);
                ui::open_url(url);

                char username[256];
                snprintf(username, 255, "%s", P.username);

                w->set_label(username);
            } else {
                ui::open_dialog(DIALOG_LOGIN);
            }
            return true;

        case GW_SUBMIT_SCORE:
            P.add_action(ACTION_SUBMIT_SCORE, 0);
            return true;

        case GW_TOOL_HELP:
            if (W->is_adventure() && adventure::player) {
                robot_parts::tool *t = adventure::player->get_tool();

                if (t) {
                    char wikiurl[256];
                    snprintf(wikiurl, 255, "https://principia-web.se/wiki/Special:GotoItem?id=%d", t->get_item_id());
                    ui::open_url(wikiurl);
                }
            }
            return true;

        case GW_IGNORE:
            break;
        default:
            {
                /* We handle the brush materials separately, since their numbers are dynamic */
                if (button_id >= GW_BRUSH_MAT_0 && button_id <= GW_BRUSH_MAT_END) {
                    G->brush_material = button_id - GW_BRUSH_MAT_0;

                    for (int x=0; x<4; ++x) {
                        G->wdg_brush_material[x]->faded = (this->brush_material != x);
                    }

                    return true;
                }

                tms_errorf("Unhandled GW: %u", button_id);
            }
            break;
    }

    return false;
}

void
game::init_gui(void)
{
    if (initialized) {
        return;
    }

    int ierr;
    tms_infof("Initializing GUI ");

    initialized = true;

    this->text_small = new p_text(font::small);

    this->texts = tms_atlas_alloc(1024, 1024, 4);
    this->texts->padding_x = 1;
    this->texts->padding_y = 1;
    this->texts->texture.filter = GL_LINEAR;
    tms_texture_clear_buffer(&this->texts->texture, 0);

    this->set_surface(new tms::surface());
    this->get_surface()->atlas = gui_spritesheet::atlas;

    menu_height = _tms.opengl_height;

    /*
    menu_xdim = settings["uiscale"]->v.f * _tms.xppcm;
    menu_ydim = settings["uiscale"]->v.f * _tms.yppcm;
    */
    menu_xdim = 1.f * _tms.xppcm;
    menu_ydim = 1.f * _tms.yppcm;

    float w = 3.f * menu_xdim;
    menu_scale = w/this->get_menu_width();
    real_menu_width =  _tms.xppcm;
    _menu_width = real_menu_width;

    menu_max_width = _tms.window_width * .8f;
    menu_min_width = _tms.xppcm/4.f;

    s_size = menu_xdim;
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::init_gui 2", ierr);

    b_w = (int)(.5f * menu_xdim);
    b_h = (int)(.5f * menu_ydim);
    b_w_pad = (int)((float)b_w + .1f * menu_xdim);
    b_h_pad = (int)((float)b_h + .1f * menu_ydim);
    b_y = (int)((float)b_h/2.f + .1f * menu_ydim);

    b_margin_y = .14f * menu_ydim;
    b_margin_x = .14f * menu_xdim;

    menu_graph = new tms::graph(2);

    menu_graph->sorting[0] = TMS_SORT_TEXTURE0;
    menu_graph->sorting[1] = TMS_SORT_TEXTURE1;
    menu_graph->sorting[2] = TMS_SORT_SHADER;
    menu_graph->sorting[3] = TMS_SORT_VARRAY;
    menu_graph->sorting[4] = TMS_SORT_MESH;
    menu_graph->sort_depth = 5;

    menu_cam = new tms::camera();
    menu_cam->set_position(0.f, 0.f, 0.f);
    menu_cam->set_direction(0, 0, -1);
    menu_cam->width = this->get_menu_width();
    menu_cam->height = _tms.window_height;
    menu_cam->owidth = this->get_menu_width();
    menu_cam->oheight = _tms.window_height;

    menu_cam->fov = 45.f;
    menu_cam->near = -1.f;
    menu_cam->far = 1.f;

    menu_cat_width = this->get_menu_width();

    int n=0;
    for (n=0; n<of::num_categories; n++) {
        catsprites[n] = this->text_small->add_to_atlas(this->texts, of::get_category_name(n));

        menu_cat_width += C_PADDING + catsprites[n]->width;

        catsprite_hints[n] = this->text_small->add_to_atlas(this->texts, of::get_category_hint(n));

        if (catsprites[n]->width > widest_catsprite) widest_catsprite = catsprites[n]->width;
        if (catsprites[n]->height > tallest_catsprite) tallest_catsprite = catsprites[n]->height;
    }

    betasprite = this->text_small->add_to_atlas(this->texts, "(BETA)");
    devsprite = this->text_small->add_to_atlas(this->texts, "(DEV)");
    filterlabel = this->text_small->add_to_atlas(this->texts, "Categories:");
    factionlabel = this->text_small->add_to_atlas(this->texts, "Faction:");
    catsprites[n] = this->text_small->add_to_atlas(this->texts, "Recent");

    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::init_gui 3", ierr);
    int num_objects = 0;
    for (int y=0; y<of::num_categories; y++) {
        for (int x=0; x<of::get_num_objects(y); x++) {
            int gid = of::get_gid(y, x);
            entity *e = of::create(gid);

            //tms_infof("%d:'%s'", gid, e->get_name());
            if (!e) {
                tms_errorf("Error creating %d", gid);
                continue;
            }
            tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::init_gui menu_item loop (g_id: %d)", ierr, gid);
            this->add_menu_item(y, e);
            tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::init_gui menu_item loop after adding menu_item (g_id: %d)", ierr, gid);
            num_objects ++;
        }
    }

    item::_init();

    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::init_gui 4", ierr);

    struct tms_texture *tex = tms_texture_alloc();
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::init_gui 5", ierr);

    tms_texture_upload(&this->texts->texture);

    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::init_gui 9", ierr);

    this->init_gearbox_edit();
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::init_gui 11", ierr);
    this->init_sandbox_menu();
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::init_gui 12", ierr);

    tms_infof("Number of objects in menu: %d", num_objects);

    this->wm = new widget_manager(this, false, true);
    this->info_label = new p_text(font::medium, ALIGN_CENTER, ALIGN_CENTER);

#ifdef TMS_BACKEND_PC
    this->hov_text = new p_text(font::medium, ALIGN_CENTER, ALIGN_BOTTOM);
    this->hov_text->active = false;
#endif

    this->wdg_submit_score = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            GW_SUBMIT_SCORE, AREA_BOTTOM_CENTER);
    this->wdg_submit_score->priority = 1000;
    this->wdg_submit_score->render_background = true;
    this->wdg_submit_score->set_label("Submit score", font::xlarge);

    this->wdg_unable_to_submit_score = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            GW_IGNORE, AREA_BOTTOM_CENTER);
    this->wdg_unable_to_submit_score->priority = 1000;
    this->wdg_unable_to_submit_score->render_background = true;
    this->wdg_unable_to_submit_score->set_label("Unable to submit score", font::large);
    this->wdg_unable_to_submit_score->set_tooltip("Can't submit score for state-loaded levels.");

    this->wdg_username = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            GW_USERNAME, AREA_TOP_LEFT);
    this->wdg_username->priority = 2000;
    this->wdg_username->label = pscreen::text_username;

    this->wdg_playpause = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_PLAYPAUSE, AREA_TOP_LEFT,
            gui_spritesheet::get_sprite(S_PLAY), 0);
    this->wdg_playpause->priority = 1000;
    this->wdg_playpause->set_tooltip("Play or pause the simulation!");

    this->wdg_orthographic = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_ORTHOGRAPHIC, AREA_TOP_LEFT,
            gui_spritesheet::get_sprite(S_ORTHOGRAPHIC), 0);
    this->wdg_orthographic->priority = 900;
    this->wdg_orthographic->set_tooltip("Toggle orthographic view");

    this->wdg_layervis = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_LAYERVIS, AREA_TOP_LEFT,
            gui_spritesheet::get_sprite(S_LAYERVIS_3), 0);
    this->wdg_layervis->priority = 800;
    this->wdg_layervis->set_tooltip("Toggle layer visibility");

    this->wdg_mode = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_MODE, AREA_TOP_LEFT,
            gui_spritesheet::get_sprite(S_CONFIG), 0);
    this->wdg_mode->priority = 700;
    this->wdg_mode->set_tooltip("Change mode");

    this->wdg_advanced = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_ADVANCED, AREA_TOP_LEFT,
            gui_spritesheet::get_sprite(S_ADVUP), 0);
    this->wdg_advanced->priority = 600;
    this->wdg_advanced->set_tooltip("Toggle advanced options");

    this->wdg_help = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_HELP, AREA_TOP_LEFT,
            gui_spritesheet::get_sprite(S_HELP), 0);
    this->wdg_help->priority = 800;
    this->wdg_help->set_tooltip("Level description");

    this->wdg_error = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_ERROR, AREA_TOP_LEFT,
            gui_spritesheet::get_sprite(S_EMPTY), gui_spritesheet::get_sprite(S_ERROR));
    this->wdg_error->priority = 900;
    this->wdg_error->set_tooltip("There's an error in your level");

    this->wdg_default_layer = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_DEFAULT_LAYER, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_LAYER0), 0);
    this->wdg_default_layer->priority = 1000;
    this->wdg_default_layer->set_tooltip("Default layer");

    /* MODE: Multiselect */
    this->wdg_multisel = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_MULTISEL_IMPORT_EXPORT, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_IMPORT), 0);
    this->wdg_multisel->priority = 1500;
    this->wdg_multisel->set_tooltip("Import/export object");

    /* MODE: Multiselect, Selection, Connection Edit */
    this->wdg_remove = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_REMOVE, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_CLOSE), 0);
    this->wdg_remove->priority = 900;
#ifdef TMS_BACKEND_PC
    this->wdg_remove->set_tooltip("Remove object (key binding: Delete)");
#else
    this->wdg_remove->set_tooltip("Remove object");
#endif
    this->wdg_remove->marker = true;
    this->wdg_remove->marker_color.r = 1.5f;

    /* MODE: Draw */
    this->wdg_layer_inclusion = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_BRUSH_LAYER_INCLUSION, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_LAYER_INCLUSION), 0);
    this->wdg_layer_inclusion->priority = 900;
    this->wdg_layer_inclusion->set_tooltip("Layer inclusion");

    static const char *materials[NUM_TERRAIN_MATERIALS] = {
        "Eraser",
        "Grass",
        "Dirt",
        "Stone",
        "Granite",
    };

#ifdef DEBUG
    for (int x=0; x<NUM_TERRAIN_MATERIALS; ++x) {
        tms_assertf(materials[x] != 0, "Material string missing in game-gui!");
    }
#endif

    this->wdg_brush_material[0] = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_BRUSH_MAT_0, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_CLOSE), 0);
    this->wdg_brush_material[0]->priority = 700;
    this->wdg_brush_material[0]->set_tooltip(materials[0]);

    /* Materials */
    for (int x=1; x<NUM_TERRAIN_MATERIALS; ++x) {
        this->wdg_brush_material[x] = this->wm->create_widget(
                this->get_surface(), TMS_WDG_BUTTON,
                GW_BRUSH_MAT_0+x, AREA_BOTTOM_LEFT,
                gui_spritesheet::get_sprite(S_TPIXEL_MATERIAL0+x-1), 0);
        this->wdg_brush_material[x]->priority = 700-x;
        this->wdg_brush_material[x]->set_tooltip(materials[x]);
    }

    /* MODE: Connection Edit */
    this->wdg_connstrength = this->wm->create_widget(
            this->get_surface(), TMS_WDG_SLIDER,
            GW_IGNORE, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_SLIDER_2), gui_spritesheet::get_sprite(S_SLIDER_HANDLE));
    this->wdg_connstrength->priority = 800;
    this->wdg_connstrength->set_label("Strength");
    this->wdg_connstrength->on_change = connstrength_on_change;
    this->wdg_connstrength->snap[0] = .05f;
    this->wdg_connstrength->data3 = 0; // pointer to the current connection
    this->wdg_connstrength->size.x = .5f* 3.f * _tms.xppcm;

    this->wdg_conndamping = this->wm->create_widget(
            this->get_surface(), TMS_WDG_SLIDER,
            GW_IGNORE, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_SLIDER_2), gui_spritesheet::get_sprite(S_SLIDER_HANDLE));
    this->wdg_conndamping->priority = 700;
    this->wdg_conndamping->set_label("Damping");
    this->wdg_conndamping->on_change = conndamping_on_change;
    this->wdg_conndamping->snap[0] = .01f;
    this->wdg_conndamping->data3 = 0; // pointer to the current connection
    this->wdg_conndamping->size.x = .5f* 3.f * _tms.xppcm;

    this->wdg_menu = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_MENU, AREA_GAME_TOP_RIGHT,
            gui_spritesheet::get_sprite(S_MENU), 0);
    this->wdg_menu->priority = 1000;
    this->wdg_menu->set_tooltip("Menu");

    this->wdg_quickadd = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_QUICKADD, AREA_GAME_TOP_RIGHT,
            gui_spritesheet::get_sprite(S_SEARCH), 0);
    this->wdg_quickadd->priority = 900;
    this->wdg_quickadd->set_tooltip("Quickadd");
    this->wdg_quickadd->on_long_press = my_long_press;

    this->wdg_info = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_INFO, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_INFO), 0);
    this->wdg_info->priority = 2000;
    this->wdg_info->set_tooltip("Object help");
    this->wdg_info->marker = true;
    this->wdg_info->marker_color.g = 1.5f;

    this->wdg_layer = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_LAYER_DOWN_CYCLE, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_LAYER0), 0);
    this->wdg_layer->priority = 1800;
#ifdef TMS_BACKEND_PC
    this->wdg_layer->set_tooltip("Change object layer (key binding: Z/X)");
#else
    this->wdg_layer->set_tooltip("Change object layer");
#endif

    this->wdg_layer_down = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_LAYER_DOWN, AREA_TOP_CENTER,
            gui_spritesheet::get_sprite(S_MINUS), 0);
    this->wdg_layer_down->priority = 1800;
#ifdef TMS_BACKEND_PC
    this->wdg_layer_down->set_tooltip("Layer down (key binding: Z)");
#else
    this->wdg_layer_down->set_tooltip("Layer down");
#endif

    this->wdg_layer_up = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_LAYER_UP, AREA_TOP_CENTER,
            gui_spritesheet::get_sprite(S_PLUS), 0);
    this->wdg_layer_up->priority = 1700;
#ifdef TMS_BACKEND_PC
    this->wdg_layer_up->set_tooltip("Layer up (key binding: X)");
#else
    this->wdg_layer_up->set_tooltip("Layer up");
#endif

    this->wdg_lock = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_TOGGLE_LOCK, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_BTN_LOCK), 0);
    this->wdg_lock->priority = 1700;
#ifdef TMS_BACKEND_PC
    this->wdg_lock->set_tooltip("Toggle lock (key binding: N)");
#else
    this->wdg_lock->set_tooltip("Toggle lock");
#endif

    this->wdg_detach = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_DETACH, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_DC), 0);
    this->wdg_detach->priority = 1600;
#ifdef TMS_BACKEND_PC
    this->wdg_detach->set_tooltip("Disconnect from other objects (key binding: T)");
#else
    this->wdg_detach->set_tooltip("Disconnect from other objects");
#endif
    this->wdg_detach->marker = true;
    this->wdg_detach->marker_color.b = 1.5f;

    this->wdg_unplug = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_UNPLUG, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_UNPLUG), 0);
    this->wdg_unplug->priority = 1500;
    this->wdg_unplug->set_tooltip("Unplug cable");

    this->wdg_axis = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_TOGGLE_AXIS_ROT, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_AXIS), 0);
    this->wdg_axis->priority = 1400;
    this->wdg_axis->set_tooltip("Toggle axis rotation");

    this->wdg_moveable = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_TOGGLE_MOVEABLE, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_MOVABLE), 0);
    this->wdg_moveable->priority = 1300;
    this->wdg_moveable->set_tooltip("Toggle moveable");

    this->wdg_config = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_CONFIG, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_CONFIG), 0);
    this->wdg_config->priority = 1200;
#ifdef TMS_BACKEND_PC
    this->wdg_config->set_tooltip("Configure object (key binding: Y)");
#else
    this->wdg_config->set_tooltip("Configure object");
#endif

    this->wdg_tracker = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_TRACKER, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_SELECT), 0);
    this->wdg_tracker->priority = 1100;
    this->wdg_tracker->set_tooltip("Track object");

    this->wdg_cwccw = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_TOGGLE_CLOCKWISE, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_CW), 0);
    this->wdg_cwccw->priority = 1090;
    this->wdg_cwccw->set_tooltip("Toggle clockwise");

    this->wdg_freq_up = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_FREQ_UP, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_PLUS), 0);
    this->wdg_freq_up->priority = 1080;
#ifdef TMS_BACKEND_PC
    this->wdg_freq_up->set_tooltip("Increase wireless frequency by 1 (5 for rightclick).");
#else
    this->wdg_freq_up->set_tooltip("Increase wireless frequency by 1.");
#endif

    this->wdg_freq_down = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_FREQ_DOWN, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_MINUS), 0);
    this->wdg_freq_down->priority = 1070;
#ifdef TMS_BACKEND_PC
    this->wdg_freq_down->set_tooltip("Decrease wireless frequency by 1 (5 for rightclick).");
#else
    this->wdg_freq_down->set_tooltip("Decrease wireless frequency by 1.");
#endif

    for (uint8_t x=0; x<ENTITY_MAX_SLIDERS; ++x) {
        this->wdg_selection_slider[x] = this->wm->create_widget(
                this->get_surface(), TMS_WDG_SLIDER,
                GW_IGNORE, AREA_BOTTOM_LEFT,
                gui_spritesheet::get_sprite(S_SLIDER_2), gui_spritesheet::get_sprite(S_SLIDER_HANDLE));
        this->wdg_selection_slider[x]->priority = 200 - x;
        this->wdg_selection_slider[x]->on_change = forward_slider_on_change;
        this->wdg_selection_slider[x]->data3 = INT_TO_VOID(x); // ID of current slider
        this->wdg_selection_slider[x]->size.x = .5f* 3.f * _tms.xppcm;
    }

    this->wdg_additive = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_ADDITIVE, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_PLUS), 0);
    this->wdg_additive->priority = 1300;
    this->wdg_additive->set_tooltip("Additive selection");

    this->wdg_follow_connections = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_FOLLOW_CONNECTIONS, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_MULTICONN), 0);
    this->wdg_follow_connections->priority = 600;
    this->wdg_follow_connections->set_tooltip("Follow connections");
    this->wdg_follow_connections->set_label("Options");
    this->wdg_follow_connections->label->calculate(ALIGN_LEFT, ALIGN_CENTER);
    this->wdg_follow_connections->lmodx = -0.5f;

    this->wdg_follow_cables = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_FOLLOW_CABLES, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_MULTICABLE), 0);
    this->wdg_follow_cables->priority = 590;
    this->wdg_follow_cables->set_tooltip("Follow cables");

    this->wdg_select_through_layers = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_SELECT_THROUGH_LAYERS, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_SELECT_THROUGH), 0);
    this->wdg_select_through_layers->priority = 580;
    this->wdg_select_through_layers->set_tooltip("Select through layers");

    this->wdg_box_select = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_BOX_SELECT, AREA_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_BOXSELECT), 0);
    this->wdg_box_select->priority = 1400;
    this->wdg_box_select->set_tooltip("Box select");

    this->wdg_multi_help = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            GW_BOX_SELECT, AREA_TOP_CENTER);
    this->wdg_multi_help->set_label("Multiselect mode");
    this->wdg_multi_help->clickthrough = 1;
    this->wdg_multi_help->priority = 1000;

    this->wdg_current_tool = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            GW_TOOL_HELP, AREA_SUB_HEALTH);
    this->wdg_current_tool->label = adventure::current_tool;
    this->wdg_current_tool->priority = 1000;

    this->wdg_disconnect_from_rc = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_DISCONNECT_FROM_RC, AREA_BOTTOM_CENTER,
            gui_spritesheet::get_sprite(S_CLOSE), 0);
    this->wdg_disconnect_from_rc->priority = 1000;
    this->wdg_disconnect_from_rc->set_tooltip("Disconnect from RC");

    this->wdg_close_panel_edit = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            GW_CLOSE_PANEL_EDIT, AREA_BOTTOM_CENTER,
            gui_spritesheet::get_sprite(S_CLOSE), 0);
    this->wdg_close_panel_edit->priority = 1000;
    this->wdg_close_panel_edit->set_tooltip("Close");

    this->wdg_panel_slider_knob = this->wm->create_widget(
            this->get_surface(), TMS_WDG_KNOB,
            GW_PANEL_SLIDER_KNOB, AREA_BOTTOM_CENTER,
            gui_spritesheet::get_sprite(S_EMPTY), 0);
    this->wdg_panel_slider_knob->priority = 0;
    this->wdg_panel_slider_knob->set_draggable(true);

    this->refresh_widgets();

    this->selection.disable();

    this->init_panel_edit();
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::init_gui 10", ierr);
}

void
game::refresh_info_label()
{
    this->info_label->active = false;

    if (this->get_mode() == GAME_MODE_EDIT_PANEL) {
        return;
    }

    bool ui = W->is_paused() || W->is_adventure();
    bool adventure_playing = W->is_adventure() && !W->is_paused();

    if (this->selection.enabled() && this->selection.e && this->selection.e->get_property_entity() && ui) {
        entity *e = this->selection.e->get_property_entity();

        /* Info label */
        {
            char tmp[256];
            e->write_quickinfo(tmp);

            if (G->state.sandbox && settings["display_object_id"]->v.b) {
                char tmp2[256];
                e->write_object_id(tmp2);

                strcat(tmp, tmp2);
            }

            this->info_label->set_text(tmp);
            this->info_label->active = true;
            if (adventure_playing) {
                struct widget_area *area = &this->wm->areas[AREA_TOP_CENTER];
                /* We calculate the y position using a button widgets height */
                float x = area->base_x;
                float y = area->base_y + (area->imody * (this->wdg_info->size.y + this->wdg_info->padding.y));
                this->info_label->set_position(x, y);
                this->info_label->calculate(ALIGN_CENTER, ALIGN_TOP, NL_DIR_DOWN);
            } else {
                struct widget_area *area = &this->wm->areas[AREA_BOTTOM_LEFT];
                /* We calculate the y position using a button widgets height */
                float x = area->base_x + (area->imodx * this->wdg_info->padding.x);
                float y = area->base_y + (area->imody * (this->wdg_info->size.y + this->wdg_info->padding.y));
                this->info_label->set_position(x, y);
                this->info_label->calculate(ALIGN_LEFT, ALIGN_BOTTOM, NL_DIR_UP);
            }
        }
    } else if (this->get_mode() == GAME_MODE_MULTISEL && W->is_paused() && !this->selection.m && this->multi.import) {
        char tmp[256];
        snprintf(tmp, 255, "Loaded partial:\n%.*s", this->multi.import->lvl.name_len, this->multi.import->lvl.name);
        this->info_label->set_text(tmp);
        this->info_label->active = true;

        struct widget_area *area = &this->wm->areas[AREA_BOTTOM_LEFT];
        /* We calculate the y position using a button widgets height */
        float x = area->base_x + (area->imodx * this->wdg_info->padding.x);
        float y = area->base_y + (area->imody * (this->wdg_info->size.y + this->wdg_info->padding.y));
        this->info_label->set_position(x, y);
        this->info_label->calculate(ALIGN_LEFT, ALIGN_BOTTOM, NL_DIR_DOWN);
    }
}

void
game::refresh_axis_rot()
{
    bool ui = W->is_paused() || W->is_adventure();
    bool adventure_playing = W->is_adventure() && !W->is_paused();

    if (this->selection.enabled() && this->selection.e && this->selection.e->get_property_entity() && ui) {
        entity *e = this->selection.e->get_property_entity();
        this->wdg_axis->faded = !e->get_axis_rot();

        struct tms_sprite *spr = 0;
        if ((spr = e->get_axis_rot_sprite())) {
            this->wdg_axis->s[0] = spr;
        } else {
            this->wdg_axis->s[0] = gui_spritesheet::get_sprite(S_AXIS);
        }

        const char *new_tooltip = e->get_axis_rot_tooltip();
        if (new_tooltip) {
            this->wdg_axis->set_tooltip(new_tooltip);
        } else {
            this->wdg_axis->set_tooltip("Toggle axis rotation");
        }
    }
}

/* REFRESH WIDGETS */
void
game::refresh_widgets()
{
    this->wm->remove_all();

    if (settings["render_gui"]->is_false()) {
        return;
    }

#ifdef SCREENSHOT_BUILD
    return;
#endif

    this->refresh_info_label();

    if (!this->state.submitted_score && this->state.finished && W->level.flag_active(LVL_ALLOW_HIGH_SCORE_SUBMISSIONS)
            && !W->level.flag_active(LVL_DISABLE_ENDSCREENS)) {
        if (W->level_id_type == LEVEL_DB) {
            this->wdg_submit_score->add();
        } else {
            this->wdg_unable_to_submit_score->add();
        }
    }

    if (this->get_mode() == GAME_MODE_EDIT_PANEL) {
        this->wdg_close_panel_edit->add();

        if (this->panel_edit_need_scroll) {
            this->wdg_panel_slider_knob->add();
        }

        this->wm->rearrange();
        return;
    }

    this->wdg_username->add();
    this->wdg_menu->add();

#ifdef TMS_BACKEND_MOBILE
    if (this->state.sandbox) {
        this->wdg_quickadd->add();
    }
#endif

    if (this->get_mode() == GAME_MODE_DRAW && G->brush_layer_inclusion) {
        G->wdg_default_layer->s[0] = gui_spritesheet::get_sprite(S_ILAYER0+tclampi(G->state.edit_layer, 0, 2));
    } else {
        G->wdg_default_layer->s[0] = gui_spritesheet::get_sprite(S_LAYER0+tclampi(G->state.edit_layer, 0, 2));
    }

    if (W->is_paused()) {
        this->wdg_playpause->s[0] = gui_spritesheet::get_sprite(S_PLAY);
    } else {
        this->wdg_playpause->s[0] = gui_spritesheet::get_sprite(S_PAUSE);
        if (this->errors.size()) {
            this->wdg_error->add();
        }
    }

    /* Bottom-left */
    bool ui = W->is_paused() || W->is_adventure();
    bool adventure_playing = W->is_adventure() && !W->is_paused();

    if (W->is_adventure()) {
        if (adventure::is_player_alive()) {
            if (adventure::player->is_attached_to_activator() || (this->current_panel && this->current_panel != adventure::player)) {
                if (settings["touch_controls"]->v.b) {
                    this->wdg_disconnect_from_rc->add();
                }
            }
        }
    }

    if (this->selection.enabled() && this->selection.e && this->selection.e->get_property_entity() && ui) {
        entity *e = this->selection.e->get_property_entity();

        if (e->is_wireless()) {
            this->wdg_freq_up->add();
            this->wdg_freq_down->add();
        }

        /* Info */
        {
            this->wdg_info->add();

            if (adventure_playing) {
                this->wdg_info->area = &this->wm->areas[AREA_TOP_CENTER];
            } else {
                this->wdg_info->area = &this->wm->areas[AREA_BOTTOM_LEFT];
            }
        }

        if (W->is_paused()) {
            if (!e->flag_active(ENTITY_DISABLE_LAYERS) && (this->state.sandbox || !W->level.flag_active(LVL_DISABLE_LAYER_SWITCH))) {
                this->wdg_layer->add();

                this->wdg_layer->s[0] = gui_spritesheet::get_sprite(S_LAYER0+tclampi(e->get_layer(), 0, 2));
                this->wdg_layer->faded = (!this->state.sandbox && W->is_puzzle() && W->level.flag_active(LVL_DISABLE_LAYER_SWITCH));
            }
        } else if (W->is_adventure() && !e->flag_active(ENTITY_DISABLE_LAYERS) && !W->level.flag_active(LVL_DISABLE_LAYER_SWITCH)) {
            this->wdg_layer_down->add();
            this->wdg_layer_up->add();

            this->wdg_layer_down->faded = (e->get_layer() == 0);
            this->wdg_layer_up->faded = (e->get_layer() == 2);
        }

        if (W->is_paused() && this->state.sandbox && !e->flag_active(ENTITY_IS_STATIC)) {
            this->wdg_lock->add();

            if (e->is_locked()) {
                this->wdg_lock->s[0] = gui_spritesheet::get_sprite(S_BTN_LOCK);
            } else {
                this->wdg_lock->s[0] = gui_spritesheet::get_sprite(S_BTN_UNLOCK);
            }
        }

        /* Detach */
        if (((W->is_paused() && this->state.sandbox) || !W->level.flag_active(LVL_DISABLE_CONNECTIONS)) &&
                (e->allow_connections() || e->flag_active(ENTITY_IS_COMPOSABLE)
                    || e->g_id == O_BALL_PIPELINE || e->g_id == O_SHAPE_EXTRUDER
                    || e->g_id == O_OILRIG || e->g_id == O_WEIGHT || e->g_id == O_LADDER
                    || e->g_id == O_LADDER_STEP)) {
            this->wdg_detach->add();

            this->wdg_detach->faded = (e->conn_ll == 0);

            if (adventure_playing) {
                this->wdg_detach->area = &this->wm->areas[AREA_TOP_CENTER];
            } else {
                this->wdg_detach->area = &this->wm->areas[AREA_BOTTOM_LEFT];
            }
        }

        /* Unplug */
        if ((W->is_paused() && G->state.sandbox) || W->cables.size() > 0) {
            if (e->is_edevice()) {
                this->wdg_unplug->add();

                if (adventure_playing) {
                    this->wdg_unplug->area = &this->wm->areas[AREA_TOP_CENTER];
                } else {
                    this->wdg_unplug->area = &this->wm->areas[AREA_BOTTOM_LEFT];
                }

                this->wdg_unplug->faded = !((e->get_edevice())->any_socket_used());
                {
                    if (W->is_puzzle() && !this->state.sandbox) {
                        this->wdg_unplug->faded = true;

                        edevice *ed = this->selection.e->get_edevice();
                        for (int x=0; x<ed->num_s_in; x++) {
                            if (ed->s_in[x].p && (ed->s_in[x].p->is_moveable() || (ed->s_in[x].p->c && ed->s_in[x].p->c->is_moveable()))) {
                                this->wdg_unplug->faded = false;
                                break;
                            }
                        }
                        for (int x=0; x<ed->num_s_out; x++) {
                            if (ed->s_out[x].p && (ed->s_out[x].p->is_moveable() || (ed->s_out[x].p->c && ed->s_out[x].p->c->is_moveable()))) {
                                this->wdg_unplug->faded = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

        /* Axis rotation */
        if (e->flag_active(ENTITY_ALLOW_AXIS_ROT) && (W->is_paused() && this->state.sandbox)) {
            this->wdg_axis->add();

            this->refresh_axis_rot();
        }

        /* Moveable */
        /* FIXME: Damper, Open Pivot and Rubberband can't be moveable right now. fix sometime */
        if (W->is_puzzle() && (W->is_paused() && this->state.sandbox)
                && !(e->g_id == 16 || e->g_id == 69 || e->g_id == 19 || e->g_id == 67 || e->g_id == 95 || e->g_id == 96)
                && e->conn_ll == 0
           ) {
            this->wdg_moveable->add();

            this->wdg_moveable->faded = !e->is_moveable();
        }

        /* Config */
        if ((e->flag_active(ENTITY_HAS_CONFIG) && W->is_paused() && this->state.sandbox) || // paused mode
            (e->flag_active(ENTITY_HAS_INGAME_CONFIG) && !W->is_paused() && W->is_adventure()) // adventure mode
           ) {
            this->wdg_config->add();
        }

        /* Tracker */
        if (e->flag_active(ENTITY_HAS_TRACKER) && ((W->is_paused() && this->state.sandbox) || W->is_adventure())) {
            this->wdg_tracker->add();
        }

        /* CW/CCW */
        if (((W->is_paused() && this->state.sandbox) || W->is_adventure()) && e->is_motor()) {
            this->wdg_cwccw->add();

            motor *m = static_cast<motor*>(e);
            this->wdg_cwccw->s[0] = m->get_dir() ? gui_spritesheet::get_sprite(S_CW) : gui_spritesheet::get_sprite(S_CCW);
        }

        /* Remove */
        if ((W->is_paused() && this->state.sandbox)) {
            this->wdg_remove->add();
        }

        /* Sliders */
        if ((W->is_paused() && G->state.sandbox) || (W->level.type == LCAT_ADVENTURE && false/*replace with an entity specific option where sliders can be changed in-game*/)) {
            uint8_t num_sliders = std::min<uint8_t>(e->num_sliders, ENTITY_MAX_SLIDERS);
            for (int x=0; x<num_sliders; ++x) {
                this->wdg_selection_slider[x]->add();
                this->wdg_selection_slider[x]->value[0] = e->get_slider_value(x);
                this->wdg_selection_slider[x]->snap[0] = e->get_slider_snap(x);
                this->wdg_selection_slider[x]->set_label(e->get_slider_label(x));
                this->wdg_selection_slider[x]->set_tooltip(e->get_slider_tooltip(x));
            }
        }
    } else if (W->is_paused()) {
        switch (this->get_mode()) {
            case GAME_MODE_MULTISEL:
                this->wdg_multisel->add();
                this->wdg_box_select->add();
                this->wdg_multi_help->add();
                if (this->multi.box_select == 0) {
                    this->wdg_follow_connections->add();
                    this->wdg_follow_cables->add();
                    this->wdg_select_through_layers->add();
                }

                this->wdg_additive->add();

                this->wdg_follow_connections->faded = !this->multi.follow_connections;
                this->wdg_follow_cables->faded = !this->multi.follow_cables;
                this->wdg_additive->faded = !this->multi.additive_selection;
                this->wdg_select_through_layers->faded = !this->multi.select_through_layers;
                this->wdg_box_select->faded = (this->multi.box_select == 0);

                if (this->selection.enabled()) {
                    this->wdg_remove->add();
                    this->wdg_config->add();
                    this->wdg_multisel->s[0] = gui_spritesheet::get_sprite(S_EXPORT);
                    this->wdg_multisel->set_tooltip("Export selection");
                } else {
                    if (G->multi.import) {
                        this->wdg_multisel->s[0] = gui_spritesheet::get_sprite(S_CLOSE);
                        this->wdg_multisel->set_tooltip("Unload object");
                    } else {
                        this->wdg_multisel->s[0] = gui_spritesheet::get_sprite(S_IMPORT);
                        this->wdg_multisel->set_tooltip("Load object");
                    }
                }
                break;

            case GAME_MODE_DRAW:
                this->wdg_default_layer->add();
                this->wdg_layer_inclusion->add();

                G->wdg_layer_inclusion->faded = !G->brush_layer_inclusion;

                for (int x=0; x<NUM_TERRAIN_MATERIALS; ++x) {
                    this->wdg_brush_material[x]->add();

                    this->wdg_brush_material[x]->faded = (this->brush_material != x);
                }
                break;

            case GAME_MODE_CONN_EDIT:
                if (this->selection.enabled() && this->selection.c) {
                    this->wdg_remove->add();
                    this->wdg_connstrength->add();

                    /* Only display the conndamping slider when level version is below 1.5 */
                    /* for 1.5 and up, we use the joint friction level property for all pivot connections */
                    if (this->selection.c->type == CONN_PIVOT && W->level.version < LEVEL_VERSION_1_5) {
                        this->wdg_conndamping->add();
                    }

                    // Get/attach data from current connection
                    connection *c = this->selection.c;
                    this->wdg_connstrength->data3 = c;
                    this->wdg_connstrength->value[0] = tclampf(c->max_force / CONN_MAX_FORCE, 0.f, 1.f);

                    this->wdg_conndamping->data3 = c;
                    this->wdg_conndamping->value[0] = tclampf(c->damping / CONN_DAMPING_MAX, 0.f, 1.f);
                }
                break;

            default:
                if (!this->selection.enabled() && this->state.sandbox) {
                    this->wdg_default_layer->add();
                }
                break;
        }
    } else {
        if (adventure_playing) {
            this->wdg_current_tool->add();
        }
    }

    if (((G->state.sandbox && G->state.advanced_mode) || (G->state.test_playing && W->level.type != LCAT_ADVENTURE)) || W->is_paused()) {
        this->wdg_layervis->add();

        switch (G->layer_vis) {
            case 0b001: G->wdg_layervis->s[0] = gui_spritesheet::get_sprite(S_LAYERVIS_1); break;
            case 0b011: G->wdg_layervis->s[0] = gui_spritesheet::get_sprite(S_LAYERVIS_2); break;
            default: case 0b111: G->wdg_layervis->s[0] = gui_spritesheet::get_sprite(S_LAYERVIS_3); break;
        }
    }

    if ((this->state.sandbox || this->state.test_playing || W->is_paused() || W->is_puzzle()) && (W->level.type != LCAT_ADVENTURE || W->is_paused())) {
        this->wdg_playpause->add();
    }

    if (G->state.abo_architect_mode) {
        this->wdg_orthographic->faded = 0;
    } else {
        this->wdg_orthographic->faded = 1;
    }

    if (W->is_paused() && this->state.sandbox && this->state.advanced_mode) {
        this->wdg_orthographic->add();
        this->wdg_mode->add();
    }

    if (!this->state.sandbox && !this->state.test_playing && this->get_mode() != GAME_MODE_INVENTORY
        && W->level.type != LCAT_ADVENTURE) {
        if (W->level.descr_len) {
            this->wdg_help->add();
        }
    }

    if (W->is_paused()) {
        this->wdg_advanced->add();
        if (this->state.advanced_mode) {
            this->wdg_advanced->s[0] = gui_spritesheet::get_sprite(S_ADVUP);
        } else {
            this->wdg_advanced->s[0] = gui_spritesheet::get_sprite(S_ADVDOWN);
        }
    }

    this->wm->rearrange();
}

void
game::refresh_gui(void)
{
    menu_height = _tms.opengl_height;
    menu_xdim = 1.f * _tms.xppcm;
    menu_ydim = 1.f * _tms.yppcm;
    float w = 3.f * menu_xdim;
    menu_scale = w/_menu_width;
    real_menu_width = _tms.xppcm;
    this->set_menu_width(real_menu_width);
    menu_max_width = _tms.window_width * .8f;
    menu_min_width = _tms.xppcm/4.f;
    s_size = menu_xdim;
    b_w = (int)(.5f * menu_xdim);
    b_h = (int)(.5f * menu_ydim);
    b_w_pad = (int)((float)b_w + .1f * menu_xdim);
    b_h_pad = (int)((float)b_h + .1f * menu_ydim);
    b_y = (int)((float)b_h/2.f + .1f * menu_ydim);
    menu_cat_width = this->get_menu_width();

    b_margin_y = .14f * menu_ydim;
    b_margin_x = .14f * menu_xdim;

    this->wm->init_areas();
    this->wm->rearrange();
}

void
game::init_sandbox_menu()
{
    int n = 0;

    for (int y=0; y<of::num_categories; y++){
        int twidth = 512;
        int theight = 256;

        if (of::get_num_objects(y) > 10)
            theight = 512;
        if (of::get_num_objects(y) > 25)
            theight = 1024;

        for (int x = 0; x < of::get_num_objects(y); x++) {
            int ix = x % 5;
            int iy = x / 5;

            menu_objects[n].image.bl.x = SIZE_PER_MENU_ITEM / (float)twidth  * (float)ix;
            menu_objects[n].image.bl.y = SIZE_PER_MENU_ITEM / (float)theight * (float)iy;

            menu_objects[n].image.tr.x = SIZE_PER_MENU_ITEM / (float)twidth  * (float)(ix+1);
            menu_objects[n].image.tr.y = SIZE_PER_MENU_ITEM / (float)theight * (float)(iy+1);

            menu_objects[n].image.width  = SIZE_PER_MENU_ITEM;
            menu_objects[n].image.height = SIZE_PER_MENU_ITEM;

            n++;
        }
    }

    {
        int twidth = 512;
        int theight = 1024;

        for (int x = 0; x < NUM_ITEMS; ++x) {
            int ix = x%8;
            int iy = x/8;

            item_options[x].image.bl = (tvec2){(float)SIZE_PER_OBJECT_ITEM/twidth * (float)ix,     (float)SIZE_PER_OBJECT_ITEM/theight * (float)iy};
            item_options[x].image.tr = (tvec2){(float)SIZE_PER_OBJECT_ITEM/twidth * (float)(ix+1), (float)SIZE_PER_OBJECT_ITEM/theight * (float)(iy+1)};
            item_options[x].image.width  = SIZE_PER_OBJECT_ITEM;
            item_options[x].image.height = SIZE_PER_OBJECT_ITEM;
        }
    }
}

/**
 * Should only be called when we are generating new sandbox menu textures
 * to be bundled in the release version.
 **/
void
game::create_sandbox_menu()
{
#ifdef CREATE_SANDBOX_TEXTURES
    glDisable(GL_BLEND);

    tms_assertf(glGetError() == 0, "VAFAN s 1");

    int n = 0;

    struct tms_camera *cam = tms_camera_alloc();
    cam->near = -1.f;
    cam->far = 1.f;
    cam->owidth = s_size;
    cam->oheight = s_size * (_tms.yppcm / _tms.xppcm);
    tms_camera_set_position(cam, 0.f, 0.f, 0.f);
    tms_camera_set_direction(cam, 0, 0, -1);
    cam->width = 2.f;
    cam->height = 2.f;
    tms_camera_calculate(cam);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);

    tms_assertf(glGetError() == 0, "VAFAN s 2");

    /* back up the ddraw matrices */
    float omv[16], op[16];
    memcpy(omv, this->get_surface()->ddraw->modelview, 16*sizeof(float));
    memcpy(op, this->get_surface()->ddraw->projection, 16*sizeof(float));

    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, 1.f);

    for (int y=0; y<of::num_categories; y++) {
        int twidth = 512;
        int theight = 256;

        if (of::get_num_objects(y) > 10)
            theight = 512;
        if (of::get_num_objects(y) > 25)
            theight = 1024;

        struct tms_fb *fb = tms_fb_alloc(twidth, theight, 0);
        tms_fb_add_texture(fb, GL_RGBA, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
        tms_assertf(glGetError() == 0, "VAFAN s -1");
        tms_fb_enable_depth(fb, GL_DEPTH_COMPONENT16);

        tms_assertf(glGetError() == 0, "VAFAN s 0");

        tms_fb_bind(fb);

        tms_assertf(glGetError() == 0, "VAFAN s BIND");

        glClearColor(0.f, .0f, .0f, 0.f);
        //glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_BLEND);

        glEnable(GL_SCISSOR_TEST);

        int begin_n = n;

        for (int x=0; x<of::get_num_objects(y); x++) {
            int ix = (x % 5)*SIZE_PER_MENU_ITEM;
            int iy = (x / 5)*SIZE_PER_MENU_ITEM;

            glScissor(ix, iy, SIZE_PER_MENU_ITEM, SIZE_PER_MENU_ITEM);
            glViewport(ix, iy, SIZE_PER_MENU_ITEM, SIZE_PER_MENU_ITEM);

            entity *e = menu_objects[n].e;

            char override_path[512];
            snprintf(override_path, 512, "../data-src/override/%u.png", e->g_id);

            if (!file_exists(override_path)) {
                cam->width = 2.0f * 1.f/e->menu_scale;
                cam->height = 2.0f * 1.f/e->menu_scale;
                tms_camera_calculate(cam);

                glColorMask(1,1,1,1);
                glDepthMask(0);
                for (int i=0; i<6; i++) {
                    glActiveTexture(GL_TEXTURE0+i);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                glActiveTexture(GL_TEXTURE0);

                glBindTexture(GL_TEXTURE_2D, 0);

                ix = x%5;
                iy = x/5;

                menu_objects[n].image.bl.x = SIZE_PER_MENU_ITEM / (float)twidth  * (float)ix;
                menu_objects[n].image.bl.y = SIZE_PER_MENU_ITEM / (float)theight * (float)iy;

                menu_objects[n].image.tr.x = SIZE_PER_MENU_ITEM / (float)twidth  * (float)(ix+1);
                menu_objects[n].image.tr.y = SIZE_PER_MENU_ITEM / (float)theight * (float)(iy+1);

                menu_objects[n].image.width  = SIZE_PER_MENU_ITEM;
                menu_objects[n].image.height = SIZE_PER_MENU_ITEM;

                tms_infof("%u (%s) at %.2f/%.2f.", menu_objects[n].e->g_id, menu_objects[n].e->get_name(), VEC2_INLINE(menu_objects[n].image.bl));

                /* object */
                tms_graph_add_entity_with_children(menu_graph, menu_objects[n].e);

                glColorMask(1,1,1,1);
                glDepthMask(1);
                menu_graph->render(cam, this);
                tms_graph_remove_entity_with_children(menu_graph, menu_objects[n].e);
            } else {
                tms_infof("%u (%s) is being overridden", menu_objects[n].e->g_id, menu_objects[n].e->get_name());
            }

            n++;

            tms_assertf(glGetError() == 0, "VAFAN s hej %d", n);
        }

        n = begin_n;

        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, _tms.opengl_width, _tms.opengl_height);

        /* Save all objects to an SDL_Surface. */
        char name[256];
        sprintf(name, "sandbox-menu-%u.png", y);
        SDL_Surface *srf = SDL_CreateRGBSurface(SDL_SWSURFACE, twidth, theight, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
        glReadPixels(0, 0, twidth, theight, GL_RGBA, GL_UNSIGNED_BYTE, srf->pixels);

        for (int x=0; x<of::get_num_objects(y); x++) {
            int ix = (x % 5)*SIZE_PER_MENU_ITEM;
            int iy = (x / 5)*SIZE_PER_MENU_ITEM;

            entity *e = menu_objects[n].e;

            char override_path[512];
            snprintf(override_path, 512, "../data-src/override/%u.png", e->g_id);

            if (file_exists(override_path)) {
                struct tms_texture tex;
                tms_texture_load(&tex, override_path);

                ix = x%5;
                iy = x/5;

                menu_objects[n].image.bl.x = SIZE_PER_MENU_ITEM / (float)twidth  * (float)ix;
                menu_objects[n].image.bl.y = SIZE_PER_MENU_ITEM / (float)theight * (float)iy;

                menu_objects[n].image.tr.x = SIZE_PER_MENU_ITEM / (float)twidth  * (float)(ix+1);
                menu_objects[n].image.tr.y = SIZE_PER_MENU_ITEM / (float)theight * (float)(iy+1);

                menu_objects[n].image.width  = SIZE_PER_MENU_ITEM;
                menu_objects[n].image.height = SIZE_PER_MENU_ITEM;

                SDL_Surface *img_srf = SDL_CreateRGBSurface(SDL_SWSURFACE, SIZE_PER_MENU_ITEM, SIZE_PER_MENU_ITEM, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

                size_t buf_size = tex.width * tex.height * tex.num_channels;

                SDL_Rect src_rect, dst_rect;

                static const int CROP = 10;

                src_rect.x = CROP;
                src_rect.y = CROP;
                src_rect.w = SIZE_PER_MENU_ITEM - (CROP*2);
                src_rect.h = SIZE_PER_MENU_ITEM - (CROP*2);
                dst_rect.x = (SIZE_PER_MENU_ITEM * (float)ix)   + CROP;
                dst_rect.y = (SIZE_PER_MENU_ITEM * (float)(iy)) + CROP;
                dst_rect.w = SIZE_PER_MENU_ITEM - (CROP*2);
                dst_rect.h = SIZE_PER_MENU_ITEM - (CROP*2);

                uint8_t *dst = (uint8_t*)img_srf->pixels;
                uint8_t *src = (uint8_t*)tex.data;

                for (int j=0; j<buf_size; ++j) {
                    *dst++ = *src++;
                }

                SDL_BlitSurface(img_srf, &src_rect, srf, &dst_rect);

                SDL_FreeSurface(img_srf);
            }
            ++ n;
        }

        tms_infof("saved: %s", name);

        /* Save the surface as a PNG file. */
        SDL_SavePNG(srf, name);
        SDL_FreeSurface(srf);

        tms_fb_unbind(fb);
    }

    /* create snapshots of all items */
    {
        int twidth = 512;
        int theight = 1024;

        struct tms_fb *fb = tms_fb_alloc(twidth, theight, 0);
        tms_fb_add_texture(fb, GL_RGBA, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
        tms_assertf(glGetError() == 0, "VAFAN s -1");
        tms_fb_enable_depth(fb, GL_DEPTH_COMPONENT16);

        tms_assertf(glGetError() == 0, "VAFAN s 0");

        tms_fb_bind(fb);

        tms_assertf(glGetError() == 0, "VAFAN s BIND");

        glClearColor(.0f, .0f, .0f, 0.f);
        //glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_BLEND);

        glEnable(GL_SCISSOR_TEST);

        for (int x=0; x<NUM_ITEMS; ++x) {
            entity *e = new item();
            e->properties[0].v.i = x;
            ((item*)e)->set_item_type(x);
            ((item*)e)->recreate_shape();
            e->update();
            int ix = (x % 8)*SIZE_PER_OBJECT_ITEM;
            int iy = (x / 8)*SIZE_PER_OBJECT_ITEM;

            glScissor(ix, iy,  SIZE_PER_OBJECT_ITEM, SIZE_PER_OBJECT_ITEM);
            glViewport(ix, iy, SIZE_PER_OBJECT_ITEM, SIZE_PER_OBJECT_ITEM);

            cam->width = 2.0f * 1.f/e->menu_scale;
            cam->height = 2.0f * 1.f/e->menu_scale;
            tms_camera_calculate(cam);

            glColorMask(1,1,1,1);
            glDepthMask(0);
            for (int i=0; i<6; i++) {
                glActiveTexture(GL_TEXTURE0+i);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            glActiveTexture(GL_TEXTURE0);

            glBindTexture(GL_TEXTURE_2D, 0);

            /* object */
            tms_graph_add_entity_with_children(menu_graph, e);

            ix = x%8;
            iy = x/8;
            item_options[x].image.bl = (tvec2){(float)SIZE_PER_OBJECT_ITEM/twidth * (float)ix,     (float)SIZE_PER_OBJECT_ITEM/theight * (float)iy};
            item_options[x].image.tr = (tvec2){(float)SIZE_PER_OBJECT_ITEM/twidth * (float)(ix+1), (float)SIZE_PER_OBJECT_ITEM/theight * (float)(iy+1)};
            item_options[x].image.width  = SIZE_PER_OBJECT_ITEM;
            item_options[x].image.height = SIZE_PER_OBJECT_ITEM;

            glColorMask(1,1,1,1);
            glDepthMask(1);
            menu_graph->render(cam, this);
            tms_graph_remove_entity_with_children(menu_graph, e);

            tms_assertf(glGetError() == 0, "VAFAN s hej %d", n);

            delete e;
        }

        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, _tms.opengl_width, _tms.opengl_height);

        /* save to file */
        SDL_Surface *srf = SDL_CreateRGBSurface(SDL_SWSURFACE, twidth, theight, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
        glReadPixels(0, 0, twidth, theight, GL_RGBA, GL_UNSIGNED_BYTE, srf->pixels);
        tms_infof("saved: items.png");
        SDL_SavePNG(srf, "items.png");
        SDL_FreeSurface(srf);

        tms_fb_unbind(fb);
    }

    tms_assertf(glGetError() == 0, "VAFAN s 3");

    glColorMask(1,1,1,1);
    glDepthMask(1);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, _tms.opengl_width, _tms.opengl_height);

    tms_ddraw_set_matrices(this->get_surface()->ddraw, omv, op);

    tms_assertf(glGetError() == 0, "VAFAN s 4");
    tms_infof("Sandbox textures generated successfully. Running ./utils/update-sandbox-menu.sh ...");
    system("../utils/update-sandbox-menu.sh");
    tms_infof("OK!");
    exit(0);
#endif
    tms_infof("Creating sandbox menu only supported on Linux in DEBUG mode");
}

void
game::render_gui(void)
{
    if (settings["render_gui"]->is_false()) {
        return;
    }

    int ierr;
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render_gui begin", ierr);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindTexture(GL_TEXTURE_2D, gui_spritesheet::atlas->texture.gl_texture);

    if (!W->is_paused() && !W->is_puzzle()) {
        if (W->level.show_score) {
            char score_str[32];
            sprintf(score_str, "%u", this->get_score());

            this->score_text->set_text(score_str);

            float y = _tms.window_height - this->wm->get_margin_y();

            if (this->wdg_menu->surface) {
                //y = this->wdg_menu->pos.y;
            }

            this->score_text->set_position(_tms.window_width - _tms.xppcm*.25f - b_w_pad, y);

            this->add_text(this->score_text);
        }
    }

    if (this->state.finished) {
        if (!W->level.flag_active(LVL_DISABLE_ENDSCREENS)) {
            if (this->state.success) {
                int stepdiff = W->step_count - this->state.finish_step;
                float time = (stepdiff * WORLD_STEP)/1000000.f * G->get_time_mul();

                float a = 1.f - time*.5f;

                if (W->level.pause_on_finish)
                    a = 1.f;

                if (a > 0) {
                    this->add_text(gui_spritesheet::t_win,
                            _tms.window_width/2.f, _tms.window_height/2.f,
                            true, false,
                            1.f, 1.f, 1.f, a);
                }

                if (!W->level.pause_on_finish && !W->level.flag_active(LVL_DISABLE_CONTINUE_BUTTON) && this->state.pkg) {
                    this->add_text(gui_spritesheet::t_continue, _tms.window_width/2.f, 50.f);
                }
            } else {
                this->add_text(gui_spritesheet::t_lose, _tms.window_width/2.f, _tms.window_height/2.f);
            }
        }
    } else if (this->state.waiting) {
        this->add_text(gui_spritesheet::t_get_ready, _tms.window_width/2.f, _tms.window_height/2.f);
    } else if (adventure::dead) {
        int stepdiff = W->step_count - adventure::death_step;
        float time = (stepdiff * WORLD_STEP)/1000000.f * G->get_time_mul();

        float a = 1.f - time*.5f;

        if (a > 0) {
            this->add_text(gui_spritesheet::t_player_death,
                    _tms.window_width/2.f, _tms.window_height/2.f,
                    true, false,
                    1.f, 1.f, 1.f, a);
        }
    }

    if (this->numfeed_timer > 0.f) {
        this->numfeed_text->set_text(this->numfeed_str);

        this->add_text(this->numfeed_text);

        this->numfeed_text->color.a = tclampf(this->numfeed_timer / .5f, 0.f, 1.f);

        this->numfeed_timer -= _tms.dt;
    }

    switch (this->get_mode()) {
        case GAME_MODE_EDIT_PANEL:
            this->render_panel_edit();
            return;

        case GAME_MODE_FACTORY:
            this->render_factory();
            return;

        case GAME_MODE_REPAIR_STATION:
            this->render_repair_station();
            return;

        case GAME_MODE_EDIT_GEARBOX:
            this->render_gearbox_edit();
            return;

        case GAME_MODE_SELECT_OBJECT:
            /* TODO: indicator that the user needs to select an object */
            return;
    }

    if (W->is_paused()) {
        if (this->selection.enabled())
            this->render_selection_gui();
        else {
            this->render_noselection_gui();
        }
    } else {
        if (W->is_adventure() && this->get_mode() != GAME_MODE_INVENTORY) {
            if (this->selection.enabled()) {
                this->render_selection_gui();
            } else {
                this->render_noselection_gui();
            }

        }
    }

    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, 1.f);

    if (this->state.test_playing && W->is_paused()) {
        this->add_text(gui_spritesheet::t_test_playing_back, _tms.window_width/2.f, _tms.window_height/2.f);
    }

    if (!W->is_paused() && !W->is_puzzle()) {
        if (W->is_adventure() && adventure::player != 0 && this->selection.e == 0) {
            float height     = _tms.yppcm * .2f;
            float bar_height = height - _tms.yppcm * 0.05f;

            float base_y = _tms.window_height - this->wm->get_margin_y();
            /* render hp bar */
            if (!W->level.flag_active(LVL_DISABLE_DAMAGE)) {

                base_y -= height/2.f;

                float w = _tms.xppcm*2.f;
                float hp_percent = adventure::player->get_hp() / adventure::player->get_max_hp();
                float hpw = hp_percent * w;
                float armour_width = adventure::player->get_armour()/adventure::player->get_max_armour()*w;

                float mul = 1.f;
                if (hp_percent < .2f) {
                    mul = tclampf((cos(((double)_tms.last_time/100000.) * ((1.f-hp_percent)) * 4.f) * 2.f + 1.0f), 1.f, 4.f);
                }

                tms_ddraw_set_color(this->get_surface()->ddraw, 0.f, 0.f, 0.f, .75f);
                tms_ddraw_square(this->get_surface()->ddraw,
                        _tms.window_width/2.f,
                        base_y,
                        w + .05f*_tms.xppcm,
                        height
                        );

                /* HP */
                tms_ddraw_set_color(this->get_surface()->ddraw, 1.f*mul, 0.33f*mul, 0.33f*mul, .75f);
                tms_ddraw_square(this->get_surface()->ddraw,
                        _tms.window_width/2.f,
                        base_y,
                        hpw,
                        bar_height
                        );

                /* ARMOR */
                tms_ddraw_set_color(this->get_surface()->ddraw, 0.f, 1.0f, 1.0f, 0.15f);
                tms_ddraw_square(this->get_surface()->ddraw,
                        _tms.window_width/2.f,
                        _tms.window_height - _tms.yppcm/4.f,
                        armour_width,
                        bar_height
                        );

                base_y -= (height/2.f) * 1.25f;
            }

            height     = _tms.yppcm * .1f;
            bar_height = height - _tms.yppcm * 0.05f;

            /* render reload bar */
            robot_parts::weapon *w = adventure::player->get_weapon();
            if (w) {
                base_y -= height/2.f;

                float cd_left = w->get_cooldown_fraction();
                if (cd_left < 0.05f) {
                    cooldown_time -= _tms.dt*3.f;
                } else {
                    cooldown_time = 3.f;
                }
                float a = tclampf(cooldown_time, 0.f, 1.f) * .75f;
                if (a > 0.01f) {
                    float w = _tms.xppcm*2.f;
                    float reload_width = cd_left * w;
                    float hpw = adventure::player->get_hp()/ 100.f*w;
                    float base_height = _tms.yppcm*.05f;

                    tms_ddraw_set_color(this->get_surface()->ddraw, 0.f, 0.f, 0.f, a);
                    tms_ddraw_square(this->get_surface()->ddraw,
                            _tms.window_width/2.f,
                            base_y,
                            w + .05f*_tms.xppcm,
                            height
                            );

                    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, .45f, .0f, a);
                    tms_ddraw_square(this->get_surface()->ddraw,
                            _tms.window_width/2.f,
                            base_y,
                            reload_width,
                            bar_height
                            );
                }
            }

            {
                int8_t num_left = 0, num_right = 0;

                float base_x = _tms.window_width/2.f;
                for (int n=0; n<NUM_BARS; ++n) {
                    if (adventure::bars[n].time > 0.f) {
                        adventure::bars[n].time -= _tms.dt*3.f;

                        float a = tclampf(adventure::bars[n].time, 0.f, 1.f) * .75f;
                        if (a > 0.01f) {
                            float w = _tms.xppcm/16.f;
                            float h = _tms.yppcm*.405f;
                            float x;
                            if (adventure::bars[n].side == BAR_SIDE_LEFT) {
                                x = base_x - (_tms.xppcm*1.1f) - num_left++ * w * 2.f;
                            } else {
                                x = base_x + (_tms.xppcm*1.1f) + num_right++ * w * 2.f;
                            }

                            float bar_h = adventure::bars[n].value * h;

                            tms_ddraw_set_color(this->get_surface()->ddraw, 0.f, 0.f, 0.f, a);
                            tms_ddraw_square(this->get_surface()->ddraw,
                                    x,
                                    _tms.window_height - (_tms.yppcm/4.f) - (_tms.yppcm*0.075f),
                                    w + .05f*_tms.xppcm,
                                    h + .05f*_tms.yppcm
                                    );

                            tvec3 c = adventure::bars[n].color;
                            tms_ddraw_set_color(this->get_surface()->ddraw, c.r, c.g, c.b, a);
                            tms_ddraw_square(this->get_surface()->ddraw,
                                    x,
                                    _tms.window_height - (_tms.yppcm/4.f) - (_tms.yppcm*0.075f),
                                    w,
                                    bar_h
                                    );
                        }
                    }
                }
            }
        }
    }


    if (W->is_paused() && this->state.sandbox) {
        this->render_sandbox_menu();
        tms_assertf((ierr = glGetError()) == 0, "gl error %d after rendering sandbox menu", ierr);
    }

    if (this->get_mode() == GAME_MODE_INVENTORY) {
        this->render_inventory();
        return;
    }

    if (this->active_hori_wdg && this->active_vert_wdg && this->active_hori_wdg->type == TMS_WDG_RADIAL) {
        tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, 1.f);

        tms_ddraw_sprite(this->get_surface()->ddraw, gui_spritesheet::get_sprite(S_RADIAL_KNOB),
                this->wdg_base_x, this->wdg_base_y,
                0.45f * _tms.xppcm, 0.45f * _tms.yppcm);

        static const float MIN_CIRCLE_DIA = _tms.xppcm * 1.0f;
        static const float MAX_CIRCLE_DIA = _tms.xppcm * 3.5f;

        const float dist = tvec2_dist(tvec2f(this->wdg_base_x, this->wdg_base_y), move_pos);
        float circle_dia = dist;
        if (circle_dia < MIN_CIRCLE_DIA) {
            circle_dia = MIN_CIRCLE_DIA;
        } else if (circle_dia > MAX_CIRCLE_DIA) {
            circle_dia = MAX_CIRCLE_DIA;
        }
        const float a = this->active_hori_wdg->value[0] * 2.f * M_PI + (_tms.emulating_portrait ? M_PI/2.f : 0.f);

        tms_ddraw_set_color(this->get_surface()->ddraw, 0.2f, 0.2f, 0.2f, 0.4f);

        tms_ddraw_circle(this->get_surface()->ddraw,
                this->wdg_base_x, this->wdg_base_y,
                circle_dia, circle_dia);

        tms_ddraw_set_color(this->get_surface()->ddraw, 0.f, 0.f, 0.f, 1.f);

        tms_ddraw_lcircle(this->get_surface()->ddraw,
                this->wdg_base_x, this->wdg_base_y,
                circle_dia, circle_dia);

        float sn, cs;
        tmath_sincos(a, &sn, &cs);

        tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, 0.9f);

        const float knob_x = this->wdg_base_x + circle_dia * cs;
        const float knob_y = this->wdg_base_y + circle_dia * sn;

        tms_ddraw_sprite(this->get_surface()->ddraw, gui_spritesheet::get_sprite(S_RADIAL_KNOB),
                knob_x,
                knob_y,
                0.30f * _tms.xppcm, 0.30f * _tms.yppcm);

        tms_ddraw_line(this->get_surface()->ddraw,
                this->wdg_base_x, this->wdg_base_y,
                knob_x, knob_y);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render_gui end", ierr);
}

int
game::get_menu_width()
{
    return _menu_width;
}

void
game::set_menu_width(int new_menu_width)
{
    _menu_width = new_menu_width;

    if (this->wm) {
        this->wm->rearrange();
    }
}

void
game::render_edev_labels()
{
    if (this->get_mode() == GAME_MODE_SELECT_SOCKET)
        return;
    if (this->get_mode() == GAME_MODE_EDIT_PANEL)
        return;
    if (this->get_mode() == GAME_MODE_FACTORY)
        return;
    if (this->get_mode() == GAME_MODE_REPAIR_STATION)
        return;

    float alpha = 1.f;

    if (this->cam->_position.z > 13.f) {
        alpha = 1.f - (this->cam->_position.z - 13.f) / 2.f;
    }

    if (this->cam->_position.z > 15.f) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, this->texts->texture.gl_texture);
    float mv[16];

    for (std::vector<edevice*>::iterator i = W->electronics.begin();
            i != W->electronics.end();
            i++) {
        edevice *ed = (*i);
        entity *e = ed->get_entity();
        int last_layer = -1;
        if (e && e->get_property_entity() && e->g_id != O_RECEIVER && e->g_id != O_MINI_TRANSMITTER && e->g_id != O_JUMPER && !tms_graph_is_entity_culled(this->graph, e)) {
            e = e->get_property_entity();
            struct menu_obj o = menu_objects[gid_to_menu_pos[e->g_id]];

            float x = e->get_position().x;
            float y = e->get_position().y;

            tvec3 dd = tms_camera_project(this->cam, this->cam->_position.x, this->cam->_position.y,e->get_layer()*LAYER_DEPTH + LAYER_DEPTH/2.f);
            tvec3 v1 = tms_camera_unproject(this->cam, 0.f, 0.f, dd.z);
            tvec3 v2 = tms_camera_unproject(this->cam, 0.f, _tms.yppcm*.2f, dd.z);
            float th = (v2.y-v1.y)*.7f;
            float w = o.name->width / o.name->height * th;

            if (e->get_layer() != last_layer) {
                tmat4_copy(mv, this->cam->view);
                tmat4_translate(mv, 0, 0, e->get_layer()*LAYER_DEPTH + LAYER_DEPTH/2.f);
                tms_ddraw_set_matrices(this->dd, mv, this->cam->projection);
            }

            //tms_ddraw_set_color(this->dd, 0.f, 0.f, 0.f, .2f * (.5f * cos((double)_tms.last_time/100000.)));
            tms_ddraw_set_color(this->dd, 0.f, 0.f, 0.f, .5f*alpha);
            tms_ddraw_square(this->dd, x, y+.125f, w+.1f, th+.1f);

            tms_ddraw_set_color(this->dd, 1.f, 1.f, 1.f, (.9f + .1f * cos((double)_tms.last_time/100000.))*alpha);
            //tms_ddraw_set_color(this->dd, 1.f, 1.f, 1.f, .5f + .50f * cos((double)_tms.last_time/100000.));

            if (o.name) {
                tms_ddraw_sprite(this->dd,
                    o.name, x, y+.125f, w, th);
            }

            last_layer = e->get_layer();
        }
    }
    tmat4_copy(mv, this->cam->view);
    tms_ddraw_set_matrices(this->dd, mv, this->cam->projection);
}

struct tms_texture*
game::get_item_texture()
{
    static struct tms_texture *item_texture;

    if (!item_texture) {
        item_texture = tms_texture_alloc();

        tms_texture_load(item_texture, "data/textures/menu_items.png");
        tms_texture_flip_y(item_texture);
        tms_texture_set_filtering(item_texture, GL_LINEAR);
        tms_texture_upload(item_texture);
        tms_texture_free_buffer(item_texture);
    }

    return item_texture;
}

struct tms_texture*
game::get_sandbox_texture(int n)
{
    static struct tms_texture *sandbox_texture[10]; /* XXX keep in sync with of::num_categories */

    /* make sure the current category's texture is loaded */
    if (!sandbox_texture[n]) {
        sandbox_texture[n] = tms_texture_alloc();
        char name[256];

        sprintf(name, "data/textures/sandbox-menu-%d.png", n);
        tms_texture_load(sandbox_texture[n], name);
        tms_texture_flip_y(sandbox_texture[n]);

        tms_texture_set_filtering(sandbox_texture[n], GL_LINEAR);
        tms_texture_upload(sandbox_texture[n]);
        tms_texture_free_buffer(sandbox_texture[n]);
    }

    return sandbox_texture[n];
}

void
game::render_sandbox_menu()
{
    int ierr;
    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render_sandbox_menu begin", ierr);

    struct tms_texture *tex = this->get_sandbox_texture(curr_category);

    if (this->get_menu_width() < menu_min_width) this->set_menu_width(menu_min_width);
    if (this->get_menu_width() > menu_max_width) this->set_menu_width(menu_max_width);

    if (_tms.dt > 0.00001f && _tms.dt < 1.f) {
        //menu_cam_vel *= powf(_tms.dt, CAM_DAMPING);
        menu_cam_vel *= powf(CAM_DAMPING, _tms.dt);
        menu_cam->_position.y += menu_cam_vel;

        if (!settings["smooth_menu"]->v.b) {
            menu_cam_vel = 0.f;
        }
    }

    if (menu_cam->_position.y < 0.f) menu_cam->_position.y *= .8f;

    menu_cam->width = this->get_menu_width();
    menu_cam->height = _tms.window_height;
    menu_cam->owidth = this->get_menu_width();
    menu_cam->oheight = _tms.window_height;
    menu_cam->calculate();

    float x1 = _tms.window_width - this->get_menu_width();

    tvec3 menu_white = MENU_WHITE_F;
    tvec3 menu_black = MENU_BLACK_F;
    tvec3 menu_gray = MENU_GRAY_F;

    tms_assertf((ierr = glGetError()) == 0, "gl error %d before doing menu highlight stuff", ierr);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(1);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);
    glScissor(_tms.window_width - this->get_menu_width(), _tms.yppcm/2.f+catsprites[of::num_categories]->height/2.f, this->get_menu_width(), menu_height - _tms.yppcm/2.f+catsprites[of::num_categories]->height);
#define HL_MUL .1f
    glClearColor(menu_black.r+menu_highlight*HL_MUL, menu_black.g+menu_highlight*HL_MUL, menu_black.b+menu_highlight*HL_MUL, 1.f);
#undef HL_MUL
    menu_highlight *= .8f;
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    tms_assertf((ierr = glGetError()) == 0, "gl error %d after doing menu highlight stuff", ierr);

    //menu_width ++;
    //

    float menu_tint = .4f + (.6f * (1.f-category_selector_alpha));

    tms_ddraw_set_color(this->get_surface()->ddraw, TVEC3_INLINE_M(menu_white, menu_tint), 1.0f);

    float btn_outer_x = _tms.xppcm * 1.1f;
    float btn_outer_y = _tms.yppcm * 1.1f;
    float top = -_tms.yppcm/4.f + (_tms.window_height - btn_outer_y/2.f);

    int xx = roundf((this->get_menu_width()) / btn_outer_x);
    if (xx < 1) xx = 1;
    int yy = floor((_tms.window_height-btn_outer_y) / btn_outer_y);

    float a_x = 0.f;
    float a_y = 0.f;
    int ww = 0;

    float pp = top;

    float cy = menu_cam->_position.y;
    if (cy > (btn_outer_y*yy)) {
        //cy -= (btn_outer_y*(xx)*yy);
    }

    int div = ceil((float)menu_objects_cat[curr_category].size()/((float)xx));

    if (div < yy) div = yy;

    float max_y = 0.f;

    tms_assertf((ierr = glGetError()) == 0, "gl error %d before rendering menu objects", ierr);
    glBindTexture(GL_TEXTURE_2D, tex->gl_texture);
    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, menu_tint);
    for (int x=0; x<menu_objects_cat[curr_category].size(); x++) {
        struct menu_obj o = menu_objects[menu_objects_cat[curr_category][x]];

        if (x != 0 && (x%div) == 0) {
            a_x += btn_outer_x;
            pp = top;
            ww++;
        }

        if (o.highlighted) {
            tms_ddraw_set_color(this->get_surface()->ddraw, 2.f, 2.f, 2.f, 1.f);
        }
        tms_ddraw_sprite(this->get_surface()->ddraw, &o.image,
                x1 + a_x +btn_outer_x/2.f,
                cy + pp,
                btn_outer_x, btn_outer_y);
        if (o.highlighted) {
            tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, menu_tint);
        }

        pp -= btn_outer_y;
        if (std::abs(pp-top) > max_y) max_y = std::abs(pp-top);
    }
    tms_assertf((ierr = glGetError()) == 0, "gl error %d after rendering menu objects", ierr);

    //tms_infof("cam y %f max y %f", menu_cam->_position.y, max_y);

    /* render the object names */
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, this->texts->texture.gl_texture);
    tms_ddraw_set_color(this->get_surface()->ddraw, 1.0f, 1.0f, 1.f, 1.f);
#if 0
    float xxx = 0.f;
    for (int x=0; x<of::num_categories; x++) {
        if (x == 1)
            tms_ddraw_set_color(this->get_surface()->ddraw, 0.5f, 0.5f, 0.5f, 1.f);

        int c = (curr_category+x)%of::num_categories;

        float h = ch * .8f;
        float w = (h / catsprites[c]->height) * catsprites[c]->width;

        tms_ddraw_sprite(this->get_surface()->ddraw, catsprites[c],
                x1+w/2.f + _tms.xppcm * .1f + xxx,
                _tms.window_height-ch/2.f,
                w, h);

        xxx += w + _tms.xppcm*.125f;
    }
#endif
    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f*menu_tint, 1.f*menu_tint, 1.f*menu_tint, 1.f);

    pp = top;
    a_x = 0.f;
    a_y = 0.f;
    ww = 0;
    for (int x=0; x<menu_objects_cat[curr_category].size(); x++) {
        struct menu_obj o = menu_objects[menu_objects_cat[curr_category][x]];

        if (x != 0 && (x%div) == 0) {
            a_x += btn_outer_x;
            pp = top;
            ww++;
        }

        if (o.e->flag_active(ENTITY_IS_BETA)) {
            /* render beta label */
            tms_ddraw_set_color(this->get_surface()->ddraw, 1.f*menu_tint, 1.f*menu_tint, 0.2f*menu_tint, 1.f);
            tms_ddraw_sprite(this->get_surface()->ddraw,
                betasprite,
                x1 + a_x +btn_outer_x/2.f,
                cy+pp - betasprite->height/2.f,
                //roundf(p.x), roundf(p.y-30),
                betasprite->width*.8, betasprite->height*.8);
            tms_ddraw_set_color(this->get_surface()->ddraw, 1.f*menu_tint, 1.f*menu_tint, 1.0f*menu_tint, 1.f);
        }

        if (o.e->flag_active(ENTITY_IS_DEV)) {
            // render dev label
            tms_ddraw_set_color(this->get_surface()->ddraw, 1.f*menu_tint, 0.2f*menu_tint, 0.2f*menu_tint, 1.f);
            tms_ddraw_sprite(this->get_surface()->ddraw,
                devsprite,
                x1 + a_x +btn_outer_x/2.f,
                cy+pp - devsprite->height/2.f,
                //roundf(p.x), roundf(p.y-30),
                devsprite->width*.8, devsprite->height*.8);
            tms_ddraw_set_color(this->get_surface()->ddraw, 1.f*menu_tint, 1.f*menu_tint, 1.0f*menu_tint, 1.f);
        }

        tms_ddraw_sprite(this->get_surface()->ddraw,
            o.name,
            x1 + a_x +btn_outer_x/2.f,
            cy+pp - _tms.yppcm/2.5f,
            //roundf(p.x), roundf(p.y-30),
            o.name->width*.8, o.name->height*.8);
            //s_size,//o.name->width*scale,
            //(o.name->height/o.name->width) * (s_size*(_tms.yppcm/_tms.xppcm)));
            //o.name->height*scale);
        pp -= btn_outer_y;
    }

    /* render category label */
    tms_ddraw_set_color(this->get_surface()->ddraw, TVEC3_INLINE_M(menu_white, menu_tint), 1.f);
    float ch = _tms.yppcm * .25f;
    float cw = (ch / catsprites[curr_category]->height) * catsprites[curr_category]->width;
    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width - this->get_menu_width()/2.f,
            _tms.window_height - ch/2.f,
            this->get_menu_width(),
            ch
            );

    tms_ddraw_set_color(this->get_surface()->ddraw, TVEC3_INLINE_M(menu_black, menu_tint), 1.f);
    {
        int c = curr_category;

        float h = ch * .8f;
        float w = (h / catsprites[c]->height) * catsprites[c]->width;

        glBindTexture(GL_TEXTURE_2D, gui_spritesheet::atlas->texture.gl_texture);
        tms_ddraw_set_color(this->get_surface()->ddraw, TVEC3_INLINE(menu_white), 1.f);
        tms_ddraw_sprite(this->get_surface()->ddraw, gui_spritesheet::get_sprite(S_DROPDOWN),
                x1 + h/1.5f,
                _tms.window_height-ch/2.f,
                h*1.55f, h*1.45f);
        glBindTexture(GL_TEXTURE_2D, this->texts->texture.gl_texture);

        tms_ddraw_set_color(this->get_surface()->ddraw, TVEC3_INLINE(menu_black), 1.f);
        tms_ddraw_sprite(this->get_surface()->ddraw, catsprites[c],
                x1 + h + w/2.f + _tms.xppcm * .1f,
                _tms.window_height-ch/2.f,
                w, h);
    }

    glEnable(GL_BLEND);
    glScissor(_tms.window_width - this->get_menu_width(), 0, this->get_menu_width(), _tms.yppcm/2.f+ch);
    glClearColor(menu_black.r, menu_black.g, menu_black.b, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f*menu_tint, 1.f*menu_tint, 1.f*menu_tint, 1.f);

    int last_cat = -1;
    for (int x=0; x<MAX_RECENT; x++) {
        int p = this->recent[x];

        if (p == -1)
            continue;

        struct menu_obj o = menu_objects[gid_to_menu_pos[p]];

        if (o.category != last_cat) {
            last_cat = o.category;
            glBindTexture(GL_TEXTURE_2D, this->get_sandbox_texture(last_cat)->gl_texture);
        }

        tms_ddraw_sprite(this->get_surface()->ddraw, &o.image,
                x1 + btn_outer_x/4.f + (btn_outer_x*x) / 2.f,
                btn_outer_y/4.f,
                btn_outer_x/2.f,
                btn_outer_y/2.f);
    }

    /* render Recent label */
    glBindTexture(GL_TEXTURE_2D, this->texts->texture.gl_texture);
    tms_ddraw_set_color(this->get_surface()->ddraw, TVEC3_INLINE_M(menu_white, menu_tint), 1.f);
    ch = _tms.yppcm * .25f;
    cw = (ch / catsprites[of::num_categories]->height) * catsprites[of::num_categories]->width;
    float h = ch * .8f;
    float w = (h / catsprites[of::num_categories]->height) * catsprites[of::num_categories]->width;
    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width - this->get_menu_width()/2.f,
            _tms.yppcm/2.f+ch/2.f,
            this->get_menu_width(),
            ch
            );
    tms_ddraw_set_color(this->get_surface()->ddraw, TVEC3_INLINE(menu_black), 1.f);
    tms_ddraw_sprite(this->get_surface()->ddraw, catsprites[of::num_categories],
            x1+cw/2.f + _tms.xppcm * .1f,
            _tms.yppcm/2.f+ch/2.f,
            w,
            h
            );

    glDisable(GL_SCISSOR_TEST);

    bool resizing = false;

    for (int x=0; x<MAX_P; x++)
        if (resizing_menu[x])
            resizing = true;

    tms_ddraw_set_color(this->get_surface()->ddraw, menu_gray.r+resizing*.2f, menu_gray.g+resizing*.2f, menu_gray.b+resizing*.2f, 1.f);

    /* render resize thing */
    tms_ddraw_square(this->get_surface()->ddraw,
            _tms.window_width - this->get_menu_width(),
            _tms.window_height / 2.f,
            _tms.xppcm*.1f,
            _tms.window_height*.667f
            );

    max_y -= _tms.window_height-btn_outer_y-80;
    if (menu_cam->_position.y > max_y) {
        menu_cam->_position.y -= max_y;
        menu_cam->_position.y *= .80f;
        menu_cam->_position.y += max_y;
    }

    if (!resizing) {
        if (this->get_menu_width() > _tms.xppcm/2.f) {
            float ww = (float)this->get_menu_width() / (float)btn_outer_x;
            ww = roundf(ww) * btn_outer_x;

            float dt = .1f;
            this->set_menu_width((int)((float)this->get_menu_width() * (1.f-dt) + (ww*(dt))));
        }
    }

    if (category_selector_open) {
        category_selector_alpha += _tms.dt*6.f;
        if(category_selector_alpha > 1.f) category_selector_alpha = 1.f;
    }

    glBindTexture(GL_TEXTURE_2D, this->texts->texture.gl_texture);
    if (category_selector_alpha > 0.f) {
        tms_ddraw_set_color(this->get_surface()->ddraw, TVEC3_INLINE(menu_white), 1.f*category_selector_alpha);

        float ch = _tms.yppcm * .35f;
        float h = ch;
        float w = (h / tallest_catsprite) * widest_catsprite;

        float height = h * of::num_categories;
        float width = w;

        float x1 = std::max((float)category_selector_x, _tms.window_width - this->get_menu_width() + width/2.f);
        if (x1 > _tms.window_width - width/2.f)
            x1 = _tms.window_width - width/2.f;
        float y1 = 0.f;

        tms_ddraw_square(this->get_surface()->ddraw,
                x1,
                _tms.window_height - height/2.f,
                width,
                height
                );

        x1 -= width/2.f;

        for (int x=0; x<of::num_categories; x++) {
            float cw = (ch / catsprites[curr_category]->height) * catsprites[curr_category]->width;

            int c = x;

            float h = ch * .8;
            float w = (h / catsprites[c]->height) * catsprites[c]->width;
            tms_ddraw_set_color(this->get_surface()->ddraw, TVEC3_INLINE(menu_black), 1.f*category_selector_alpha);
            tms_ddraw_sprite(this->get_surface()->ddraw, catsprites[c],
                    x1+w/2.f + _tms.xppcm * .1f,
                    _tms.window_height-ch/2.f + y1,
                    w, h);

            y1 -= ch;
        }

        if (!category_selector_open) {
            category_selector_alpha -= _tms.dt*6.f;
            if (category_selector_alpha < 0.f) category_selector_alpha = 0.f;
        }
    }

    tms_assertf((ierr = glGetError()) == 0, "gl error %d in game::render_sandbox_menu end", ierr);
}

void
game::render_noselection_gui(void)
{
    float sx = get_bmenu_x();
    float px = 0*b_w_pad;

    /*
    // XXX: If this is needed, we will use the widgetmanager for it.
    if (!this->state.sandbox) {
        if (W->is_adventure()) {
            if (adventure::player && adventure::is_player_alive()) {
                robot_parts::tool *t = adventure::player->get_tool();
                if (t && t->flag_active(ENTITY_HAS_CONFIG)) {
                    float adv_sx = _tms.window_width / 2.f - (1 * b_w_pad) / 2.f +b_w_pad/2.f;
                    float adv_sy = _tms.window_height - get_bmenu_y() - b_h_pad;

                    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, 1.f);
                    tms_ddraw_sprite(this->get_surface()->ddraw, gui_spritesheet::get_sprite(S_CONFIG), adv_sx, adv_sy, b_w, b_h);
                }
            }
        }
    }
    */
}

void
game::handle_ingame_object_button(int button_id)
{

    if (!this->selection.e) return;
    /* we might receive the button from somewhere else other than the widget (like a key press),
     * so we have to validate that the button is actually usable.
     * this is handled on a per-button basis */

    switch (button_id) {
        case GW_LAYER_UP:
            if (this->wdg_layer_up->surface) {
                if (this->selection.e->get_layer()<2 && this->ingame_layerswitch_test(this->selection.e, 1)) {
                    this->selection.e->set_layer((this->selection.e->get_layer()+1));
                } else {
                    tms_infof("layerswitch not allowed");
                }
            }
            break;

        case GW_LAYER_DOWN:
            if (this->wdg_layer_down->surface) {
                if (this->selection.e->get_layer()>0 && this->ingame_layerswitch_test(this->selection.e, -1)) {
                    this->selection.e->set_layer((this->selection.e->get_layer()-1));
                } else {
                    tms_infof("layerswitch not allowed");
                }
            }
            break;

        case GW_CONFIG:
            if (this->wdg_config->surface) {
                if (this->selection.e->flag_active(ENTITY_HAS_INGAME_CONFIG)) {
                    if (this->selection.e->flag_active(ENTITY_IS_CONTROL_PANEL)) {
                        this->set_mode(GAME_MODE_EDIT_PANEL);
                    } else {
                        switch (this->selection.e->g_id) {
                            case O_FACTORY:
                                this->set_mode(GAME_MODE_FACTORY);
                                tms_infof("clicked factory config");
                                break;
                        }
                    }
                }
            }
            break;
    }

    G->refresh_widgets();
}

void
game::render_selection_gui(void)
{
}

void
game::render_inventory(void)
{
    if (!W->is_adventure()) {
        this->set_mode(GAME_MODE_DEFAULT);
        return;
    }

    int iw = _tms.xppcm*.375f;
    int ih = _tms.yppcm*.375f;

    tms_ddraw_set_color(this->get_surface()->ddraw, .1f, 0.1f, .1f, 1.f);
    tms_ddraw_square(this->get_surface()->ddraw, iw*6.f, _tms.window_height/2.f,
            iw/2.f, _tms.window_height);

    tms_ddraw_set_color(this->get_surface()->ddraw, .2f, .2f, .2f, 1.f);
    tms_ddraw_square(this->get_surface()->ddraw, iw*3.f, _tms.window_height/2.f,
            iw*6.f, _tms.window_height);

    float x = this->get_bmenu_x();
    float y = _tms.window_height - this->get_bmenu_y() - this->inventory_scroll_offset;
    int j = 0;
    for (int n=0; n<NUM_RESOURCES; ++n) {
        if (adventure::player->get_num_resources(n)) {
            if (j++%2==0) {
                tms_ddraw_set_color(this->get_surface()->ddraw, .3f, .3f, .3f, 0.3f);
            } else {
                tms_ddraw_set_color(this->get_surface()->ddraw, .4f, .4f, .4f, 0.3f);
            }
            tms_ddraw_square(this->get_surface()->ddraw, iw*2.f, y,
                    iw*3.f, ih*1.5f);

            this->render_num(x, y, iw, ih, adventure::player->get_num_resources(n), 0, 0.f, false);
            y -= ih*1.5f;
        }
    }

    float base_y = _tms.window_height - this->get_bmenu_y() - this->inventory_scroll_offset;
    double cp = cos((double)_tms.last_time/200000.);

    if (base_y > _tms.window_height) {
        tms_ddraw_set_color(this->get_surface()->ddraw, .7f+cp, .7f+cp, .7f+cp, .35f+(cp*.2f));
        tms_ddraw_circle(this->get_surface()->ddraw, iw*3.f, _tms.window_height+ih,
                iw*4.f, ih*1.3f);
    }

    if (this->inventory_highest_y < -(ih*1.75f)) {
        tms_ddraw_set_color(this->get_surface()->ddraw, .7f+cp, .7f+cp, .7f+cp, .35f+(cp*.2f));
        tms_ddraw_circle(this->get_surface()->ddraw, iw*3.f, -ih,
                iw*4.f, ih*1.3f);
    }
}

int
game::inventory_handle_event(tms::event *ev)
{
    int iw = _tms.xppcm*.375f;
    int ih = _tms.yppcm*.375f;

    float max_x = iw * 5.f;

    int pid = ev->data.motion.pointer_id;
    tvec2 sp = (tvec2){ev->data.motion.x, ev->data.motion.y};

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
            {
                if (sp.x < iw*6.f) {
                    float diff = ev->data.scroll.y * 15.f;
                    if (this->inventory_highest_y > 0.f && diff < 0.f) {
                        return EVENT_DONE;
                    }
                    this->inventory_scroll_offset += diff;

                    if (this->inventory_scroll_offset > 0.f) {
                        this->inventory_scroll_offset = 0.f;
                    }

                    this->refresh_inventory_widgets();

                    return EVENT_DONE;
                }
            }
            break;

        case TMS_EV_POINTER_DOWN:
            {
                if (sp.x < iw*6.f) {
                    tdown_p[pid] = (tvec2){sp.x, sp.y};

                    return EVENT_DONE;
                } else {
                    this->set_mode(GAME_MODE_DEFAULT);
                    return EVENT_DONE;
                }
            }
            break;

        case TMS_EV_POINTER_UP:
            {
                tdown_p[pid] = (tvec2){0,0};
                if (sp.x < iw*6.f) {
                    return EVENT_DONE;
                }
            }
            break;

        case TMS_EV_POINTER_DRAG:
            {
                if (sliding_menu[pid]) {
                    float diff = tdown_p[pid].y - sp.y;
                    if (this->inventory_highest_y > -this->get_bmenu_y()/2.f && diff < 0.f) {
                        return EVENT_DONE;
                    }
                    this->inventory_scroll_offset += diff;

                    if (this->inventory_scroll_offset > 0.f) {
                        this->inventory_scroll_offset = 0.f;
                    }

                    this->refresh_inventory_widgets();
                    tdown_p[pid].y = sp.y;

                    return EVENT_DONE;
                } else {
                    if (sp.x < iw*6.f) {
                        //if (std::abs(tdown_p[pid].y - sp.y) > _tms.xppcm*.4f) {
                            sliding_menu[pid] = true;
                        //}
                        return EVENT_DONE;
                    }
                }
            }
            break;
    }

    return EVENT_CONT;
}

void
game::render_num(float x, float y, int iw, int ih, float num, int precision/*=2*/, float extra_scale/*=0.f*/, bool render_background/*=true*/)
{
    /* FIXME: add a scale argument to add_text */
    this->add_text(num, font::small, x+iw, y, TV_WHITE, precision);
}

void
game::begin_tracker(entity *e)
{
    if (e) {
        switch (e->g_id) {
            case O_ESCRIPT:
                ui::message("Click an object to get some information about it.");
                break;

            case O_RC_ACTIVATOR:
                ui::message("Select the RC to connect to.\nSelecting the RC activator object itself will make it deactivate RCs instead.");
                break;

            case O_PLAYER_ACTIVATOR:
                ui::message("Select a creature.");
                break;

            case O_CAM_TARGETER:
                ui::message("Select which object to target");
                break;

            case O_OBJECT_FIELD:
                ui::message("Select the object type which the Object field should detect.");
                break;

            case O_ID_FIELD:
                ui::message("Select the object which the ID field should detect.");
                break;

            case O_TARGET_SETTER:
                ui::message("Select which object the target setter should set.");
                break;

            case O_MINI_EMITTER:
            case O_EMITTER:
                ui::message("Click on an object to emit identical copies.");
                break;

            case O_MINI_ABSORBER:
            case O_ABSORBER:
                ui::message("Select which object type to absorb.");
                break;

            case O_OBJECT_FINDER:
                ui::message("Click on an object to track it.");
                break;

            case O_HP_CONTROL:
                ui::message("Select which robot the HP controller should control.");
                break;

            case O_ROBOTMAN:
                ui::message("Select which robot the Robot Manager should manage.");
                break;

            case O_VENDOR:
                ui::message("Select what currency to accept.");
                break;
        }
    }

    this->set_mode(GAME_MODE_SELECT_OBJECT);
    this->selection.save();
    this->selection.disable();
}
