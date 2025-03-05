#include "creature.hh"
#include "adventure.hh"
#include "anchor.hh"
#include "backpack.hh"
#include "checkpoint.hh"
#include "factory.hh"
#include "fxemitter.hh"
#include "game.hh"
#include "item.hh"
#include "robot_parts.hh"
#include "settings.hh"
#include "world.hh"

creature::creature()
    : cur_activator(0)
    , cur_riding(0)
    , activator_joint(0)
    , last_attacker_id(0)
    , damage_multiplier(1.f)
{
    this->last_damage_tick = 0;
    this->set_flag(ENTITY_DO_STEP,      true);
    this->set_flag(ENTITY_DO_PRE_STEP,  true);
    this->set_flag(ENTITY_DO_MSTEP,     true);
    this->set_flag(ENTITY_IS_CREATURE,  true);

    /* XXX make sure we don't unload the player if the camera isnt focused on him! */
    this->set_flag(ENTITY_DYNAMIC_UNLOADING, true);

    this->gravity_forces = b2Vec2(0.f, 0.f);
    this->shock_forces = 0.f;
    this->angular_damping = 0.f;
    this->feet_offset = .225f;
    this->creature_flags = 0ull;

    this->max_hp = CREATURE_BASE_MAX_HP;

    /* make sure all shapes, bodies, etc are null */
    for (int x=0; x<CREATURE_MAX_FEET_BODIES; x++) {
        this->j_feet[x] = 0;
    }
    this->j_feet_count = 0;
    this->body_shape = 0;
    this->f_lback = 0;
    this->f_lfront = 0;
    this->tool = 0;
    this->weapon = 0;

    this->feet = 0;
    this->head = 0;

    memset(this->equipments, 0, sizeof(this->equipments));
    memset(this->inventory, 0, sizeof(this->inventory));
    memset(this->tools, 0, sizeof(this->tools));
    memset(this->weapons, 0, sizeof(this->weapons));

    this->j_head = 0;
    /* -- */

    this->num_weapons = 0;
    this->num_tools = 0;
    this->features = 0ull;
    this->current_checkpoint = 0;
    this->circuits = 0u;

    this->circuits_compat =
         CREATURE_CIRCUIT_REGENERATION
       | CREATURE_CIRCUIT_ZOMBIE
        ;

    this->look_dir = 0;
    this->new_dir = DIR_LEFT;
    this->i_dir = this->new_dir;
    this->last_i_dir = -10.f;
    this->dir = this->new_dir;
    this->layer_dir = 0;

    this->_state = CREATURE_WALK;
    this->set_state(CREATURE_WALK);
    this->balance = 0;
    this->roam_optimal_big_distance = 0.f;
    this->roam_optimal_small_distance = 0.f;
    this->gives_score = true;

    this->layer_blend = 0.f;
    this->layer_new = 0.f;
    this->layer_old = 0.f;

    this->ground_handler.self = this;

    this->cull_effects_method = CULL_EFFECTS_DISABLE;

    /* specific per creature type */
    this->menu_scale = .75f;
    this->recreate_head_on_dir_change = false;
    this->width = 1.f;
    this->height = 2.f;
    this->fixed_dir = false;
}

void
creature::init()
{
    entity::init();

    this->activators.clear();
    this->_mstep_layermove = false;
    this->_mstep_jump = 0;
    memset(this->damage_accum, 0, sizeof(this->damage_accum));

    /* XXX maybe we should save this between states? */
    this->gather.found_ground_v[0] = false;
    this->gather.found_ground_v[1] = false;
    this->gather.found_ground_v[2] = false;
    this->gather.found_obstacle[0] = false;
    this->gather.found_obstacle[1] = false;
    this->gather.found_obstacle[2] = false;
    this->gather.found_ground = false;

    this->blocked.unset();

    /* roam state */
    this->shoot_target = false;
    this->roam_target = 0; /* overrided by roam_target_id later */

    /* temporary stuff */
    this->query_layer = 0;
    this->found_ground = 0;
    this->found_entity = 0;

    this->lback_count = 0;
    this->lfront_count = 0;
    this->lfront_tpixel_count = 0;

    this->real_lback_count = 0;
    this->real_lfront_count = 0;
    this->real_lfront_tpixel_count = 0;

    this->lback_tick = 0;
    this->lfront_tick = 0;
    this->lfront_tpixel_tick = 0;

    this->target_dist = 0.f;
    this->target_layer = 0;
    this->gravity_forces = b2Vec2(0.f, 0.f);
    this->shock_forces = 0.f;

    this->dir_timer = rand()%CREATURE_DIRCHANGE_TIME;

    this->j_climb = 0;

    /* XXX move to setup */
    this->ladder_id = 0;
    this->ladder_time = 0;
}

/**
 * Make sure animal/robot_base call recalculate_effects
 * at the end of their setup() or restore()
 **/
void
creature::setup()
{
    tms_debugf("creature setup %u", this->id);
    entity::setup();

    this->max_armour = ROBOT_MAX_ARMOUR;
    this->armour = 0.f;

    this->default_pos = this->_pos;
    this->default_angle = this->_angle;
    this->default_layer = this->get_layer();

    /* modifiers, those commented out are set specifically by animal/robot_base */
    this->attack_damage_modifier = 1.f;
    this->speed_modifier = -1.f;
    this->jump_strength_multiplier = 0.f;
    this->base_cooldown_multiplier = 1.f;
    this->cooldown_multiplier = this->base_cooldown_multiplier;
    // this->damage_multiplier = 1.f;
    // this->base_jump_strength
    // this->base_speed

    /* walking */
    this->on_ground = 0.f;

    this->death_step = 0;
    this->finished = false;
    this->jumping = 0;
    this->jump_time = 0;

    this->creature_flags = 0ull;

    /* roam state */
    this->roam_target_id = 0;
    this->roam_target_type = TARGET_NONE;
    this->roam_target_pos.SetZero();
    this->previous_roam_target_id = 0;
    this->target_side = 0;
    this->target_side_accum = 0.f;

    this->layermove_pretimer = 0;
    this->layermove_timer = 0;
    this->velocity = b2Vec2(0.f, 0.f);
    this->last_ground_speed = b2Vec2(0.f, 0.f);

    this->motion = MOTION_DEFAULT;

    if (this->feet){
        this->feet->reset_angles();
    }

    if (this->id == G->state.adventure_id) {
        this->init_adventure();
    }
}

void
creature::restore()
{
    entity::restore();

    for (int x=1; x<this->get_num_bodies(); x++) {
        if (this->get_body(x)) {
            this->get_body(x)->SetLinearVelocity(b2Vec2(this->states[x-1][0], this->states[x-1][1]));
            this->get_body(x)->SetAngularVelocity(this->states[x-1][2]);
        }
    }

    /* initialize things read by read_state */

    if (this->current_checkpoint) {
        this->set_checkpoint(this->current_checkpoint);
    }

    if (this->cur_activator) {
        this->attach_to(this->cur_activator->get_activator_entity());
    }
}

void
creature::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::write_state(lvl, lb);
    /* write_state wrote the first body */
    for (int x=1; x<this->get_num_bodies(); x++) {
        b2Vec2 velocity = this->get_body(x) ? this->get_body(x)->GetLinearVelocity() : b2Vec2(0.f, 0.f);
        float avel = this->get_body(x) ? this->get_body(x)->GetAngularVelocity() : 0.f;
        lb->ensure(3*sizeof(float));
        lb->w_float(velocity.x);
        lb->w_float(velocity.y);
        lb->w_float(avel);
    }

    lb->w_s_float(this->on_ground);
    lb->w_s_uint32(this->death_step);
    lb->w_s_uint8(this->finished);

    lb->w_s_float(this->base_cooldown_multiplier);
    lb->w_s_float(this->base_speed);
    lb->w_s_float(this->base_jump_strength);

    lb->w_s_float(this->attack_damage_modifier);
    lb->w_s_float(this->speed_modifier);
    lb->w_s_float(this->jump_strength_multiplier);
    lb->w_s_float(this->cooldown_multiplier);
    lb->w_s_float(this->damage_multiplier);

    lb->w_s_int32(this->logic_timer);

    lb->w_s_float(this->hp);
    lb->w_s_float(this->max_hp);

    lb->w_s_float(this->max_armour);
    lb->w_s_float(this->armour);

    lb->w_s_int32(this->_state);

    lb->w_s_float(this->i_dir);
    lb->w_s_int32(this->look_dir);
    lb->w_s_int32(this->dir);

    lb->w_s_int32(this->jumping);

    lb->w_s_float(this->default_pos.x);
    lb->w_s_float(this->default_pos.y);
    lb->w_s_float(this->default_angle);
    lb->w_s_float(this->default_layer);

    lb->w_s_uint32(this->roam_target_id);
    lb->w_s_uint8(this->roam_target_type);
    lb->w_s_float(this->roam_target_pos.x);
    lb->w_s_float(this->roam_target_pos.y);
    lb->w_s_uint32(this->previous_roam_target_id);
    lb->w_s_uint8(this->target_side);
    lb->w_s_float(this->target_side_accum);

    lb->w_s_int32(this->layermove_pretimer);
    lb->w_s_int32(this->layermove_timer);

    lb->w_s_float(this->velocity.x);
    lb->w_s_float(this->velocity.y);

    lb->w_s_float(this->last_ground_speed.x);
    lb->w_s_float(this->last_ground_speed.y);

    lb->w_s_uint8(this->motion);

    /* exclude current movement of adventure robot from the flags saved */
    uint64_t mask = ~(
              (this->id == G->state.adventure_id ? CREATURE_MOVING_LEFT : 0)
            | (this->id == G->state.adventure_id ? CREATURE_MOVING_RIGHT : 0)
            | (this->id == G->state.adventure_id ? CREATURE_MOVING_UP : 0)
            | (this->id == G->state.adventure_id ? CREATURE_MOVING_DOWN : 0)
            );
    lb->w_s_uint64(this->creature_flags & mask);

    lb->w_s_id(this->current_checkpoint ? this->current_checkpoint->id : 0);
    lb->w_s_id(this->cur_activator ? this->cur_activator->get_activator_entity()->id : 0);

    lb->w_s_uint32(this->circuits);

    /* equipments */
    lb->w_s_uint8(this->bolt_set);
    for (int x=0; x<NUM_EQUIPMENT_TYPES_1_5; x++) {
        lb->w_s_uint32(this->equipments[x] ? this->equipments[x]->get_equipment_type() : 0);
        if (this->equipments[x]) {
            this->equipments[x]->write_state(lvl, lb);
        }
    }

    for (int x=0; x<NUM_RESOURCES_1_5; x++) {
        lb->w_s_uint64(this->inventory[x]);
    }
}

void
creature::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::read_state(lvl, lb);
    /* read_state read the first body */
    for (int x=1; x<this->get_num_bodies(); x++) {
        this->states[(x-1)][0] = lb->r_float();
        this->states[(x-1)][1] = lb->r_float();
        this->states[(x-1)][2] = lb->r_float();
    }

    this->on_ground = lb->r_float();
    this->death_step = lb->r_uint32();
    this->finished = (bool)lb->r_uint8();

    /* base states, these rarely change, but base_speed for example can change if a robot steps on a command pad */
    this->base_cooldown_multiplier = lb->r_float();
    this->base_speed = lb->r_float();
    this->base_jump_strength = lb->r_float();

    /* current modifiers */
    this->attack_damage_modifier = lb->r_float();
    this->speed_modifier = lb->r_float();
    this->jump_strength_multiplier = lb->r_float();
    this->cooldown_multiplier = lb->r_float();
    this->damage_multiplier = lb->r_float();

    this->logic_timer = lb->r_int32();

    this->hp = lb->r_float();
    this->max_hp = lb->r_float();

    this->max_armour = lb->r_float();
    this->armour = lb->r_float();

    this->_state = lb->r_int32();

    this->last_i_dir = this->i_dir = lb->r_float();

    this->look_dir = lb->r_int32();
    this->dir = lb->r_int32();

    this->jumping = lb->r_int32();
    this->jump_time = 0;

    this->default_pos.x = lb->r_float();
    this->default_pos.y = lb->r_float();
    this->default_angle = lb->r_float();
    this->default_layer = lb->r_float();

    this->roam_target_id = lb->r_uint32();
    this->roam_target_type = lb->r_uint8();
    this->roam_target_pos.x = lb->r_float();
    this->roam_target_pos.y = lb->r_float();
    this->previous_roam_target_id = lb->r_uint32();
    this->target_side = lb->r_uint8();
    this->target_side_accum = lb->r_float();

    this->layermove_pretimer = lb->r_int32();
    this->layermove_timer = lb->r_int32();

    this->velocity.x = lb->r_float();
    this->velocity.y = lb->r_float();

    this->last_ground_speed.x = lb->r_float();
    this->last_ground_speed.y = lb->r_float();

    this->motion = lb->r_uint8();

    this->creature_flags = lb->r_uint64();

    this->current_checkpoint = (checkpoint*)W->get_entity_by_id(lb->r_id());
    entity* act = W->get_entity_by_id(lb->r_id());
    this->cur_activator = act ? act->get_activator() : 0;

    this->circuits = lb->r_uint32();

    this->set_bolt_set(lb->r_uint8());
    for (int x=0; x<NUM_EQUIPMENT_TYPES_1_5; x++) {
        this->set_equipment(x, lb->r_uint32());
        if (this->equipments[x]) {
            this->equipments[x]->read_state(lvl, lb);
        }
    }

    for (int x=0; x<NUM_RESOURCES_1_5; x++) {
        this->inventory[x] = lb->r_uint64();
    }
}

creature::~creature()
{
    if (this->body_shape) {
        delete this->body_shape;
    }
}

void
creature::remove_from_world()
{
    for (int x=0; x<NUM_EQUIPMENT_TYPES; x++) {
        if (this->equipments[x]) {
            this->equipments[x]->remove_from_world();
        }
    }

    for (int x=0; x<CREATURE_MAX_FEET_BODIES; x++) {
        this->j_feet[x] = 0;
    }
    this->j_feet_count = 0;

    if (adventure::player == this) {
        adventure::player = 0;
    }

    entity::remove_from_world();
}

