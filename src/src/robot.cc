#include "robot.hh"
#include "model.hh"
#include "game.hh"
#include "adventure.hh"

#define HEAD_SPEED_UP 2.f
#define HEAD_SPEED_DOWN -2.f
#define ACTION_OFF_ANGULAR_VELOCITY 1.f
#define ACTION_OFF_ANGLE_DIST (M_PI/8.f)

robot::robot()
{
    this->m_scale = 1.f;
    this->eye_pos = b2Vec2(0.f, 0.655f);

    this->neck_pos = (tvec2){0.f, 0.f};
    this->head_pos = (tvec2){0.f, 0.655f};

    this->robot_type = ROBOT_TYPE_ROBOT;

    this->damage_multiplier = 2.f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_BODY));

    this->properties[0].v.f = ROBOT_DEFAULT_SPEED; /* Robot speed */
    this->properties[1].v.i8 = CREATURE_IDLE; /* Robot state */
    this->properties[3].v.i = 450; /* Roaming */

    this->properties[ROBOT_PROPERTY_FEET].v.i8 = FEET_BIPED;
    this->properties[ROBOT_PROPERTY_HEAD].v.i8 = HEAD_ROBOT;
    this->properties[ROBOT_PROPERTY_FRONT].v.i8 = FRONT_EQUIPMENT_ROBOT_FRONT;
    this->properties[ROBOT_PROPERTY_BACK].v.i8 = BACK_EQUIPMENT_ROBOT_BACK;

    if (W && W->level.version >= LEVEL_VERSION_1_5) {
        this->set_property(ROBOT_PROPERTY_EQUIPMENT, "51;0;1;8");
    } else {
        this->set_property(ROBOT_PROPERTY_EQUIPMENT, "0");
    }

    this->width = .3f;

    this->body_shape = static_cast<b2Shape*>(new b2PolygonShape());
    ((b2PolygonShape*)this->body_shape)->SetAsBox(.3f, .3f, b2Vec2(.0f,0), 0);

    ((b2PolygonShape*)this->body_shape)->Scale(this->get_scale());

    /*
    this->add_weapon(WEAPON_ARM_CANNON, false);
    this->equip_weapon(WEAPON_ARM_CANNON, false);
    */

    this->target_aim_angle = -100.f;

    //this->max_hp = ROBOT_MAX_HP;

    this->circuits_compat |= CREATURE_CIRCUIT_SOMERSAULT;
    this->features |= CREATURE_FEATURE_HEAD;
    this->features |= CREATURE_FEATURE_BACK_EQUIPMENT;
    this->features |= CREATURE_FEATURE_FRONT_EQUIPMENT;
    this->features |= CREATURE_FEATURE_WEAPONS;
    this->features |= CREATURE_FEATURE_TOOLS;

    this->set_material(&m_robot_skeleton);
}

robot::~robot()
{
}

void
robot::on_load(bool created, bool has_state)
{
    robot_base::on_load(created, has_state);

    if (!has_state) {
    }
}

void robot::init()
{
    robot_base::init();
    this->pending_action_off = false;
}


void
robot::setup()
{
    tms_debugf("robot::setup");
    robot_base::setup();

    this->damage_multiplier = 2.f;

    tms_debugf("resetting weapon settings");
    if (this->weapon) this->weapon->reset_settings();
}

#define ROBOT_SPIN_FORCE 20.f
#define ROBOT_SOMERSAULT_FORCE 4.5f

void
robot::on_pause()
{
    robot_base::on_pause();

    if (this->weapon) this->weapon->reset_settings();
}

