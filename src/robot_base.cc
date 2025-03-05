#include "robot_base.hh"
#include "material.hh"
#include "game.hh"
#include "world.hh"
#include "fxemitter.hh"
#include "item.hh"
#include "adventure.hh"
#include "faction.hh"
#include "ui.hh"
#include "anchor.hh"

#include <Box2D/Box2D.h>

#include <math.h>

#define ROBOT_LADDER_CLIMB_SPEED .6f

creature_effect *robot_base::panic_effect    = new creature_effect(EFFECT_TYPE_SPEED, EFFECT_METHOD_MULTIPLICATIVE, 1.5f, 2*1000*1000);
creature_effect *robot_base::slowdown_effect = new creature_effect(EFFECT_TYPE_SPEED, EFFECT_METHOD_MULTIPLICATIVE, 0.1f, 1*1000*1000);

robot_base::robot_base()
{
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_CUSTOM_GHOST_UPDATE,  true);
    this->set_flag(ENTITY_IS_ROBOT,             true);
    this->set_flag(ENTITY_IS_MAGNETIC,          true);
    this->set_flag(ENTITY_DO_STEP,              true);
    this->set_flag(ENTITY_DO_MSTEP,             true);
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS,    true);
    this->set_flag(ENTITY_HAS_CONFIG,           true);
    this->set_flag(ENTITY_IS_INTERACTIVE,       false);

    this->dialog_id = DIALOG_ROBOT;

    this->f_body = 0;
    this->f_sensor = 0;
    this->num_sliders = 2;
    this->menu_scale = .75f;
    this->width = 1.f;
    this->height = 1.f;
    this->type = ENTITY_ROBOT;
    this->cull_effects_method = CULL_EFFECTS_DISABLE;
    this->update_method = ENTITY_UPDATE_CUSTOM;
    this->set_material(&m_robot_tinted);
    this->set_uniform("~color", ROBOT_COLOR, 1.f);

    this->robot_type = ROBOT_TYPE_ROBOT;

    /* default eye position is centroid */
    this->eye_pos = b2Vec2(0.f, 0.f);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->set_num_properties(14);
    this->properties[ROBOT_PROPERTY_SPEED].type = P_FLT;  /* Robot speed */
    this->properties[ROBOT_PROPERTY_STATE].type = P_INT8; /* Robot state */
    this->properties[ROBOT_PROPERTY_ROAMING].type = P_INT8; /* Roaming */
    this->properties[ROBOT_PROPERTY_ATTACK_DELAY]  .type = P_INT;  /* Attack delay (in ms) */
    this->properties[ROBOT_PROPERTY_DIR].type = P_INT8; /* Initial robot direction */
    this->properties[ROBOT_PROPERTY_HP].type = P_FLT;  /* Initial robot health in fraction */
    this->properties[ROBOT_PROPERTY_FACTION].type = P_INT8; /* faction */
    this->properties[ROBOT_PROPERTY_EQUIPMENT].type = P_STR;  /* initial list of equipment */
    this->properties[ROBOT_PROPERTY_FEET].type = P_INT8; /* feet */
    this->properties[ROBOT_PROPERTY_HEAD].type = P_INT8; /* head */
    this->properties[ROBOT_PROPERTY_BACK].type = P_INT8; /* back equipment */
    this->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].type = P_INT8; /* head equipment */
    this->properties[ROBOT_PROPERTY_FRONT].type = P_INT8; /* front equipment */
    this->properties[ROBOT_PROPERTY_BOLT_SET].type = P_INT8; /* bolts */

    this->properties[ROBOT_PROPERTY_SPEED].v.f = ROBOT_DEFAULT_SPEED;
    this->properties[ROBOT_PROPERTY_STATE].v.i8 = CREATURE_IDLE;
    this->properties[ROBOT_PROPERTY_ROAMING].v.i8 = 0;
    this->properties[ROBOT_PROPERTY_ATTACK_DELAY].v.i = 500;
    this->properties[ROBOT_PROPERTY_DIR].v.i8 = 2;
    this->properties[ROBOT_PROPERTY_HP].v.f = 1.f;
    this->properties[ROBOT_PROPERTY_FACTION].v.i8 = FACTION_ENEMY;
    this->set_property(ROBOT_PROPERTY_EQUIPMENT, "");
    this->properties[ROBOT_PROPERTY_FEET].v.i8 = FEET_BIPED;
    this->properties[ROBOT_PROPERTY_HEAD].v.i8 = HEAD_NULL;
    this->properties[ROBOT_PROPERTY_BACK].v.i8 = BACK_EQUIPMENT_NULL;
    this->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8 = HEAD_EQUIPMENT_NULL;
    this->properties[ROBOT_PROPERTY_FRONT].v.i8 = FRONT_EQUIPMENT_NULL;
    this->properties[ROBOT_PROPERTY_BOLT_SET].v.i8 = BOLT_SET_STEEL;

    this->balance = new stabilizer(0);

    this->handler = new robot_base::cb_handler(this);
    this->vision_handler = new robot_base::cb_vision_handler(this);

    this->feet_offset = .45f;
    this->feet_width = .5f; /* only for quadruped */

    /* initialize a few variables that needs to be initialized in the constructor,
     * because some stuff like the legs depend on them to calculate their rendering */
    this->consume_timer = 0.f;
    this->action_active = false;

    this->balance->limit = 100.f;

    /* by default, all robots support the riding circuit */
    this->circuits_compat |= CREATURE_CIRCUIT_RIDING;
}


void
robot_base::construct()
{
    /* set roaming to true automatically if this is an adventure level */
    this->properties[2].v.i8 = (G && W && W->level.type == LCAT_ADVENTURE);
}

robot_base::~robot_base()
{
    delete this->handler;
    delete this->vision_handler;
}

void
robot_base::on_load(bool created, bool has_state)
{
    creature::on_load(created, has_state);

    this->angular_damping = ROBOT_DAMPING;
    this->set_faction(this->properties[6].v.i8);

    this->hp = this->max_hp * this->properties[5].v.f;

    if (W->is_paused() && !has_state && !created) {
        this->clear_equipment();
        this->refresh_equipment();
        this->equip_defaults();
    }

    if (this->properties[4].v.i8 == 1) {
        this->new_dir = DIR_LEFT;
    } else if (this->properties[4].v.i8 == 2) {
        this->new_dir = DIR_RIGHT;
    } else {
        this->new_dir = (rand()%2 == 0)?1:-1;
    }

    this->dir = this->new_dir;
    this->look_dir = this->new_dir;
    this->i_dir = this->new_dir;

    this->reset_angles();
}