void
creature::on_load(bool created, bool has_state)
{
    this->layer_new = this->get_layer();
    this->layer_old = this->get_layer();
    this->layer_blend = 1.f;

    if (!has_state) {
        /* if we have a state we load the legs in read_state instead,
         * otherwise we create them here */

        this->set_bolt_set(this->get_default_bolt_set());
        this->set_equipment(EQUIPMENT_HEAD, this->get_default_head_type());
        this->set_equipment(EQUIPMENT_HEADWEAR, this->get_default_head_equipment_type());
        this->set_equipment(EQUIPMENT_FEET, this->get_default_feet_type());
        this->set_equipment(EQUIPMENT_BACK, this->get_default_back_equipment_type());
        this->set_equipment(EQUIPMENT_FRONT, this->get_default_front_equipment_type());
    }

    this->recalculate_effects();
}

void
creature::on_pause()
{
    if (this->feet) this->feet->reset_angles();
}

bool
creature::set_bolt_set(int type)
{
    this->bolt_set = type;
    return true;
}

void
creature::add_to_world()
{
    b2BodyDef bd;
    b2BodyType bt;

    if (W->is_paused() && W->is_puzzle() && !this->is_moveable()) {
        /* Robots base body must be made static in puzzle levels if they're not supposed to be moveable.
         * This is to prevent cheating :-) */
        bt = b2_staticBody;
    } else {
        bt = b2_dynamicBody;
    }

    bd.type = bt;
    bd.position = this->_pos;
    bd.angle = this->_angle;

    b2Body *b = W->b2->CreateBody(&bd);
    this->body = b;
    this->body->SetSleepingAllowed(false);

    if (this->balance) {
        this->balance->body = this->body;
    }

    this->create_fixtures();
    this->reset_friction();
    this->reset_damping();

    for (int x=0; x<NUM_EQUIPMENT_TYPES; x++) {
        if (this->equipments[x]) {
            this->equipments[x]->add_to_world();
        }
    }

    this->recalculate_mass_distribution();
}

bool
creature::set_equipment(int e_category, int e_type)
{
    if (_equipment_required_features[e_category] != 0
        && !this->has_feature(_equipment_required_features[e_category])) {
        tms_debugf("creature does not support this equipment type");
        return false;
    }

    if (this->equipments[e_category]) {
        if (this->equipments[e_category]->get_equipment_type() == e_type) {
            return false;
        }

        this->equipments[e_category]->remove_as_child();

        this->equipments[e_category]->remove_from_world();

        delete this->equipments[e_category];

        /* special things */
        if (e_category == EQUIPMENT_HEAD) {
            this->head = 0;
        } else if (e_category == EQUIPMENT_FEET) {
            this->feet = 0;
        }
    }

    this->equipments[e_category] = robot_parts::equipment::make(this, e_category, e_type);

    if (this->equipments[e_category]) {
        this->equipments[e_category]->set_layer(this->get_layer());

        if (this->body) {
            this->equipments[e_category]->add_to_world();
            this->recalculate_mass_distribution();
        }

        this->equipments[e_category]->add_as_child();
    }

    /* special things */
    if (e_category == EQUIPMENT_HEAD) {
        this->head = static_cast<robot_parts::head_base*>(this->equipments[e_category]);
    } else if (e_category == EQUIPMENT_FEET) {
        this->feet = static_cast<robot_parts::feet_base*>(this->equipments[e_category]);
    }

    return true;
}

b2Vec2
creature::get_gravity()
{
    b2Vec2 g = W->get_gravity();
    g += this->gravity_forces;

    return g;
}

float
creature::get_gravity_angle()
{
    b2Vec2 g = this->get_gravity();
    return atan2(g.y, g.x);
}

void
creature::set_ground_speed(float tangent, float normal)
{
    b2Vec2 f = b2Vec2(tangent, normal);

    if (f.LengthSquared() > 100000.f) {
        f = b2Vec2(0.f, 0.f);
    }

    /*
    float curr = this->get_tangent_speed();

    if (this->is_moving_right()) {
        curr -= fmaxf(curr, -this->get_speed()/10.f*1.1f);
    } else if (this->is_moving_left()) {
        curr -= fminf(curr, this->get_speed()/10.f*1.1f);
    }
    */

    if (b2Distance(this->last_ground_speed, f) > CREATURE_BALANCE_THRESHOLD) {
        this->lose_balance();
    }

    //if (fabsf(f) - fabsf(curr) > 100.f)
        //this->last_ground_speed = curr;
    //else {
        this->last_ground_speed = .8f*this->last_ground_speed + .2f*f;
    //}
}

void
creature::set_jump_state(int s, float force_mul, b2Vec2 bias)
{
    if (this->j_feet_count <= 0) {
        return;
    }

    switch (s) {
        default:
        case 0:
            if (this->jumping == 1) {
                this->set_jump_state(2); /* reset some stuff */
            }
            this->jumping = 0;
            this->create_feet_joint(0);

            this->on_jump_end();
            break;

        case 1: /* start jump */
            this->jumping = 1;
            this->on_ground = 0.f;
            this->create_feet_joint(1, bias);

            for (int x=0; x<this->j_feet_count; x++) {
                this->j_feet[x]->SetMotorSpeed(-(this->get_jump_strength())*force_mul*(-(CREATURE_FEET_SPEED)*.75f));
                this->j_feet[x]->SetMaxMotorForce(this->get_jump_strength()*this->get_total_mass()*CREATURE_FEET_FORCE);

                if (this->feet->get_body(x)) {
                    this->feet->get_body(x)->SetFixedRotation(true);
                }
            }
            this->body->SetFixedRotation(true);
            this->jump_time = 0;

            this->on_jump_begin();
            break;

        case 2: /* return feet */
            this->jumping = 2;
            this->body->SetFixedRotation(false);

            for (int x=0; x<this->j_feet_count; x++) {
                if (this->feet->get_body(x)) {
                    this->feet->get_body(x)->SetFixedRotation(false);
                }

                this->j_feet[x]->SetMotorSpeed(-CREATURE_FEET_SPEED);
                this->j_feet[x]->SetMaxMotorForce(CREATURE_FEET_FORCE/2.f * this->get_total_mass());
            }

            break;
    }

    this->recalculate_mass_distribution();
}

float
creature::get_down_angle()
{
    return this->body->GetAngle() - M_PI/2.f;
}

void
creature::deactivate_feet()
{
    for (int x=0; x<this->j_feet_count; x++) {
        if (this->j_feet[x]) {
            if (!this->is_dead()) {
                this->j_feet[x]->SetMaxMotorForce(CREATURE_FEET_FORCE*this->get_total_mass());
                this->j_feet[x]->SetMotorSpeed(-CREATURE_FEET_SPEED);
            } else {
                this->j_feet[x]->SetMaxMotorForce(.2f*CREATURE_FEET_FORCE*this->get_total_mass());
                this->j_feet[x]->SetMotorSpeed(0.f);
            }
        }
    }
    if (this->feet) this->feet->set_on(false);
}

void
creature::activate_feet()
{
    for (int x=0; x<this->j_feet_count; x++) {
        if (this->j_feet[x]) {
            this->j_feet[x]->SetMaxMotorForce(CREATURE_FEET_FORCE*this->get_total_mass());
            this->j_feet[x]->SetMotorSpeed(CREATURE_FEET_SPEED);
        }
    }

    if (this->feet) this->feet->set_on(true);
}

/**
 * If the creature has feet, we need to make sure that the feet weigh
 * a specific ratio of the total weight for jump forces to work correctly
 **/
void
creature::recalculate_mass_distribution()
{
    b2Body *b;
    b2MassData m;

    for (int x=0; x<this->get_num_bodies(); x++) {
        if (this->get_body(x)) {
            this->get_body(x)->ResetMassData();
        }
    }

    if (this->feet) {
        float total = this->get_upper_mass();
        //total += .5f; /* weight of the feet, not really that important */

        int nf = this->feet->get_num_bodies();

        //float t = total * (CREATURE_FEET_BODY_RATIO/(float)nf);
        float t = total * CREATURE_FEET_BODY_RATIO;

        float inv = 1.f-(CREATURE_FEET_BODY_RATIO);

        for (int x=0; x<this->get_num_bodies(); x++) {
            if ((b = this->get_body(x))) {
                float new_mass;
                b->GetMassData(&m);

                if (x >= 1 && x <= 4) {
                    /* this is a feet body */
                    new_mass = t;
                } else {
                    /* something other than feet */
                    new_mass = m.mass * inv;
                }

                m.I = (new_mass/m.mass) * m.I;
                m.mass = new_mass;
                b->SetMassData(&m);
            }
        }
    }
}

void
creature::create_feet_joint(int mode, b2Vec2 bias)
{
    this->destroy_feet_joint();

    if (this->body && this->feet) {
        for (int x=0; x<this->feet->get_num_bodies(); x++) {
            b2PrismaticJointDef pjd;
            pjd.tolerance = .03f;
            pjd.maxMotorForce = CREATURE_FEET_FORCE * this->get_total_mass();
            pjd.motorSpeed = CREATURE_FEET_SPEED * this->get_feet_speed();
            pjd.enableLimit = true;

            if (W->is_paused()) {
                pjd.enableMotor = false;
                pjd.upperTranslation = this->feet->get_offset();
                pjd.lowerTranslation = 0.f;
            } else {
                if (mode == 1) {
                    pjd.lowerTranslation = CREATURE_JUMP_LEN * this->get_scale();
                } else if (mode == 2) {
                    pjd.lowerTranslation = this->feet->get_offset()-.1f;
                } else {
                    pjd.lowerTranslation = 0.f;
                }

                pjd.enableMotor = (mode != 2);
                //pjd.upperTranslation = this->feet->get_offset();
                pjd.upperTranslation = this->feet->get_offset() * this->get_scale();
            }

            pjd.collideConnected = false;
            pjd.localAnchorA = b2Vec2(0.f, 0.f);
            pjd.localAnchorB = b2Vec2(0.f, 0.f);//this->feet->get_body(x)->GetLocalCenter();
            pjd.bodyA = this->body;
            pjd.bodyB = this->feet->get_body(x);

            if (mode == 1) {
                pjd.localAxisA = this->body->GetLocalVector(-this->get_gravity());
            } else {
                pjd.localAxisA = b2Vec2(0.f, 1.f);
            }

            pjd.localAxisA += bias;

            tms_infof("feet joint: %.2f to %.2f. %.2f/%.2f", pjd.lowerTranslation, pjd.upperTranslation, VEC2_INLINE(pjd.localAxisA));

            pjd.referenceAngle = 0.f;
            this->j_feet[x] = (b2PrismaticJoint*)this->body->GetWorld()->CreateJoint(&pjd);
        }

        this->j_feet_count = this->feet->get_num_bodies();
    }
}

void
creature::destroy_feet_joint()
{
    for (int x=0; x<j_feet_count; x++) {
        if (this->j_feet[x]) {
            this->body->GetWorld()->DestroyJoint(this->j_feet[x]);
            this->j_feet[x] = 0;
        }
    }

    this->j_feet_count = 0;
}

void
creature::detach_feet()
{
}

/* check if we're somehow blocked, unable to move */
void
creature::roam_check_blocked()
{
    if (this->new_dir != 0) {
        tms_debug_roam_cb("User attempting to move %d", this->new_dir);
        // We are attempting to move forward
        b2Vec2 r = this->get_position();
        float blend = .55f;
        this->blocked.pt.x = (1.f - blend) * this->blocked.pt.x + r.x * blend;
        this->blocked.pt.y = (1.f - blend) * this->blocked.pt.y + r.y * blend;

        float tangent_dist = fabsf(this->get_tangent_distance(this->blocked.pt));

        /* TODO: readd this block check and make sure it works ok */

        if (tangent_dist < 0.05f && !this->blocked.is_blocked) {
            tms_debug_roam_cb("Setting blocked");
            this->blocked.set();
        } else if (tangent_dist > 1.f) {
            tms_debug_roam_cb("Unsetting blocked, tangent dist: %.2f", tangent_dist);
            this->blocked.unset();
        }
    } else {
        //this->blocked.pt = this->get_position()-b2Vec2(-1.f, 0.f);
        //this->blocked.unset();
    }

    if (this->blocked.is_blocked) {
        tms_debug_roam_cb("blocked? %s %d", this->blocked.is_blocked ? "YES" : "NO", this->blocked.stage);
        tms_debug_roam_cb("attempted layers %d %d %d",
                this->blocked.attempted_layers[this->blocked.stage][0],
                this->blocked.attempted_layers[this->blocked.stage][1],
                this->blocked.attempted_layers[this->blocked.stage][2]);
    }
}

/**
 * Target got absorbed (if the target is an interactive item, it was destroyed)
 **/
void
creature::roam_on_target_absorbed()
{
    this->roam_unset_target();
    this->roam_target_id = this->previous_roam_target_id;
    this->previous_roam_target_id = 0;

    if (this->blocked.is_blocked && this->blocked.stage == 1) {
        this->blocked.previous_stage(this->get_layer());
    }
}

void
creature::roam_retarget()
{
    if (this->blocked.is_blocked && this->blocked.stage == 1
        && this->previous_roam_target_id == 0) {
        //tms_infof("looking for a new target %d %p", this->gather.found_obstacle[0], this->gather.obstacle[0]);
        if (W->level.flag_active(LVL_ENABLE_INTERACTIVE_DESTRUCTION) && this->gather.found_obstacle[0] && this->gather.obstacle[0] && (this->gather.obstacle[0]->flag_active(ENTITY_IS_INTERACTIVE))) {
            this->previous_roam_target_id = this->roam_target_id;
            this->roam_target_id = this->gather.obstacle[0]->id;
            //tms_infof("setting target to %u", this->roam_target_id);
        } else
            this->blocked.done = true;
    }
}

/**
 * Return true if we should untarget the current target
 **/