void
robot::step()
{
    robot_base::step();

    if (this->weapon) {
        this->weapon->step();
    }

    if (this->tool) {
        this->tool->step();
    }

    /* XXX: Checking for uninitialized... */

    if (pending_action_off) {
        // AAA
    }

    if (this->body->GetAngularVelocity()) {
        // BBB
    }

    if (this->get_down_angle()) {
        // CCC
    }

    if (this->get_gravity_angle()) {
        // DDD
    }

    if (pending_action_off &&
            (fabsf(this->body->GetAngularVelocity()) < ACTION_OFF_ANGULAR_VELOCITY
            || fabsf(tmath_adist(this->get_down_angle(), this->get_gravity_angle())) < ACTION_OFF_ANGLE_DIST)) {
        pending_action_off = false;
        robot_base::action_off();

        this->gives_score = true;
        this->fixed_dir = false;

        if (this->tool) {
            this->tool->set_arm_fold(0.f);
        }

        if (this->weapon) {
            this->weapon->set_arm_fold(0.f);
        }

        this->create_feet_joint(0);
        this->activate_feet();

        this->reset_damping();
        this->reset_friction();

        if (this->j_head && this->j_head->GetType() == e_prismaticJoint) {
            b2PrismaticJoint *pj = static_cast<b2PrismaticJoint*>(this->j_head);
            pj->SetMotorSpeed(HEAD_SPEED_UP);
        }
    }

    if (this->is_action_active() && this->body && this->is_alive()) {

        if (this->feet) {
            int done = 0;
            for (int x=0; x<this->feet->get_num_bodies(); x++) {
                if (this->j_feet[x]) {
                    if (this->j_feet[x]->GetLowerLimit() < 0.f && this->j_feet[x]->GetJointTranslation() >= this->feet->get_offset()-0.1f) {
                        done ++;
                    }
                }
            }
            if (done == this->feet->get_num_bodies()) {
                tms_debugf("all feet done, setting joint mode to 2");
                this->create_feet_joint(2);
                this->deactivate_feet();
            }
        }

        /* only apply rolling force if we're in contact with something that is not a sensor */
        b2ContactEdge *c;
        for (c=this->body->GetContactList(); c; c = c->next) {

            b2Fixture *f = 0, *my;
            if (c->contact->GetFixtureA()->GetBody() == this->body) {
                f = c->contact->GetFixtureB();
                my = c->contact->GetFixtureA();
            } else {
                f = c->contact->GetFixtureA();
                my = c->contact->GetFixtureB();
            }

            if (f && !f->IsSensor() && world::fixture_in_layer(f, this->get_layer())) {
                if (f->GetUserData() != this && c->contact->IsTouching()) {
                    //tms_debugf("APPLYING SPIN");
                    this->body->ApplyTorque(
                            (this->is_moving_left()?ROBOT_SPIN_FORCE:this->is_moving_right()?-ROBOT_SPIN_FORCE:0.f)
                            * this->get_upper_mass() * 1.f/G->get_time_mul());
                    break;
                }
            }
        }
    }
}

void
robot::update()
{
    robot_base::update();

    if (this->tool) {
        this->tool->update();
    }

    if (this->weapon) {
        this->weapon->update();
    }
}

void
robot::create_head_joint()
{
    if (this->body && this->equipments[EQUIPMENT_HEAD] && this->equipments[EQUIPMENT_HEAD]->body) {
        b2PrismaticJointDef pjd;
        pjd.tolerance = .03f;
        pjd.maxMotorForce = 300.f * this->equipments[EQUIPMENT_HEAD]->get_body(0)->GetMass();
        pjd.motorSpeed = HEAD_SPEED_UP;
        pjd.enableLimit = true;
        pjd.enableMotor = !W->is_paused();
        pjd.lowerTranslation = this->get_scale() * (W->is_paused()?.6f:0.15f);
        pjd.upperTranslation = this->get_scale() * (.655f);
        pjd.collideConnected = false;
        pjd.localAnchorA = b2Vec2(0.f, 0.f);
        pjd.localAnchorB = b2Vec2(0.f, 0.f);
        pjd.bodyA = this->body;
        pjd.bodyB = this->equipments[EQUIPMENT_HEAD]->body;
        pjd.localAxisA = b2Vec2(0.f, 1.f * this->get_scale());
        pjd.referenceAngle = 0.f;

        this->j_head = (b2PrismaticJoint*)this->body->GetWorld()->CreateJoint(&pjd);
        tms_debugf("Created head joint. %d", this->j_head->GetType());
    }
}

void
robot::update_effects()
{
    if (this->weapon && this->weapon->do_update_effects) {
        this->weapon->update_effects();
    }

    if (this->tool && this->tool->do_update_effects) {
        this->tool->update_effects();
    }
}

