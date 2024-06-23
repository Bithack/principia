#include "animal.hh"
#include "world.hh"
#include "model.hh"
#include "game.hh"
#include "ui.hh"

#define ANIMAL_AGING_SPEED 0.00001f

struct animal_data animal_data[NUM_ANIMAL_TYPES] = {
    {
        "Cow",
        BODY_SHAPE_RECT,
        (tvec2){.75f, .5f}, /* body size */
        (tvec2){0.8f, .1f}, /* neck pos */
        (tvec2){1.2f, .2f}, /* head pos */
        (tvec2){38 * DEGTORAD, 25 * DEGTORAD}, /* neck angle limits */
        FEET_QUADRUPED,
        HEAD_COW,
        .5f, // feet offset
        1.f, // feet width
        15.f, // speed
        2.f,
        &m_animal,
        &mesh_factory::models[MODEL_COW].mesh,
        (tvec3){.9f, .9f, .9f},
    },
    {
        "Pig",
        BODY_SHAPE_RECT,
        (tvec2){.56f, .3f}, /* body size */
        (tvec2){0.6f, .0f}, /* neck pos */
        (tvec2){0.75f, .0f}, /* head pos */
        (tvec2){20 * DEGTORAD, 20 * DEGTORAD}, /* neck angle limits */
        FEET_QUADRUPED,
        HEAD_PIG,
        .35f,
        .5f,
        19.f,
        2.f,
        &m_animal,
        &mesh_factory::models[MODEL_PIG].mesh,
        (tvec3){.9f, .7f, .85f},
    },
    {
        "Ostrich",
        BODY_SHAPE_RECT,
        (tvec2){.56f, .3f}, /* body size */
        (tvec2){0.59f, .48f}, /* neck pos */
        (tvec2){0.59f, .48f}, /* head pos */
        (tvec2){60 * DEGTORAD, 60 * DEGTORAD}, /* neck angle limits */
        FEET_BIPED,
        HEAD_OSTRICH,
        1.f,
        .5f,
        30.f,
        6.f,
        &m_animal,
        &mesh_factory::models[MODEL_OSTRICH].mesh,
        (tvec3){.9f, .7f, .85f},
    }
};

animal::animal(uint32_t animal_type)
    : creature()
    , activator(ATTACHMENT_JOINT, ACTIVATOR_REQUIRE_RIDING_CIRCUIT)
{
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_IS_MAGNETIC,          true);
    this->set_flag(ENTITY_DO_STEP,              true);
    this->set_flag(ENTITY_DO_MSTEP,             true);
    this->set_flag(ENTITY_DO_TICK,              true);
    this->set_flag(ENTITY_HAS_CONFIG,           true);
    this->set_flag(ENTITY_MUST_BE_DYNAMIC,      true);
    this->set_flag(ENTITY_HAS_ACTIVATOR,       true);

    this->features |= CREATURE_FEATURE_HEAD;
    this->features |= CREATURE_FEATURE_CAN_BE_RIDDEN;

    this->dialog_id = DIALOG_ANIMAL;

    this->update_method = ENTITY_UPDATE_CUSTOM;
    this->set_uniform("~color", 0.0f, 0.0f, 0.0f, 1.f);

    this->angular_damping = ANIMAL_ANGULAR_DAMPING;

    this->set_num_properties(2);
    this->properties[0].type = P_INT; // animal type
    this->properties[0].v.i = -1;
    this->properties[1].type = P_FLT; // age
    this->properties[1].v.f = .5f;

    this->body_shape = 0;
    this->speed = 0.f;

    this->dir = DIR_RIGHT;
    this->look_dir = DIR_RIGHT;
    this->i_dir = DIR_RIGHT;
    this->do_recreate_shape = false;

    this->balance = new stabilizer(0);
    this->balance->limit = 100.f;
    this->balance->max_force = 1.f;

    this->num_sliders = 1;
}

void
animal::reset_damping()
{
    this->set_damping(ANIMAL_ANGULAR_DAMPING);
}

