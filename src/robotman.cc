#include "entity.hh"
#include "robotman.hh"
#include "model.hh"
#include "material.hh"
#include "game.hh"
#include "robot_base.hh"
#include "adventure.hh"
#include "faction.hh"

#define BASE_HP_MODIFIER 10.f

/**
 * Outputs:
 * 0 = Weapon arm angle
 * 1 = Tool arm angle
 * 2 = On weapon fire
 * 3 = On tool use?
 * 4 = Current layer. 0.0 = layer 1, 0.5 = layer 2, 1.0 = layer 3
 * 5 = Head removed
 * 6 = Moving left - User attempting to move left (adventure robot only)
 * 7 = Moving right - User attempting to move right (adventure robot only)
 * 8 = Look dir (left = 0.0, right = 1.0)
 * 9 = dir - Last movement dir (left = 0.0, right = 1.0)
 * 10 = action active
 * 11 = Attached to anything     (adventure robot only)
 * 12 = Movement left feedback   (adventure robot only)
 * 13 = Movement right feedback  (adventure robot only)
 * 14 = jump feedback            (adventure robot only)
 * 15 = aim feedback             (adventure robot only)
 * 16 = layerup feedback         (adventure robot only)
 * 17 = layerdown feedback       (adventure robot only)
 * 18 = action feedback          (adventure robot only)
 *
 * Inputs:
 * 0 = Enable godmode
 * 1 = Speed modifier (0.0-1.0). Only enabled if the cable is plugged in.
 *     A value of 0.0 means the robot is unable to walk.
 * 2 = Disable action (useful for disabling box mode until certain powerup is enabled)
 * 3 = Jump strength multiplier
 * 4 = HP increase (heal), base increase = 10 hp
 * 5 = HP decrease (damage), base decrease = 10 hp
 * 6 = Max HP increase, base increase = 10 hp . Heals for the same amount added.
 * 7 = Max HP decrease, base decrease = 10 hp
 * 8 = Weapon damage multiplier 1.0 = 5.0 multiplier, 0.0 = 0.0 multiplier
 * 9 = Toggle robot action
 * 10 = Walk left
 * 11 = Walk right
 * 12 = Jump
 * 13 = Aim
 * 14 = Attack
 * 15 = Toggle roaming (non-adventure robots only)
 * 16 = Cycle available weapons
 * 17 = Cycle faction
 **/

robotman::robotman()
    : target(0)
{
    this->set_flag(ENTITY_HAS_TRACKER,  true);
    this->set_flag(ENTITY_DO_STEP,      true);

    this->menu_scale = .75f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOTMAN));
    this->set_material(&m_edev);

    this->set_num_properties(1);
    this->properties[0].type = P_INT; // target id
    this->properties[0].v.i = 0;

    delete [] this->s_out;
    this->num_s_out = RMAN_NUM_OUT;
    this->s_out = new socket_out[this->num_s_out];
    delete [] this->s_in;
    this->num_s_in = RMAN_NUM_IN;
    this->s_in = new socket_in[this->num_s_in];

    this->scaleselect = true;

    this->set_as_rect(2.5f/2.f, 2.2f/2.f);

    float w = -this->width + EDEV_SOCKET_SIZE/2.f + 0.05f;
    float h = this->height - EDEV_SOCKET_SIZE/2.f - 0.05f;
    float _h = this->height - 1.7f;

    for (int n=0; n<RMAN_NUM_IN; ++n) {
        int x = n % ROBOTMAN_SOCK_NUMCOL;
        int y = n / ROBOTMAN_SOCK_NUMCOL;
        float _x = w + x*.3f;
        float _y = h - y*.3f;
        this->s_in[n].lpos = b2Vec2(_x, _y);
        this->s_in[n].ctype = CABLE_RED;
        this->s_in[n].angle = M_PI/2.f;
    }

    this->s_out[0].lpos = b2Vec2(-1.05f, _h+.3f);
    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].angle = M_PI/2.f;
    this->s_out[1].lpos = b2Vec2(-0.75f, _h+.3f);
    this->s_out[1].ctype = CABLE_RED;
    this->s_out[1].angle = M_PI/2.f;

    for (int n=2; n<RMAN_NUM_OUT; ++n) {
        int x = (n-2) % ROBOTMAN_SOCK_NUMCOL;
        int y = (n-2) / ROBOTMAN_SOCK_NUMCOL;
        float _x =  w + x*.3f;
        float _y = _h - y*.3f;
        this->s_out[n].lpos = b2Vec2(_x, _y);
        this->s_out[n].ctype = CABLE_RED;
        this->s_out[n].angle = M_PI/2.f;
    }

    this->s_in[RMAN_WALK_LEFT].tag = SOCK_TAG_LEFT;
    this->s_in[RMAN_WALK_RIGHT].tag = SOCK_TAG_RIGHT;
    this->s_in[RMAN_JUMP].tag = SOCK_TAG_UP;
    this->s_in[RMAN_ATTACK].tag = SOCK_TAG_ATTACK;
    this->s_in[RMAN_RESPAWN].tag = SOCK_TAG_RESPAWN;
    this->s_in[RMAN_FREEZE].tag = SOCK_TAG_FREEZE;
}