void
robot::attack(int add_cooldown)
{
    if (this->is_action_active()) return;
    if (this->is_frozen()) return;
    if (this->finished) return;
    if (!this->weapon) return;

    this->attack_stop();

    int r = this->weapon->pre_attack();

    if (this->look_dir != DIR_LEFT && this->look_dir != DIR_RIGHT) return;
    if (r == EVENT_DONE) return;
    if ((float)this->look_dir - this->i_dir > .25f) return;

    this->weapon->attack(add_cooldown);
}

void
robot::attack_stop()
{
    if (!this->weapon) return;
    this->weapon->attack_stop();
}

void
robot::action_on()
{
    robot_base::action_on();

    if (this->action_active) {
        this->gives_score = false;
        this->fixed_dir = true;
        if (this->tool) {
            this->tool->set_arm_fold(1.f);
        }
        if (this->weapon) {
            this->weapon->set_arm_fold(1.f);
        }

        for (int x=0; x<4; x++) {
            if (this->get_body(x)) {
                this->get_body(x)->SetAngularVelocity(0.f);
            }
        }

        bool jumped = this->jumping > 0;
        this->set_jump_state(0);

        this->create_feet_joint(1);
        this->deactivate_feet();

        this->set_damping(.5f);
        this->set_friction(.9f);

        if (jumped && this->has_circuit(CREATURE_CIRCUIT_SOMERSAULT)) {
            float mass = this->get_upper_mass();
            this->on_ground = 0.f;
            if (this->is_moving_left()) {
                tms_infof("moving left, jumping %d", this->jumping);
                this->body->ApplyTorque( 1.f / G->get_time_mul() * ROBOT_SOMERSAULT_FORCE * 30.f * mass);
            } else if (this->is_moving_right()) {
                tms_infof("moving right, jumping %d", this->jumping);
                this->body->ApplyTorque(1.f / G->get_time_mul() * -ROBOT_SOMERSAULT_FORCE * 30.f * mass);
            }

            this->body->ApplyForceToCenter(1.f / G->get_time_mul() * mass*-15.f*W->get_gravity());
        }

        if (this->j_head && this->j_head->GetType() == e_prismaticJoint) {
            b2PrismaticJoint *pj = static_cast<b2PrismaticJoint*>(this->j_head);
            pj->SetMotorSpeed(HEAD_SPEED_DOWN);
        }

        /* Disconnect from any ladder we might be connected too */
        this->stop_climbing();
    }
}

void
robot::set_position(float x, float y, uint8_t fr)
{
    if (fr == 2 && this->body) {
        b2Vec2 o = this->body->GetWorldVector(b2Vec2(0.f, .655f));
        x -= o.x;
        y -= o.y;
    }

    robot_base::set_position(x, y, fr);
}

void
robot::action_off()
{
    /* we deactivate the action whenever the angular velocity is
     * low or the the distance to standing angle is small */
    this->pending_action_off = true;
}

void
robot::roam_aim()
{
    b2Vec2 r = this->get_position();
    b2Vec2 o = this->get_roam_target_pos();
    o -= r;

    float a = atan2f(o.y, o.x);
    float tangent_dist = this->get_tangent_distance(this->get_roam_target_pos());

    a -= this->get_angle();

    float amod = 0.f;

    if (this->weapon) {
        switch (this->weapon->get_weapon_type()) {
            case WEAPON_ARM_CANNON:
            case WEAPON_BOMBER:
                amod = (tangent_dist < 0.f ? -1 : 1) * (this->target_dist * 0.025f);
                break;

            case WEAPON_ROCKET:
                amod = (tangent_dist < 0.f ? -1 : 1) * ((-this->target_dist * 0.005f) + 0.125f);
                break;
        }
    }

    a += amod;

    this->roam_target_aim = a;

    this->target_aim_angle = robot_base::real_arm_angle(this, a);
}

void
robot::roam_attack()
{
    if (this->weapon) {
        float angle_diff = this->weapon->get_angle_diff(this->target_aim_angle);

        if (this->shoot_target) {
            if (angle_diff < .05f) {
                float fire_rate = this->properties[3].v.i*1000 + rand()%50000;

                if (this->creature_flag_active(CREATURE_IS_ZOMBIE)) {
                    fire_rate *= .5f;
                } else {
                    fire_rate *= 1.f-this->mood.get(MOOD_ANGER);
                }

                this->attack(fire_rate);
            } else {
                tms_debug_roam_a("%p [%d] can't attack due to angle diff", this, this->faction->id);
            }
        } else {
            this->attack_stop();
            tms_debug_roam_a("%p [%d] can't attack due to shoot target being false", this, this->faction->id);
        }
    }
}