void
animal::init()
{
    creature::init();

    this->logic_timer_max = ANIMAL_LOGIC_TIMER_MAX;
    this->new_dir = DIR_LEFT;
    this->previous_age_state = AGE_CHILD;
    this->age_state = AGE_CHILD;
    this->set_scale(0.45f);
    this->do_update_fixture = true;
}

void
animal::restore()
{
    creature::restore();
    //this->setup();

    this->recalculate_effects();
}

void
animal::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    creature::write_state(lvl,lb);
}

void
animal::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    creature::read_state(lvl,lb);
}

void
animal::setup()
{
    creature::setup();

    this->damage_multiplier = 1.5f;
    this->hp = this->max_hp;

    this->target_neck_angle = 0.f;

    this->logic_timer = rand()%this->logic_timer_max;

    this->recalculate_effects();
}

void
animal::on_pause()
{
    creature::on_pause();
}

void
animal::update_fixture()
{
    if (this->body) {
        uint32_t at = this->get_animal_type();
        b2PolygonShape *shape = static_cast<b2PolygonShape*>(this->f_body->GetShape());
        float w = animal_data[at].body_size.w * this->get_scale();
        float h = animal_data[at].body_size.h * this->get_scale();
        shape->SetAsBox(w, h, b2Vec2(0,0), 0);
        this->f_body->Refilter();
        this->body->ResetMassData();
    }

    if (this->feet) {
        this->feet->update_fixture();
    }
}

void
animal::update()
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

    float a = this->get_angle();

    tmat4_load_identity(this->M);
    tmat4_translate(this->M, t.p.x, t.p.y, this->get_z());
    tmat4_rotate(this->M, a*(180.f/M_PI), 0, 0, -1);
    tmat4_rotate(this->M, -90 * this->i_dir + tilt * (180.f/M_PI), 0, 1, 0);

    tmat3_copy_mat4_sub3x3(this->N, this->M);

    creature::update();

    tmat4_scale(this->M, this->get_scale(), this->get_scale(), this->get_scale());
}

void
animal::tick()
{
    if (this->do_recreate_shape) {
        this->recreate_shape();
        this->do_recreate_shape = false;
    }
}

void
animal::update_age(bool force)
{
    if (this->properties[1].v.f < 0.f) {
        this->properties[1].v.f = 0.f;
    } else if (this->properties[1].v.f > 1.f) {
        this->properties[1].v.f = 1.f;
    }

    if (this->properties[1].v.f < 1.f/3.f) {
        this->age_state = AGE_CHILD;
    } else if (this->properties[1].v.f < 2.f/3.f) {
        this->age_state = AGE_TEENAGER;
    } else {
        this->age_state = AGE_ADULT;
    }

    if (force || this->previous_age_state != this->age_state) {
        switch (this->age_state) {
            case AGE_CHILD:     this->set_scale(0.45f); break;
            case AGE_TEENAGER:  this->set_scale(0.6f); break;
            case AGE_ADULT:     this->set_scale(1.0f); break;
        }

        if (!force) this->do_update_fixture = true;
    }

    this->balance->max_force = 1.f*this->get_scale()*this->get_scale();
    this->previous_age_state = this->age_state;
}

void
animal::step()
{
    this->properties[1].v.f += ANIMAL_AGING_SPEED;
    this->update_age();

    if (this->do_update_fixture) {
        this->update_fixture();
        this->do_update_fixture = false;
    }

    if (this->j_head) {
        float a = this->target_neck_angle;
        float c_angle = ((b2RevoluteJoint*)this->j_head)->GetJointAngle();
        float dist = tmath_adist(c_angle, a);
        ((b2RevoluteJoint*)this->j_head)->SetMotorSpeed(dist * 10.f * this->get_scale());
    }

    creature::step();
}

void
animal::mstep()
{
    creature::mstep();
}