void
on_robotman_target_absorbed(entity *self, void *userdata)
{
    robotman *rm = static_cast<robotman*>(self);
    rm->unsubscribe((entity*)rm->get_target());
    rm->set_target(0);
}

void
robotman::init()
{
    if (this->properties[0].v.i != 0) {
        entity *e = W->get_entity_by_id(this->properties[0].v.i);
        if (e && e->is_robot()) {
            this->target = static_cast<robot_base*>(e);
            this->subscribe(this->target, ENTITY_EVENT_REMOVE, &on_robotman_target_absorbed);
        }
    }
}

void
robotman::setup()
{
    memset(this->values, 0, sizeof(float)*RMAN_NUM_IN);
    memset(this->previous_values, 0, sizeof(float)*RMAN_NUM_IN);
}

void
robotman::step()
{
    if (!this->target) return;

    for (int x=0; x<RMAN_NUM_IN; ++x) {
        float val = this->values[x];
        float prev_val = this->previous_values[x];

        bool v = (bool)(val > 0.f && (int)roundf(val));
        bool prev_v = (bool)(prev_val > 0.f && (int)roundf(prev_val));
        switch (x) {
            case RMAN_ENABLE_GODMODE:
                if (val > 0.f) {
                    this->target->set_creature_flag(CREATURE_GODMODE, v);
                }
                break;

            case RMAN_SPEED_MODIFIER:
                if (val > 0.f) {
                    this->target->speed_modifier = val;
                }
                break;

            case RMAN_DISABLE_ACTION:
                if (val > 0.f) {
                    this->target->set_creature_flag(CREATURE_DISABLE_ACTION, v);
                }
                break;

            case RMAN_JUMP_STRENGTH_MODIFIER:
                if (val > 0.f) {
                    this->target->jump_strength_multiplier = tclampf(val, 0.f, 1.f);
                }
                break;

            case RMAN_HP_INCREASE:
                if (val > 0.f) {
                    float amount = -(val * BASE_HP_MODIFIER);
                    this->target->damage(amount, 0, DAMAGE_TYPE_OTHER, DAMAGE_SOURCE_WORLD, 0);
                }
                break;

            case RMAN_HP_DECREASE:
                if (val > 0.f && !this->target->is_dead()) {
                    float amount = (val * BASE_HP_MODIFIER);
                    this->target->damage(amount, 0, DAMAGE_TYPE_OTHER, DAMAGE_SOURCE_WORLD, 0);
                }
                break;

            case RMAN_MAX_HP_INCREASE:
                if (val > 0.f) {
                    float amount = (val * BASE_HP_MODIFIER);

                    if (this->target->increase_max_hp(amount)) {
                        this->target->damage(-amount, 0, DAMAGE_TYPE_OTHER, DAMAGE_SOURCE_WORLD, 0);
                    }
                }
                break;

            case RMAN_MAX_HP_DECREASE:
                if (val > 0.f) {
                    float amount = -(val * BASE_HP_MODIFIER);

                    this->target->increase_max_hp(amount);
                }
                break;

            case RMAN_WEAPON_DAMAGE_MULTIPLIER:
                if (val > 0.f) {
                    this->target->set_attack_damage_modifier(val*5.f);
                }
                break;

            case RMAN_TOGGLE_ACTION:
                if (v && !this->target->is_dead()) {
                    if (this->target->is_action_active()) {
                        this->target->action_off();
                    } else {
                        this->target->action_on();
                    }
                }
                break;

            case RMAN_WALK_LEFT:
                if (val > 0.f) {
                    if (v && !prev_v) {
                        this->target->move(DIR_LEFT);
                        this->target->look(DIR_LEFT);
                    }
                } else if (prev_v && !v) {
                    tms_infof("STOP MOVING LEFT -----------------");
                    this->target->stop_moving(DIR_LEFT);
                }
                break;

            case RMAN_WALK_RIGHT:
                if (val > 0.f) {
                    if (v && !prev_v) {
                        this->target->move(DIR_RIGHT);
                        this->target->look(DIR_RIGHT);
                    }
                } else if (prev_v && !v) {
                    this->target->stop_moving(DIR_RIGHT);
                }
                break;

            case RMAN_STOP_MOVEMENT:
                if (v) {
                    this->target->stop_moving(DIR_LEFT);
                    this->target->stop_moving(DIR_RIGHT);
                }
                break;

            case RMAN_JUMP:
                if (val > 0.f) {
                    if (v && this->target->is_standing()) {
                        this->target->jump(false);
                    }
                }
                break;

            case RMAN_AIM:
                if (val > 0.f) {
                    float a = val;

                    /* offset and wrap the angle */
                    a = twrapf(a - 1.00f, -1.f, 1.f);

                    /* convert a value between -1 and 1 to a value the robot can work with */
                    a *= (M_PI*2.f);

                    a -= this->target->get_angle();
                    this->target->aim(a);
                }
                break;

            case RMAN_ATTACK:
                if (v) {
                    this->target->attack();
                }
                break;

            case RMAN_ATTACH_NEAREST:
                if (v && !this->target->is_attached_to_activator()) {
                    this->target->activate_closest_activator();
                }
                break;

            case RMAN_DEATTACH:
                if (v) {
                    this->target->detach();
                }
                break;

            case RMAN_RESPAWN:
                if (v) {
                    this->target->respawn();
                }
                break;

            case RMAN_FREEZE:
                if (v) {
                    tms_infof("SET FROZEN");
                    this->target->set_creature_flag(CREATURE_FROZEN, true);
                } else if (prev_v && !v) {
                    tms_infof("UNSET FROZEN");
                    this->target->set_creature_flag(CREATURE_FROZEN, false);
                }
                break;

            case RMAN_TOGGLE_ROAM:
                if (v && this->target != adventure::player) {
                    this->target->properties[ROBOT_PROPERTY_ROAMING].v.i8 = !this->target->properties[ROBOT_PROPERTY_ROAMING].v.i8;
                }
                break;

            case RMAN_CYCLE_WEAPONS:
                if (v) {
                    robot_parts::weapon *w = this->target->get_weapon();
                    if (w) {
                        for (int x=w->get_weapon_type()+1; x<w->get_weapon_type()+NUM_WEAPONS; ++x) {
                            int wid = x % NUM_WEAPONS;
                            if (this->target->weapons[wid]) {
                                this->target->equip_weapon(wid);
                                break;
                            }
                        }
                    }
                }
                break;

            case RMAN_CYCLE_FACTIONS:
                if (v) {
                    faction_info *current_faction = this->target->get_faction();
                    faction_info *fi = this->target->set_faction((current_faction->id+1)%NUM_FACTIONS);
                    this->target->set_uniform("~color", fi->color.r, fi->color.g, fi->color.b, 1.f);
                }
                break;
        }
    }
}