void
robot_base::init()
{
    /* TODO: move over things from setup() */
    creature::init();

    this->logic_timer_max = ROBOT_LOGIC_TIMER_MAX;
    this->min_box_ticks = 0;
    this->roam_target_aim = 0.f;

    this->jump_action = 0;
    this->jump_action_time = 0;

    this->mood.data = this;
}

void
robot_base::restore()
{
    creature::restore();

    this->recalculate_effects();
}

void
robot_base::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    creature::read_state(lvl, lb);

    memset(this->tools, 0, sizeof(this->tools));
    memset(this->weapons, 0, sizeof(this->weapons));

    if (this->has_feature(CREATURE_FEATURE_WEAPONS)) {
        this->num_weapons = lb->r_uint16();
        for (int x=0; x<num_weapons; x++) {
            uint8_t weapon_id = lb->r_uint8();

            this->weapons[x] = robot_parts::weapon::make(weapon_id, this);

            if (this->weapons[x]) {
                this->weapons[x]->read_state(lvl, lb);
            }
        }

        uint8_t active_weapon_slot = lb->r_uint8();
        if (this->weapons[active_weapon_slot]) {
            this->set_weapon(this->weapons[active_weapon_slot]->get_weapon_type(), this->weapons[active_weapon_slot]);
        }
    }
    if (this->has_feature(CREATURE_FEATURE_TOOLS)) {
        this->num_tools = lb->r_uint16();
        for (int x=0; x<num_tools; x++) {
            uint8_t tool_id = lb->r_uint8();

            this->tools[x] = robot_parts::tool::make(tool_id, this);

            if (this->tools[x]) {
                this->tools[x]->read_state(lvl, lb);
            }
        }

        uint8_t active_tool_slot = lb->r_uint8();
        if (this->tools[active_tool_slot]) {
            this->set_tool(this->tools[active_tool_slot]->get_tool_type(), this->tools[active_tool_slot]);
        }
    }
}

void
robot_base::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    creature::write_state(lvl, lb);

    if (this->has_feature(CREATURE_FEATURE_WEAPONS)) {
        lb->w_s_uint16(this->num_weapons);

        robot_parts::weapon *curr = this->get_weapon();
        uint8_t active_weapon_slot = 0;

        for (int x=0; x<this->num_weapons; ++x) {
            robot_parts::weapon *w = this->weapons[x];
            lb->w_s_uint8(w->get_weapon_type());
            w->write_state(lvl, lb);

            if (curr == w) {
                active_weapon_slot = x;
            }
        }

        lb->w_s_uint8(active_weapon_slot);
    }
    if (this->has_feature(CREATURE_FEATURE_TOOLS)) {
        lb->w_s_uint16(this->num_tools);

        robot_parts::tool *curr = this->get_tool();
        uint8_t active_tool_slot = 0;

        for (int x=0; x<this->num_tools; ++x) {
            robot_parts::tool *t = this->tools[x];
            lb->w_s_uint8(t->get_tool_type());
            t->write_state(lvl, lb);

            if (curr == t) {
                active_tool_slot = x;
            }
        }

        lb->w_s_uint8(active_tool_slot);
    }
}

void
robot_base::setup()
{
    tms_debugf("robot_base::setup");
    creature::setup();

    if (this->properties[5].v.f <= 0.f) {
        this->set_creature_flag(CREATURE_GODMODE, true);
    }

    this->jump_strength = this->base_jump_strength = ROBOT_JUMP_FORCE;

    this->logic_timer = rand()%this->logic_timer_max;

    if (this->id == G->state.adventure_id) {
        this->properties[ROBOT_PROPERTY_ROAMING].v.i8 = 0;
        /* speed already set by init_adventure */

        /* player-robots by default equip the regeneration circuit,
         * I'm thinking this should be placed somewhere else? */
        this->set_has_circuit(CREATURE_CIRCUIT_REGENERATION, true);
    } else {
        this->base_speed = this->speed = this->properties[0].v.f;
    }

    this->set_state(this->properties[1].v.i8);

    this->refresh_equipment();
    this->equip_defaults();

    this->recalculate_effects();

    if (W->is_playing()) {
        this->create_layermove_sensors();
    }
}

void
robot_base::equip_defaults()
{
    /* equip the weapon and tool with the highest id */

    if (this->num_weapons) {
        this->equip_weapon(this->weapons[this->num_weapons-1]->get_weapon_type(), false);
    }
    if (this->num_tools) {
        this->equip_tool(this->tools[this->num_tools-1]->get_tool_type(), false);
    }
}

void
robot_base::clear_equipment()
{
    for (int x=0; x<NUM_WEAPONS; ++x) {
        this->remove_weapon(this->weapons[x]);
    }

    for (int x=0; x<NUM_TOOLS; ++x) {
        this->remove_tool(this->tools[x]);
    }
}

void
robot_base::refresh_equipment()
{
    this->num_weapons = 0;
    this->num_tools = 0;

    std::vector<char*> strings = p_split(this->properties[7].v.s.buf, this->properties[7].v.s.len, ";");

    for (std::vector<char*>::iterator it = strings.begin();
            it != strings.end(); ++it) {
        int item_id = atoi(*it);
        tms_infof("Item id=%d from string %s", item_id, this->properties[7].v.s.buf);
        item *_item = new item(item_id);

        this->consume(_item, true, true);

        delete _item;
    }
}

void
robot_base::on_pause()
{
    creature::on_pause();

    this->refresh_equipment();
    this->equip_defaults();
}