void
robot::roam_gather_sight()
{
    b2Vec2 r = this->get_position();
    b2Vec2 target_pos = this->get_roam_target_pos();

    this->shoot_target = false;

    if (this->weapon) {
        if (this->get_layer() == target_layer) {
            if (this->target_dist < this->weapon->get_max_range()) {
                W->raycast(this->handler, r, target_pos);
            }
        }
    }
}

int
robot::get_optimal_walking_dir(float tangent_dist)
{
    robot_parts::weapon *w = this->get_weapon();

    if (!w) {
        return creature::get_optimal_walking_dir(tangent_dist);
    }

    const int target_dir = (tangent_dist < 0.f ? DIR_LEFT : DIR_RIGHT);

    if (this->target_dist > this->roam_optimal_big_distance*TARGET_DIST_SCALE) {
        tms_debug_roam_ud("%p move toward target", this);
        tms_debug_roam_ud("  why? target_dist %.2f > optimal_big_distance %.2f",
                this->target_dist,
                this->roam_optimal_big_distance*TARGET_DIST_SCALE
                );

        return target_dir;
    } else if (!this->shoot_target && this->target_dist > 2.0f*TARGET_DIST_SCALE) {
        tms_debug_roam_ud("%p move toward target", this);
        tms_debug_roam_ud("  why? !shoot_target(%d) && target_dist %.2f > %.2f",
                (int)this->shoot_target,
                this->target_dist,
                2.f*TARGET_DIST_SCALE
                );

        return target_dir;
    } else if (this->target_dist > w->get_max_range()) {
        tms_debug_roam_ud("%p move toward target", this);
        tms_debug_roam_ud("  why? target_dist %.2f > %.2f",
                this->target_dist,
                w->get_max_range()
                );

        return target_dir;
    } else if (this->target_dist < this->roam_optimal_small_distance*TARGET_DIST_SCALE && this->previous_roam_target_id == 0) {
        tms_debug_roam_ud("%p move away from target", this);
        tms_debug_roam_ud("  why? target_dist %.2f < optimal_small_distance %.2f && %d == 0", this->target_dist, this->roam_optimal_small_distance *TARGET_DIST_SCALE, this->previous_roam_target_id);
        /* move away from the target */
        return (tangent_dist < 0.f ? DIR_RIGHT : DIR_LEFT);
    }

    tms_debug_roam_ud("%p don't move", this);
    return 0;
}

void
robot::on_death()
{
    robot_base::on_death();

    b2Fixture *fx = this->get_head_fixture();

    if (fx) {
        fx->SetFriction(m_robot_head.friction);
    }
}

#define PANICKED_AIM_LO -.125f
#define PANICKED_AIM_HI  .125f

void
robot::aim(float a)
{
    if (this->is_action_active()) return;
    if (this->finished) return;
    if (!this->weapon) return;
    if (this->is_panicked()) return;
        //a += PANICKED_AIM_LO + (float)(rand())/((float)(RAND_MAX/(PANICKED_AIM_HI-PANICKED_AIM_LO)));

    this->weapon->set_arm_angle(a, this->is_roaming()?(CREATURE_ROAM_AIM_SPEED * G->get_time_mul()):1.f);
}

float
robot::get_adjusted_damage(float amount, b2Fixture *f, damage_type dt, uint8_t damage_source, uint32_t attacker_id)
{
    if (this->is_action_active()) {
        if (fx == this->get_head_fixture()) {
            return 0.f;
        }

        if (dt == DAMAGE_TYPE_FORCE || dt == DAMAGE_TYPE_OTHER) {
            amount *= 0.3f;
        }
    }

    return creature::get_adjusted_damage(amount, f, dt, damage_source, attacker_id);
}

void
robot::modify_aim(float da)
{
    robot_parts::weapon *w = this->get_weapon();

    if (!w) {
        return;
    }

    w->set_arm_angle(w->get_arm_angle() + da, 0.1f);
}