bool
creature::roam_neglect()
{
    if (!this->roam_target) return true;

    if (this->roam_target->flag_active(ENTITY_IS_CREATURE)) {
        creature *c = static_cast<creature*>(this->roam_target);

        if (c->is_dead()) {
            return true;
        }
    }

    if (this->roam_target_type == TARGET_ANCHOR) {
        anchor *anch = static_cast<anchor*>(this->roam_target);

        if (!anch->is_active()) {
            return true;
        }
    }

    if (!W->level.flag_active(LVL_UNLIMITED_ENEMY_VISION) && this->roam_target_type != TARGET_ANCHOR) {
        if (this->target_dist > 12.5f/* && !this->can_see(this->roam_target)*/) {
            tms_debugf("too long distance - %.2f - %s - %d - %d", this->target_dist, this->roam_target->get_name(), this->roam_target_id, this->roam_target_type);
            return true;
        }
    }

    return false;
}

/* Default behaviour: Look at the target */
void
creature::roam_look()
{
    b2Vec2 target_pos = this->get_roam_target_pos();
    float tangent_dist = this->get_tangent_distance(target_pos);

    if (!this->is_panicked()) {
        if (tangent_dist < 0.f) {
            this->look(DIR_LEFT);
        } else {
            this->look(DIR_RIGHT);
        }
    } else {
        if (tangent_dist < 0.f) {
            this->look(DIR_RIGHT);
        } else {
            this->look(DIR_LEFT);
        }
    }
}

/* Default behaviour: Aim at the target */
void
creature::roam_aim()
{
    b2Vec2 r = this->get_position();
    b2Vec2 o = this->roam_target->get_position();
    o -= r;

    b2Vec2 vel = this->roam_target->get_body(0)->GetLinearVelocity();
    float a = atan2f(o.y, o.x);

    a -= this->get_angle();

    /* XXX: Adjust aim according to the targets velocity and distance, THIS NEEDS TWEAKING */
    //a += (vel.x*0.05f);

    /* XXX: This needs to take gravity into account */
    if (this->look_dir == 1) {
        a += (this->target_dist * 0.025f);
    } else {
        a -= (this->target_dist * 0.025f);
    }

    //this->aim(a);
    this->roam_target_aim = a;
}

/* Default behaviour: Do nothing */
void
creature::roam_attack()
{
}

void
creature::roam_update_dir()
{
    b2Vec2 r = this->get_position();
    //tms_debugf("%.2f/%.2f | %.2f/%.2f", this->roam_target_pos.x, this->roam_target_pos.y, r.x, r.y);
    b2Vec2 target_pos = this->get_roam_target_pos();

    float tangent_dist = this->get_tangent_distance(target_pos);

    if (this->is_panicked()) {
        this->new_dir = (tangent_dist < 0.f ? DIR_RIGHT : DIR_LEFT);
    } else {
        if (this->target_side == 0) {
            if ((this->roam_target_type == TARGET_POSITION || (this->roam_target && this->get_layer() != this->roam_target->get_layer()))
                && this->target_dist-4.f*TARGET_DIST_SCALE < .25f
                && ((tangent_dist < 0.f) == (this->dir == DIR_LEFT))
                    ) {
                this->target_side = (tangent_dist < 0.f) ? DIR_LEFT : DIR_RIGHT;
                tms_debug_roam_ud("Set target side to %d", this->target_side);
            } else {
                target_side_accum *= .97f;
            }
        }

        if (this->target_side != 0) {
            if (this->target_side * tangent_dist < -(1.f+this->target_side_accum)) {
                this->target_side = 0;
                this->target_side_accum += 1.f;
            } else {
                tms_debug_roam_ud("New dir set to target side, %d", this->target_side);
                this->new_dir = this->target_side;
                return;
            }
        }

        robot_parts::weapon *w = this->get_weapon();

        this->new_dir = this->get_optimal_walking_dir(tangent_dist);
    }
}