void
robot_base::on_damage(float dmg, b2Fixture *f, damage_type dt, uint8_t damage_source, uint32_t attacker_id)
{
    creature::on_damage(dmg, f, dt, damage_source, attacker_id);

    if (this->hp <= 0.f) {
        return;
    }

    this->mood.add(MOOD_ANGER, (this->damage_accum[dt]/60.f)*this->faction->mood_sensitivity[MOOD_ANGER]);
    this->mood.add(MOOD_FEAR,  (this->damage_accum[dt]/80.f)*this->faction->mood_sensitivity[MOOD_FEAR]);

    if (damage_source != DAMAGE_SOURCE_BULLET || attacker_id == 0) {
        return;
    }

    // we use get_entity_by_id in case the robot who fired the bullet has been absorbed
    entity *damage_source_ent = W->get_entity_by_id(attacker_id);

    if (!damage_source_ent || !damage_source_ent->flag_active(ENTITY_IS_ROBOT)) {
        return;
    }

    robot_base *attacker = static_cast<robot_base*>(damage_source_ent);

    /* TODO: Should each robot keep a personal copy of the faction data, and be able to make changes to it if provoked?
     * So if a neutral creature is attacked, after a while it can become hostile to the "Friendly" faction */
    if (attacker->faction->id != this->faction->id) {
        float mval[NUM_MOODS];
        for (int x=0; x<NUM_MOODS; ++x) {
            mval[x] = this->mood.get(x);
        }
        float cowardice = mval[MOOD_FEAR] > mval[MOOD_BRAVERY] ? mval[MOOD_FEAR] - mval[MOOD_BRAVERY] : 0.f;
        if (cowardice > 3.f && rand()%(this->is_roaming() ? 20 : 2) == 0 && this->robot_type == ROBOT_TYPE_ROBOT && !this->is_action_active()) {
            tms_debug_roam("I'M BEING ATTACKED BY %d of faction %d, BOX!", attacker_id, attacker->faction->id);
            /* TODO: When being boxed, all movement should be stopped, and the target should be maintained.
             * This way, when the robot is unboxed he will hopefully be in the same angle as he was when he boxed,
             * and be able to shoot the enemy immediately. */

            this->set_creature_flag(CREATURE_BOX_ME, true);
            this->roam_unset_target();
            this->min_box_ticks = 15;
        } else if (this->is_roaming() && (!this->roam_target_id || rand()%50 == 0) && this->roam_can_target(attacker, false)) {
            tms_debug_roam("attacker id: %d", attacker_id);
            tms_debug_roam("faction_id: %d", attacker->faction->id);
            this->roam_set_target(attacker);
        }
    }
}

void
robot_base::roam_set_target(entity *e)
{
    creature::roam_set_target(e);

    /* signal nearby friends of our finding */
    if (e->is_creature()) {
        this->roam_set_target_type();

        if (this->roam_target_type == TARGET_ENEMY) {
            this->set_creature_flag(CREATURE_QUERY_ROBOTS, true);

            b2AABB aabb;
            aabb.lowerBound = this->local_to_world(b2Vec2(-5.f, -5.f), 0);
            aabb.upperBound = this->local_to_world(b2Vec2( 5.f,  5.f), 0);
            this->results.clear();
            W->b2->QueryAABB(this, aabb);

            this->set_creature_flag(CREATURE_QUERY_ROBOTS, false);

            for (std::set<entity*>::iterator it = this->results.begin();
                    it != this->results.end(); ++it) {
                robot_base *rob = static_cast<robot_base*>(*it);

                switch (this->faction->relationship[rob->faction->id]) {
                    case FRIENDLY:
                        if (rob->roam_target_id == 0) {
                            rob->roam_target_id = e->id;
                        }
                        break;
                }
            }
        }
    }
}

void
robot_base::on_death()
{
    creature::on_death();

    {
        // Search for nearby robots
        this->set_creature_flag(CREATURE_QUERY_ROBOTS, true);

        b2AABB aabb;
        aabb.lowerBound = this->local_to_world(b2Vec2(-10.f, -10.f), 0);
        aabb.upperBound = this->local_to_world(b2Vec2( 10.f,  10.f), 0);
        this->results.clear();
        W->b2->QueryAABB(this, aabb);

        this->set_creature_flag(CREATURE_QUERY_ROBOTS, false);

        for (std::set<entity*>::iterator it = this->results.begin();
                it != this->results.end(); ++it) {
            robot_base *rob = static_cast<robot_base*>(*it);

            switch (this->faction->relationship[rob->faction->id]) {
                case FRIENDLY:
                    // friends will get scared
                    rob->mood.add(MOOD_FEAR, .4f * rob->faction->mood_sensitivity[MOOD_FEAR]);
                    break;

                case ENEMY:
                    // enemies will get bravery and lose fear
                    rob->mood.add(MOOD_BRAVERY, .4f * rob->faction->mood_sensitivity[MOOD_BRAVERY]);
                    rob->mood.add(MOOD_FEAR,  -(.4f * rob->faction->mood_sensitivity[MOOD_FEAR]));
                    break;
            }
        }

    }

    this->set_friction(.9f);

    /*
    b2Vec2 dd = -W->get_gravity();
    dd *= this->get_weight() * 1.5f;
    this->body->ApplyForce(dd, this->body->GetWorldCenter());
    */

    G->lock();
    G->emit(new explosion_effect(
                //this->local_to_world(b2Vec2(-.5f + (rand()%100) / 100.f, -.5f + (rand()%100) / 100.f), 0),
                this->local_to_world(b2Vec2(0.f, .5f), 0),
                this->get_layer(),
                false, .25f
                ), 0);

    G->unlock();
}

/**
 * Apply default motion specials, when no other thing is involved
 * called when motion is MOTION_DEFAULT
 **/
