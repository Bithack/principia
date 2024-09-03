#include "adventure.hh"
#include "game.hh"
#include <tms/bindings/cpp/cpp.hh>
#include "settings.hh"
#include "ui.hh"
#include "backpack.hh"
#include "checkpoint.hh"
#include "object_factory.hh"
#include "panel.hh"
#include "tpixel.hh"
#include "fxemitter.hh"
#include "robot.hh"
#include "widget_manager.hh"
#include "factory.hh"
#include "item.hh"
#include "misc.hh"
#include "plant.hh"
#include "gui.hh"

#define MINING_INTERVAL 75000
#define MINING_DAMAGE   1.f

static discharge_effect *mining_discharge = 0;

tms_wdg *adventure::w_move_slider = 0;
tms_wdg *adventure::w_layer_up = 0;
tms_wdg *adventure::w_layer_down = 0;
tms_wdg *adventure::w_aim = 0;
tms_wdg *adventure::w_action = 0;
tms_wdg *adventure::w_jump = 0;

int adventure::num_weapons = 0;
int adventure::num_tools = 0;

tvec2 adventure::weapon_icon_pos[NUM_WEAPONS];
tvec2 adventure::tool_icon_pos[NUM_WEAPONS];

static bool slider_prev_focused = false;
static bool aim_prev_focused = false;

static int inventory_begin_y = 0;
static int icon_width = 0;
static int icon_height = 0;
static float margin_x = 0;
static float margin_y = 0;

bool adventure::dead = false;
uint32_t adventure::death_step = 0;
int64_t adventure::death_wait = 0;

enum {
    ZAP_TYPE_NONE    = -1,
    ZAP_TYPE_TERRAIN = 0,
    ZAP_TYPE_JUNK    = 1,
    ZAP_TYPE_PLANT   = 2,

    NUM_ZAP_TYPES
};

bool adventure::kill_player = false;
int adventure::kill_player_counter = 0;

uint64_t adventure::mining_timer = 0;
bool adventure::mining = false;
bool adventure::first_mine = true;
int  adventure::mining_zap_type = ZAP_TYPE_NONE;
b2Vec2 adventure::mining_pos;
int adventure::pending_layermove;
int adventure::layermove_attempts;
int adventure::last_mouse_x;
int adventure::last_mouse_y;
factory *adventure::current_factory = 0;
float adventure::highlight_inventory[NUM_RESOURCES];
int adventure::last_picked_up_resource = RESOURCE_WOOD;
joint_info *adventure::ji = new joint_info(JOINT_TYPE_BACKPACK, 0);
bool adventure::key_down[ADVENTURE_NUM_FOCUSED];
p_text *adventure::current_tool = 0;
bar adventure::bars[NUM_BARS] = {
    { 0.f, 0.f, tvec3f(1.f, 1.f, 1.f), BAR_SIDE_LEFT },
    { 0.f, 0.f, tvec3f(1.f, 1.f, 1.f), BAR_SIDE_RIGHT },
    { 0.f, 0.f, tvec3f(1.f, 1.f, 1.f), BAR_SIDE_LEFT },
};
creature *adventure::player = 0;
bool adventure::widgets_active = false;

enum input_action {
    IA_MOVE_LEFT,
    IA_MOVE_RIGHT,
    IA_ACTION,
    IA_JUMP,
    IA_DROP_INTERACTING,
    IA_LAYER_UP,
    IA_LAYER_DOWN,
    IA_DETACH,
    IA_ATTACK,
    IA_AIM_UP,
    IA_AIM_DOWN,
    IA_SELF_DESTRUCT,
    IA_HEAL_FULL, /* debug */
    IA_BTN_INFO,
    IA_BTN_LAYER_UP,
    IA_BTN_LAYER_DOWN,
};

class mining_handler : public b2RayCastCallback
{
  public:
    mining_handler(int layer){this->layer = layer;result=0;pt=b2Vec2(0.f,0.f);zap_type = ZAP_TYPE_NONE;};

    int zap_type;
    int layer;
    entity *result;
    b2Fixture *result_fx;
    void *result_udata2;
    b2Vec2 pt;

    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
    {
        if (f->IsSensor()) {
            return -1.f;
        }

        entity *r = static_cast<entity*>(f->GetUserData());

        if (!r) {
            return -1.f;
        }

        /*
        if (r->get_layer() != layer) {
            return -1.f;
        }
        */

        if (r == adventure::player) {
            return -1.f;
        }

        if (!world::fixture_in_layer(f, layer)) {
            return -1.f;
        }

        if (r->g_id == O_RESOURCE) {
            return -1.f;
        }

        this->zap_type = ZAP_TYPE_NONE;

        if (r->g_id == O_TPIXEL || r->g_id == O_CHUNK) {
            this->zap_type = ZAP_TYPE_TERRAIN;
        } else if (r->g_id == O_PLANT) {
            if (f->GetUserData2() == 0) { /* ignore leafs */
                return -1;
            }
            this->zap_type = ZAP_TYPE_PLANT;
        } else if (r->g_id == O_ITEM) {
            item *i = static_cast<item*>(r);

            /* If the item is not zappable, we can zap right through it!
             * Useufl in the current state of items, where the only item
             * we would make non-zappable is bullets. */
            if (!i->is_zappable()) {
                return -1.f;
            }

            this->zap_type = ZAP_TYPE_JUNK;
        } else if (r->is_creature()) {
            this->zap_type = ZAP_TYPE_JUNK;
        } else if (r->is_zappable()) {
            this->zap_type = ZAP_TYPE_JUNK;
        }

        if (this->zap_type != ZAP_TYPE_NONE) {
            if (adventure::first_mine || this->zap_type == adventure::mining_zap_type || adventure::mining_zap_type == ZAP_TYPE_NONE) {
                this->result = r;
                this->result_udata2 = f->GetUserData2();
                this->result_fx = f;
                this->pt = pt;

                G->finished_tt(TUTORIAL_ZAP_WOOD);
                G->close_tt(TUTORIAL_TEXT_ZAP_WOOD);
            }
        }

        return fraction;
    }
};

static void
handle_layer_up()
{
    if (adventure::player->motion == MOTION_DEFAULT
     && !W->level.flag_active(LVL_DISABLE_LAYER_SWITCH)) {

        adventure::pending_layermove = -1;
        adventure::layermove_attempts = 0;
    }

    adventure::player->move(DIR_UP);
}