void
animal::on_load(bool created, bool has_state)
{
    uint32_t at = this->get_animal_type();
    if (at >= NUM_ANIMAL_TYPES) at = NUM_ANIMAL_TYPES - 1;

    this->set_animal_type(at);
    this->update_age(true);

    creature::on_load(created, has_state);
}

void
animal::recreate_shape()
{
    uint32_t at = this->get_animal_type();

    if (at >= NUM_ANIMAL_TYPES) {
        this->set_animal_type(NUM_ANIMAL_TYPES - 1);
    }

    tms_assertf(this->body, "Animal called recreate_shape without a body");

    if (this->body) {
        while (this->body->GetFixtureList()) {
            this->body->DestroyFixture(this->body->GetFixtureList());
        }
    }

    if (this->body_shape) {
        delete this->body_shape;
        this->body_shape = 0;
    }

    this->create_fixtures();
}

void
animal::create_fixtures()
{
    uint32_t at = this->get_animal_type();

    switch (animal_data[at].body_shape) {
        case BODY_SHAPE_RECT:
            {
                this->body_shape = static_cast<b2Shape*>(new b2PolygonShape());
                ((b2PolygonShape*)this->body_shape)->SetAsBox(animal_data[at].body_size.x, animal_data[at].body_size.y);
            }
            break;

        case BODY_SHAPE_CIRCLE:
            {
                this->body_shape = static_cast<b2Shape*>(new b2CircleShape());
                ((b2PolygonShape*)this->body_shape)->m_radius = animal_data[at].body_size.x;
            }
            break;
    }

    b2FixtureDef fd_body;
    fd_body.shape = this->body_shape;
    fd_body.friction    = 1.f;
    fd_body.density     = 1.f;
    fd_body.restitution = 0.1f;
    fd_body.filter = world::get_filter_for_layer(this->get_layer(), 15);

    (this->f_body = this->body->CreateFixture(&fd_body))->SetUserData(this);

    if (W->is_playing()) {
        this->create_layermove_sensors();

        b2Vec2 p = this->local_to_body(b2Vec2(0, 0), 0);

        b2CircleShape c;
        c.m_radius = 1.f;
        c.m_p = p;

        b2FixtureDef fd;
        fd.isSensor = true;
        fd.restitution = 0.f;
        fd.friction = FLT_EPSILON;
        fd.density = 0.00001f;
        fd.filter = world::get_filter_for_layer(this->get_layer(), this->layer_mask);
        fd.shape = &c;

        (this->fx_sensor = this->body->CreateFixture(&fd))->SetUserData(this);
    }

    if (this->feet) {
        this->feet->disable_sound = true;
    }

    this->update_fixture();
}

void
animal::create_head_joint()
{
    this->destroy_head_joint();

    if (this->body && this->equipments[EQUIPMENT_HEAD] && this->equipments[EQUIPMENT_HEAD]->body) {
        uint32_t at = this->get_animal_type();

        b2RevoluteJointDef rjd;
        rjd.maxMotorTorque = 1.f*this->get_scale();
        rjd.motorSpeed = 0.5f;
        rjd.enableLimit = true;
        rjd.enableMotor = !W->is_paused();

        if (this->look_dir < 0) {
            rjd.lowerAngle = -animal_data[at].neck_angle.y;
            rjd.upperAngle = +animal_data[at].neck_angle.x;
        } else {
            rjd.lowerAngle = -animal_data[at].neck_angle.x;
            rjd.upperAngle = +animal_data[at].neck_angle.y;
        }
        rjd.collideConnected = false;
        rjd.localAnchorA = this->get_scale() * b2Vec2(this->look_dir * this->neck_pos.x, this->neck_pos.y);

        rjd.localAnchorB = this->get_scale() * b2Vec2(-this->look_dir * (this->head_pos.x-this->neck_pos.x), -(this->head_pos.y-this->neck_pos.y));
        rjd.bodyA = this->body;
        rjd.bodyB = this->equipments[EQUIPMENT_HEAD]->body;
        rjd.referenceAngle = 0.f;

        this->j_head = (b2RevoluteJoint*)this->body->GetWorld()->CreateJoint(&rjd);
        tms_debugf("ANIMAL created head joint. %d", this->j_head->GetType());
    } else {
        tms_debugf("no head???? Body: %p. Head: %p", this->body, this->equipments[EQUIPMENT_HEAD]);
        if (this->equipments[EQUIPMENT_HEAD]) {
            tms_debugf("head body: %p", this->equipments[EQUIPMENT_HEAD]->body);
        }
    }
}