void
robot_base::apply_default_motion()
{
    bool apply = false;

    if (!this->creature_flag_active(CREATURE_LOST_BALANCE)) {
        /* apply in-air movement */
        if (this == adventure::player/* || true*/) {
            if (!this->fixed_dir && this->equipments[EQUIPMENT_FEET] != 0) {
                double xv = this->creature_flag_active(CREATURE_MOVING_LEFT) ? -1. : this->creature_flag_active(CREATURE_MOVING_RIGHT) ? 1. : 0.f;

                //tms_infof("xv: %f", xv);

                double tspeed = this->get_tangent_speed();
                double diff = tspeed - this->last_ground_speed.x;

                //tms_infof("tangent speed: %f", (float)tspeed);

                b2Vec2 impulse;
                bool apply_impulse = false;
                bool braking = false;

                if (false && std::abs(diff) > 10.f) {
                    tms_debugf("ROBOT LOST BALANCE");
                    this->lose_balance();
                } else {
                    if (std::abs(xv) > .1) {
                        if ((fabs(diff) < 3. || copysign(1., xv) != copysign(1., diff))) {
                            xv*=.125;
                            apply_impulse = true;
                        }
                    } else {
                        /* in air braking */
                        braking = true;
                        apply_impulse = true;
                        xv = -diff*.125f;
                    }

                    if (apply_impulse) {
                        if ((braking || this->on_ground <= 0.f) && this->motion != MOTION_CLIMBING) {
                            impulse = this->get_tangent_vector(xv);

                            for (int x=0; x<this->get_num_bodies(); x++) {
                                b2Body *b;
                                if ((b = this->get_body(x))) {
                                    b->ApplyLinearImpulse(this->get_scale()*G->get_time_mul()*impulse, b->GetWorldCenter());
                                }
                            }
                        }
                    }
                }
            }
        } else if (this->on_ground <= 0.f && this->is_roaming() && this->jump_action != ROBOT_JUMP_ACTION_NONE && this->jump_time > this->jump_action_time) {
            /* apply in-air action */
            //float xv = this->creature_flag_active(CREATURE_MOVING_LEFT) ? -1. : this->creature_flag_active(CREATURE_MOVING_RIGHT) ? 1. : 0.f;
            float xv = this->dir;
            int ld;
            switch (this->jump_action) {
                case ROBOT_JUMP_ACTION_MOVE:
                    tms_debugf("applying jump action MOVE");
                    this->body->ApplyLinearImpulse(this->get_scale()*G->get_time_mul()*this->body->GetWorldVector(b2Vec2(xv*2.f, 0)), this->body->GetWorldCenter());
                    break;

                case ROBOT_JUMP_ACTION_LAYERMOVE:
                    if (target_layer < this->get_layer())
                        ld = -1;
                    else
                        ld = +1;
                    if (!this->layermove(ld))
                        this->layermove(-ld);
                    _mstep_layermove = true;
                    tms_debugf("applying jump action LAYERMOVE");
                    break;
            }
            this->jump_action = ROBOT_JUMP_ACTION_NONE;
        }
    }
}

void
robot_base::apply_climbing_motion()
{
    /* XXX currently only the adventure robot can climb ladders */
    entity *la = W->get_entity_by_id(this->ladder_id);

    if (la) {
        if (!this->j_climb) {
            this->set_damping(0.f);
#if 0
            b2FrictionJointDef fjd;
            fjd.Initialize(la->get_body(0), this->get_body(0), this->get_position());
            fjd.maxForce = 49.f;
            fjd.maxTorque = 10.f;
            fjd.collideConnected = true;
#elif 1
            b2MotorJointDef fjd;
            fjd.Initialize(this->get_body(0), la->get_body(0));
            fjd.maxForce = this->get_total_mass()*200.f;
            fjd.angularOffset = fjd.angularOffset + tmath_adist(la->get_body(0)->GetAngle(), this->get_body(0)->GetAngle());
            fjd.maxTorque = 20.f;
            fjd.correctionFactor = .5f;
            //fjd.correctionFactor = 0.f;
            fjd.collideConnected = true;
#else
            b2WeldJointDef fjd;
            fjd.Initialize(this->get_body(0), la->get_body(0), this->get_body(0)->GetPosition());
            fjd.collideConnected = true;
#endif
            this->j_climb = W->b2->CreateJoint(&fjd);
        }

        this->last_ground_speed = b2Vec2(0.f, 0.f);

        b2Vec2 imp=b2Vec2(0.f,0.f);
        if (this->creature_flag_active(CREATURE_MOVING_UP))
            imp += b2Vec2(0.f, .055f);
        if (this->creature_flag_active(CREATURE_MOVING_DOWN))
            imp += b2Vec2(0.f, -.055f);
        if (this->creature_flag_active(CREATURE_MOVING_LEFT))
            imp += b2Vec2(-.055f, .0f);
        if (this->creature_flag_active(CREATURE_MOVING_RIGHT))
            imp += b2Vec2(.055f, .0f);

        {
            b2MotorJoint *j = (b2MotorJoint*)this->j_climb;
            b2Vec2 curr_offset = j->GetBodyA()->GetLocalPoint(j->GetBodyB()->GetPosition());
            ((b2MotorJoint*)this->j_climb)->SetLinearOffset(curr_offset-imp);
        }

        float stepcount = 0.f;
        if (this->feet) stepcount = this->feet->stepcount;

        if (imp.LengthSquared() > 0.f) {
            if (this->feet) {
                this->feet->stepcount += .2f*G->get_time_mul();
            }
            //this->get_body(0)->ApplyForceToCenter(force);
            //this->natural_forces += force;
        }

        if (this->look_dir - DIR_BACKWARD_1 > this->look_dir - DIR_BACKWARD_2) {
            this->look_dir = DIR_BACKWARD_2;
        } else {
            this->look_dir = DIR_BACKWARD_1;
        }

        robot_parts::tool *t = this->get_tool();
        if (t) {
            t->set_arm_angle(6.5f+cos(stepcount*.8f)*.25f);
        }

        robot_parts::weapon *w = this->get_weapon();
        if (w) {
            w->set_arm_angle(6.5f+cos(M_PI+stepcount*.8f)*.25f);
        }

        if (this->ladder_time < 16000) {
            return;
        }
    }

    /* reach here if we're not climbing */
    this->stop_climbing();
}

void
robot_base::stop_climbing()
{
    this->set_creature_flag(CREATURE_CLIMBING_LADDER, false);
    this->ladder_time = 16000;
    this->ladder_id = 0;

    if (this->j_climb) {
        this->reset_damping();
        W->b2->DestroyJoint(this->j_climb);
        this->j_climb = 0;
    }
}

void
robot_base::step()
{
    creature::step();

    float ga = this->get_gravity_angle();
    this->balance->target = ga + M_PI/2.f;

    /* always increase jump time so we can check time since last jump at any time */
    this->jump_time += G->timemul(WORLD_STEP);

    /* XXX: To stop step() if the robot is boxed */
    if (this->robot_type == ROBOT_TYPE_ROBOT && this->is_action_active()) {
        return;
    }

    b2Vec2 p = this->get_position();

    if (!this->finished) {
        //tms_infof("on_ground: %f, standing: %d", on_ground, standing);

        switch (this->motion) {
            case MOTION_DEFAULT: this->apply_default_motion(); break;
            case MOTION_CLIMBING: this->apply_climbing_motion(); break;
        }
    }
}