int
creature::get_optimal_walking_dir(float tangent_dist)
{
    if ((this->target_dist > this->roam_optimal_big_distance*TARGET_DIST_SCALE)
            || (!this->shoot_target && this->target_dist > 2.0f*TARGET_DIST_SCALE)) {
        tms_debug_roam_ud("%p move toward target", this);
        tms_debug_roam_ud("  why? target_dist %.2f > optimal_big_distance %.2f || !shoot_target(%d) && target_dist %.2f > %.2f",
                this->target_dist,
                this->roam_optimal_big_distance*TARGET_DIST_SCALE,
                (int)this->shoot_target,
                this->target_dist, 2.f*TARGET_DIST_SCALE);
        /* move toward the target */
        return (tangent_dist < 0.f ? DIR_LEFT : DIR_RIGHT);
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
creature::roam_jump()
{
    /* Jumping */

    if (this->on_ground <= 0.f)
        return;

    /*if (fabsf(tmath_adist(this->get_down_angle(), this->get_gravity_angle())) > .75f && !this->jumping) {
        this->jump(false);
    } else */if (this->gather.found_obstacle[0]) {

        /* dont try to jump over other living robots, it looks stupid */
        if (this->gather.obstacle[0] && (this->gather.obstacle[0]->flag_active(ENTITY_IS_ROBOT) && ((creature*)this->gather.obstacle[0])->is_alive())) {
            //tms_infof("don't jump over robot");
            return;
        }

        if (this->gather.obstacle[1] && this->gather.obstacle[1]->g_id == O_SPIKES) {
            //tms_infof("don't jump over spikes");
            return;
        }

        if (!this->jumping) {
            if (this->gather.found_obstacle[2]) {
                //tms_infof("can't jump, blocked by: %s", this->gather.obstacle[2]->get_name());
                if (!this->blocked.is_blocked) {
                    this->blocked.set();
                } else if (this->blocked.stage == 0) {
                    this->blocked.done = true;
                }
            } else {
                if ((this->new_dir != 0 && !this->blocked.is_blocked) || (this->blocked.is_blocked && this->blocked.stage == 0)) {
                    _mstep_jump = this->gather.found_obstacle[1] ? 2 : 1;
                    this->jump_action = ROBOT_JUMP_ACTION_MOVE;
                    this->jump_action_time = _mstep_jump*100000;
                    this->blocked.unset();
                }
            }
        }
    } else if (!this->gather.found_ground_v[0] && (this->gather.found_ground_v[1] || this->gather.found_ground_v[2])) {
        /* TODO: Finetune jumping strength */
        _mstep_jump = true;
        this->jump_action = ROBOT_JUMP_ACTION_MOVE;
        this->jump_action_time = _mstep_jump*100000;
        this->blocked.unset();
    }
}

/**
 * Gather information on what we can see, particularly about our target
 **/
void
creature::roam_gather_sight()
{
}

/**
 *
 **/
void
creature::roam_set_target_type()
{
    this->roam_target_type = TARGET_ITEM;
}

void
creature::roam_unset_target()
{
    this->attack_stop();
    this->roam_target = 0;
    this->roam_target_id = 0;
    this->roam_target_type = TARGET_NONE;
}

/**
 * Set up the current target from the saved id if we have one
 **/
void
creature::roam_setup_target()
{
    if (this->roam_target_type == TARGET_POSITION) {
        this->target_dist = b2Distance(this->get_position(), this->get_roam_target_pos());
    } else if (this->roam_target_id != 0) {
        if (this->roam_target_id == this->id) {
            this->roam_unset_target();
            return;
        }

        this->roam_target = W->get_entity_by_id(this->roam_target_id);

        if (!this->roam_target) {
            this->roam_on_target_absorbed();
        } else {
            this->roam_set_target_type();

            b2Vec2 r = this->get_position();
            b2Vec2 target_pos = this->roam_target->get_position();
            this->target_dist = b2Distance(r, target_pos);
            this->target_layer = this->roam_target->get_layer();

            if (this->roam_neglect()) {
                this->roam_unset_target();
            }
        }
    }
}

void
creature::on_release_playing()
{
    this->recalculate_mass_distribution();
}

/* gather information about the surroundings */
void
creature::roam_gather()
{
    if (this->dir != 0) {
        this->gather.found_ground = false;
        this->found_ground = 0;

        int o = ((this->new_dir != 0 ? this->new_dir : this->dir) == DIR_LEFT ? 0 : 1);

        float random_offset = trandf(-.15f, .15f);
        float r = 0.f;
        float g = 0.f;
        float b = 0.f;
        for (int x=0; x<3; x++) {
            this->gather.found_ground_v[x] = false;
#ifdef DEBUG
            if (x == 0) {
                r = .9f; g = .2f; b = .2f;
            } else if (x == 1) {
                r = .2f; g = .9f; b = .2f;
            } else if (x == 2) {
                r = .2f; g = .2f; b = .9f;
            }
#endif
            float w = this->width;
            b2Vec2 starts[2] = {
                this->local_to_world(b2Vec2(-w-(float)x*.2f+random_offset, -.1f), 0),
                this->local_to_world(b2Vec2( w+(float)x*.2f+random_offset, -.1f), 0),
            };
            b2Vec2 ends[2] = {
                this->local_to_world(b2Vec2(-w-(float)x*.2f+random_offset, -3.f), 0),
                this->local_to_world(b2Vec2( w+(float)x*.2f+random_offset, -3.f), 0),
            };

            this->query_layer = this->get_layer();
            W->raycast(&this->ground_handler, starts[o], ends[o], r, g, b);

            if (this->found_ground) {
                this->gather.found_ground_v[x] = true;
                break;
            }
        }

        this->gather.found_ground = this->found_ground > 0;

        float random_length = random_offset;
        random_offset = trandf(-.005f, .005f);
        /* look for obstacles */
        this->found_ground = 0;
        for (int x=0; x<3; x++) {
            this->found_ground = 0;
            float w = (this->width * 0.8f) + random_length;

            b2Vec2 starts[2] = {
                this->local_to_world(b2Vec2(-w, -.3f+(float)x*.5f+random_offset), 0),
                this->local_to_world(b2Vec2( w, -.3f+(float)x*.5f+random_offset), 0),
            };
            b2Vec2 ends[2] = {
                this->local_to_world(b2Vec2(-(w+0.5f), -.3f+(float)x*.5f+random_offset), 0),
                this->local_to_world(b2Vec2( (w+0.5f), -.3f+(float)x*.5f+random_offset), 0),
            };

            this->query_layer = this->get_layer();
            W->raycast(&this->ground_handler, starts[o], ends[o]);

            this->gather.found_obstacle[x] = this->found_ground > 0;
            this->gather.obstacle[x] = this->found_entity;
        }
    } else {
        this->gather.found_ground = false;
        this->gather.found_ground_v[0] = false;
        this->gather.found_ground_v[1] = false;
        this->gather.found_ground_v[2] = false;
        this->gather.found_obstacle[0] = false;
        this->gather.found_obstacle[1] = false;
        this->gather.found_obstacle[2] = false;
    }

}

/**
 * Perform target actions such as picking up items if they're close enough
 **/
void
creature::roam_perform_target_actions()
{
    if (this->roam_target_type == TARGET_ITEM) {
        if (this->target_layer == this->get_layer() && this->target_dist < 2.f) {
            b2Body *b = this->roam_target->get_body(0);
            if (b) {
                b2Vec2 force = this->get_position() - this->roam_target->get_position();
                force *= 50.f*b->GetMass();
                b->ApplyForceToCenter(force);

                G->emit(new discharge_effect(
                            this->get_position(),
                            this->roam_target->get_position(),
                            this->get_layer() * LAYER_DEPTH + .5f,
                            this->get_layer() * LAYER_DEPTH + .5f,
                            5,
                            .1f
                            ), 0);
            }
        }
    }

    if (this->roam_target_type == TARGET_POSITION) {
        if (this->target_dist < 1.f) {
            tms_debugf("reached target position");
            this->roam_unset_target();
        }
    }
}

/* Default behaviour for roam walk is to move toward the target */
void
creature::roam_walk()
{
    /* Walking */
    if (!this->gather.found_ground) {
        tms_debug_roam_w("No ground found, stopping.");
        this->stop();

        if (this->is_wandering()) {
            tms_debug_roam_w(" > We are wandering, speed up dirchange.");
            this->dir_timer += G->timemul(this->logic_timer_max);
        }

        if (this->blocked.is_blocked) {
            tms_debug_roam_w(" > We are blocked, we are done with this layer/direction.");
            this->blocked.done = true;
        }

        return;
    }
    /*
    if (this->blocked.is_blocked && this->blocked.done) {
        tms_debug_roam_w("blocked :(");
        this->new_dir = 0;
    }
    */
    if (this->gather.found_obstacle[0] && this->gather.obstacle[0]) {
        tms_debug_roam_w("Obstacle found: %s. Stop walking", this->gather.obstacle[0]->get_name());
        this->new_dir = 0;
    }

    if (this->new_dir == 0) {
        tms_debug_roam_w("Stopping.");
        this->stop();

        if (this->blocked.is_blocked) {
            this->blocked.done = true;
        }
    } else {
        // If the robot is set to wander, always force look dir to be same as walking dir
        if (this->is_wandering() && this->new_dir != 0) {
            this->look(this->new_dir);
        }

        if (this->new_dir != this->dir) {
            tms_debug_roam_w("Moving %s", this->new_dir == DIR_LEFT ? "left" : "right");
            this->move(this->new_dir);
        }

        if (this->blocked.is_blocked) {
            tms_debug_roam_w(" > We were previously blocked, unset.");
            this->blocked.unset();
        }
            this->go();
    }
}

void
creature::roam_set_target(entity *e)
{
    this->roam_target = e;
    this->roam_target_id = e->id;
}

void
creature::roam_layermove()
{
    int t_inc = 500000;

    if (this->layermove_pretimer > 0) {
        this->layermove_pretimer -= G->timemul(this->logic_timer_max);
        if (this->layermove_pretimer < 0) this->layermove_pretimer = 0;
    }

    if (this->layermove_timer > 0) {
        this->layermove_timer -= G->timemul(this->logic_timer_max);
    }

    /* don't try to change layer if we're lying down */
    if (fabsf(tmath_adist(this->get_down_angle(), this->get_gravity_angle())) > .75f) {
        tms_debug_roam_ls("Can't change layer, we're lying down.");
        return;
    }

    if (this->blocked.is_blocked) {
        tms_debug_roam_ls("We're blocked!. Layermove timer: %d", this->layermove_timer);
        if (this->layermove_timer <= 0) {
            int s = this->blocked.stage;
            tms_debug_roam_ls("Current blockstage: %d", s);
            if (s < 2 && this->blocked.done) {
                tms_debug_roam_ls("we get here");
                this->blocked.attempted_layers[s][this->get_layer()] = true;

                int tb = this->get_layer()-1;
                int tbb = this->get_layer()-2;
                int tf = this->get_layer()+1;
                int tff = this->get_layer()+2;
                if (tb < 0) tb = 0;
                if (tbb < 0) tbb = 0;
                if (tf > 2) tf = 2;
                if (tff > 2) tff = 2;

                bool left=false;
                bool right=false;

                if (!this->blocked.attempted_layers[s][tb] || !this->blocked.attempted_layers[s][tbb])
                    left = true;
                if (!this->blocked.attempted_layers[s][tf] || !this->blocked.attempted_layers[s][tff])
                    right = true;

                if (left && right) {
                    if (rand()%2 == 0)
                        this->layer_dir = -1;
                    else
                        this->layer_dir = 1;

                    tms_debug_roam_ls("Both left and right set, random layerdir set to %d", this->layer_dir);
                } else if (left) {
                    this->layer_dir = -1;
                } else if (right) {
                    this->layer_dir = 1;
                } else {
                    this->layer_dir = 0;
                    this->blocked.next_stage();
                }
                layermove_pretimer = 0;
                t_inc = 1000000;

                tms_debug_roam_ls("Layerdir set to %d", this->layer_dir);
            }
        }
    } else if ((rand()%150 == 0) && layermove_timer <= 0) {
        tms_debug_roam_ls("Perform layermove due to randomness");
        if (rand()%2 == 0)
            this->layer_dir = 1;
        else
            this->layer_dir = -1;
        t_inc = 1000000;
        layermove_pretimer = 0;
    } else if (this->get_layer() != target_layer) {
        if (layermove_pretimer == 0) {
            if (target_layer < this->get_layer())
                this->layer_dir = -1;
            else
                this->layer_dir = +1;
        } else if (layermove_pretimer == -1) {
            layermove_pretimer = 1000000;
        }
    }

    if (this->layer_dir != 0) {
        float leg_dist = this->feet ? this->feet->get_offset() : 0.f;
        b2Vec2 start = this->local_to_world(b2Vec2(0.f, -(leg_dist+.05f)), 0);
        b2Vec2 end = this->local_to_world(b2Vec2(.0f, -2.f), 0);

        this->found_ground = 0;
        this->query_layer = this->get_layer()+this->layer_dir;
        W->raycast(&this->ground_handler, start, end, .9f, .2f, .9f, 7500);

        if (!this->found_ground) {
            tms_debug_roam_ls("Found no ground to layerswitch to in layer %d", this->query_layer);
            this->layer_dir = 0;
            if (this->blocked.is_blocked) {
                for (int x=1; x<=2; x++) {
                    int layer = this->get_layer()+this->layer_dir*x;
                    if (layer >= 0 && layer <= 2)
                        this->blocked.attempted_layers[this->blocked.stage][layer] = true;
                }
            }
        } else {
            if (this->found_entity && this->found_entity->g_id == O_SPIKES) {
                this->layer_dir = 0;
            }
        }
    }

    if (this->layer_dir != 0) {
        tms_debug_roam_ls("We would like to layerswitch in dir %d", this->layer_dir);

        if (this->layermove_timer <= 0 && this->layermove_pretimer == 0) {
            tms_debug_roam_ls(" > We have not layermoved in a while, we can continue.");

            if (this->layermove(this->layer_dir)) {
                this->layermove_pretimer = -1;
                this->layermove_timer = t_inc;
                tms_debug_roam_ls("   > Success!");
            } else {
                tms_debug_roam_ls("   > Failure!");
                if (this->blocked.is_blocked) {
                    for (int x=1; x<=2; x++) {
                        int layer = this->get_layer()+this->layer_dir*x;
                        if (layer >= 0 && layer <= 2)
                            this->blocked.attempted_layers[this->blocked.stage][layer] = true;
                    }
                } else {
                    if (!this->jumping && this->jump_time > 1000000) {
                        tms_debug_roam_ls("  > Setting jump action to layermove.");
                        _mstep_jump = 2;
                        this->jump_action = ROBOT_JUMP_ACTION_LAYERMOVE;
                        this->jump_action_time = 200000;
                        this->layermove_pretimer = -1;
                        this->layermove_timer = t_inc;
                    }
                }
            }
        } else {
            tms_debug_roam_ls(" > Need to wait a little bit before we can layermove again. (%d)", this->layermove_timer);
        }
    }
}

void
creature::roam_wander()
{
    this->reset_limbs();

    if (!this->is_wandering()) {
        this->set_creature_flag(CREATURE_WANDERING, true);
    }

    this->dir_timer += G->timemul(this->logic_timer_max);

    if (this->dir_timer > CREATURE_DIRCHANGE_TIME) {
        this->dir_timer -= CREATURE_DIRCHANGE_TIME;
        this->dir_timer += rand() % (CREATURE_DIRCHANGE_TIME); /* to un-syncronize the robots */

        if (this->get_state() == CREATURE_WALK) {
            this->new_dir = 0;
        } else {
            this->new_dir = (rand()%2)?-1:1;
            this->roam_gather();
            this->look(this->new_dir);
        }
        tms_debug_roam_wa("New dir: %s", new_dir == DIR_LEFT ? "Left" : (new_dir == DIR_RIGHT ? "Right" : "stop"));
    }

    this->roam_walk();
}

bool
creature::roam_can_target(entity *e, bool must_see)
{
    return false;
}

bool
creature::layermove(int dir)
{
    tms_debugf("has_attachment? maybe;-) frozen? %d climbingladder? %d canlayermove? %d",
            this->is_frozen(), this->is_climbing_ladder(), this->can_layermove());

    if (this->is_attached_to_activator()) {
        tms_debugf("Can't layermove because we are attached to an activator!");
        return false;
    }
    if (this->is_frozen()) {
        tms_debugf("Can't layermove because we are frozen!");
        return false;
    }
    if (!this->can_layermove()) {
        tms_debugf("Can't layermove because.. we can't layermove!");
        return false;
    }
    if (this->is_climbing_ladder()) {
        tms_debugf("Can't layermove because we are climbing the ladder");
        return false;
    }
    if (!this->equipments[EQUIPMENT_FEET]) {
        tms_debugf("Can't layermove because we have no feet!");
        return false;
    }

    int newlayer = this->get_layer()+dir;
    if (newlayer < 0) newlayer = 0;
    else if (newlayer > 2) newlayer = 2;

    if (newlayer < this->get_layer()) {
        if (this->lback_count > 0) {
            tms_debugf("Can't layermove because lback_count = %d", this->lback_count);
            return false;
        }
    } else if (newlayer > this->get_layer()) {
        if (this->lfront_count > 0) {
            tms_debugf("Can't layermove because lfront_count = %d", this->lfront_count);
            return false;
        }
    } else {
        tms_debugf("Can't layermove because newlayer = %d (%d)", newlayer, this->get_layer());
        return false;
    }

    if (this->has_attachment()) {
        tms_debugf("Can't layermove because we have an attachment :S");
        return false;
    }

    this->i_dir = dir == 1 ? .5f : (this->look_dir == -1 ? -2.f : 2.f);

    this->layer_new = (float)newlayer;
    this->layer_old = this->get_layer();
    this->layer_blend = 0.f;

    this->set_layer(newlayer);

    return true;
}

void
creature::set_position(float x, float y, uint8_t fr)
{
    entity::set_position(x,y,fr);

    if (this->feet) {
        for (int x=0; x<this->feet->get_num_bodies(); x++) {
            b2Body *b = this->feet->get_body(x);
            if (b) {
                b->SetTransform(
                        this->local_to_world(this->get_scale()*this->feet->get_body_offset(x), 0),
                        this->get_angle());
            }
        }
    }

    if (this->head) {
        b2Body *b = this->head->body;

        if (b) {
            this->head->body->SetTransform(
                    this->local_to_world(b2Vec2(this->look_dir*this->head_pos.x*this->get_scale(), this->head_pos.y*this->get_scale()), 0),
                    this->get_angle());
        }
    }
}

void
creature::set_layer(int l)
{
    this->layer_old = this->get_layer();

    if (!W->is_paused()) {
        G->lock();
        entity::set_layer(l);
        this->set_fixture_layer(l);
        G->unlock();
    } else {
        entity::set_layer(l);
        this->set_fixture_layer(l);
    }

    this->layer_new = this->get_layer();

    if (W->is_playing()) {
        this->_mstep_layermove = true;
    }
}

void
creature::set_fixture_layer(int l)
{
    for (int x=0; x<NUM_EQUIPMENT_TYPES; x++) {
        if (this->equipments[x]) {
            this->equipments[x]->set_layer(l);
        }
    }
}

/* get the speed relative to gravity rotated 90 degrees */
double
creature::get_tangent_speed()
{
    b2Vec2 vel = this->body->GetLinearVelocity();
    b2Vec2 g = -this->get_gravity();

    double ll = g.Length();
    double tx, ty;

    if (ll < 0.1f) {
        if (this->body) {
            b2Vec2 bb = this->body->GetWorldVector(b2Vec2(1.f, 0.f));
            tx = bb.x;
            ty = bb.y;
        } else {
            tx = 1.f;
            ty = 0.f;
        }
    } else {
        double l = 1./ll;

        tx = g.y * l;
        ty = -g.x * l;
    }

    /* now dot the velocity with the tangent */

    return vel.x*tx + vel.y*ty;
}

b2Vec2 creature::get_normal_vector(float mag)
{
    b2Vec2 vel = this->body->GetLinearVelocity();
    b2Vec2 g = -this->get_gravity();
    float l = g.Length();
    g.x *= 1.f/l;
    g.y *= 1.f/l;

    return mag*g;
}

double
creature::get_normal_speed()
{
    b2Vec2 vel = this->body->GetLinearVelocity();
    b2Vec2 g = -this->get_gravity();

    float tmp = g.y;
    g.y = g.x;
    g.x = -tmp;

    double ll = g.Length();
    double tx, ty;

    if (ll < 0.1f) {
        if (this->body) {
            b2Vec2 bb = this->body->GetWorldVector(b2Vec2(0.f, 1.f));
            tx = bb.x;
            ty = bb.y;
        } else {
            tx = 1.f;
            ty = 0.f;
        }
    } else {
        double l = 1./ll;

        tx = g.y * l;
        ty = -g.x * l;
    }

    /* now dot the velocity with the tangent */

    return vel.x*tx + vel.y*ty;
}

void
creature::try_regain_balance()
{
    this->balance_regain += G->timemul(WORLD_STEP);
    if (this->balance_regain >= 400000) {
        this->set_creature_flag(CREATURE_LOST_BALANCE, false);
        this->reset_damping();
        this->last_ground_speed = b2Vec2(this->get_tangent_speed(), 0.f);
    }
}

void
creature::lose_balance()
{
    this->last_ground_speed = b2Vec2(0.f, 0.f);
    this->set_friction(.9f);
    this->set_creature_flag(CREATURE_LOST_BALANCE, true);
    this->set_damping(.1f);
    this->balance_regain = 0;
}

void
creature::pre_step()
{
    this->gravity_forces = b2Vec2(0.f, 0.f);

    if (this->real_lfront_count == 0 && this->lfront_count > 0) {
        if (this->lfront_tick >= 2) {
            this->lfront_count = this->real_lfront_count;
        }
        this->lfront_tick ++;
    }

    if (this->real_lback_count == 0 && this->lback_count > 0) {
        if (this->lback_tick >= 2) {
            this->lback_count = this->real_lback_count;
        }
        this->lback_tick ++;
    }

    if (this->real_lfront_tpixel_count == 0 && this->lfront_tpixel_count > 0) {
        if (this->lfront_tpixel_tick >= 2) {
            this->lfront_tpixel_count = this->real_lfront_tpixel_count;
        }
        this->lfront_tpixel_tick ++;
    }

    if (this->real_lfront_tpixel_count == 1 && this->lfront_tpixel_count == 0) {
        if (this->lfront_tpixel_tick >= 4) {
            this->lfront_tpixel_count = this->real_lfront_tpixel_count;
        }
        this->lfront_tpixel_tick ++;
    }
}

/**
 * called from step if rounded value of i_dir has changed
 **/
void
creature::on_dir_change()
{
    int new_dir = (int)roundf(this->i_dir);

    for (int x=0; x<NUM_EQUIPMENT_TYPES; x++) {
        if (this->equipments[x]) {
            this->equipments[x]->on_dir_change();
        }
    }

    this->last_i_dir = roundf(this->i_dir);

    this->recalculate_mass_distribution();
}

void
creature::step()
{
    //tms_debugf("ground speed %f %f, on_ground=%f", this->last_ground_speed.x, this->last_ground_speed.y, this->on_ground);
    //tms_debugf("real velocity: %f %f", this->body ? this->body->GetLinearVelocity().x : 0 ,this->body ? this->body->GetLinearVelocity().y : 0);
    if (this->shock_forces > 300.f) {
        this->lose_balance();
    }

    this->shock_forces = 0.f;

    if (this->equipments[EQUIPMENT_BACK]) {
        this->equipments[EQUIPMENT_BACK]->step();
    }

    if (this->motion == MOTION_RIDING && this->cur_riding) {
        this->i_dir = this->cur_riding->i_dir;
    }

    {
        float blend = .05f;
        b2Vec2 vel = this->body ? this->body->GetLinearVelocity() : b2Vec2(0.f, 0.f);
        this->velocity = (1.f-blend) * this->velocity + blend*vel;

        blend = .01f;
        if (this->on_ground <= 0.f) {
            this->last_ground_speed = (1.f-blend) * this->last_ground_speed + blend*vel;
        }
    }

    this->ladder_time += G->timemul(WORLD_STEP);

    if (this->ladder_time > 16000) {
        this->ladder_time = 16000;
    }

    bool recalculate = false;
    std::map<uint8_t, creature_effect>::iterator e_it = this->effects.begin();
    while (e_it != this->effects.end()) {
        e_it->second.time -= G->timemul(WORLD_STEP);

        if (e_it->second.time <= 0) {
            this->effects.erase(e_it++);
            recalculate = true;
        } else {
            ++e_it;
        }
    }

    if (recalculate) {
        this->recalculate_effects();
    }

    // handle body rotation logic
    {
        float b1 = .92f + ((1.f-G->get_time_mul()) * .08f);
        float b2 = .08f - ((1.f-G->get_time_mul()) * .08f);
        this->i_dir = this->i_dir * b1 + (float)this->look_dir * b2;
    }

    if (roundf(this->i_dir) != roundf(this->last_i_dir)) {
        this->on_dir_change();
        /* if the creature has changed direction, rebuild all fixtures */
    }

    float ref_angle = this->get_gravity_angle();
    float _a = this->get_down_angle();

    bool standing = this->is_standing();

    float b = WORLD_STEP/1000000.f * 12.f * G->get_time_mul();

    for (int x=0; x<this->j_feet_count; x++) {
        if (this->j_feet[x]) {
            const float inv_dt = 1.f/(((float)(WORLD_STEP+WORLD_STEP_SPEEDUP) * .000001f * G->get_time_mul()));

            const b2Vec2 reaction_force = this->j_feet[x]->GetReactionForce(inv_dt);
            if (reaction_force.x < FLT_EPSILON && reaction_force.y < FLT_EPSILON) {
                continue;
            }

            if (reaction_force.Length() > 1000.f) {
                this->detach_feet();
            }
        }
    }

    if (!this->finished) {

        if (this->creature_flag_active(CREATURE_LOST_BALANCE)) {
            if (fabsf(this->body->GetAngularVelocity()) < 2.f) {
                this->try_regain_balance();
            }
        }

        if (this->feet) {
            this->feet->step();
        }

        bool apply = false;
        if ((this->on_ground > 0.f && standing)) {
            apply = true;
        } else if (/*this == adventure::player*/ !this->is_roaming() && !this->fixed_dir) {
            apply = true;
            //this->reset_angles();
        }

        if (apply && !this->creature_flag_active(CREATURE_LOST_BALANCE)
                && this->balance
                && this->motion != MOTION_CLIMBING) {
            this->balance->apply_forces();
        }

        this->on_ground -= WORLD_STEP/1000000.f * G->get_time_mul();
        if (this->on_ground <= 0.f) {
            this->on_ground = 0.f;
            //this->reset_angles();
        }
    }

    if (this->layer_blend < 1.f) {
        this->layer_blend += WORLD_STEP/1000000.f * 5.f * G->get_time_mul();
    }

    if (_mstep_jump) {
        this->jump(true, _mstep_jump == 1 ? .66f : 1.f);
        _mstep_jump = 0;
    }

    if (_mstep_layermove) {
        this->create_layermove_sensors();
        _mstep_layermove = false;
    }

    if (this->jumping) {
        float t = -100.f;
        for (int x=0; x<this->j_feet_count; x++) {
            t = std::max(t, this->j_feet[x]->GetJointTranslation());
        }

        switch (this->jumping) {
            case 1:
                if (this->jump_time >= 200000
                    || t < CREATURE_JUMP_LEN*this->get_scale()) {
                    this->set_jump_state(2);
                }
                break;

            case 2:
                if (t > 0.f) {
                    this->set_jump_state(0);
                }
                break;
        }
    } else if (this->is_dead()) {
    } else {
    }

    this->apply_destruction();

    if (!this->is_alive() || this->creature_flag_active(CREATURE_IS_ZOMBIE)) {
        if (rand()%(int)(90.f/G->get_time_mul()) == 0) {
            G->emit(new smoke_effect(
                        this->local_to_world(b2Vec2(-.5f + (rand()%100) / 100.f, -.5f + (rand()%100) / 100.f), 0),
                        this->get_layer(),
                        .5f,
                        .5f
                        ), 0);
            G->emit(new discharge_effect(
                        this->local_to_world(b2Vec2(-.5f + (rand()%100) / 100.f, -.5f + (rand()%100) / 100.f), 0),
                        this->local_to_world(b2Vec2(-.5f + (rand()%100) / 100.f, -.5f + (rand()%100) / 100.f), 0),
                        this->get_layer() * LAYER_DEPTH + .5f,
                        this->get_layer() * LAYER_DEPTH + .5f,
                        5,
                        .1f
                        ), 0);
        }
    }

    this->apply_damages();

    if (this->is_alive()) {
        if (this->creature_flag_active(CREATURE_IS_ZOMBIE)) {
            this->damage(.025f * G->get_time_mul(), 0, DAMAGE_TYPE_OTHER, DAMAGE_SOURCE_WORLD, 0);
        } else {
            if (this->has_circuit(CREATURE_CIRCUIT_REGENERATION)) {
                this->hp = std::min(this->hp + .05f * G->get_time_mul(), this->max_hp);
            }
        }
    } else {
        if (W->step_count - this->death_step < 5000) {
            if (rand()%(int)(30.f/G->get_time_mul()) == 0) {
                G->emit(new spark_effect(
                            this->local_to_world(b2Vec2(-.5f + (rand()%100) / 100.f, -.5f + (rand()%100) / 100.f), 0),
                            this->get_layer()
                            ), 0);
            }
        }

        return;
    }

    if (this->j_head) {
        float max_force = 1000.f;
        b2Vec2 reaction_force = this->j_head->GetReactionForce(1.f/(G->timemul(WORLD_STEP) * .000001));
        if (reaction_force.x > FLT_EPSILON && reaction_force.y > FLT_EPSILON) {
            float force_module = reaction_force.Length() * G->get_time_mul();

            if (force_module > max_force) {
                tms_debugf("Detaching head due to force %.2f > %.2f", force_module, max_force);
                this->set_creature_flag(CREATURE_LOST_HEAD, true);
            }
        }
    }

    if (this->is_player()) {
        /* Decide whether caveview should be enabled or disabled */
        if (W->level.flag_active(LVL_DISABLE_CAVEVIEW) == false && this->get_layer() <= 1) {
            if (this->lfront_tpixel_count > 0) {
                G->caveview_size += .03f;
                if (G->caveview_size > 2.f) G->caveview_size = 2.f;
            } else {
                G->caveview_size -= .01f;//.53f;
                if (G->caveview_size < 1.f) G->caveview_size = 0.f;
            }
        } else {
            G->caveview_size = 0.f;
        }
    }
}

void
creature::damage(float amount, b2Fixture *f, damage_type dt, uint8_t damage_source, uint32_t attacker_id)
{
    if (W->level.flag_active(LVL_DISABLE_DAMAGE)) {
        return;
    }

    float real_dmg = this->get_adjusted_damage(amount, f, dt, damage_source, attacker_id);
    this->on_damage(real_dmg, f, dt, damage_source, attacker_id);

    this->last_damage_tick = _tms.last_time;
}

float
creature::get_adjusted_damage(float amount, b2Fixture *f, damage_type dt, uint8_t damage_source, uint32_t attacker_id)
{
    float dmg = amount * this->damage_multiplier;

    robot_parts::equipment *eq = this->get_equipment_by_fixture(f);

    if (eq) {
        dmg = eq->get_adjusted_damage(dmg, f, dt, damage_source, attacker_id);
    } else if (f == this->get_body_fixture()) {
        /* if the inner body is hit, but we have back or front protection, get their values instead */
        if (this->equipments[EQUIPMENT_FRONT]) {
            dmg = this->equipments[EQUIPMENT_FRONT]->get_adjusted_damage(dmg, f, dt, damage_source, attacker_id);
        } else if (this->equipments[EQUIPMENT_BACK]) {
            dmg = this->equipments[EQUIPMENT_BACK]->get_adjusted_damage(dmg, f, dt, damage_source, attacker_id);
        }
    }
    //tms_debugf("creature adjusted damage %f->%f [%p]", amount, dmg, f);

    return dmg;
}

void
creature::on_damage(float dmg, b2Fixture *f, damage_type dt, uint8_t damage_source, uint32_t attacker_id)
{
    const float threshold = 15.f + this->bolt_set * 20.f;

    /* TODO XXX extra strength per equipment type */
    if (this->bolt_set != BOLT_SET_DIAMOND) {
        if (dmg > threshold && dt == DAMAGE_TYPE_FORCE) {
            if (this->equipments[EQUIPMENT_BACK] && f == this->equipments[EQUIPMENT_BACK]->fx) {
                this->set_creature_flag(CREATURE_LOST_BACK, true);
            } else if (this->equipments[EQUIPMENT_FRONT] && f == this->equipments[EQUIPMENT_FRONT]->fx) {
                this->set_creature_flag(CREATURE_LOST_FRONT, true);
            } else if (this->equipments[EQUIPMENT_HEAD] && f == this->equipments[EQUIPMENT_HEAD]->fx) {
                this->set_creature_flag(CREATURE_LOST_HEAD, true);
            } else if (this->equipments[EQUIPMENT_FEET] && this->is_foot_fixture(f)) {
                //this->set_creature_flag(CREATURE_LOST_FEET, true);
            }
        }
    }

    this->last_attacker_id = attacker_id;

    this->damage_accum[dt] += dmg;
}

void
creature::apply_destruction()
{
    for (int x=0; x<NUM_EQUIPMENT_TYPES; x++) {
        if (this->equipments[x] && _equipment_destruction_flags[x] != 0) {
            if (this->creature_flag_active(_equipment_destruction_flags[x])) {
                uint32_t item_id = this->equipments[x]->get_item_id();;

                this->set_creature_flag(_equipment_destruction_flags[x], false);
                this->equipments[x]->separate();
            }
        }
    }

    if (this->is_player()) {
        return;
    }

    if (this->creature_flag_active(CREATURE_LOST_WEAPON)) {
        this->set_creature_flag(CREATURE_LOST_WEAPON, false);

        robot_parts::weapon *w = this->get_weapon();

        if (w) {
            uint32_t item_id = w->get_item_id();

            this->drop_item(item_id);

            this->remove_weapon(this->get_weapon());
        }
    }

    if (this->creature_flag_active(CREATURE_LOST_TOOL)) {
        this->set_creature_flag(CREATURE_LOST_TOOL, false);

        robot_parts::tool *t = this->get_tool();

        if (t) {
            uint32_t item_id = t->get_item_id();

            this->drop_item(item_id);

            this->remove_tool(this->get_tool());
        }
    }
}

void
creature::drop_item(uint32_t item_id)
{
    item *i = static_cast<item*>(of::create(O_ITEM));
    if (i) {
        i->set_item_type(item_id);
        i->set_position(this->get_position());
        i->set_layer(this->get_layer());

        b2Vec2 vel(trandf(2.5f, 5.f), trandf(4.5f, 7.f));

        vel.x *= this->look_dir;

        const b2Vec2 grav = W->get_gravity();
        if (grav.y >  1.f) {
            vel.y *= -1.f;
        }

        G->emit(i, this, vel);
    }
}

void
creature::mstep()
{
    switch (this->motion) {
        case MOTION_DEFAULT:
            break;

        case MOTION_CLIMBING:
            if (!this->ladder_id) {
                this->motion = MOTION_DEFAULT;
            }
            /* make sure we're still climbing */
            break;
        case MOTION_BODY_EQUIPMENT:
            if (!this->equipments[EQUIPMENT_BACK]) {
                this->motion = MOTION_DEFAULT;
            }
            break;
        case MOTION_RIDING:
            break;
    }

    if (this->is_player()) {
        if (G->state.new_adventure && rand()%20==0) {
            if (true) {
                class _cb : public b2QueryCallback {
                  public:
                    entity *result;
                    int what;
                    b2Vec2 offs;

                    _cb()
                    {
                        this->result = 0;
                        this->offs = b2Vec2(0.f, 0.f);
                    }
                    bool ReportFixture(b2Fixture *f)
                    {
                        entity *e;
                        if (f->GetUserData() && (e = static_cast<entity*>(f->GetUserData()))) {

                            if (!(settings["tutorial"]->v.u32 & TUTORIAL_PICKUP_EQUIPMENT)) {
                                if (e->is_item()) {
                                    item *i = static_cast<item*>(e);

                                    if (i->item_category == ITEM_CATEGORY_CIRCUIT
                                        || i->item_category == ITEM_CATEGORY_HEAD
                                        || i->item_category == ITEM_CATEGORY_BACK
                                        || i->item_category == ITEM_CATEGORY_FEET
                                        || i->item_category == ITEM_CATEGORY_LOOSE_HEAD
                                        || i->item_category == ITEM_CATEGORY_FRONT
                                        ) {
                                        this->result = e;
                                        this->offs = b2Vec2(0.f, 1.5f);
                                        this->what = TUTORIAL_TEXT_PICKUP_EQUIPMENT;
                                        return false;
                                    }
                                }
                            }

                            if (!(settings["tutorial"]->v.u32 & TUTORIAL_REPAIR_STATION_DROP)) {
                                if (e->g_id == O_REPAIR_STATION) {
                                    robot_parts::tool *t;
                                    if ((t = adventure::player->get_tool())) {
                                        robot_parts::compressor *c = static_cast<robot_parts::compressor*>(t);

                                        if (c->item_index != 0) {
                                            this->result = e;
                                            this->what = TUTORIAL_TEXT_REPAIR_STATION_DROP;
                                            this->offs = b2Vec2(0.f, e->height*2.f+1);
                                        }
                                    }
                                }
                            }

                            if (!(settings["tutorial"]->v.u32 & TUTORIAL_ZAP_WOOD)) {
                                if (e->g_id == O_PLANT) {
                                    this->result = e;
                                    this->what = TUTORIAL_TEXT_ZAP_WOOD;
                                    this->offs = b2Vec2(0.f, 2.f);
                                }
                            }
                        }

                        return true;
                    };
                } cb;
                b2AABB aabb;
                aabb.lowerBound.Set(this->get_position().x-1, this->get_position().y-1);
                aabb.upperBound.Set(this->get_position().x+1, this->get_position().y+1);
                W->b2->QueryAABB(&cb, aabb);

                if (cb.result) {
                    G->add_tt(cb.what, cb.result, cb.offs);
                }
            }
        }
    }

    if (this->is_alive() && !this->is_frozen()) {
        this->logic_timer += G->timemul(WORLD_STEP);

        if (this->logic_timer >= this->logic_timer_max) {
            this->perform_logic();
            this->logic_timer -= this->logic_timer_max+rand() % (this->logic_timer_max/10);
        }
    } else if (this->is_dead()) {
        uint64_t timediff = _tms.last_time - this->last_damage_tick;

        if (this->hp < 0.f && timediff > 1000000) {
            float mul = tclampf((timediff-1000000)/500000.f, 0.f, 2.f);
            this->hp += 0.15f * G->get_time_mul() * mul;

            if (this->hp > 0.f) {
                this->hp = 0.f;
            }

            G->add_hp(this, std::abs(CREATURE_CORPSE_HEALTH+this->hp)/std::abs(CREATURE_CORPSE_HEALTH), TV_HP_GRAY);
        }
    }
}

void
creature::update()
{
    for (int x=0; x<NUM_EQUIPMENT_TYPES; x++) {
        if (this->equipments[x]) {
            this->equipments[x]->update();
        }
    }
}

void
creature::look(int dir, bool force/*=false*/)
{
    if (force) {
        this->look_dir = dir;
        return;
    }

    if (this->fixed_dir) {
        return;
    }

    if (this->is_dead()) {
        return;
    }

    if (this->is_frozen()) {
        return;
    }

    this->look_dir = dir;
}

float32
creature::cb_ground_handler::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    entity *r = static_cast<entity*>(f->GetUserData());

    if (f->IsSensor()) {
        return -1.f;
    }

    /* Special case for the ground */
    for (int x=0; x<4; ++x) {
        if (W->ground_fx[x] && W->ground_fx[x] == f) {
            self->ground_pt = pt;
            self->found_entity = 0;
            self->found_ground ++;
            return fraction;
        }
    }

    if (world::fixture_get_layer(f) != self->query_layer) {
        return -1.f;
    }

    if (r) {
        /* ignore self */
        if (r == this->self) {
            return -1.f;
        }

        if (r->flag_active(ENTITY_IS_BULLET)) {
            return -1.f;
        }

        if (r->g_id == O_SPIKES) {
            self->ground_pt = pt;
            self->found_entity = r;
            self->found_ground ++;
            return 0.f;
        }
    }

    self->ground_pt = pt;
    self->found_entity = r;
    self->found_ground ++;

    return fraction;
}

void
creature::stop_moving(int dir)
{
    if (this->finished) return;

    if (this->motion == MOTION_RIDING && this->is_player()) {
        if (this->cur_riding) {
            this->cur_riding->stop_moving(dir);
            return;
        }
    }

    if (dir == DIR_LEFT) {
        this->set_creature_flag(CREATURE_MOVING_LEFT, false);

        if (this->creature_flag_active(CREATURE_MOVING_RIGHT))
            this->move(DIR_RIGHT);
        else
            this->stop();
    } else if (dir == DIR_RIGHT) {
        this->set_creature_flag(CREATURE_MOVING_RIGHT, false);

        if (this->creature_flag_active(CREATURE_MOVING_LEFT))
            this->move(DIR_LEFT);
        else
            this->stop();
    } else if (dir == DIR_UP) {
        this->set_creature_flag(CREATURE_MOVING_UP, false);
    } else if (dir == DIR_DOWN) {
        this->set_creature_flag(CREATURE_MOVING_DOWN, false);
    }
}

void
creature::move(int dir, bool force/*=false*/)
{
    if (!force && (this->finished || this->is_frozen())) return;

    if (this->motion == MOTION_RIDING && this->is_player()) {
        if (this->cur_riding) {
            this->cur_riding->move(dir, force);
            return;
        }
    }

    if (dir == DIR_UP || dir == DIR_DOWN) {
        if (this->motion == MOTION_DEFAULT && this->ladder_id && this->ladder_time < 16000) {

            entity *e = W->get_entity_by_id(this->ladder_id);
            if (e) {
                float d = fabsf(tmath_adist(e->get_angle(), this->get_angle()));
                if (d < 0.2f || d > M_PI-0.2f) {
                    this->motion = MOTION_CLIMBING;
                    tms_debugf("switching to climbing motion");
                }
            }
        }
    }

    if (dir == DIR_UP) {
        this->set_creature_flag(CREATURE_MOVING_UP, true);
        return;
    } else if (dir == DIR_DOWN) {
        this->set_creature_flag(CREATURE_MOVING_DOWN, true);
        return;
    }

    this->dir = dir;
    this->go();

    if (this->dir == DIR_LEFT) {
        this->set_creature_flag(CREATURE_MOVING_LEFT, true);
    } else if (this->dir == DIR_RIGHT) {
        this->set_creature_flag(CREATURE_MOVING_RIGHT, true);
    }
}

/**
 * Get a vector along the gravity tangent of the given magnitude
 **/
b2Vec2
creature::get_tangent_vector(float mag)
{
    b2Vec2 g = -W->get_gravity();

    float ll = g.Length();

    if (ll < 0.1f) {
        return b2Vec2(0.f, 0.f);
        /*
        if (this->body)
            return this->body->GetWorldVector(b2Vec2(1.f, 0.f));
        else
            return b2Vec2(0.f, -1.f);
            */
    }

    float l = 1.f/ll;
    g.x *= l;
    g.y *= l;

    b2Vec2 tangent = b2Vec2(g.y * mag, -g.x * mag);

    return tangent;
}

/**
 * Get the distance to p along the tangent axis
 **/
float
creature::get_tangent_distance(b2Vec2 p)
{
    p -= this->get_position();
    b2Vec2 tangent = this->get_tangent_vector(1.f);
    return p.x * tangent.x + p.y * tangent.y;
}

bool
creature::is_player()
{
    if (W->is_paused()) {
        return this->id == G->state.adventure_id;
    }

    return this == adventure::player;
}

float
creature::get_z(void)
{
    return (this->layer_new * (this->layer_blend) +this->layer_old * (1.f-this->layer_blend)) * LAYER_DEPTH;
}

bool
creature::jump(bool forward_force, float force_mul /*=1.f*/)
{
    if (!this->can_jump()) return false;
    if (this->finished) return false;
    if (this->is_frozen()) return false;

    if (this->motion == MOTION_RIDING && this->is_player()) {
        if (this->cur_riding) {
            this->cur_riding->jump(forward_force, force_mul);
            return true;
        }
    }

    if (this->equipments[EQUIPMENT_BACK]) {
        if (static_cast<robot_parts::back*>(this->equipments[EQUIPMENT_BACK])->on_jump()) {
            return true;
        }
    }

    if (W->level.flag_active(LVL_DISABLE_JUMP)) return false;

#if 0
    if (this->jump_joint) {
        W->b2->DestroyJoint(this->jump_joint);
        this->jump_joint = 0;
    }

    //b2Vec2 g = -W->get_gravity();
    b2Vec2 g = -W->get_gravity();

    if (this == adventure::player)
        g += -this->get_body(0)->m_force;

    g.Normalize();

    this->jump_body->SetTransform(this->local_to_world(b2Vec2(.0f, -.25f), 0), this->body->GetAngle());

    b2PrismaticJointDef pjd;
    pjd.collideConnected = false;
    pjd.enableMotor = false;

    if (this->roam) {
        pjd.maxMotorForce = (this->get_jump_strength()+(2.f*ROBOT_DENSITY_MUL))*500.f;
    } else {
        pjd.maxMotorForce = this->get_jump_strength()*150.f;
    }

    pjd.motorSpeed = -(this->get_jump_strength());
    pjd.motorSpeed *= force_mul;
    pjd.bodyA = this->body;
    pjd.bodyB = this->jump_body;
    if (forward_force) {
        if (this->look_dir == DIR_LEFT)
            pjd.localAnchorA = b2Vec2(.125f, -.25f);
        else if (this->look_dir == DIR_RIGHT)
            pjd.localAnchorA = b2Vec2(-.125f, -.25f);
    } else {
        pjd.localAnchorA = b2Vec2(0.f, -.25f);
    }
    pjd.localAnchorB = b2Vec2(0.f, 0.f);
    pjd.localAxisA = b2Vec2(0.f, 1.f);
    //pjd.localAxisA = this->body->GetLocalVector(g);
    pjd.upperTranslation = .0f;
    pjd.lowerTranslation = -.5f;
    pjd.enableLimit = true;

    this->jump_body->GetFixtureList()->SetSensor(false);
    this->jump_joint = (b2PrismaticJoint*)W->b2->CreateJoint(&pjd);
    this->jump_joint->EnableMotor(true);
#endif

    if (this->j_feet_count) {
        b2Vec2 dir_bias = b2Vec2(0.f,0.f);
        if (forward_force) {
            //dir_bias = b2Vec2(this->look_dir*3.f, 0.f);
        }
        this->set_jump_state(1, force_mul, dir_bias);
    }

    return true;
}

void
creature::stop_jump()
{
    if (this->equipments[EQUIPMENT_BACK]) {
        static_cast<robot_parts::back*>(this->equipments[EQUIPMENT_BACK])->on_stop_jump();
    }
    /*
    if (this->jumping && this->body) {
        this->body->ApplyLinearImpulse(this->body->GetWorldVector(b2Vec2(0.f, -2.f)), this->body->GetWorldCenter());
        if (this->jumping == 1)
            this->set_jump_state(2);
    }
    */
}

bool
creature::drop_resource(uint8_t resource_type, uint64_t amount, b2Vec2 relative_pos, float vel)
{
    amount = this->inventory[resource_type] > amount ? amount : this->inventory[resource_type];

    if (amount) {
        this->inventory[resource_type] -= amount;
        if (adventure::player == this && adventure::current_factory) {
            adventure::current_factory->add_resources(resource_type, amount);
        } else {
            resource *r = static_cast<resource*>(of::create(O_RESOURCE));
            r->set_resource_type(resource_type);
            r->set_amount(amount);
            b2Vec2 pos = this->get_position();
            pos += relative_pos;
            r->set_position(pos);
            r->set_layer(this->get_layer());
            G->lock();
            b2Vec2 v = relative_pos;
            v.Normalize();
            G->emit(r, 0, this->get_body(0)->GetLinearVelocity() + vel*v);
            G->unlock();
        }

        return true;
    }

    return false;
}

void
creature::add_resource(uint8_t resource_type, int amount)
{
    if (resource_type < NUM_RESOURCES) {
        if (amount != 0) {
            this->inventory[resource_type] += amount;

            if (this == adventure::player) {
                adventure::highlight_inventory[resource_type] = 1.0f;
                adventure::last_picked_up_resource = resource_type;
                G->refresh_inventory_widgets();
            }

            if (amount > 0) {
                G->add_loot(this, resource_type, amount);
            }
        }
    }
}

void
creature::activate_activator(activator *act)
{
    if (!act) return;

    switch (act->attachment_type) {
        case ATTACHMENT_JOINT:
            {
                entity *actent = act->get_activator_entity();

                if (actent->is_creature()) {
                    creature *cactent = static_cast<creature*>(actent);

                    if (cactent->is_dead()) {
                        return;
                    }
                }
                this->attach_to(act->get_activator_entity());
                this->cur_activator = act;
            }
            break;

        default:
            /* Do nothing */
            break;
    }

    act->activate(this);
}

void
creature::activate_closest_activator(int offset/*=0*/)
{
    if (this->activators.empty()) {
        return;
    }

    activator *closest_activator = 0;

    std::vector<activator*> acts;

    std::copy(this->activators.begin(), this->activators.end(), std::back_inserter(acts));

    std::sort(acts.begin(), acts.end(), game_sorter::distance_to_creature(this));

    if (offset < acts.size()) {
        closest_activator = acts.at(offset);
    } else {
        closest_activator = acts.front();
    }

    if (closest_activator) {
        this->activate_activator(closest_activator);
    }
}

void
creature::respawn()
{
    this->detach();

    b2Vec2 p;
    float a;
    int l;

    if (this->current_checkpoint) {
        p = this->current_checkpoint->local_to_world(b2Vec2(0.f, 1.f), 0);
        a = this->current_checkpoint->get_angle();
        l = this->current_checkpoint->get_layer();
        tms_infof("Respawning at checkpoint %p (%.2f/%.2f)", this->current_checkpoint, p.x, p.y);
    } else if (W->level.flag_active(LVL_ALLOW_RESPAWN_WITHOUT_CHECKPOINT)) {
        p = this->default_pos;
        a = this->default_angle;
        l = this->default_layer;

        tms_infof("Respawning at initial position (%.2f/%.2f)", p.x, p.y);
    } else {
        return;
    }

    this->reset_damping();
    this->reset_friction();
    this->hp = max_hp;
    this->set_state(CREATURE_IDLE);
    this->set_flag(ENTITY_IS_INTERACTIVE, false);

    this->creature_flags = 0ull;

    this->finished = false;

    if (this->is_action_active()) {
        this->action_off();
    }

    this->activate_feet();
    this->recalculate_effects();

    this->set_position(p.x, p.y, 0);
    this->set_angle(a);
    this->set_layer(l);
    this->layer_new = this->get_layer();
    this->layer_old = this->get_layer();
    this->layer_blend = 1.f;

    if (this->current_checkpoint) {
        this->body->SetLinearVelocity(this->current_checkpoint->body->GetLinearVelocity());
        this->body->SetAngularVelocity(this->current_checkpoint->body->GetAngularVelocity());
        G->add_highlight(this->current_checkpoint, false, 1.f);
        this->current_checkpoint->spawned = true;
    } else {
        this->body->SetLinearVelocity(b2Vec2(0.f,0.f));
        this->body->SetAngularVelocity(0.f);
    }

    G->add_highlight(this, false, 1.f);
}

void
creature::on_death()
{
    this->signal(ENTITY_EVENT_DEATH);

    if (this->has_circuit(CREATURE_CIRCUIT_ZOMBIE)) {
        if (!this->creature_flag_active(CREATURE_IS_ZOMBIE)) {
            this->set_creature_flag(CREATURE_IS_ZOMBIE, true);
            this->hp = this->max_hp / 2.f;

            this->creature_flags &= ~(
                      CREATURE_PANICKED
                    );

            if (this != adventure::player) {
                G->add_hp(this, this->hp/this->max_hp);
            }

            this->recalculate_effects();

            tms_debugf("ZOMBIE MODE ENABLED");
            return;
        }
    }

    this->hp = 0.f;

    this->attack_stop();

    this->creature_flags = 0ull;
    this->set_damping(.5f);
    this->finished = true;
    this->set_state(CREATURE_DEAD);
    this->death_step = W->step_count;
    this->set_jump_state(0);
    this->create_feet_joint(0);
    this->deactivate_feet();

    if (this == adventure::player) {
        /* report death to adventure class */
        W->events[WORLD_EVENT_PLAYER_DIE] ++;
        adventure::on_player_die();
    } else {
        for (int n=0; n<NUM_RESOURCES; ++n) {
            this->drop_resource(n, this->get_num_resources(n), b2Vec2(-.25f+.5f*(rand()%10)/10.f, -.25f+.5f*(rand()%10)/10.f), 2.f);
        }
        W->events[WORLD_EVENT_ENEMY_DIE] ++;
        this->set_flag(ENTITY_IS_INTERACTIVE, true);
        this->initialize_interactive();

        if (W->level.flag_active(LVL_ABSORB_DEAD_ENEMIES)) {
            G->timed_absorb(this, W->level.dead_enemy_absorb_time);
        }
    }

    if (this->is_player()) {
        return;
    }

    if (this->has_feature(CREATURE_FEATURE_WEAPONS)) {
        robot_parts::weapon *w = this->get_weapon();
        if (w && w->dropped_on_death()) {
            this->set_creature_flag(CREATURE_LOST_WEAPON, true);
        }
    }

    if (this->has_feature(CREATURE_FEATURE_TOOLS)) {
        robot_parts::tool *t = this->get_tool();
        if (t && t->dropped_on_death()) {
            this->set_creature_flag(CREATURE_LOST_TOOL, true);
        }
    }
}

void
creature::on_touch(b2Fixture *my, b2Fixture *other)
{
    entity *e = (entity*)other->GetUserData();

    if (e) {
        if (my == this->f_lback && (!other->IsSensor() || (e->g_id == O_SPIKES && this->is_roaming()))) {
            if ((this->g_id != O_SPIKEBOT && this->g_id != O_MINIBOT) || e != this->roam_target) {
                this->real_lback_count ++;
                this->lback_count = this->real_lback_count;
            }
        } else if (my == this->f_lfront && (!other->IsSensor() || (e->g_id == O_SPIKES && this->is_roaming()))) {
            if (world::fixture_get_lower_layer(my) == world::fixture_get_layer(other)) {
                if ((this->g_id != O_SPIKEBOT && this->g_id != O_MINIBOT) || e != this->roam_target) {
                    this->real_lfront_count ++;
                    this->lfront_count = this->real_lfront_count;

                    /*
                    tms_debugf("LL ++ real_lfront_count.");
                    tms_debugf("LL New value: %d", this->real_lfront_count);
                    tms_debugf("LL Fixture: %p", other);
                    */
                }
                if ((e->g_id == O_TPIXEL || e->g_id == O_CHUNK)) {
                    this->real_lfront_tpixel_count ++;
                    if (this->real_lfront_tpixel_count > 1) {
                        this->lfront_tpixel_count = this->real_lfront_tpixel_count;
                    }
                    this->lfront_tpixel_tick = 0;
                }
            } else {
                if ((e->g_id == O_TPIXEL || e->g_id == O_CHUNK)) {
                    this->real_lfront_tpixel_count ++;
                    if (this->real_lfront_tpixel_count > 1) {
                        this->lfront_tpixel_count = this->real_lfront_tpixel_count;
                    }
                    this->lfront_tpixel_tick = 0;
                }
            }
        }
    }
}

void
creature::on_untouch(b2Fixture *my, b2Fixture *other)
{
    entity *e = (entity*)other->GetUserData();

    if (e) {
        if (my == this->f_lback && (!other->IsSensor() || (e->g_id == O_SPIKES && this->is_roaming()))) {
            if ((this->g_id != O_SPIKEBOT && this->g_id != O_MINIBOT) || e != this->roam_target) {
                this->real_lback_count --;
                this->lback_tick = 0;
            }
        }
        else if (my == this->f_lfront && (!other->IsSensor() || (e->g_id == O_SPIKES && this->is_roaming()))) {
            if (world::fixture_get_lower_layer(my) == world::fixture_get_layer(other)) {
                if ((this->g_id != O_SPIKEBOT && this->g_id != O_MINIBOT) || e != this->roam_target) {
                    this->real_lfront_count --;
                    this->lfront_tick = 0;

                    /*
                    tms_debugf("LL -- real_lfront_count.");
                    tms_debugf("LL New value: %d", this->real_lfront_count);
                    tms_debugf("LL Fixture: %p", other);
                    */
                }
                if ((e->g_id == O_TPIXEL || e->g_id == O_CHUNK)) {
                    this->real_lfront_tpixel_count --;
                    this->lfront_tpixel_tick = 0;
                }
            } else {
                if ((e->g_id == O_TPIXEL || e->g_id == O_CHUNK)) {
                    this->real_lfront_tpixel_count --;
                    this->lfront_tpixel_tick = 0;
                }
            }
        }

        if (this->real_lback_count < 0) this->real_lback_count = 0;
        if (this->real_lfront_count < 0) this->real_lfront_count = 0;
        if (this->real_lfront_tpixel_count < 0) this->real_lfront_tpixel_count = 0;
    }
}

b2Body*
creature::get_body(uint8_t fr)
{
    switch (fr) {
        case 0:
            return this->body;

        case 1:
        case 2:
        case 3:
        case 4:
            if (this->feet) {
                return this->feet->get_body(fr - 1);
            }
            break;

        case 5:
            if (this->head) {
                return this->head->get_body(0);
            }
            break;
    }

    return 0;
}

void
creature::destroy_layermove_sensors()
{
    if (this->f_lback) {
        this->body->DestroyFixture(this->f_lback);
        this->f_lback = 0;
    }
    if (this->f_lfront) {
        this->body->DestroyFixture(this->f_lfront);
        this->f_lfront = 0;
    }
    this->real_lfront_count = 0;
    this->real_lfront_tpixel_count = 0;
    this->real_lback_count = 0;
    this->lfront_tick = 0;
    this->lback_tick = 0;
    this->lfront_tpixel_tick = 0;
}

void
creature::create_layermove_sensors()
{
    this->destroy_layermove_sensors();

    b2FixtureDef fd;
    fd.density = 0.00001f;
    fd.isSensor = true;

    b2Shape *r;

    if (this->body_shape->m_type == b2Shape::e_circle) {
        b2CircleShape *cs = new b2CircleShape();
        cs->m_radius = this->body_shape->m_radius * 1.25f;
        cs->m_p = ((b2CircleShape*)this->body_shape)->m_p;
        if (!(adventure::player && adventure::player->id == this->id)) {
            cs->m_p += b2Vec2(0.f, 0.2f * this->get_scale());
        } else {
            cs->Scale(0.5f);
        }
        /*
            ((b2CircleShape*)r)->m_radius = ((b2CircleShape*)this->body_shape)->m_radius * 1.25f;
            ((b2CircleShape*)r)->m_p = ((b2CircleShape*)this->body_shape)->m_p + b2Vec2(0.f, 0.2f);
        } else {
            ((b2CircleShape*)r)->m_radius = ((b2CircleShape*)this->body_shape)->m_radius * 0.4f;
            ((b2CircleShape*)r)->m_p = ((b2CircleShape*)this->body_shape)->m_p;
        }
        */

        r = cs;
    } else {
        r = new b2PolygonShape();

        static const int vc = 4;
        b2Vec2 vertices[vc];

        for (int x=0; x<vc; ++x) {
            vertices[x] = ((b2PolygonShape*)this->body_shape)->GetVertex(x);
            vertices[x].x *= 1.1f;
        }

        ((b2PolygonShape*)r)->Set(vertices, vc);
    }

    fd.shape = r;

    switch (this->get_layer()) {
        case 0:
            fd.filter = world::get_filter_for_multilayer(0, 15, 15);
            (this->f_lfront = this->body->CreateFixture(&fd))->SetUserData(this);
            break;

        case 1:
            fd.filter = world::get_filter_for_layer(0, 15);
            (this->f_lback = this->body->CreateFixture(&fd))->SetUserData(this);

            fd.filter = world::get_filter_for_layer(2, 15);
            (this->f_lfront = this->body->CreateFixture(&fd))->SetUserData(this);
            break;

        case 2:
            fd.filter = world::get_filter_for_layer(1, 15);
            (this->f_lback = this->body->CreateFixture(&fd))->SetUserData(this);
            break;
    }

    delete r;
}

void
creature::set_state(int new_state)
{
    int cur_state = this->get_state();

    /* Any 'exit state code' can be placed here */
    switch (cur_state) {

    }

    if (cur_state != new_state) {
        /* Any 'init new state' code can be placed here */
        switch (new_state) {

        }
    }

    this->_state = new_state;
}

void
creature::set_friction(float v)
{
    return;
    if (this->get_body_fixture()) {
        this->get_body_fixture()->SetFriction(v);
    }

    for (int x=0; x<NUM_EQUIPMENT_TYPES; x++) {
        if (x == EQUIPMENT_FEET) {
            continue;
        }

        if (this->equipments[x] && this->equipments[x]->fx) {
            this->equipments[x]->fx->SetFriction(v);
        }
    }
}

void
creature::set_damping(float v)
{
    this->angular_damping = v;

    for (uint32_t x=0; x<this->get_num_bodies(); ++x) {
        b2Body *b = this->get_body(x);

        if (b) {
            b->SetAngularDamping(v);
            b->SetLinearDamping(0);
        }
    }
}

/* TODO: store "default damping" as a variable in creature, which we can always reset to
 * in case damping for animals need to be different */
void
creature::reset_damping()
{
    this->set_damping(ROBOT_DAMPING);
}

void
creature::destroy_head_joint()
{
    if (this->j_head) {
        tms_debugf("Destroying head joint %p", this->j_head);
        this->body->GetWorld()->DestroyJoint(this->j_head);
        this->j_head = 0;
    }
}

stabilizer::stabilizer(b2Body *b)
{
    this->body = b;
    this->max_force = .8f;
    this->limit = .4f;
    this->multiplier = 1.f;

    this->target = 0.f;
}

float
stabilizer::get_offset()
{
    //return this->target-this->body->GetAngle();
    //
    if (!this->body) return 0;

    return -tmath_adist(this->target, this->body->GetAngle());
}

void
stabilizer::apply_forces()
{
    float i = this->get_offset();

    i*=this->multiplier;
    //tms_infof("dist :%f, %f", i, this->target);

    float ai = fabsf(i);

    if (this->body) {
        if (ai < this->limit) {
            if (ai > this->max_force)
                i = copysignf(this->max_force, i);

            this->body->ApplyAngularImpulse(i*G->get_time_mul());
        }
    }
}

b2Vec2
creature::get_smooth_velocity()
{
    return this->last_ground_speed;
}

float
creature::real_arm_angle(creature *c, float a)
{
    if (a < M_PI/2.f && a > -M_PI/2.f) {
        if (c->look_dir == DIR_RIGHT) {
            a = (a + M_PI/2.f)/M_PI;
        } else {
            if (a < 0.f) a += M_PI*2.f;
            a -= M_PI/2.f;
            a = (M_PI-a)/M_PI;
        }
    } else {
        if (c->look_dir == DIR_LEFT) {
            if (a < 0.f) a += M_PI*2.f;
            a -= M_PI/2.f;
            a = (M_PI-a)/M_PI;
        } else {
            a = (a + M_PI/2.f)/M_PI;
        }
    }

    return a;
}

bool
creature::apply_effect(uint8_t item_type, const creature_effect &e)
{
    if (e.time == 0) {
        bool refresh_hp = false;
        /* Apply immediately, only additive effects. this is for permanent effects such as armor */
        float *current;
        float max;
        switch (e.type) {
            case EFFECT_TYPE_HEALTH:
                current = &this->hp;
                max = this->get_max_hp();
                if (this != adventure::player) {
                    refresh_hp = true;
                }
                break;

            case EFFECT_TYPE_ARMOUR:
                current = &this->armour;
                max = this->get_max_armour();
                break;

            default: return false;
        }

        if (*current == max) {
            return false;
        } else if (*current+e.modifier > max) {
            *current = max;
        } else {
            *current += e.modifier;
        }

        if (refresh_hp) {
            G->add_hp(this, this->get_hp()/this->get_max_hp());
        }
    } else {
        std::pair<std::map<uint8_t, creature_effect>::iterator, bool> result = this->effects.insert(std::pair<uint8_t, creature_effect>(item_type, e));
        if (!result.second) {
            result.first->second.time = e.time;
        } else {
            this->recalculate_effects();
        }
    }

    return true;

}

void
creature::recalculate_effects()
{
    if (this->creature_flag_active(CREATURE_IS_ZOMBIE)) {
        this->speed = this->base_speed * .5f;
        this->jump_strength = this->base_jump_strength * .5f;
        this->cooldown_multiplier = this->base_cooldown_multiplier * .5f;
        return;
    }

    /* RESET NON-BASE VALUES */
    this->speed = this->base_speed;
    this->jump_strength = this->base_jump_strength;
    this->cooldown_multiplier = this->base_cooldown_multiplier;

    uint8_t num_speed_mod = 0, num_jump_strength_mod = 0, num_cooldown_multiplier_mod = 0;
    float speed_mod = 0.f, jump_strength_mod = 0.f, cooldown_multiplier_mod = 0.f;

    for (std::map<uint8_t, creature_effect>::iterator it = this->effects.begin();
            it != this->effects.end(); ++it) {
        creature_effect e = it->second;

        switch (e.type) {
            case EFFECT_TYPE_SPEED:
                if (e.method == EFFECT_METHOD_ADDITIVE) {
                    this->speed += e.modifier;
                } else if (e.method == EFFECT_METHOD_MULTIPLICATIVE) {
                    speed_mod = (speed_mod + e.modifier) / (++num_speed_mod);
                }
                break;

            case EFFECT_TYPE_JUMP_STRENGTH:
                if (e.method == EFFECT_METHOD_ADDITIVE) {
                    this->jump_strength += e.modifier;
                } else if (e.method == EFFECT_METHOD_MULTIPLICATIVE) {
                    jump_strength_mod = (jump_strength_mod + e.modifier) / (++num_jump_strength_mod);
                }
                break;

            case EFFECT_TYPE_CD_REDUCTION:
                if (e.method == EFFECT_METHOD_ADDITIVE) {
                    this->cooldown_multiplier += e.modifier;
                } else if (e.method == EFFECT_METHOD_MULTIPLICATIVE) {
                    cooldown_multiplier_mod = (cooldown_multiplier_mod + e.modifier) / (++num_cooldown_multiplier_mod);
                }
                break;
        }
    }

    if (num_speed_mod != 0) this->speed *= speed_mod;
    if (num_jump_strength_mod != 0) this->jump_strength *= jump_strength_mod;
    if (num_cooldown_multiplier_mod != 0) this->cooldown_multiplier *= cooldown_multiplier_mod;

    /*
    tms_debugf("New speed: %.2f", this->speed);
    tms_debugf("New jump strength: %.2f", this->jump_strength);
    tms_debugf("New cooldown multiplier: %.2f", this->cooldown_multiplier);
    */
}

void
creature::set_checkpoint(checkpoint *c)
{
    if (this->current_checkpoint) {
        this->current_checkpoint->set_uniform("~color", CHECKPOINT_COLOR_INACTIVE);
    }

    checkpoint *last = this->current_checkpoint;
    this->current_checkpoint = c;

    if (!W->is_paused()) {
        if (this->current_checkpoint) {
            this->current_checkpoint->set_uniform("~color", CHECKPOINT_COLOR_ACTIVE);
            if (this->current_checkpoint != last) {
                G->add_highlight(this->current_checkpoint, false, 1.f);
                this->current_checkpoint->activated = true;
            }
        }
    }
}

/**
 * Set this creature's tool to the given tool by id,
 * if a pointer to a tool is passed, that pointer is used instead.
 * This allows the robot to keep an inventory of many tools
 * and keep their states persistant.
 **/
bool
creature::set_tool(int tool_id, robot_parts::tool *t/*=0*/)
{
    if (!this->has_feature(CREATURE_FEATURE_TOOLS)) {
        tms_debugf("entity %u with gid %u does not support tools", this->id, this->g_id);
        return false;
    }

    if (this->tool) {
        if (this->tool->get_tool_type() == tool_id) {
            return true;
        }

        this->tool->stop();

        if (this->tool->parent) {
            tms_entity_remove_child(this, this->tool);
        }
        if (this->tool->scene) {
            tms_scene_remove_entity(this->tool->scene, this->tool);
        }

        if (!t) {
            /* XXX NOTE if we received a pointer to a new tool, we always
             * assume that we should never free the tools, since they are
             * handled externally, like by the robot */
            delete this->tool;
        }
        this->tool = 0;
    }

    if (t) {
        this->tool = t;
    } else {
        this->tool = robot_parts::tool::make(tool_id, this);
    }

    if (!this->tool) {
        return false;
    }

    tms_entity_set_prio_all(static_cast<tms_entity*>(this->tool), this->get_layer());

    tms_entity_add_child(this, this->tool);
    if (this->scene) {
        tms_scene_add_entity(this->scene, this->tool);
    }

    return true;
}

/**
 * See the comment on ::set_tool() about the optional pointer argument
 **/
bool
creature::set_weapon(int weapon_id, robot_parts::weapon *w/*=0*/)
{
    if (!this->has_feature(CREATURE_FEATURE_WEAPONS)) {
        tms_debugf("entity %u with gid %u does not support weapons", this->id, this->g_id);
        return false;
    }

    if (this->weapon) {
        if (this->weapon->get_weapon_type() == weapon_id) {
            return true;
        }

        this->weapon->attack_stop();

        if (this->weapon->parent) {
            tms_entity_remove_child(this, this->weapon);
        }
        if (this->weapon->scene) {
            tms_scene_remove_entity(this->weapon->scene, this->weapon);
        }

        if (!w) {
            delete this->weapon;
        }

        this->weapon = 0;
    }

    if (w) {
        this->weapon = w;
    } else {
        this->weapon = robot_parts::weapon::make(weapon_id, this);
    }

    if (!this->weapon) {
        return false;
    }

    tms_entity_set_prio_all(static_cast<tms_entity*>(this->weapon), this->get_layer());

    tms_entity_add_child(this, this->weapon);
    if (this->scene)
        tms_scene_add_entity(this->scene, this->weapon);

    return true;
}

/* Remove the given weapon */
void
creature::remove_weapon(robot_parts::weapon *w)
{
    if (!w) {
        return;
    }

    if (w->parent) {
        tms_entity_remove_child(this, w);
    }

    if (w->scene) {
        tms_scene_remove_entity(w->scene, w);
    }

    if (this->weapon == w) {
        this->weapon = 0;
    }

    for (int x=0; x<NUM_WEAPONS; ++x) {
        if (this->weapons[x] == w) {
            this->weapons[x] = 0;
            delete w;

            -- this->num_weapons;

            break;
        }
    }
}

/* Remove the given tool */
void
creature::remove_tool(robot_parts::tool *t)
{
    if (!t) {
        return;
    }

    if (t->parent) {
        tms_entity_remove_child(this, t);
    }

    if (t->scene) {
        tms_scene_remove_entity(t->scene, t);
    }

    if (this->tool == t) {
        this->tool = 0;
    }

    for (int x=0; x<this->num_tools; ++x) {
        if (this->tools[x] == t) {
            this->tools[x] = 0;
            delete t;

            -- this->num_tools;

            break;
        }
    }
}

void
on_cur_riding_death(entity *self, void *userdata)
{
    creature *c = static_cast<creature*>(self);
    entity *e = static_cast<entity*>(userdata);

    if (e == c->cur_riding) {
        c->detach();
    }
    c->unsubscribe(e);
}

void
creature::attach_to(entity *e)
{
    tms_infof("creature attach to %s", e->get_name());
    this->detach();

    int dir = 0;
    b2Vec2 rpos = this->get_position();

    b2Vec2 pl = e->world_to_local(rpos, 0);

    tms_infof("xy %f %f, %f", pl.x, pl.y, e->get_width());

    if (e->g_id == O_BACKPACK
        || e->flag_active(ENTITY_IS_CONTROL_PANEL)
        || (e->flag_active(ENTITY_IS_CREATURE)
                    && static_cast<creature*>(e)->has_feature(CREATURE_FEATURE_CAN_BE_RIDDEN)
                    )) {
    } else {
        return;
    }

    b2WeldJointDef wjd;
    wjd.bodyA = this->get_body(0);
    wjd.bodyB = e->get_body(0);

    b2Vec2 pt;
    if (!e->flag_active(ENTITY_IS_CREATURE)) {
        if (rpos.x > e->get_position().x) {
            //tms_infof("------------- LEFT");
            dir = DIR_LEFT; /* spaghetti! :DD */
            this->set_attached(DIR_RIGHT);
        } else {
            //tms_infof("------------- RIGHT");
            dir = DIR_RIGHT;
            this->set_attached(DIR_LEFT);
        }

        pt = e->get_body(0)->GetLocalPoint(this->get_body(0)->GetPosition());
        wjd.referenceAngle = wjd.bodyB->GetAngle() - wjd.bodyA->GetAngle();
    } else {
        //pt = e->get_body(0)->GetLocalPoint(b2Vec2(0.f,));
        //this->fixed_dir = true;
        wjd.referenceAngle = 0.f; /* XXX */
        pt = b2Vec2(0.f, .8f);
    }

    //wjd.localAnchorA = pt;
    wjd.localAnchorA = b2Vec2(0.f,0.f);
    wjd.localAnchorB = pt;
    //wjd.localAnchorB = e->local_to_body(pt, 0);
    /*
    if (dir == DIR_LEFT) {
        wjd.referenceAngle = (wjd.bodyB->GetAngle() - wjd.bodyA->GetAngle()) + tmath_adist(wjd.bodyB->GetAngle(), wjd.bodyA->GetAngle());
    } else {
        wjd.referenceAngle = wjd.bodyB->GetAngle();
    }
    */
    wjd.collideConnected = false;
    wjd.frequencyHz = 0.f;

    this->cur_riding = 0;

    if (e->g_id == O_BACKPACK) {
        backpack *b = (backpack*)e;

        if (!b->c.pending) {
            if (b->c.o->flag_active(ENTITY_IS_CONTROL_PANEL)) {
                tms_infof("HAS CONTROL PANEL!");
                G->set_control_panel(b->c.o);
            }
        }

    } else if (e->flag_active(ENTITY_IS_CONTROL_PANEL)) {
        G->set_control_panel(e);
    } else if (e->flag_active(ENTITY_IS_CREATURE)) {
        this->cur_riding = static_cast<creature*>(e);
        this->deactivate_feet();
        this->motion = MOTION_RIDING;

        this->cur_riding->set_flag(CREATURE_BEING_RIDDEN, true);
        this->cur_riding->set_speed(50.f);
        this->cur_riding->stop();
        this->cur_riding->stop_moving(DIR_LEFT);
        this->cur_riding->stop_moving(DIR_RIGHT);

        this->subscribe(this->cur_riding, ENTITY_EVENT_DEATH, on_cur_riding_death, this->cur_riding);
        this->subscribe(this->cur_riding, ENTITY_EVENT_REMOVE, on_cur_riding_death, this->cur_riding);
    }

    (this->activator_joint = (b2WeldJoint*)W->b2->CreateJoint(&wjd))->SetUserData(&adventure::ji);

    G->refresh_widgets();
}

void
creature::detach()
{
    if (this->activator_joint) {
        W->b2->DestroyJoint(this->activator_joint);
    }

    if (this->cur_riding) {
        this->activate_feet();
        this->motion = MOTION_DEFAULT;
        tms_debugf("Disconnecting from ride %p", this->cur_riding);
        this->cur_riding->set_flag(CREATURE_BEING_RIDDEN, false);
        this->cur_riding->set_speed(15.f); /* XXX */
        this->cur_riding->stop();
        this->cur_riding->stop_moving(DIR_LEFT);
        this->cur_riding->stop_moving(DIR_RIGHT);
    }

    if (this->is_player()) {
        G->set_control_panel(adventure::player);
    }

    this->cur_activator = 0;
    this->cur_riding = 0;
    this->activator_joint = 0;

    this->unset_attached();

    G->refresh_widgets();
}

/**
 * Apply accumulated damages since the last step
 *
 * Because we need to differentiate between the separate damage types
 * to know how we should apply the damages, we will need to store damage_accum
 * as an array with size NUM_DAMAGE_TYPES.
 * In apply_damages, we loop through the damage accum for each damage type and
 * apply it as the damage type commands.
 **/
void
creature::apply_damages()
{
    for (int dt=0; dt<NUM_DAMAGE_TYPES; ++dt) {
        if (this->is_dead()) {
            if (dt == DAMAGE_TYPE_ELECTRICITY && this->damage_accum[dt] > 0.f) {
                if (W->level.flag_active(LVL_DEAD_CREATURE_DESTRUCTION)) {
                    this->hp -= this->damage_accum[dt];
                    if (this->is_player()) {
                        this->hp = 0; /* force the hp to 0 to make sure the creature isnt absorbed
                                         immediately if the player is switched to another creature
                                         and this one has very low negative hp */
                    } else {
                        if (W->level.flag_active(LVL_ABSORB_DEAD_ENEMIES)) {
                            /* Reset the timer on our absorb if we took damage while dead! */
                            G->timed_absorb(this, W->level.dead_enemy_absorb_time);
                        }

                        G->add_hp(this, std::abs(CREATURE_CORPSE_HEALTH+this->hp)/std::abs(CREATURE_CORPSE_HEALTH), TV_HP_GRAY);

                        if (this->hp < -CREATURE_CORPSE_HEALTH) {
                            /* XXX FIXME: TODO: Drop items if we were zapped to destruction? */
                            G->lock();
                            G->absorb(this);
                            G->unlock();
                        }
                    }
                }
            }
        } else {
            if (this->damage_accum[dt] > 0.f) {
                if (this->creature_flag_active(CREATURE_GODMODE)) {
                    this->damage_accum[dt] = 0.f;
                    return;
                }

                float amount = this->damage_accum[dt];
                if (this->get_armour() > 0.f) {
                    amount *= 0.5f; /* Reduce damage taken by half and reduce armor by a different value */
                    this->armour -= amount * 0.25f;
                }
                G->add_highlight(this, false, fminf(amount/5.f, 1.f));

                this->hp -= amount;

                //tms_infof("dmg %f, new hp: %f", amount, this->hp);

                if (this != adventure::player) {
                    G->add_hp(this, this->hp/this->max_hp);
                }

                if (this->hp <= 0.f) {
                    this->on_death();
                }
            } else if (this->damage_accum[dt] < 0.f) {
                float amount = -this->damage_accum[dt];
                float cur_hp = this->hp;
                float new_hp = this->hp + amount;

                if (new_hp > this->max_hp) {
                    new_hp = this->max_hp;
                }

                this->hp = new_hp;

                if (this != adventure::player && new_hp != cur_hp) {
                    G->add_hp(this, this->hp/this->max_hp);
                }
            }
        }

        this->damage_accum[dt] = 0.f;
    }
}

/* plz */
bool
creature::has_attachment()
{
    b2Body *b, *o, *jba, *jbb;
    b2Joint *j;

    for (int x=0; x<this->get_num_bodies(); ++x) {
        b = this->get_body(x);
        if (b) {
            for (b2JointEdge *je = b->GetJointList(); je; je = je->next) {
                j = je->joint;

                jba = j->GetBodyA();
                jbb = j->GetBodyB();
                if (jba == b) {
                    o = jbb;
                } else if (jbb == b) {
                    o = jba;
                } else {
                    return true;
                }

                bool is_self = false;
                for (int n=0; n<this->get_num_bodies(); ++n) {
                    if (o == this->get_body(n)) {
                        is_self = true;
                        break;
                    }
                }

                if (!is_self) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool
creature::last_attacker_was_player() const
{
    entity *e = W->get_entity_by_id(this->last_attacker_id);

    if (e && e->is_creature()) {
        return ((creature*)e)->is_player();
    }

    return false;
}