edevice*
robotman::solve_electronics(void)
{
    for (int x=0; x<RMAN_NUM_OUT; ++x) {
        if (this->s_out[x].written()) continue;

        if (!this->s_out[x].p || !this->target) {
            this->s_out[x].write(0.f);
        } else {
            float v = 0.f;

            switch (x) {
                case RMAN_WEAPON_ARM_ANGLE:
                    {
                        robot_parts::weapon *w = this->target->get_weapon();
                        if (w) {
                            v = w->get_arm_angle();
                        }
                    }
                    break;

                case RMAN_TOOL_ARM_ANGLE:
                    {
                        robot_parts::tool *t = this->target->get_tool();
                        if (t) {
                            v = t->get_arm_angle();
                        }
                    }
                    break;

                case RMAN_ON_WEAPON_FIRE:
                    {
                        robot_parts::weapon *w = this->target->get_weapon();
                        if (w && w->fired) {
                            v = 1.f;
                        }
                    }
                    break;

                case RMAN_ON_TOOL_USE:
                    // XXX: Not implemented
                    break;

                case RMAN_CURRENT_LAYER:
                    v = this->target->get_layer() * 0.5f;
                    break;

                case RMAN_HEAD_REMOVED:
                    if (this->target->head == 0) {
                        v = 1.f;
                    }
                    break;

                case RMAN_MOVING_LEFT:
                    if (this->target->is_moving_left())
                        v = 1.f;
                    break;

                case RMAN_MOVING_RIGHT:
                    if (this->target->is_moving_right())
                        v = 1.f;
                    break;

                case RMAN_I_DIR:
                    v = this->target->i_dir;
                    break;

                case RMAN_DIR:
                    if (this->target->dir == DIR_RIGHT)
                        v = 1.f;
                    break;

                case RMAN_ACTION_ACTIVE:
                    if (this->target->is_action_active())
                        v = 1.f;
                    break;

                case RMAN_IS_ATTACHED:
                    if (this->target->is_attached_to_activator())
                        v = 1.f;
                    break;

                case RMAN_FB_LEFT:
                    if (adventure::focused(ADVENTURE_FOCUSED_LEFT))
                        v = 1.f;
                    break;
                case RMAN_FB_RIGHT:
                    if (adventure::focused(ADVENTURE_FOCUSED_RIGHT))
                        v = 1.f;
                    break;
                case RMAN_FB_JUMP:
                    if (adventure::focused(ADVENTURE_FOCUSED_JUMP))
                        v = 1.f;
                    break;
                case RMAN_FB_AIM:
                    if (adventure::focused(ADVENTURE_FOCUSED_AIM))
                        v = 1.f;
                    break;
                case RMAN_FB_LAYERUP:
                    if (adventure::focused(ADVENTURE_FOCUSED_LAYER_UP))
                        v = 1.f;
                    break;
                case RMAN_FB_LAYERDOWN:
                    if (adventure::focused(ADVENTURE_FOCUSED_LAYER_DOWN))
                        v = 1.f;
                    break;
                case RMAN_FB_ACTION:
                    if (adventure::focused(ADVENTURE_FOCUSED_ACTION))
                        v = 1.f;
                    break;

                default:
                    tms_warnf("Unknown out-action, %d", x);
                    break;
            }

            this->s_out[x].write(v);
        }
    }

    for (int x=0; x<RMAN_NUM_IN; ++x) {
        if (!this->s_in[x].is_ready()) {
            return this->s_in[x].get_connected_edevice();
        } else {
            // store the inputs into an array, which we can read from ::step
            // where it's safe to perform the actions
            float v = this->s_in[x].get_value();
            this->previous_values[x] = this->values[x];
            this->values[x] = (this->s_in[x].p ? v : -1.f);
        }
    }

    return 0;
}

void
robotman::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::write_state(lvl, lb);

    for (int x=0; x<RMAN_NUM_IN; ++x) {
        lb->w_s_float(this->previous_values[x]);
        lb->w_s_float(this->values[x]);
    }
}

void
robotman::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::read_state(lvl, lb);

    for (int x=0; x<RMAN_NUM_IN; ++x) {
        this->previous_values[x] = lb->r_float();
        this->values[x] = lb->r_float();
    }
}