void
robot_base::perform_logic()
{
    if (this->is_alive()) {
        float mval[NUM_MOODS];
        for (int x=0; x<NUM_MOODS; ++x) {
            mval[x] = this->mood.get(x);
            if (mval[x] > this->faction->mood_base[x]) {
                this->mood.add(x, this->faction->mood_decr[x]);
            }
        }

        if (this != adventure::player && !this->creature_flag_active(CREATURE_IS_ZOMBIE)) {
            {
                // Make mood checks
                float cowardice = mval[MOOD_FEAR] > mval[MOOD_BRAVERY] ? mval[MOOD_FEAR] - mval[MOOD_BRAVERY] : 0.f;

                if (cowardice > .3f) {
                    if (!this->is_panicked()) {
                        if (rand()%5 == 0) {
                            this->set_creature_flag(CREATURE_PANICKED, true);
                        }
                    }
                } else if (cowardice < .15f) {
                    if (this->is_panicked()) {
                        if (rand()%3 == 0) {
                            this->set_creature_flag(CREATURE_PANICKED, false);
                        }
                    }
                }
            }

            {
                // Make health checks
                float hp_fraction = this->get_hp()/this->max_hp;

                if (this->creature_flag_active(CREATURE_FOUND_REACHABLE_OIL)) {
                } else {
                    if (hp_fraction < .3f) {
                        if (rand()%5 == 0) {
                            this->set_creature_flag(CREATURE_QUERY_OIL, true);

                            b2AABB aabb;
                            aabb.lowerBound = this->local_to_world(b2Vec2(-3.f, -3.f), 0);
                            aabb.upperBound = this->local_to_world(b2Vec2( 3.f,  3.f), 0);
                            this->results.clear();
                            W->b2->QueryAABB(this, aabb);

                            this->set_creature_flag(CREATURE_QUERY_OIL, false);

                            entity *nearest = 0;
                            float nearest_dist = 10.f;

                            for (std::set<entity*>::iterator it = this->results.begin();
                                    it != this->results.end(); ++it) {
                                float dist = b2Distance(this->get_position(), (*it)->get_position());

                                if (dist < nearest_dist) {
                                    nearest = (*it);
                                    nearest_dist = dist;
                                }
                            }

                            if (nearest) {
                                this->previous_roam_target_id = this->roam_target_id;
                                this->roam_target_id = nearest->id;
                            }
                        }
                    }
                }
            }

            if (this->robot_type == ROBOT_TYPE_ROBOT) {
                if (this->creature_flag_active(CREATURE_BOX_ME)) {
                    this->action_on();
                    this->set_creature_flag(CREATURE_BOX_ME, false);
                } else if (this->is_action_active() && --this->min_box_ticks <= 0) {
                    if (rand()%3 == 0) {
                        //if (fabsf(tmath_adist(this->get_down_angle(), this->get_gravity_angle())) < .6f) {
                            this->action_off();
                        //}
                    }
                }
            }
        }

        if (this->is_roaming() && this->id != G->state.adventure_id) {
            if (this->is_panicked()) {
                this->apply_effect(PANIC_ID, *panic_effect);
            }

            this->roam_target = 0;

            if (rand()%10 == 0) {
                this->look_for_target();
            }

            this->roam_setup_target();

            if (this->roam_target || this->roam_target_type == TARGET_POSITION) {
                // disable wandering speed if we were previously wandering
                if (this->is_wandering()) {
                    this->set_creature_flag(CREATURE_WANDERING, false);
                }

                bool hostile = false;

                if (this->roam_target_type == TARGET_ENEMY) {
                    hostile = true;
                }

                this->refresh_optimal_distance();

                this->layer_dir = 0;
                // XXX: Is there any reason for why we set new_dir here every step?
                //this->new_dir = DIR_LEFT;

                /* Check whether the target is within shooting range,
                 * and if anything is in the way of our shooting. */
                this->roam_gather_sight();

                /* Check if we're somehow blocked, unable to move */
                this->roam_check_blocked();

                /* Calculate the distance between the robot and its target,
                 * decide whether or not it needs to move toward
                 * or away from the target. */
                this->roam_update_dir();

                /* Gather data about surroundings.
                 * If roam_update_dir has told us we need to move in a direction,
                 * that's the direction we will gather data for */
                this->roam_gather();

                this->roam_look();

                if (hostile && !this->is_panicked()) {
                    this->roam_attack();
                }

                this->roam_jump();

                if (!W->level.flag_active(LVL_DISABLE_ROAM_LAYER_SWITCH)) {
                    this->roam_layermove();
                }

                this->roam_walk();

                if (!this->is_panicked()) {
                    this->roam_aim();
                }

                this->roam_perform_target_actions();
            } else {
                this->roam_gather();

                this->roam_jump();

                this->roam_target_id = 0;

                /* TODO: This does not work in custom gravity/angles */
                this->look_for_target();

                this->roam_wander();
            }
        }
    }
}

void
robot_base::roam_set_target_type()
{
    if (this->roam_target->flag_active(ENTITY_IS_ROBOT)) {
        switch (this->faction->relationship[((robot_base*)this->roam_target)->faction->id]) {
            case FRIENDLY:
                this->roam_target_type = TARGET_FRIEND;
                break;

            default:
            case ENEMY:
                this->roam_target_type = TARGET_ENEMY;
                break;
        }
    } else if (this->roam_target->g_id == O_GUARDPOINT) {
        this->roam_target_type = TARGET_ANCHOR;
    } else if (this->roam_target->g_id == O_ITEM) {
        this->roam_target_type = TARGET_ITEM;
    } else {
        this->roam_target_type = TARGET_POSITION;
    }
}

void
robot_base::mstep()
{
    if (!this->body) {
        return;
    }

    creature::mstep();

    if (this->is_roaming() && this->is_alive()) {
        this->aim(this->roam_target_aim);
    }
}

void
robot_base::reset_angles()
{
    float ga = this->get_gravity_angle();
    this->balance->target = ga + M_PI/2.f;

    if (this->feet) this->feet->reset_angles();
}


void
robot_base::ghost_update()
{
    this->update();
}

#define CONSUME_SCALE .3f