void
animal::action_on()
{
    switch (this->get_animal_type()) {
        case ANIMAL_TYPE_OSTRICH:
            this->speed_modifier = 2.f;
            break;
    }
}

void
animal::action_off()
{
    switch (this->get_animal_type()) {
        case ANIMAL_TYPE_OSTRICH:
            this->speed_modifier = -1.f;
            break;
    }
}

void
animal::set_animal_type(uint32_t at)
{
    if (at >= NUM_ANIMAL_TYPES) at = NUM_ANIMAL_TYPES - 1;

    this->properties[0].v.i = at;
    this->base_speed = this->speed = animal_data[at].default_speed;
    this->base_jump_strength = this->jump_strength = animal_data[at].default_jump_strength;
    this->feet_width    = animal_data[at].feet_width;
    this->feet_offset   = animal_data[at].feet_offset;
    this->head_pos      = animal_data[at].head_pos;
    this->neck_pos      = animal_data[at].neck_pos;

    this->set_material(animal_data[at].material);
    this->set_mesh(*animal_data[at].mesh);

    this->set_equipment(EQUIPMENT_HEAD, animal_data[at].head_type);
    this->set_equipment(EQUIPMENT_FEET, animal_data[at].feet_type);

    this->set_uniform("~color", animal_data[at].color.r, animal_data[at].color.g, animal_data[at].color.b, 1.f);

    this->recreate_head_on_dir_change = true;
}

void
animal::reset_friction()
{
    this->set_friction(m_robot.friction);
}

bool
animal::is_roaming()
{
    return this->id != G->state.adventure_id && !this->flag_active(CREATURE_BEING_RIDDEN);
}

void
animal::perform_logic()
{
    if (this->is_alive()) {
        if (this->id != G->state.adventure_id) {
            this->roam_setup_target();

            if (this->roam_target) {
                if (this->is_wandering()) {
                    this->set_speed(this->base_speed);
                    this->set_creature_flag(CREATURE_WANDERING, false);
                }

                bool hostile = false;

                if (this->roam_target_type == TARGET_ENEMY) {
                    hostile = true;
                }

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

                if (hostile) {
                    this->roam_attack();
                }

                this->roam_jump();

                if (!W->level.flag_active(LVL_DISABLE_ROAM_LAYER_SWITCH)) {
                    this->roam_layermove();
                }

                this->roam_walk();

                this->roam_aim();
            } else {
                this->roam_gather();

                this->roam_jump();

                this->roam_target_id = 0;

                this->roam_wander();
            }
        }
    }
}

void
animal::attack(int add_cooldown)
{
    uint32_t at = this->get_animal_type();
    //float amin = animal_data[at].neck_angle.x;
    //float amax = animal_data[at].neck_angle.y;

    switch (at) {
        case ANIMAL_TYPE_OSTRICH:
            this->target_neck_angle = roundf(this->i_dir)*-45;
            break;

        default:
            this->target_neck_angle = roundf(this->i_dir)*45;
            break;
    }
}

void
animal::attack_stop()
{
    this->target_neck_angle = 0.f;
}

void
animal::activate(creature *by)
{
    tms_infof("the animal has been activated");
}

void
animal::on_touch(b2Fixture *my, b2Fixture *other)
{
    creature::on_touch(my, other);

    if (my == this->fx_sensor) {
        this->activator_touched(other);
    }
}

void
animal::on_untouch(b2Fixture *my, b2Fixture *other)
{
    creature::on_untouch(my, other);

    if (my == this->fx_sensor) {
        this->activator_untouched(other);
    }
}