static void
handle_layer_down()
{
    if (adventure::player->motion == MOTION_DEFAULT
     && !W->level.flag_active(LVL_DISABLE_LAYER_SWITCH)) {

        bool on_ladder_step = false;

        if (adventure::player->feet) {
            for (int x=0; x<adventure::player->feet->get_num_bodies() && !on_ladder_step; x++) {
                if (adventure::player->get_body(1+x)) {
                    /* make sure we're not on a ladder step */
                    b2ContactEdge *c;
                    for (c=adventure::player->get_body(1+x)->GetContactList(); c; c = c->next) {

                        b2Fixture *f = 0;
                        if (c->contact->GetFixtureA()->GetBody() == adventure::player->get_body(1+x)) {
                            f = c->contact->GetFixtureB();
                        } else {
                            f = c->contact->GetFixtureA();
                        }

                        if (f && f->GetUserData() && static_cast<entity*>(f->GetUserData())->g_id == O_LADDER_STEP) {
                            if (c->contact->IsTouching()) {
                                on_ladder_step = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (!on_ladder_step) {
            adventure::pending_layermove = +1;
            adventure::layermove_attempts = 0;
        }
    }

    adventure::player->move(DIR_DOWN);
}

/* currently only handles keyboard actions */
static int
handle_input_action(const tms::event *ev, const enum input_action ia)
{
    if (ev->type == TMS_EV_KEY_PRESS || ev->type == TMS_EV_POINTER_DOWN) {
        switch (ia) {
            case IA_MOVE_LEFT:
                adventure::key_down[ADVENTURE_FOCUSED_LEFT] = true;
                adventure::player->move(DIR_LEFT);
                break;

            case IA_MOVE_RIGHT:
                adventure::key_down[ADVENTURE_FOCUSED_RIGHT] = true;
                adventure::player->move(DIR_RIGHT);
                break;

            case IA_ACTION:
                adventure::key_down[ADVENTURE_FOCUSED_ACTION] = true;
                if (adventure::player->cur_activator && !adventure::player->is_action_active()) {
                    adventure::player->detach();
                } else if (!adventure::player->activators.empty() && !adventure::player->is_action_active()) {
                    int offset = 0;
                    if (ev->data.key.mod & TMS_MOD_SHIFT) {
                        offset = 1;
                    } else if (ev->data.key.mod & TMS_MOD_CTRL) {
                        offset = 2;
                    }
                    adventure::player->activate_closest_activator(offset);
                } else {
                    if (!W->level.flag_active(LVL_DISABLE_ROBOT_SPECIAL_ACTION)) {
                        if (adventure::player->is_action_active()) {
                            adventure::player->action_off();
                        } else {
                            adventure::player->action_on();
                        }
                    }
                }
                return EVENT_DONE;

            case IA_JUMP:
                adventure::key_down[ADVENTURE_FOCUSED_JUMP] = true;
                adventure::player->jump(false);
                return EVENT_DONE;

            case IA_DROP_INTERACTING:
                G->drop_interacting();
                return EVENT_DONE;

            case IA_LAYER_UP:
                adventure::key_down[ADVENTURE_FOCUSED_LAYER_UP] = true;
                handle_layer_up();
                return EVENT_DONE;

            case IA_LAYER_DOWN:
                adventure::key_down[ADVENTURE_FOCUSED_LAYER_DOWN] = true;
                handle_layer_down();
                return EVENT_DONE;

            case IA_DETACH:
                adventure::player->detach();
                break;

            case IA_ATTACK:
                adventure::key_down[ADVENTURE_FOCUSED_AIM] = true;
                adventure::player->attack();
                return EVENT_DONE;

            case IA_SELF_DESTRUCT:
                adventure::kill_player_counter = 0;
                adventure::kill_player = true;
                return EVENT_DONE;

            case IA_HEAL_FULL:
                adventure::player->set_hp(adventure::player->get_max_hp());
                return EVENT_DONE;

            case IA_BTN_INFO:
                G->handle_ingame_object_button(GW_INFO);
                return EVENT_DONE;

            case IA_BTN_LAYER_UP:
                G->handle_ingame_object_button(GW_LAYER_UP);
                return EVENT_DONE;

            case IA_BTN_LAYER_DOWN:
                G->handle_ingame_object_button(GW_LAYER_DOWN);
                return EVENT_DONE;

            default:
                break;
        }
    } else if (ev->type == TMS_EV_KEY_DOWN) {
        static const float aim_mod = 0.0075;

        switch (ia) {
            case IA_AIM_UP:
                {
                    const float cur_aim = adventure::player->get_aim();
                    if (!adventure::player->get_weapon()) {
                        break;
                    }

                    adventure::player->get_weapon()->set_arm_angle_raw(cur_aim+aim_mod);
                }
                return EVENT_DONE;

            case IA_AIM_DOWN:
                {
                    const float cur_aim = adventure::player->get_aim();
                    if (!adventure::player->get_weapon()) {
                        break;
                    }

                    adventure::player->get_weapon()->set_arm_angle_raw(cur_aim-aim_mod);
                }
                return EVENT_DONE;

            default:
                break;
        }
    } else if (ev->type == TMS_EV_KEY_UP || ev->type == TMS_EV_POINTER_UP) {
        switch (ia) {
            case IA_MOVE_LEFT:
                adventure::key_down[ADVENTURE_FOCUSED_LEFT] = false;
                adventure::player->stop_moving(DIR_LEFT);
                break;

            case IA_MOVE_RIGHT:
                adventure::key_down[ADVENTURE_FOCUSED_RIGHT] = false;
                adventure::player->stop_moving(DIR_RIGHT);
                break;

            case IA_LAYER_UP:
                adventure::key_down[ADVENTURE_FOCUSED_LAYER_UP] = false;
                adventure::player->stop_moving(DIR_UP);
                break;

            case IA_LAYER_DOWN:
                adventure::key_down[ADVENTURE_FOCUSED_LAYER_DOWN] = false;
                adventure::player->stop_moving(DIR_DOWN);
                break;

            case IA_JUMP:
                adventure::key_down[ADVENTURE_FOCUSED_JUMP] = false;
                adventure::player->stop_jump();
                break;

            case IA_ATTACK:
                adventure::key_down[ADVENTURE_FOCUSED_AIM] = false;
                adventure::player->attack_stop();
                break;

            case IA_ACTION:
                adventure::key_down[ADVENTURE_FOCUSED_ACTION] = false;
                break;

            case IA_SELF_DESTRUCT:
                adventure::kill_player = false;
                break;

            default:
                break;
        }
    }

    return EVENT_CONT;
}

void
adventure_on_change(tms_wdg *w, float values[2])
{
    if (!adventure::player || adventure::player->is_dead()) return;
    int data = VOID_TO_INT(w->data);
    float value = values[0];

    if (w->type == TMS_WDG_BUTTON) {
        /* button is pressed */

        if (value == 1.f) {
            switch (data) {
                case ADVENTURE_BTN_LAYER_UP: handle_layer_up(); break;
                case ADVENTURE_BTN_LAYER_DOWN: handle_layer_down(); break;
                case ADVENTURE_BTN_JUMP:
                    adventure::player->jump(false);
                    break;
                case ADVENTURE_BTN_ACTION:
                    if (!W->level.flag_active(LVL_DISABLE_ROBOT_SPECIAL_ACTION)) {
                        if (adventure::player->is_action_active()) {
                            adventure::w_action->s[0] = gui_spritesheet::get_sprite(S_EXPORT);
                            adventure::player->action_off();
                        } else {
                            adventure::w_action->s[0] = gui_spritesheet::get_sprite(S_IMPORT);
                            adventure::player->action_on();
                        }
                    }
                    break;
                case ADVENTURE_BTN_ATTACK:
                    adventure::player->attack();
                    break;
            }
        } else {
            switch (data) {
                case ADVENTURE_BTN_ATTACK:
                    adventure::player->attack_stop();
                    break;
                case ADVENTURE_BTN_JUMP:
                    adventure::player->stop_jump();
                    break;
                case ADVENTURE_BTN_LAYER_UP: adventure::player->stop_moving(DIR_UP); break;
                case ADVENTURE_BTN_LAYER_DOWN: adventure::player->stop_moving(DIR_DOWN); break;
            }
        }
    } else {
        switch (data) {
            case ADVENTURE_BTN_AIM:
                {
                    float a = value;

                    switch (adventure::player->g_id) {
                        case O_BOMBER:
                        case O_LOBBER:
                            break;

                        default:
                            /* offset and wrap the angle */
                            a = twrapf(a - 1.00f, -1.f, 1.f);

                            /* convert a value between -1 and 1 to a value the robot can work with */
                            a *= (M_PI*2.f);

                            a -= adventure::player->get_angle();
                            break;
                    }

                    adventure::player->aim(a);
                }
                break;
        }
    }
}
void
adventure_on_focus_change(tms_wdg *w)
{
    int data = VOID_TO_INT(w->data);

    if (data == ADVENTURE_BTN_AIM) {
        if (w->focused) {
            adventure::player->attack();
        } else {
            adventure::player->attack_stop();
        }
    }
}

void
adventure::init()
{
    if (!current_tool) {
        current_tool = new p_text(font::small);
        current_tool->set_text("");
    }
    tms_debugf("adventure INIT called ---------------");
    adventure::player = 0;
    adventure::widgets_active = false;
    for (int n=0; n<NUM_RESOURCES; ++n) {
        adventure::highlight_inventory[n] = 0.f;
    }
}

void
adventure::set_player(creature *r, bool snap_camera/*=true*/, bool set_rc/*=true*/)
{
    if (W->level.type != LCAT_ADVENTURE) {
        tms_errorf("can not set player in a non-adventure level");
        return;
    }

    if (adventure::player) {
        adventure::player->stop_moving(DIR_LEFT);
        adventure::player->stop_moving(DIR_RIGHT);
        adventure::player->attack_stop();
        adventure::player->tool_stop();
    }

    adventure::player = r;

    if (r) {
        adventure::player->stop_moving(DIR_LEFT);
        adventure::player->stop_moving(DIR_RIGHT);
        G->state.adventure_id = r->id;

        if (adventure::current_tool) {
            adventure::tool_changed();
        }
    } else {
        G->state.adventure_id = 0;
    }

    G->cam_rel_pos = b2Vec2(0.f, 0.f);
    G->cam_vel.x = 0.f;
    G->cam_vel.y = 0.f;

    G->set_follow_object(0, snap_camera); /* 0 defaults to robot in adventure mode */
    if (set_rc) {
        G->set_control_panel(adventure::player);
    }
}

void
adventure::respawn()
{
    adventure::dead = false;

    if (adventure::player) {
        adventure::pending_layermove = 0;

        W->events[WORLD_EVENT_PLAYER_RESPAWN] ++;
    }

    G->caveview_size = 0.f;
    G->cam_rel_pos.x = 0.f;
    G->cam_rel_pos.y = 0.f;
    G->adv_rel_pos.x = 0.f;
    G->adv_rel_pos.y = 0.f;
}

void
adventure::on_player_die()
{
    G->play_sound(SND_PLAYER_DEATH, 0.f, 0.f, 0, 1.f, false, 0, true);

    adventure::player->detach();
    G->set_control_panel(0);
    G->drop_interacting();

    if (adventure::mining) {
        adventure::end_mining();
    }

    adventure::dead = true;
    adventure::death_step = W->step_count;
    adventure::death_wait = DOUBLETIME_TO_INT64(W->level.time_before_player_can_respawn);
}

void
adventure::checkpoint_activated(checkpoint *cp)
{
    if (!adventure::player) {
        robot_base *r = static_cast<robot_base*>(of::create_with_id(O_ROBOT, W->level.get_adventure_id()));

        b2Vec2 p = cp->local_to_world(b2Vec2(0.f, 1.f), 0);
        float a = cp->get_angle();
        int l = cp->get_layer();

        r->_pos = p;
        r->_angle = a;
        r->set_layer(l);
        r->layer_new = r->get_layer();
        r->layer_old = r->get_layer();
        r->layer_blend = 1.f;
        G->emit(r, 0, b2Vec2(0.f,0.f), true);
        adventure::set_player(r, false);
        adventure::respawn();
    }
}

#define KILL_COUNTER_MAX 50

void
adventure::step()
{
    if (adventure::player) {

        adventure::refresh_widgets();

        if (!adventure::player->is_dead() && adventure::kill_player) {
            adventure::kill_player_counter ++;
            G->add_hp(adventure::player, (adventure::kill_player_counter/(float)KILL_COUNTER_MAX), TV_HP_LGRAY, 0.2f);

            if (adventure::kill_player_counter >= KILL_COUNTER_MAX) {
                P.add_action(ACTION_SELF_DESTRUCT, 0);

                adventure::kill_player = false;
            }
        }

        if (adventure::player->is_dead() && adventure::death_wait > 0) {
            adventure::death_wait -= WORLD_STEP;
        }

        float blend = .996f;
        b2Vec2 av = adventure::player->get_tangent_vector(adventure::player->look_dir*2.f);

        av = adventure::player->get_tangent_vector(adventure::player->is_moving_left()*-2.f + adventure::player->is_moving_right()*2.f);
        //av += adventure::player->get_normal_vector(1.f);

        /*b2Vec2 av = adventure::player->body ? adventure::player->body->GetLinearVelocity() : b2Vec2(0.f,0.f);
        av.x *= 2.f;
        av.y *= 2.f;
        av.x = tclampf(av.x, -4.f, 4.f);
        av.y = tclampf(av.y, -4.f, 4.f);
        */

        if (adventure::player->is_moving_left() || adventure::player->is_moving_right()) {
            G->adv_rel_pos.x = G->adv_rel_pos.x * blend + av.x * (1.f-blend);
            G->adv_rel_pos.y = G->adv_rel_pos.y * blend + av.y * (1.f-blend);
        }

        /* Check the movement slider */
        if (adventure::widgets_active) {
            if (w_move_slider->focused) {
                if (w_move_slider->value[0] < 0.5f) {
                    adventure::player->move(DIR_LEFT);
                    adventure::player->look(DIR_LEFT);
                    adventure::player->stop_moving(DIR_RIGHT);
                } else {
                    adventure::player->move(DIR_RIGHT);
                    adventure::player->look(DIR_RIGHT);
                    adventure::player->stop_moving(DIR_LEFT);
                }
            } else if (slider_prev_focused) {
                adventure::player->stop_moving(DIR_LEFT);
                adventure::player->stop_moving(DIR_RIGHT);
                w_move_slider->value[0] = 0.5f;
            }

            /*
            if (w_aim->focused && !aim_prev_focused) {
                adventure::player->attack();
            }
            */

            slider_prev_focused = w_move_slider->focused;
            aim_prev_focused = w_aim->focused;
        }
        b2Vec2 player_pos = adventure::player->get_position();

        if (adventure::pending_layermove != 0) {
            if (adventure::player->layermove(adventure::pending_layermove)) {
                adventure::pending_layermove = 0;
            } else {
                /* layermove failed */
                adventure::layermove_attempts ++;
                if (adventure::layermove_attempts > 30) {
                    adventure::pending_layermove = 0;
                }
            }
        }

#ifdef TMS_BACKEND_PC
        if (settings["control_type"]->v.u8 == 1) {
            tvec3 o;
            W->get_layer_point(G->cam, adventure::last_mouse_x, adventure::last_mouse_y, 0, &o);

            b2Vec2 r = player_pos;

            if (!adventure::player->is_currently_climbing()) {
                if (!G->shift_down() && ((r-b2Vec2(o.x, o.y)).LengthSquared() < .3f)) {
                    adventure::player->look(DIR_FORWARD);
                } else {
                    if (!G->shift_down()) {
                        if (adventure::player->get_tangent_distance(b2Vec2(o.x, o.y)) < 0.f) {
                            adventure::player->look(DIR_LEFT);
                        } else {
                            adventure::player->look(DIR_RIGHT);
                        }
                    }

                    b2Vec2 oo = b2Vec2(o.x, o.y);
                    oo -= player_pos;

                    float a = atan2f(oo.y, oo.x);

                    a -= adventure::player->get_angle();

                    adventure::player->aim(a);
                }
            }
        }
#endif


#ifdef DEBUG
        int mining_interval = (int)((float)MINING_INTERVAL * (G->ctrl_down() ? .075f : 1.f));
#else
        static const int mining_interval = MINING_INTERVAL;
#endif

        if (adventure::mining_timer >= std::min(100000, mining_interval/4)) {
            if (mining_discharge) {
                G->remove_entity(mining_discharge);
                delete mining_discharge;
                mining_discharge = 0;
            }
        }

        if (adventure::mining_timer >= mining_interval) {
            robot_parts::tool *t = adventure::player->get_tool();
            if (adventure::mining && t && t->get_arm_type() == TOOL_ZAPPER) {
                robot_parts::miner *miner = static_cast<robot_parts::miner*>(t);
                mining_handler cb(adventure::player->get_layer());

                b2Vec2 start = adventure::player->get_position();
                b2Vec2 end = mining_pos;

                float mining_dist = b2Distance(end, start);

                b2Vec2 dir = end-start;
                dir.Normalize();

                float angle = atan2f(dir.y, dir.x);

                /*
                b2Vec2 v = (end-start);
                v.Normalize();
                v.x*=2.5f;
                v.y*=2.5f;
                end = start+v;
                */

                robot_parts::tool *t = adventure::player->get_tool();
                if (t) {
                    /* XXX: We should probably turn the robot around, but right now he's twitching
                    if (adventure::player->get_tangent_distance(b2Vec2(mining_pos.x, mining_pos.y)) < 0.f)
                        adventure::player->look(-1);
                    else
                        adventure::player->look(1);
                        */

                    b2Vec2 oo = b2Vec2(mining_pos.x, mining_pos.y);
                    oo -= player_pos;

                    float a = atan2f(oo.y, oo.x);

                    a -= adventure::player->get_angle();

                    t->set_arm_angle(a);
                }

                const int max_retries = 6;
                int retries = max_retries;

                if (adventure::first_mine) {
                    retries = 0;
                }

                do {
                    W->raycast(&cb, start, end);
                    //W->b2->RayCast(&cb, start, end);

                    if (cb.result) {
                        if ((start - cb.pt).Length() > 2.5f) {
                            continue;
                        }

                        if (cb.result->is_creature()) {
                            creature *c = static_cast<creature*>(cb.result);

                            c->damage(miner->damage, cb.result_fx, DAMAGE_TYPE_ELECTRICITY, DAMAGE_SOURCE_BULLET, adventure::player->id);
                        } else if (cb.result->is_zappable()) {
                            switch (cb.result->g_id) {
                                case O_PLANT:
                                    {
                                        plant *pl = static_cast<plant*>(cb.result);
                                        pl->damage_section(static_cast<plant_section*>(cb.result_udata2), miner->damage, DAMAGE_TYPE_ELECTRICITY);
                                    }
                                    break;

                                case O_ITEM:
                                    {
                                        item *i = static_cast<item*>(cb.result);
                                        i->damage(miner->damage);
                                    }
                                    break;

                                case O_TPIXEL:
                                case O_CHUNK:
                                    {
                                        G->damage_tpixel(cb.result, cb.result_fx, cb.result_udata2, miner->damage, cb.pt, DAMAGE_TYPE_ELECTRICITY);
                                        struct tpixel_desc *desc = static_cast<struct tpixel_desc*>(cb.result_udata2);
                                        if (desc->material >= TPIXEL_MATERIAL_DIAMOND_ORE) {
                                            G->play_sound(SND_MINING_HIT_ORE, cb.pt.x, cb.pt.y, 0, 0.3f, false, 0, true);
                                        }
                                    }
                                    break;

                                default:
                                    cb.result->entity_damage(miner->damage);
                                    break;
                            }
                        } else {
                            continue;
                        }

                        mining_discharge = new discharge_effect(
                                start, cb.pt, adventure::player->get_layer(),adventure::player->get_layer(),
                                8, 100.f
                            );
                        G->play_sound(SND_ZAPPER, cb.pt.x, cb.pt.y, 0, 0.5f, false, 0, true);
                        G->add_entity(mining_discharge);
                        adventure::mining_timer = 0;
                        adventure::first_mine = false;
                        adventure::mining_zap_type = cb.zap_type;
                        return;
                    }

                    float inc = .1f;

                    if ((retries%2)==0) {
                        inc = -inc;
                    }

                    float cs = cosf(angle+inc*(max_retries-(retries-1)));
                    float sn = sinf(angle+inc*(max_retries-(retries-1)));

                    end = start+b2Vec2(cs*mining_dist, sn*mining_dist);
                } while (--retries > 0);
            }
        }

        adventure::mining_timer += G->timemul(WORLD_STEP);
    }
}

bool
adventure::is_player_alive()
{
    return adventure::player && !adventure::player->is_dead();
}

int
adventure::handle_input_playing(tms::event *ev, int action)
{
    int ret = EVENT_CONT;

    if (!adventure::player || (adventure::player != G->current_panel && !adventure::player->is_dead() && W->level.version >= LEVEL_VERSION_1_5)) {
        return EVENT_CONT;
    }

    if (ev->type & TMS_EV_MASK_POINTER) {
        adventure::last_mouse_x = ev->data.button.x;
        adventure::last_mouse_y = ev->data.button.y;
    }

    if (adventure::player->is_dead()) {
        /* respawn robot if he's dead */

#ifdef DEBUG
        if (ev->type == TMS_EV_KEY_PRESS && ev->data.key.keycode == TMS_KEY_K) {
            b2Vec2 old_pos = adventure::player->get_position();
            int old_layer = adventure::player->get_layer();
            adventure::respawn();
            adventure::player->respawn();
            adventure::player->set_position(old_pos.x, old_pos.y, 0);

            adventure::player->set_layer(old_layer);
            adventure::player->layer_new = player->get_layer();
            adventure::player->layer_old = player->get_layer();
            adventure::player->layer_blend = 1.f;

            return EVENT_DONE;
        }
#endif

        /* minimum death time before we can respawn */
        if (adventure::death_wait <= 0) {

#ifdef TMS_BACKEND_MOBILE
            if (ev->type == TMS_EV_POINTER_DOWN) {
                adventure::respawn();
                adventure::player->respawn();
                return EVENT_DONE;
            }
#else
            if (ev->type == TMS_EV_KEY_PRESS) {
                adventure::respawn();
                adventure::player->respawn();
                return EVENT_DONE;
            }
#endif
        }

        return EVENT_DONE;
    }

    if (ev->type == TMS_EV_POINTER_DOWN) {
        tvec2 sp = (tvec2){ev->data.motion.x, ev->data.motion.y};
        float x_left = G->get_bmenu_x() - ((icon_width / 2.f) * 1.125f);
        float x_right = x_left + (icon_width * 3.f);

        float y_top = inventory_begin_y;

        int num_items = 0;

        for (int n=0; n<NUM_RESOURCES; ++n) {
            if (adventure::player->get_num_resources(n)
#ifdef TMS_BACKEND_MOBILE
                    && n == adventure::last_picked_up_resource
#endif
                    ) {
                ++num_items;
            }
        }

        if (num_items) {
            float y_bottom = y_top - ((icon_height + margin_y) * num_items);

            tms_debugf("top: %.2f. bottom: %.2f", y_top, y_bottom);
            tms_debugf("left: %.2f. right: %.2f", x_left, x_right);

            tms_debugf("Clicked %.2f/%.2f", sp.x, sp.y);

            if (sp.x >= x_left && sp.x <= x_right && sp.y <= y_top && sp.y >= y_bottom) {
                tms_debugf("OPEN INVENTORY SCREEN");
                G->set_mode(GAME_MODE_INVENTORY);
                return 1;
            }
        }

        if (adventure::player->g_id == O_ROBOT) {
            robot *r = static_cast<robot*>(adventure::player);

            /* click handlign to switch weapon and tool */
            static const float weapon_tol_x = icon_width/2.f + _tms.xppcm*.1f;
            static const float weapon_tol_y = icon_height/2.f + _tms.yppcm*.1f;

            for (int x=0; x<adventure::num_weapons; x++) {
                if (fabsf(weapon_icon_pos[x].x - sp.x) < weapon_tol_x
                    && fabsf(weapon_icon_pos[x].y - sp.y) < weapon_tol_y) {
                    r->equip_weapon(r->weapons[x]->get_weapon_type());
                    break;
                }
            }
            for (int x=0; x<adventure::num_tools; x++) {
                if (fabsf(tool_icon_pos[x].x - sp.x) < weapon_tol_x
                    && fabsf(tool_icon_pos[x].y - sp.y) < weapon_tol_y) {
                    r->equip_tool(r->tools[x]->get_arm_type());
                    break;
                }
            }
        }
    }

    switch (settings["control_type"]->v.u8) {
        case 0: /* Keyboard only */
            if (ev->type & TMS_EV_MASK_KEY) {
                switch (ev->data.key.keycode) {
                    case TMS_KEY_ESC:
                        ret = handle_input_action(ev, IA_DETACH);
                        break;

                    case TMS_KEY_R:
                        ret = handle_input_action(ev, IA_DROP_INTERACTING);
                        break;

                    case TMS_KEY_1:
                    case TMS_KEY_2:
                    case TMS_KEY_3:
                    case TMS_KEY_4:
                    case TMS_KEY_5:
                    case TMS_KEY_6:
                    case TMS_KEY_7:
                    case TMS_KEY_8:
                    case TMS_KEY_9:
                        {
                            int index = ev->data.key.keycode - TMS_KEY_1;

                            if (adventure::player->g_id == O_ROBOT) {
                                robot* r = static_cast<robot*>(adventure::player);

                                if (G->shift_down()) {
                                    if (index < r->num_tools) {
                                        r->equip_tool(r->tools[index]->get_arm_type());

                                        ret = handle_input_action(ev, IA_DROP_INTERACTING);
                                    }
                                } else {
                                    if (index < r->num_weapons) {
                                        r->equip_weapon(r->weapons[index]->get_weapon_type());
                                    }
                                }
                            }
                        }
                        break;

                    case TMS_KEY_E:
                        ret = handle_input_action(ev, IA_ACTION);
                        break;

                    case TMS_KEY_LEFT:
                        ret = handle_input_action(ev, IA_MOVE_LEFT);
                        adventure::player->look(DIR_LEFT);
                        break;

                    case TMS_KEY_RIGHT:
                        ret = handle_input_action(ev, IA_MOVE_RIGHT);
                        adventure::player->look(DIR_RIGHT);
                        break;

                    case TMS_KEY_SPACE:
                        ret = handle_input_action(ev, IA_JUMP);
                        break;

                    case TMS_KEY_LEFT_CTRL:
                    case TMS_KEY_RIGHT_CTRL:
                        ret = handle_input_action(ev, IA_ATTACK);
                        break;

                    case TMS_KEY_PAGEUP:
                        ret = handle_input_action(ev, IA_LAYER_UP);
                        break;

                    case TMS_KEY_PAGEDOWN:
                        ret = handle_input_action(ev, IA_LAYER_DOWN);
                        break;

                    case TMS_KEY_DOWN:
                        ret = handle_input_action(ev, IA_AIM_DOWN);
                        break;

                    case TMS_KEY_UP:
                        ret = handle_input_action(ev, IA_AIM_UP);
                        break;
                }
            }
            break;

        case 1: /* Keyboard and mouse */
            if (ev->type & TMS_EV_MASK_KEY) {
                switch (ev->data.key.keycode) {
                    case TMS_KEY_ESC:
                        ret = handle_input_action(ev, IA_DETACH);
                        break;

                    case TMS_KEY_R:
                        ret = handle_input_action(ev, IA_DROP_INTERACTING);
                        break;

                    case TMS_KEY_1:
                    case TMS_KEY_2:
                    case TMS_KEY_3:
                    case TMS_KEY_4:
                    case TMS_KEY_5:
                    case TMS_KEY_6:
                    case TMS_KEY_7:
                    case TMS_KEY_8:
                    case TMS_KEY_9:
                        {
                            int index = ev->data.key.keycode - TMS_KEY_1;

                            if (adventure::player->g_id == O_ROBOT) {
                                robot* r = static_cast<robot*>(adventure::player);

                                if (G->shift_down()) {
                                    if (index < r->num_tools) {
                                        r->equip_tool(r->tools[index]->get_arm_type());

                                        ret = handle_input_action(ev, IA_DROP_INTERACTING);
                                    }
                                } else {
                                    if (index < r->num_weapons) {
                                        r->equip_weapon(r->weapons[index]->get_weapon_type());
                                    }
                                }
                            }
                        }
                        break;

                    case TMS_KEY_E:
                        ret = handle_input_action(ev, IA_ACTION);
                        break;

                    case TMS_KEY_A:
                        ret = handle_input_action(ev, IA_MOVE_LEFT);
                        break;

                    case TMS_KEY_D:
                        ret = handle_input_action(ev, IA_MOVE_RIGHT);
                        break;

                    case TMS_KEY_SPACE:
                        ret = handle_input_action(ev, IA_JUMP);
                        break;

                    case TMS_KEY_W:
                        ret = handle_input_action(ev, IA_LAYER_UP);
                        break;

                    case TMS_KEY_S:
                        ret = handle_input_action(ev, IA_LAYER_DOWN);
                        break;

                    case TMS_KEY_I:
                        ret = handle_input_action(ev, IA_BTN_INFO);
                        break;

                    case TMS_KEY_X:
                        ret = handle_input_action(ev, IA_BTN_LAYER_UP);
                        break;

                    case TMS_KEY_Z:
                        ret = handle_input_action(ev, IA_BTN_LAYER_DOWN);
                        break;

                    case TMS_KEY_K:
                        ret = handle_input_action(ev, IA_SELF_DESTRUCT);
                        break;

#ifdef DEBUG
                    case TMS_KEY_U:
                        ret = handle_input_action(ev, IA_HEAL_FULL);
                        break;
#endif
                }
            } else if (ev->type == TMS_EV_POINTER_DOWN || ev->type == TMS_EV_POINTER_UP) {
#ifdef TMS_BACKEND_PC
                if (ev->data.motion.pointer_id == 1) {
                    ret = handle_input_action(ev, IA_ATTACK);
                }
#endif
            }
            break;
    }

    return ret;
}

void
adventure::refresh_widgets()
{
    tms_assertf(adventure::player, "no adventure player when refresh widgets was called.");

    if (!settings["touch_controls"]->v.b || !W->is_adventure() || !adventure::widgets_active) {
        return;
    }

    /* Make sure the layer up/down widgets are faded
     * if "Disable layer switch" is enabled and the player
     * is not capable of climbing any ladders. */
    if (W->level.flag_active(LVL_DISABLE_LAYER_SWITCH)) {
        adventure::w_layer_up->faded   = !adventure::player->is_climbing_ladder();
        adventure::w_layer_down->faded = !adventure::player->is_climbing_ladder();
    }

    /* Colorize the layer buttons if the player can climb
     * a ladder. */
    if (adventure::player->is_climbing_ladder()) {
        if (!adventure::w_layer_up->color) {
            adventure::w_layer_up->color = (tvec3*)malloc(sizeof(tvec3));
        }
        if (!adventure::w_layer_down->color) {
            adventure::w_layer_down->color = (tvec3*)malloc(sizeof(tvec3));
        }

        double cs = (cos((double)_tms.last_time/100000.) + 1.0) / 2.0;

        adventure::w_layer_up->color->r = 1.f - (cs * 0.15);
        adventure::w_layer_up->color->g = 1.f - (cs * 0.05);
        adventure::w_layer_up->color->b = 1.f - (cs * 0.15);

        adventure::w_layer_down->color->r = 1.f - (cs * 0.15);
        adventure::w_layer_down->color->g = 1.f - (cs * 0.05);
        adventure::w_layer_down->color->b = 1.f - (cs * 0.15);
    } else {
        if (adventure::w_layer_up->color) {
            free(adventure::w_layer_up->color);
            adventure::w_layer_up->color = 0;
        }
        if (adventure::w_layer_down->color) {
            free(adventure::w_layer_down->color);
            adventure::w_layer_down->color = 0;
        }
    }
}

void
adventure::init_widgets()
{
    if (!settings["touch_controls"]->v.b || !W->is_adventure()) {
        return;
    }

    if (adventure::widgets_active) return;
    tms_infof("init widgets -------------------");

    int sx = 1;
    int sy = 1;
    float x = 0.f;
    float y = 0.f;

    bool right_side = false;

    adventure::w_move_slider = tms_wdg_alloc(TMS_WDG_SLIDER, gui_spritesheet::get_sprite(S_ADVENTURE_LEFTRIGHT), 0);
    adventure::w_layer_up    = tms_wdg_alloc(TMS_WDG_BUTTON, gui_spritesheet::get_sprite(S_UP), 0);
    adventure::w_layer_down  = tms_wdg_alloc(TMS_WDG_BUTTON, gui_spritesheet::get_sprite(S_DOWN), 0);
    adventure::w_aim         = tms_wdg_alloc(TMS_WDG_BTN_RADIAL, gui_spritesheet::get_sprite(S_RADIAL_W_KNOB), gui_spritesheet::get_sprite(S_RADIAL_2));
    adventure::w_action      = tms_wdg_alloc(TMS_WDG_BUTTON, gui_spritesheet::get_sprite(S_EXPORT), 0);
    adventure::w_jump        = tms_wdg_alloc(TMS_WDG_BUTTON, gui_spritesheet::get_sprite(S_UP), 0);

    adventure::w_layer_up->data   = INT_TO_VOID(ADVENTURE_BTN_LAYER_UP);
    adventure::w_layer_down->data = INT_TO_VOID(ADVENTURE_BTN_LAYER_DOWN);
    adventure::w_aim->data        = INT_TO_VOID(ADVENTURE_BTN_AIM);
    adventure::w_action->data     = INT_TO_VOID(ADVENTURE_BTN_ACTION);
    adventure::w_jump->data       = INT_TO_VOID(ADVENTURE_BTN_JUMP);

    adventure::w_layer_up->on_change   = &adventure_on_change;
    adventure::w_layer_down->on_change = &adventure_on_change;
    adventure::w_aim->on_change        = &adventure_on_change;
    adventure::w_aim->on_focus_change  = &adventure_on_focus_change;
    adventure::w_action->on_change     = &adventure_on_change;
    adventure::w_jump->on_change       = &adventure_on_change;

    sx = 1; sy = 1;
    adventure::w_layer_up->size.x   = sx*b_w;
    adventure::w_layer_up->size.y   = sy*b_h;
    adventure::w_layer_down->size.x = sx*b_w;
    adventure::w_layer_down->size.y = sy*b_h;
    adventure::w_jump->size.x       = sx*b_w;
    adventure::w_jump->size.y       = sy*b_h;
    adventure::w_action->size.x     = sx*b_w;
    adventure::w_action->size.y     = sy*b_h;

    sx = 2; sy = 1;
    adventure::w_move_slider->size.x= sx*b_w;
    adventure::w_move_slider->size.y= sy*b_h;
    sx = 2; sy = 2;
    adventure::w_aim->size.x        = sx*b_w;
    adventure::w_aim->size.y        = sy*b_h;

    sx = 1; sy = 1;
    x = .5f, y = 2.f;
    adventure::w_layer_up->pos = (tvec2){
        (right_side?(_tms.window_width - 3*(PANEL_WDG_OUTER_X)):0.f) + (PANEL_WDG_OUTER_X)/2.f + x*PANEL_WDG_OUTER_X + ((sx-1) * .5f)*PANEL_WDG_OUTER_X,
        (PANEL_WDG_OUTER_Y)/2.f + y*PANEL_WDG_OUTER_Y + ((sy-1) * .5f)*PANEL_WDG_OUTER_Y
    };

    sx = 1; sy = 1;
    x = .5f, y = 1.f;
    adventure::w_layer_down->pos = (tvec2){
        (right_side?(_tms.window_width - 3*(PANEL_WDG_OUTER_X)):0.f) + (PANEL_WDG_OUTER_X)/2.f + x*PANEL_WDG_OUTER_X + ((sx-1) * .5f)*PANEL_WDG_OUTER_X,
        (PANEL_WDG_OUTER_Y)/2.f + y*PANEL_WDG_OUTER_Y + ((sy-1) * .5f)*PANEL_WDG_OUTER_Y
    };

    sx = 2; sy = 1;
    x = 0.f, y = 0.f;
    adventure::w_move_slider->pos = (tvec2){
        (right_side?(_tms.window_width - 3*(PANEL_WDG_OUTER_X)):0.f) + (PANEL_WDG_OUTER_X)/2.f + x*PANEL_WDG_OUTER_X + ((sx-1) * .5f)*PANEL_WDG_OUTER_X,
        (PANEL_WDG_OUTER_Y)/2.f + y*PANEL_WDG_OUTER_Y + ((sy-1) * .5f)*PANEL_WDG_OUTER_Y
    };

    right_side = true;

    sx = 1; sy = 1;
    x = 0.f, y = 0.f;
    adventure::w_jump->pos = (tvec2){
        (right_side?(_tms.window_width - 3*(PANEL_WDG_OUTER_X)):0.f) + (PANEL_WDG_OUTER_X)/2.f + x*PANEL_WDG_OUTER_X + ((sx-1) * .5f)*PANEL_WDG_OUTER_X,
        (PANEL_WDG_OUTER_Y)/2.f + y*PANEL_WDG_OUTER_Y + ((sy-1) * .5f)*PANEL_WDG_OUTER_Y
    };
    x = 0.f, y = 1.f;
    adventure::w_action->pos = (tvec2){
        (right_side?(_tms.window_width - 3*(PANEL_WDG_OUTER_X)):0.f) + (PANEL_WDG_OUTER_X)/2.f + x*PANEL_WDG_OUTER_X + ((sx-1) * .5f)*PANEL_WDG_OUTER_X,
        (PANEL_WDG_OUTER_Y)/2.f + y*PANEL_WDG_OUTER_Y + ((sy-1) * .5f)*PANEL_WDG_OUTER_Y
    };

    sx = 2; sy = 2;
    x = 1.f, y = 0.f;
    adventure::w_aim->pos = (tvec2){
        (right_side?(_tms.window_width - 3*(PANEL_WDG_OUTER_X)):0.f) + (PANEL_WDG_OUTER_X)/2.f + x*PANEL_WDG_OUTER_X + ((sx-1) * .5f)*PANEL_WDG_OUTER_X,
        (PANEL_WDG_OUTER_Y)/2.f + y*PANEL_WDG_OUTER_Y + ((sy-1) * .5f)*PANEL_WDG_OUTER_Y
    };

    G->get_surface()->add_widget(adventure::w_move_slider);
    G->get_surface()->add_widget(adventure::w_layer_up);
    G->get_surface()->add_widget(adventure::w_layer_down);
    G->get_surface()->add_widget(adventure::w_aim);
    G->get_surface()->add_widget(adventure::w_action);
    G->get_surface()->add_widget(adventure::w_jump);

    adventure::w_layer_up->faded = W->level.flag_active(LVL_DISABLE_LAYER_SWITCH);
    adventure::w_layer_down->faded = W->level.flag_active(LVL_DISABLE_LAYER_SWITCH);

    adventure::w_action->faded = W->level.flag_active(LVL_DISABLE_ROBOT_SPECIAL_ACTION);

    adventure::widgets_active = true;

    slider_prev_focused = true;
    aim_prev_focused = false;
}

void
adventure::hide_left_widgets()
{
    if (!adventure::widgets_active) return;

    G->get_surface()->remove_widget(adventure::w_move_slider);
    G->get_surface()->remove_widget(adventure::w_layer_up);
    G->get_surface()->remove_widget(adventure::w_layer_down);
}

void
adventure::show_left_widgets()
{
    if (!adventure::widgets_active) return;

    G->get_surface()->add_widget(adventure::w_move_slider);
    G->get_surface()->add_widget(adventure::w_layer_up);
    G->get_surface()->add_widget(adventure::w_layer_down);
}

void
adventure::clear_widgets()
{
    if (!adventure::widgets_active) return;

    tms_debugf("clear widgets -------------------");

    tms_wdg_free(adventure::w_move_slider);
    tms_wdg_free(adventure::w_layer_up);
    tms_wdg_free(adventure::w_layer_down);
    tms_wdg_free(adventure::w_aim);
    tms_wdg_free(adventure::w_action);
    tms_wdg_free(adventure::w_jump);

    adventure::w_move_slider = 0;
    adventure::w_layer_up = 0;
    adventure::w_layer_down = 0;
    adventure::w_aim = 0;
    adventure::w_action = 0;
    adventure::w_jump = 0;

    adventure::widgets_active = false;
}

void
adventure::reset()
{
    adventure::clear_widgets();
    adventure::last_picked_up_resource = RESOURCE_WOOD;

    adventure::dead = false;
    adventure::death_step = 0;
    adventure::death_wait = 0;

    adventure::num_weapons = 0;
    adventure::num_tools = 0;

    adventure::player = 0;
}

void
adventure::update_mining_pos(float x, float y)
{
    adventure::mining_pos = b2Vec2(x,y);
}

void
adventure::begin_mining()
{
    adventure::mining = true;
    adventure::first_mine = true;
}

void
adventure::end_mining()
{
    adventure::mining = false;
}

void
adventure::setup()
{
    adventure::pending_layermove = 0;

    adventure::dead = false;
    adventure::death_step = 0;
    adventure::death_wait = 0;
    adventure::mining = false;

    last_mouse_x = _tms.window_width/2;
    last_mouse_y = _tms.window_height/2;
}

bool
adventure::focused(int w)
{
    bool ret = false;
    if (adventure::widgets_active) {
        switch (w) {
            case ADVENTURE_FOCUSED_LEFT:
                if (w_move_slider->focused) {
                    if (w_move_slider->value[0] < 0.5f) {
                        ret = true;
                    }
                }
                break;
            case ADVENTURE_FOCUSED_RIGHT:
                if (w_move_slider->focused) {
                    if (w_move_slider->value[0] > 0.5f) {
                        ret = true;
                    }
                }
                break;
            case ADVENTURE_FOCUSED_LAYER_UP:
                if (w_layer_up->focused)
                    ret = true;
                break;
            case ADVENTURE_FOCUSED_LAYER_DOWN:
                if (w_layer_down->focused)
                    ret = true;
                break;
            case ADVENTURE_FOCUSED_AIM:
                if (w_aim->focused)
                    ret = true;
                break;
            case ADVENTURE_FOCUSED_ACTION:
                if (w_action->focused)
                    ret = true;
                break;
            case ADVENTURE_FOCUSED_JUMP:
                if (w_jump->focused)
                    ret = true;
                break;
        }
    }

    if (adventure::key_down[w]) {
        ret = true;
    }

    return ret;
}

void
adventure::render()
{
    adventure::num_weapons = 0;
    adventure::num_tools = 0;

#ifdef NO_UI
    return;
#endif
    if (!adventure::player || settings["render_gui"]->is_false()) return;

    glBindTexture(GL_TEXTURE_2D, G->get_item_texture()->gl_texture);

    icon_width  = roundf(_tms.xppcm / 2.5f);
    icon_height = roundf(_tms.yppcm / 2.5f);
    margin_x = _tms.xppcm / 15.f;
    margin_y = _tms.yppcm / 15.f;
    static float base_x = margin_x + (.5f * _tms.xppcm)/2.f;// + (icon_width / 2.f);
    float base_y = _tms.window_height - G->wm->get_margin_y() - (1.0f * _tms.yppcm)/2.f;

    /* calculate how much space we have to work with */
    int num_icons_per_column = floorf(((_tms.window_width/2.f) - ((_tms.xppcm*2.f)/2.f) - margin_x - icon_width/2.f) / (icon_width + margin_x));
    int col = 0;
    int row = 0;

    static tvec4 bg_color = { .5f, .5f, .5f, 0.5f };
    static tvec4 bg_color_sel = { .5f, 1.0f, 0.5f, 0.5f };

    struct tms_ddraw *dd = G->get_surface()->ddraw;
    struct tms_sprite *img;

    if (G->get_mode() == GAME_MODE_DEFAULT) {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        /* draw sprites */

        if (adventure::player->g_id == O_ROBOT) {
            robot *r = static_cast<robot*>(adventure::player);

            adventure::num_weapons = r->num_weapons;

            for (int n=0; n<r->num_weapons; ++n) {
                if (col >= num_icons_per_column) {
                    col = 0;
                    row ++;
                }

                uint32_t item_id = _weapon_to_item[adventure::player->weapons[n]->get_weapon_type()];
                if (item_id >= NUM_ITEMS) continue;
                img = &item_options[item_id].image;

                float x = base_x + (col * icon_width) + (col * margin_x);
                float y = base_y - (row * icon_height) - (row * margin_y);

                adventure::weapon_icon_pos[n] = (tvec2){x,y};

                bool sel = (r->weapon == r->weapons[n]);

                if (sel) {
                    tms_ddraw_set_color(dd, bg_color_sel.x, bg_color_sel.y, bg_color_sel.z, bg_color_sel.a);
                    tms_ddraw_square(dd,
                            x, y,
                            icon_width+8,
                            icon_height+8
                            );
                    tms_ddraw_set_color(dd, bg_color_sel.x/2.f, bg_color_sel.y/2.f, bg_color_sel.z/2.f, 1.f);
                    tms_ddraw_lsquare(dd,
                            x, y,
                            icon_width+8,
                            icon_height+8
                            );
                } else {
                    tms_ddraw_set_color(dd, bg_color.x, bg_color.y, bg_color.z, bg_color.a);
                    tms_ddraw_square(dd,
                            x, y,
                            icon_width,
                            icon_height
                            );
                }

                tms_ddraw_set_color(dd, 1.f, 1.f, 1.f, 1.f);

                tms_ddraw_sprite(dd, img,
                        x, y,
                        icon_width,
                        icon_height
                        );

                col ++;
            }

            if (col != 0 || row != 0) {
                col = 0;
                row ++;
            }
        }

        if (adventure::player->g_id == O_ROBOT) {
            robot *r = static_cast<robot*>(adventure::player);

            adventure::num_tools = r->num_tools;

            for (int n=0; n<r->num_tools; ++n) {
                if (col >= num_icons_per_column) {
                    col = 0;
                    row ++;
                }

                uint32_t item_id = _tool_to_item[adventure::player->tools[n]->get_arm_type()];
                if (item_id == NUM_ITEMS) continue;
                img = &item_options[item_id].image;

                float x = base_x + (col * icon_width) + (col * margin_x);
                float y = base_y - (row * icon_height) - (row * margin_y);

                adventure::tool_icon_pos[n] = (tvec2){x,y};

                bool sel = (r->tool == r->tools[n]);

                if (sel) {
                    tms_ddraw_set_color(dd, bg_color_sel.x, bg_color_sel.y, bg_color_sel.z, bg_color_sel.a);
                    tms_ddraw_square(dd,
                            x, y,
                            icon_width+8,
                            icon_height+8
                            );
                    tms_ddraw_set_color(dd, bg_color_sel.x/2.f, bg_color_sel.y/2.f, bg_color_sel.z/2.f, 1.f);
                    tms_ddraw_lsquare(dd,
                            x, y,
                            icon_width+8,
                            icon_height+8
                            );
                } else {
                    tms_ddraw_set_color(dd, bg_color.x, bg_color.y, bg_color.z, bg_color.a);
                    tms_ddraw_square(dd,
                            x, y,
                            icon_width,
                            icon_height
                            );
                }

                tms_ddraw_set_color(dd, 1.f, 1.f, 1.f, 1.f);
                tms_ddraw_sprite(dd, img,
                        x, y,
                        icon_width,
                        icon_height
                        );

                col ++;
            }
        }

        if (col)
            ++ row;

        glBindTexture(GL_TEXTURE_2D, gui_spritesheet::atlas->texture.gl_texture);

#ifdef TMS_BACKEND_PC
        for (int x=0; x<adventure::num_weapons; x++) {
            G->render_num(adventure::weapon_icon_pos[x].x, adventure::weapon_icon_pos[x].y-_tms.yppcm*.075f,
                    0,
                    0,
                    x+1,
                    0,
                    -.1f, false
                    );
        }
#endif

        float x = base_x;
        float y = base_y - (row * icon_height) - (row * margin_y);
        inventory_begin_y = y + icon_height/2.f;

        for (int n=0; n<NUM_RESOURCES; ++n) {
            if (adventure::player->get_num_resources(n)
#ifdef TMS_BACKEND_MOBILE
                    && n == adventure::last_picked_up_resource
#endif
                    ) {
                float alpha = 1.f;
                float extra_scale = adventure::highlight_inventory[n] * .5f;
                adventure::highlight_inventory[n] *= powf(.008f, _tms.dt);

                tms_ddraw_set_color(dd, 1.f+(2.f*extra_scale), 1.f+(2.f*extra_scale), 1.f+(2.f*extra_scale), alpha);
                tms_ddraw_sprite(dd, gui_spritesheet::get_sprite(S_INVENTORY_ICONS0+n),
                        x, y,
                        icon_width, icon_height
                        );

                G->render_num(x, y, icon_width, icon_height, adventure::player->get_num_resources(n), 0, extra_scale);

                y -= icon_height + margin_y;
            }
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void
adventure::tool_changed()
{
    robot_parts::tool *t = adventure::player->get_tool();

    if (t) {
        char tmp[256];
        //snprintf(tmp, 255, "(%s) (?)", t->get_name());
        snprintf(tmp, 255, "%s (?)", t->get_name());
        adventure::current_tool->set_text(tmp);
    } else {
        adventure::current_tool->set_text("");
    }

    G->refresh_widgets();
}