void
robot_base::update()
{
    b2Transform t;
    if (this->body != 0) {
        t = this->body->GetTransform();
    } else {
        t.p.x = this->_pos.x;
        t.p.y = this->_pos.y;
    }

    float tilt = 0.f;
    if (this->feet) tilt = this->feet->get_tilt();

    tmat4_load_identity(this->M);
    tmat4_translate(this->M, t.p.x, t.p.y, this->get_z());
    tmat4_rotate(this->M, this->get_angle()*(180.f/M_PI), 0, 0, -1);
    tmat4_rotate(this->M, -90 * this->i_dir + tilt * (180.f/M_PI), 0, 1, 0);
    tmat3_copy_mat4_sub3x3(this->N, this->M);
    if (this->get_scale() != 1.f) {
        tmat4_scale(this->M, this->get_scale(), this->get_scale(), this->get_scale());
    }

    if (this->consume_timer > 0.f) {
        tmat4_scale(this->M, 1.f, 1.f, 1.f+this->consume_timer*CONSUME_SCALE);
        this->consume_timer -= _tms.dt*4.f;
    }

    creature::update();
}

void
robot_base::on_jump_begin()
{
    //if (this->f_body) this->f_body->SetFriction(0.f);
}

bool
robot_base::jump(bool forward_force, float force_mul/*=1.f*/)
{
    if (this->is_currently_climbing()) {
        this->stop_climbing();
        this->motion = MOTION_DEFAULT;
        return false;
    }

    return creature::jump(forward_force, force_mul);
}

void
robot_base::on_jump_end()
{
    //if (this->f_body) this->f_body->SetFriction(this->get_material()->friction);
}

void
robot_base::reset_friction()
{
    this->set_friction(m_robot.friction);
}

void
robot_base::create_fixtures()
{
    if (!this->body_shape) {
        this->body_shape = static_cast<b2Shape*>(new b2PolygonShape());
        ((b2PolygonShape*)this->body_shape)->SetAsBox(.375f, .375f, b2Vec2(0,0), 0);
        ((b2PolygonShape*)this->body_shape)->Scale(this->get_scale());
        this->width = .375f;
    }

    b2FixtureDef fd_body; /* body */
    fd_body.shape = this->body_shape;
    fd_body.friction        = this->get_material()->friction;
    fd_body.density         = this->get_material()->density;
    fd_body.restitution     = this->get_material()->restitution;
    fd_body.filter = world::get_filter_for_layer(this->get_layer(), 15);

    if (!this->f_body) {
        (this->f_body = this->body->CreateFixture(&fd_body))->SetUserData(this);
    }

    b2Body *b = this->body;

    b2PolygonShape b_sensor;
    b_sensor.SetAsBox(.05f, .25f, b2Vec2(0,-.2f), 0);
    b_sensor.Scale(this->get_scale());

    b2FixtureDef fd_sensor;
    fd_sensor.shape = &b_sensor;
    fd_sensor.friction = FLT_EPSILON;
    fd_sensor.density = 0.f;
    fd_sensor.restitution = .0f;
    fd_sensor.isSensor = true;
    fd_sensor.filter = world::get_filter_for_layer(this->get_layer(), 15);

    if (!this->f_sensor) {
        (this->f_sensor = b->CreateFixture(&fd_sensor))->SetUserData(this);
    }
}

void
robot_base::action_on()
{
    if (this->creature_flag_active(CREATURE_DISABLE_ACTION)) return;
    if (this->is_frozen()) return;

    this->action_active = true;
}

void
robot_base::action_off()
{
    this->action_active = false;
}

float
robot_base::get_slider_snap(int s)
{
    if (s == 0) {
        return 1.f / 19.f;
    } else {
        return 1.f / 20.f;
    }
}

float
robot_base::get_slider_value(int s)
{
    if (s == 0) {
        return (this->properties[0].v.f - 1.f) / 19.f;
    } else {
        return this->properties[5].v.f;
    }
}

void
robot_base::on_slider_change(int s, float value)
{
    if (s == 0) {
        this->set_property(0, (value * 19.f) + 1.f);
        if (this == adventure::player && W->is_adventure()) {
            G->show_numfeed(this->properties[0].v.f*2.f);
        } else {
            G->show_numfeed(this->properties[0].v.f);
        }
    } else {
        this->properties[5].v.f = value;
        G->show_numfeed(value * this->max_hp);
    }
}

void
robot_base::init_adventure()
{
    creature::init_adventure();

    tms_debugf("init_adventure called on %u", this->id);

    this->set_faction(FACTION_FRIENDLY);

    this->stop();
    this->dir = 0;

    //this->set_uniform("~color", factions[FACTION_FRIENDLY].color.r, factions[FACTION_FRIENDLY].color.g, factions[FACTION_FRIENDLY].color.b, 1.f);

    this->set_speed(3.f + (this->properties[0].v.f * 2.f));
}

bool
robot_base::can_see(entity *e)
{
    this->vision_handler->can_see = false;
    this->vision_handler->target = e;

    for (int x=0; x<3; x++) {
        this->vision_handler->test_layer = x;

        b2Vec2 from = this->local_to_world(this->eye_pos, 0);

        b2Vec2 to = e->local_to_world(b2Vec2(0.f, .5-.5f*(rand()%100)/100.f), 0);
        W->raycast(this->vision_handler, from, to);

        if (this->vision_handler->can_see) {
            return true;
        }
    }

    return false;
}

float32
robot_base::cb_handler::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    entity *e = static_cast<entity*>(f->GetUserData());

    if (f->IsSensor()) {
        return -1.f;
    }

    if (e) {
        /* ignore self */
        if (e == this->self) {
            return -1.f;
        }

        /* if the layer of the robot and entity are different, continue searching */
        if (!world::fixture_in_layer(f, this->self->get_layer(), 15)) {
            return -1;
        }

        if (e->id == this->self->roam_target_id) {
            this->self->shoot_target = true;
            return fraction;
        }

        if (f->GetBody()->GetType() == b2_staticBody) {
            this->self->shoot_target = false;
            return 0.f;
        }

        if (e->flag_active(ENTITY_IS_ROBOT)) {
            robot_base *r;
            r = static_cast<robot_base*>(e);

            if (this->self->faction->relationship[r->faction->id] == ENEMY) {
                this->self->shoot_target = true;
                return fraction;
            } else {
                this->self->shoot_target = false;
                return fraction;
            }
        }
    }

    return fraction;
}

float32
robot_base::cb_vision_handler::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    entity *e = static_cast<entity*>(f->GetUserData());

    if (e) {
        if (e == this->self) {
            return -1.f;
        }

        if (!world::fixture_in_layer(f, this->test_layer, 15)) {
            return -1;
        }

        this->can_see = false;

        if (e == this->target) {
            b2Vec2 target_pos = this->target->get_position();
            b2Vec2 my_pos = this->self->get_position();

            if (this->self->look_dir == DIR_LEFT && this->self->get_tangent_distance(target_pos) < 2.0f) {
                this->can_see = true;
                return 0;
            } else if (this->self->look_dir == DIR_RIGHT && this->self->get_tangent_distance(target_pos) > -2.0f) {
                this->can_see = true;
                return 0;
            }

            //this->can_see = true;
        }
        return fraction;
    }

    return -1;
}

bool
robot_base::consume(item *c, bool silent, bool first/*=false*/)
{
    // might be a very unnecessary check ;-)
    if (!G) {
        return false;
    }

    bool ret = false;

    switch (c->item_category) {
        case ITEM_CATEGORY_POWERUP:
            if (c->ef) {
                creature_effect *e = c->ef;
                ret = this->apply_effect(c->get_item_type(), *c->ef);
            } else {
                // special ugly items!
                switch (c->get_item_type()) {
                    case ITEM_MINER_UPGRADE:
                        {
                            // probably checking a few too many times that it's actually a miner :-)
                            if (adventure::player == this && this->tools[TOOL_ZAPPER]) {
                                robot_parts::tool *t = this->tools[TOOL_ZAPPER];
                                if (t && t->get_tool_type() == TOOL_ZAPPER) {
                                    robot_parts::miner *m = static_cast<robot_parts::miner*>(t);
                                    if (m->damage < MINER_MAX_DAMAGE) {
                                        m->set_damage(m->damage + 1.f);
                                        ret = true;
                                    }
                                }
                            }
                        }
                        break;
                }
            }
            break;

        case ITEM_CATEGORY_WEAPON:
            ret = this->add_weapon(c->data_id);

            if (!silent && ret) {
                robot_parts::weapon *w = this->equip_weapon(c->data_id);

                if (w && !W->is_paused()) {
                    // Search for nearby robots
                    this->set_creature_flag(CREATURE_QUERY_ROBOTS, true);

                    b2AABB aabb;
                    aabb.lowerBound = this->local_to_world(b2Vec2(-10.f, -10.f), 0);
                    aabb.upperBound = this->local_to_world(b2Vec2( 10.f,  10.f), 0);
                    this->results.clear();
                    W->b2->QueryAABB(this, aabb);

                    this->set_creature_flag(CREATURE_QUERY_ROBOTS, false);

                    for (std::set<entity*>::iterator it = this->results.begin();
                            it != this->results.end(); ++it) {
                        robot_base *rob = static_cast<robot_base*>(*it);

                        /* TODO: How do we deal with neutral entities who are now enemies? */
                        switch (this->faction->relationship[rob->faction->id]) {
                            case FRIENDLY:
                                // inspire our friends
                                rob->mood.add(MOOD_BRAVERY, w->terror);
                                break;

                            case ENEMY:
                                // frighten our enemies
                                rob->mood.add(MOOD_FEAR, w->terror);
                                break;
                        }
                    }
                }
            }

            break;

        case ITEM_CATEGORY_TOOL:
            ret = this->add_tool(c->data_id);
            if (!silent && ret) this->equip_tool(c->data_id);
            break;

        case ITEM_CATEGORY_CIRCUIT:
            {
                if (first) {
                    uint32_t circuit = c->data_id;
                    if (circuit & this->circuits_compat) {
                        tms_debugf("compatible circuit");
                        if (this->has_circuit(circuit)) {
                            tms_debugf("we aleardy have this circuit!");
                        } else {
                            tms_debugf("consuming circuit");
                            this->set_has_circuit(circuit, true);
                            ret = true;
                        }
                    } else {
                        tms_debugf("incompatible circuit");
                    }
                }
            }
            break;

        default:
            tms_errorf("Unhandled item :(");
            break;
    }

    if (ret) {
        if (!silent) {
            G->emit(new discharge_effect(
                        this->get_position(),
                        c->get_position(),
                        this->get_layer() * LAYER_DEPTH + .5f,
                        this->get_layer() * LAYER_DEPTH + .5f,
                        5,
                        .1f
                        ), 0);
        }
    }

    return ret;
}

void
robot_base::write_quickinfo(char *out)
{
    sprintf(out, "%s, %sroaming", this->get_name(), this->properties[2].v.i8 == 1 ? "":"not ");
}

bool
robot_base::ReportFixture(b2Fixture *f)
{
    if (f->IsSensor()) {
        return true;
    }

    entity *e = static_cast<entity*>(f->GetUserData());

    if (this->creature_flag_active(CREATURE_QUERY_ROBOTS)) {
        if (e && e->flag_active(ENTITY_IS_ROBOT) && e != this) {
            this->results.insert(e);
        }

        return true;
    } else if (this->creature_flag_active(CREATURE_QUERY_OIL)) {
        if (e && e->g_id == O_ITEM && e->properties[0].v.i == ITEM_OIL && this->get_layer() == e->get_layer()) {
            item *it = static_cast<item*>(e);
            if (!it->flag_active(ENTITY_IS_ABSORBED))
                this->results.insert(e);
        }

        return true;
    } else {
        entity *e = static_cast<entity*>(f->GetUserData());

        if (e) {
            if (e == this) return true;

            if (this->roam_can_target(e)) {
                this->roam_set_target(e);
                return false;
            }

            return true;
        }

        return true;
    }

    return true;
}

bool
robot_base::is_roaming()
{
    return (bool)this->properties[ROBOT_PROPERTY_ROAMING].v.i8 && this->id != G->state.adventure_id;
}

void
robot_base::look_for_target()
{
    // search for enemy
    b2AABB aabb;
#if 0
    if (this->look_dir == DIR_LEFT) {
        aabb.lowerBound = this->local_to_world(b2Vec2(-ROBOT_VISION_DISTANCE, -2.f), 0);
        aabb.upperBound = this->local_to_world(b2Vec2(-.5f, 2.f), 0);
    } else {
        aabb.lowerBound = this->local_to_world(b2Vec2(.5f, -2.f), 0);
        aabb.upperBound = this->local_to_world(b2Vec2(ROBOT_VISION_DISTANCE, 2.f), 0);
    }
#else
    aabb.lowerBound = this->get_position()+b2Vec2(-ROBOT_VISION_DISTANCE, -ROBOT_VISION_DISTANCE);
    aabb.upperBound = this->get_position()+b2Vec2(ROBOT_VISION_DISTANCE, ROBOT_VISION_DISTANCE);
#endif

    W->b2->QueryAABB(this, aabb);
}

robot_parts::weapon *
robot_base::equip_weapon(uint8_t weapon_id, bool announce/*=true*/)
{
    robot_parts::weapon *w;

    if (!(w = this->has_weapon(weapon_id))) {
        tms_debugf("We don't have this weapon.");
        return 0;
    }

    if (!this->set_weapon(weapon_id, w)) {
        return 0;
    }

    if (announce && this->id == G->state.adventure_id) {
        ui::messagef("Equipped weapon: %s", w->get_name());
    }

    return w;
}

robot_parts::weapon*
robot_base::has_weapon(uint8_t weapon_id)
{
    for (int x=0; x<this->num_weapons; x++) {
        if (this->weapons[x]->get_weapon_type() == weapon_id) {
            return this->weapons[x];
        }
    }

    return 0;
}

robot_parts::tool*
robot_base::has_tool(uint8_t tool_id)
{
    for (int x=0; x<this->num_tools; x++) {
        if (this->tools[x]->get_tool_type() == tool_id) {
            return this->tools[x];
        }
    }

    return 0;
}

bool
robot_base::add_weapon(uint8_t weapon_id)
{
    robot_parts::weapon *w;

    if (!this->has_feature(CREATURE_FEATURE_WEAPONS)) {
        return false;
    }

    if (weapon_id >= NUM_WEAPONS) {
        return false;
    }

    if (this->has_weapon(weapon_id)) {
        return false;
    }

    if (!(w = robot_parts::weapon::make(weapon_id, this))) {
        return false;
    }

    this->weapons[this->num_weapons] = w;
    this->num_weapons ++;

    return true;
}

bool
robot_base::add_tool(uint8_t tool_id)
{
    robot_parts::tool *t;

    if (!this->has_feature(CREATURE_FEATURE_TOOLS)) {
        return false;
    }

    if (tool_id >= NUM_TOOLS) {
        return false;
    }

    if (this->has_tool(tool_id)) {
        return false;
    }

    if (!(t = robot_parts::tool::make(tool_id, this))) {
        return false;
    }

    this->tools[this->num_tools] = t;
    this->num_tools ++;

    return true;
}

bool
robot_base::equip_tool(uint8_t tool_id, bool announce/*=true*/)
{
    robot_parts::tool *t;

    if (!(t = this->has_tool(tool_id))) {
        tms_debugf("We don't have this tool.");
        return false;
    }

    if (this->tool) {
        if (this->tool->is_active()) {
            tms_debugf("We can't change tool while the current tool is active.");
            return false;
        }

        if (this->is_player() && tool->get_tool_type() == TOOL_BUILDER) {
            G->do_drop_interacting = true;
        }
    }

    this->set_tool(tool_id, t);

    if (announce && this->is_player()) {
        adventure::tool_changed();
        ui::messagef("Equipped tool: %s", t->get_name());
    }

    return true;
}

faction_info*
robot_base::set_faction(uint8_t faction_id)
{
    if (faction_id > NUM_FACTIONS) return 0;

    return this->set_faction(&factions[faction_id]);
}

faction_info*
robot_base::set_faction(faction_info *faction)
{
    this->properties[6].v.i8 = faction->id;
    this->faction = faction;

    for (int x=0; x<NUM_MOODS; ++x)
        this->mood.set(x, this->faction->mood_base[x]);

    //this->set_uniform("~color", this->faction->color.r, this->faction->color.g, this->faction->color.b, 1.f);

    return faction;
}

void
robot_base::refresh_optimal_distance(void)
{
    switch (this->roam_target_type) {
        case TARGET_ENEMY:
            this->roam_optimal_big_distance = 8.f;
            this->roam_optimal_small_distance = 4.f;
            break;

        case TARGET_ANCHOR:
            this->roam_optimal_big_distance = 4.f;
            this->roam_optimal_small_distance = 2.f;
            break;

        case TARGET_ITEM:
            this->roam_optimal_big_distance = 0.1f;
            this->roam_optimal_small_distance = 0.f;
            break;

        case TARGET_POSITION:
            this->roam_optimal_big_distance = 4.f;
            this->roam_optimal_small_distance = 2.f;
            break;

        default:
            this->roam_optimal_big_distance = 8.f;
            this->roam_optimal_small_distance = 4.f;
            break;
    }
}

void
robot_base::reset_limbs()
{
    this->roam_target_aim = -M_PI/2.f;
}

/**
 * Gather information on what we can see, particularly about our target
 **/
void
robot_base::roam_gather_sight()
{
    b2Vec2 r = this->get_position();

    b2Vec2 target_pos;
    if (this->roam_target_type == TARGET_POSITION) {
        target_pos = this->roam_target_pos;
    } else {
        target_pos = this->roam_target->get_position();
    }

    this->shoot_target = false;

    if (this->get_layer() == target_layer
            && this->target_dist < 9.f) {
        W->raycast(this->handler, r, target_pos);
    }
}

/**
 * Default target picker can target robots only, minibot replaces this with
 * a target picker that picks scrap and dead robots
 **/
bool
robot_base::roam_can_target(entity *e, bool must_see)
{
    if (!e->flag_active(ENTITY_IS_ROBOT)) return false;

    robot_base *r = static_cast<robot_base*>(e);

    if (!this->is_enemy(e)) return false;
    if (r->is_dead()) return false;

    return !must_see || this->can_see(e);
}

bool
robot_base::is_friend(entity *e)
{
    if (e->flag_active(ENTITY_IS_ROBOT)) {
        robot_base *r = static_cast<robot_base*>(e);

        if (this->faction->relationship[r->faction->id] == FRIENDLY) {
            return true;
        } else {
            return false;
        }
    }

    return false;
}

bool
robot_base::is_neutral(entity *e)
{
    if (e->flag_active(ENTITY_IS_ROBOT)) {
        robot_base *r = static_cast<robot_base*>(e);

        if (this->faction->relationship[r->faction->id] == NEUTRAL) {
            return true;
        } else {
            return false;
        }
    }

    return false;
}

bool
robot_base::is_enemy(entity *e)
{
    if (e->flag_active(ENTITY_IS_ROBOT)) {
        robot_base *r = static_cast<robot_base*>(e);

        if (this->faction->relationship[r->faction->id] == ENEMY) {
            return true;
        } else {
            return false;
        }
    }

    return false;
}
