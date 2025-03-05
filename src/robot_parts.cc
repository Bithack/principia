#include "edevice.hh"
#include "robot_base.hh"
#include "robot.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"
#include "soundmanager.hh"
#include "linebuffer.hh"
#include "explosive.hh"
#include "adventure.hh"
#include "fxemitter.hh"
#include "spritebuffer.hh"
#include "entity.hh"
#include "settings.hh"
#include "creature.hh"
#include "item.hh"
#include "fxemitter.hh"

#define ROCKET_VELOCITY 2.f
#define BOMBER_CHAMBER_ROTATION 65

#define HEAD_SPEED_UP .5f

#define MONOWHEEL_SIZE .5

int _equipment_required_features[NUM_EQUIPMENT_TYPES] =
{
    CREATURE_FEATURE_HEAD,
    0,
    CREATURE_FEATURE_BACK_EQUIPMENT,
    CREATURE_FEATURE_FRONT_EQUIPMENT,
    CREATURE_FEATURE_HEAD,
};

uint64_t _equipment_destruction_flags[NUM_EQUIPMENT_TYPES] =
{
    CREATURE_LOST_HEAD,
    CREATURE_LOST_FEET,
    CREATURE_LOST_BACK,
    CREATURE_LOST_FRONT,
    CREATURE_LOST_HEAD,
};

robot_parts::head_base::head_base(creature *c) :
    equipment(c)
{
    this->model_offset.x = 0.f;
    this->model_offset.y = 0.f;

    this->dangle();
}

void
robot_parts::head_base::dangle()
{
    this->body = 0;
    this->fx = 0;
}

void
robot_parts::head_base::set_layer(int layer)
{
    tms_entity_set_prio_all(static_cast<struct tms_entity*>(this), layer);

    if (this->fx) {
        this->fx->SetFilterData(world::get_filter_for_layer(layer, 15));
    }
}

void
robot_parts::head_base::update()
{
    tmat4_copy(this->M, this->r->M);
    tmat3_copy(this->N, this->r->N);

    if (this->r->j_head) {
        switch (this->r->j_head->GetType()) {
            case e_prismaticJoint:
                {
                    tms_assertf(this->r->get_scale() > 0.f, "scale 0 or less!");

                    float y_offset = this->model_offset.y;
                    y_offset += ((b2PrismaticJoint*)this->r->j_head)->GetJointTranslation();
                    y_offset /= this->r->get_scale() + FLT_EPSILON;
                    tmat4_translate(this->M, 0.f, y_offset, 0);
                }
                return;

            case e_revoluteJoint:
                {
                    float cs, sn;
                    b2Vec2 offset = this->r->get_position() - this->body->GetPosition();

                    float x = this->r->look_dir * (-offset.x);
                    float y = -offset.y;

                    b2RevoluteJoint *j = ((b2RevoluteJoint*)this->r->j_head);
                    float a = this->r->look_dir * j->GetJointAngle();

                    float ca = -this->r->look_dir * this->r->get_angle();
                    tmath_sincos(ca, &sn, &cs);

                    float _x = x * cs - y * sn;
                    float _y = x * sn + y * cs;

                    tmat4_translate(this->M, 0, _y, _x);
                    tmat4_rotate(this->M, a * RADTODEG, 1, 0, 0);
                    tmat4_scale(this->M, this->r->get_scale(), this->r->get_scale(), this->r->get_scale());
                }
                return;

            default:
                // nothing
                break;
        }
    }

    tmat4_translate(this->M, 0.f, 0, 0);
}

robot_parts::robot_head::robot_head(creature *c) :
    head_base(c)
{
    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
    this->set_material(&m_robot);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_HEAD));
    this->model_offset.x = 0.f;
    this->model_offset.y = 0.f;
}

robot_parts::robot_head_inside::robot_head_inside(creature *c)
    : robot_head(c)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_HEAD_INSIDE));
    this->set_material(&m_robot2);
}

void
robot_parts::robot_head::create_fixtures()
{
    int layer = this->r->get_layer();
    float w = .281f;
    float h = (.880f-.430f)/2.f;

    if (W->level.version < LEVEL_VERSION_1_5) {
        h -= .05f;
    }

    b2PolygonShape head_shape;
    head_shape.SetAsBox(w, h);
    head_shape.Scale(this->r->get_scale());

    b2FixtureDef fd_head;
    fd_head.shape = &head_shape;
    fd_head.friction = m_robot_head.friction;
    fd_head.density = m_robot_head.density;
    fd_head.restitution = m_robot_head.restitution;
    fd_head.filter = world::get_filter_for_layer(layer, 15);

    (this->fx = this->body->CreateFixture(&fd_head))->SetUserData(this->r);
}

robot_parts::pig_head::pig_head(creature *c) :
    head_base(c)
{
    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
    this->set_material(&m_animal);
    this->set_mesh(mesh_factory::get_mesh(MODEL_PIG_HEAD));
    //this->set_uniform("~color", 1.f, 0.f, 0.f, 1.f);
    this->model_offset.x = 0.f;
    this->model_offset.y = 0.f;
}

void
robot_parts::pig_head::create_fixtures()
{
    int layer = this->r->get_layer();

    float w = .281f;
    float h = (.880f-.430f)/2.f;
    b2PolygonShape head_shape;
    head_shape.SetAsBox(w, h);
    head_shape.Scale(this->r->get_scale());

    b2FixtureDef fd_head;
    fd_head.shape = &head_shape;
    /* XXX */
    fd_head.friction = m_robot_head.friction;
    fd_head.density = m_robot_head.density;
    fd_head.restitution = m_robot_head.restitution;
    fd_head.filter = world::get_filter_for_layer(layer, 15);

    (this->fx = this->body->CreateFixture(&fd_head))->SetUserData(this->r);
}

robot_parts::ostrich_head::ostrich_head(creature *c) :
    head_base(c)
{
    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
    this->set_material(&m_animal);
    this->set_mesh(mesh_factory::get_mesh(MODEL_OSTRICH_HEAD));
    this->model_offset.x = 0.f;
    this->model_offset.y = -.26f;
}

void
robot_parts::ostrich_head::create_fixtures()
{
    int layer = this->r->get_layer();
    int new_dir = (int)roundf(this->r->i_dir);

    float w = .2f;
    float h = .2f;
    b2PolygonShape head_shape;
    head_shape.SetAsBox(w, h, b2Vec2(new_dir * .1f, 1.f), 0.f);
    head_shape.Scale(this->r->get_scale());

    b2FixtureDef fd_head;
    fd_head.shape = &head_shape;
    /* XXX */
    fd_head.friction = m_robot_head.friction;
    fd_head.density = m_robot_head.density;
    fd_head.restitution = m_robot_head.restitution;
    fd_head.filter = world::get_filter_for_layer(layer, 15);

    (this->fx = this->body->CreateFixture(&fd_head))->SetUserData(this->r);
}

robot_parts::dummy_head::dummy_head(creature *c) :
    head_base(c)
{
    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
    this->set_mesh(mesh_factory::get_mesh(MODEL_DUMMY_HEAD));
    this->set_material(&m_animal);
    //this->set_uniform("color", 1.f, 0.f, 0.f, 1.f);
    this->model_offset.x = 0.f;
    this->model_offset.y = 0.f;
}

void
robot_parts::dummy_head::create_fixtures()
{
    int layer = this->r->get_layer();

    float w = .281f;
    float h = (.880f-.430f)/2.f;
    b2PolygonShape head_shape;
    head_shape.SetAsBox(w, h);
    head_shape.Scale(this->r->get_scale());

    b2FixtureDef fd_head;
    fd_head.shape = &head_shape;
    /* XXX */
    fd_head.friction = m_robot_head.friction;
    fd_head.density = m_robot_head.density;
    fd_head.restitution = m_robot_head.restitution;
    fd_head.filter = world::get_filter_for_layer(layer, 15);

    (this->fx = this->body->CreateFixture(&fd_head))->SetUserData(this->r);
}

robot_parts::cow_head::cow_head(creature *c) :
    head_base(c)
{
    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
    this->set_material(&m_animal);
    this->set_mesh(mesh_factory::get_mesh(MODEL_COW_HEAD));
    //this->set_uniform("~color", 1.f, 0.f, 0.f, 1.f);
    this->model_offset.x = 0.f;
    this->model_offset.y = 0.f;
}

void
robot_parts::cow_head::create_fixtures()
{
    int layer = this->r->get_layer();

    float w = .281f;
    float h = (.880f-.430f)/2.f;
    b2PolygonShape head_shape;
    head_shape.SetAsBox(w, h);
    head_shape.Scale(this->r->get_scale());

    b2FixtureDef fd_head;
    fd_head.shape = &head_shape;
    /* XXX */
    fd_head.friction = m_robot_head.friction;
    fd_head.density = m_robot_head.density;
    fd_head.restitution = m_robot_head.restitution;
    fd_head.filter = world::get_filter_for_layer(layer, 15);

    (this->fx = this->body->CreateFixture(&fd_head))->SetUserData(this->r);
}

robot_parts::leg::leg(int d, creature *c, feet *f)
{
    entity();
    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
    this->set_material(&m_robot_leg);
    this->set_mesh(static_cast<tms::mesh*>(const_cast<struct tms_mesh*>(tms_meshfactory_get_cube())));
    this->set_uniform("~color", .5f, .5f, .5f, 1.f);

    tms_entity_add_child(this, this->myfoot = new foot(d, this));

    this->d = d;
    this->c = c;
    this->f = f;
    this->cc = 0.f;
}

robot_parts::leg::foot::foot(int d, leg *l)
{
    entity();
    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
    this->set_material(&m_robot_foot);
    this->set_mesh(static_cast<tms::mesh*>(const_cast<struct tms_mesh*>(tms_meshfactory_get_cube())));
    this->set_uniform("~color", .9f, .9f, .9f, 1.f);
    this->d = d;
    this->l = l;
}

void
robot_parts::leg::update()
{
    tmat4_copy(this->M, this->c->M);
    tmat3_copy(this->N, this->c->N);

    tmat4_translate(this->M, 0, 0, this->f->local_x_offset);

    float a = (this->d ? 0 : 180) + this->f->stepcount * RADTODEG;
    a = fmod(a, 360.f);

    if (a < 0.f) {
        a += 360;
    }

    a = 360-a;
    float s2,c2;
    tmath_sincos(a * DEGTORAD, &s2, &c2);

    float m = 1.f;
    if (a > 270.f) {
        m = (a - 270.f)/90.f;
        a = 360.f;
    } else if (a <= 270.f && a >= 180.f) {
        m = 1.f-(a-180.f)/90.f;
        a = 180.f;
    }
    a *= DEGTORAD;

    float c, s;
    tmath_sincos(a, &s, &c);
    this->cc = (1.f-(c2+1.f)/2.f)*this->c->i_dir;//1.f-m * (c+1.f)/2.f;
    float bb = (this->c->i_dir + 1.f)/2.f;
    this->cc = this->cc * bb + ((c2+1.f)/2.f)*(1.f-bb);//1.f-m * (c+1.f)/2.f;
    s *= this->f->y;

    //tmat4_rotate(this->M, a, 1, 0, 0);
    float xx = .2f*m*s*this->c->get_scale();
    float yy = .2f*m*c*this->c->get_scale();

    b2CircleShape *c0,*c1;

    float o = this->c->j_feet[this->f->body_index] ? this->c->j_feet[this->f->body_index]->GetJointTranslation() : 0.f;

    o = std::max(o, -.55f);

    if (this->c->is_robot() && ((robot_base*)this->c)->robot_type == ROBOT_TYPE_ROBOT && this->c->is_action_active()) {
        float offset = this->f->get_offset();
        xx *= tclampf(o/-offset, 0.f, 1.f);
        yy *= tclampf(o/-offset, 0.f, 1.f);
    }

#define ABC .45f

    tmat4_translate(this->M,
            ((this->d ? -1.f : 1.f) * this->c->get_scale()) * -.15,
            (o-ABC/2.f) / (this->c->get_scale() + FLT_EPSILON),
            0);

    if (this->f->f0 && this->f->f1) {
        c0 = static_cast<b2CircleShape*>(this->f->f0->GetShape());
        c1 = static_cast<b2CircleShape*>(this->f->f1->GetShape());
        //cc = 1.f-cc;
        float blend = (c0->m_p.y+ABC) * cc + (c1->m_p.y+ABC)*(1.f-cc);
        //tms_infof("blend %f", blend);
        tmat4_translate(this->M, 0, blend, 0);
    }

    tmat4_translate(this->M, 0, xx-.25, yy);

#undef ABC

    //tmat4_rotate(this->M, -a, 1, 0, 0);

    tmat4_copy(this->transform, this->M);

    float scale_thickness = tclampf(.1 * this->c->get_scale(), .025, .4);
    float scale_length = .5f * (this->c->get_scale());

    tmat4_scale(this->M, scale_thickness, scale_length, scale_thickness);

    this->myfoot->update();
}

void
robot_parts::leg::foot::update()
{
    tmat4_copy(this->M, this->l->transform);
    float y_offset = (-.4f + .20f) * this->l->c->get_scale();
    tmat4_translate(this->M, 0, y_offset, 0);

    b2CircleShape *c0,*c1;

    if (this->l->f->f0 && this->l->f->f1 && !this->l->c->is_dead()) {
        c0 = static_cast<b2CircleShape*>(this->l->f->f0->GetShape());
        c1 = static_cast<b2CircleShape*>(this->l->f->f1->GetShape());

        float a = this->l->c->get_angle();

        float fn0 = this->l->f->foot_normal[0] - a;
        float fn1 = this->l->f->foot_normal[1] - a;

        tmat4_rotate(this->M, roundf(this->l->c->i_dir)*(180.f/M_PI)*((fn0-M_PI/2.f)*this->l->cc + (fn0 + tmath_adist(fn0, fn1)-M_PI/2.f)*(1.f-this->l->cc)), 1, 0, 0);
    }

    tmat3_copy_mat4_sub3x3(this->N, this->M);

    /*
    float sx = tclampf(.3f  * this->l->c->get_scale() * this->l->f->foot_scale, .1f, .5f);
    float sy = tclampf(.15f * this->l->c->get_scale(), .05f, .5f);
    float sz = tclampf(.4f  * this->l->c->get_scale() * this->l->f->foot_scale, .2f, .8f);
    */

    float sx = .3f  * this->l->c->get_scale() * this->l->f->foot_scale;
    float sy = .15f * this->l->c->get_scale();
    float sz = .4f  * this->l->c->get_scale() * this->l->f->foot_scale;

    tmat4_scale(this->M, sx, sy, sz);
}

void
robot_parts::arm::step()
{
    if (this->cooldown_timer > 0) {
        this->cooldown_timer -= G->timemul(WORLD_STEP) * this->c->cooldown_multiplier;
    }

    this->fired = false;

    if (this->used) {
        this->fired = true;
        this->used = false;
    }
}

void
robot_parts::arm::on_attack()
{
    this->used = true;
}

robot_parts::arm_cannon::arm_cannon(creature *c)
    : weapon(c)
{
    this->terror = 0.f;
    this->max_range = 9.f;
    this->cooldown = 100000;
    //this->cooldown = 1000;

    this->set_material(&m_robot_arm);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_GUNARM));
    this->set_uniform("~color", ARM_CANNON_COLOR, 1.f);
}

void
robot_parts::arm_cannon::attack(int add_cooldown/*=0*/)
{
    if (this->cooldown_timer <= 0) {
        float angle = this->c->get_angle() + M_PI*1.5f + this->c->look_dir*this->get_arm_angle() * M_PI;
        b2Vec2 p = this->c->local_to_world(ROBOT_ARM_POS, 0);

        item *bullet = of::create_item(ITEM_BULLET);
        bullet->set_scale(this->c->get_scale());
        G->timed_absorb(bullet, 7.5);

        b2Vec2 v;
        tmath_sincos(angle, &v.y, &v.x);

        bullet->_pos = p + .5f*v;
        bullet->_angle = 0.f;
        bullet->set_layer(this->c->get_layer());

        v *= -1.0f;
        this->c->body->ApplyLinearImpulse(this->c->get_scale()*.25f*v, this->c->body->GetWorldPoint(b2Vec2(0,.25f)));

        v *= -BULLET_VELOCITY;
        G->lock();
        if (W->level.version < LEVEL_VERSION_1_5) {
            bullet->g_id = 0;
        }
        G->emit(bullet, this->c, v+this->c->get_body(0)->GetWorldVector(this->c->last_ground_speed));
        G->play_sound(SND_ROBOT_SHOOT, p.x, p.y, 0, 1.f);
        G->unlock();

        this->cooldown_timer = this->cooldown;
        this->cooldown_timer += add_cooldown;

        this->on_attack();
    }
}

void
robot_parts::arm::update()
{
    tmat4_copy(this->M, this->c->M);
    tmat3_copy(this->N, this->c->N);

    tmat4_translate(this->M, 0.f, .25f, 0);
    //tmat4_rotate(this->M, this->arm_angle * 180.f * (1.f-this->arm_fold), 1.f, 0.f, 0.f);
    tmat4_rotate(this->M, this->arm_angle * 180.f, 1.f, 0.f, 0.f);
    tmat4_translate(this->M, 0.f, -.25f, 0);

    float mat[16];
    tmat4_load_identity(mat);
    //tmat4_rotate(mat, this->arm_angle * 180.f * (1.f-this->arm_fold), 1.f, 0.f, 0.f);
    tmat4_rotate(mat, this->arm_angle * 180.f, 1.f, 0.f, 0.f);
    tmat3_copy_mat4_sub3x3(this->N, this->M);
}

robot_parts::shotgun::shotgun(creature *c)
    : weapon(c)
{
    this->terror = 0.1f;
    this->max_range = 6.5f;
    this->cooldown = 750 * 1000;
    this->played_reload_sound = true;

    this->set_material(&m_weapon);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ARM_SHOTGUN));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);
}

void
robot_parts::shotgun::attack(int add_cooldown/*=0*/)
{
    if (this->cooldown_timer <= 0) {
        for (int n=0; n<8; ++n) {
            float angle = this->c->get_angle() + M_PI*1.5f + this->c->look_dir*this->get_arm_angle() * M_PI + (0.15f-((rand()%100)/100.f) * 0.3f);

            item *bullet = of::create_item(ITEM_SHOTGUN_PELLET);
            bullet->set_scale(this->c->get_scale());
            G->timed_absorb(bullet, 7.5);

            b2Vec2 v;
            tmath_sincos(angle, &v.y, &v.x);
            b2Vec2 p = this->c->get_position();

            bullet->_pos = p + 1.f*v;
            bullet->_angle = 0.f;
            bullet->set_layer(this->c->get_layer());

            v *= -1.0f;
            b2Vec2 impulse = v;
            impulse *= 0.2f;
            this->c->body->ApplyLinearImpulse(impulse, this->c->body->GetWorldPoint(b2Vec2(0,.25f)));

            v *= -15.f;
            G->lock();
            G->emit(bullet, this->c, v+this->c->get_body(0)->GetWorldVector(this->c->last_ground_speed));
            G->play_sound(SND_SHOTGUN_SHOOT, p.x, p.y, 0, 1.f);
            G->unlock();

        }
        this->cooldown_timer = this->cooldown * this->c->cooldown_multiplier;
        this->cooldown_timer += add_cooldown;
        this->played_reload_sound = false;

        this->on_attack();
    }
}

void
robot_parts::shotgun::step()
{
    weapon::step();

    if (!this->played_reload_sound) {
        float cd_left = this->get_cooldown_fraction();
        if (cd_left > 0.f && cd_left < 0.5f) {
            b2Vec2 p = this->c->get_position();
            G->play_sound(SND_SHOTGUN_COCK, p.x, p.y, 0, 1.f);
            this->played_reload_sound = true;
        }
    }
}

robot_parts::rocket_launcher::rocket_launcher(creature *c)
    : weapon(c)
{
    this->terror = 0.2f;
    //this->max_range = 6.5f;
    this->max_range = 17.5f;
    this->cooldown = 750 * 1000;
    this->played_reload_sound = true;
    this->fired = false;
    this->missile_switch = true;

    this->set_material(&m_weapon);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROCKET_LAUNCHER));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    for (int i=0; i<ROCKET_LAUNCHER_MISSILES; i++) {
        tms_entity_init(&this->inner[i]);
        tms_entity_set_mesh(&this->inner[i], mesh_factory::get_mesh(MODEL_MISSILE));
        tms_entity_set_material(&this->inner[i], &m_weapon); /* TODO: other material? */
        tms_entity_set_uniform4f(&this->inner[i], "~color", .2f, .2f, .2f, 1.f);

        tms_entity_add_child(this, &this->inner[i]);
    }
}

void
robot_parts::rocket_launcher::attack(int add_cooldown/*=0*/)
{
    if (this->cooldown_timer <= 0) {
        //float angle = this->c->get_angle() + M_PI*1.5f + this->c->look_dir*this->get_arm_angle() * M_PI + (0.15f-((rand()%100)/100.f) * 0.3f);
        float angle = this->c->get_angle() + M_PI*1.5f + this->c->look_dir*this->get_arm_angle() * M_PI;
        b2Vec2 p = this->c->local_to_world(ROBOT_ARM_POS, 0);

        item *bullet = of::create_item(ITEM_ROCKET);
        bullet->set_scale(this->c->get_scale());
        G->timed_absorb(bullet, 10.0);

        b2Vec2 v;
        tmath_sincos(angle, &v.y, &v.x);

        bullet->_pos = p + .75f*v;
        bullet->_angle = angle;
        bullet->set_layer(this->c->get_layer());

        v *= -1.0f;
        b2Vec2 impulse = v;
        impulse *= 1.0f;

        this->c->body->ApplyLinearImpulse(impulse, this->c->body->GetWorldPoint(b2Vec2(0,.25f)));

        v *= -ROCKET_VELOCITY;
        G->lock();
        G->emit(bullet, this->c, v+this->c->get_body(0)->GetWorldVector(this->c->last_ground_speed));
        G->play_sound(SND_ROCKET_LAUNCHER_SHOOT, p.x, p.y, 0, 1.f);
        G->unlock();

        this->cooldown_timer = this->cooldown * this->c->cooldown_multiplier;
        this->cooldown_timer += add_cooldown;
        this->played_reload_sound = false;
        this->fired = true;
        this->missile_switch = !this->missile_switch;

        this->on_attack();
    }
}

void
robot_parts::rocket_launcher::step()
{
    weapon::step();

    if (!this->played_reload_sound) {
        float cd_left = this->get_cooldown_fraction();
        if (cd_left > 0.f && cd_left < 0.5f) {
            b2Vec2 p = this->c->get_position();
            this->played_reload_sound = true;
        }
    }
}

void robot_parts::rocket_launcher::update()
{
    weapon::update();
    int a = this->missile_switch ? 0 : 1;
    int b = this->missile_switch ? 1 : 0;

    for (int i=0; i<ROCKET_LAUNCHER_MISSILES; i++) {
        tmat4_copy(this->inner[i].M, this->M);
        tmat3_copy(this->inner[i].N, this->N);
    }

    if (fired) {
        float cd_left = this->get_cooldown_fraction();

        if (cd_left > 0.f) {
            tmat4_translate(this->inner[b].M, -.425f, .5f - 0.525f * (1 - cd_left), -.1f);
            tmat4_translate(this->inner[a].M, -.425f, -.025f, -.1f + .17f * (1 - cd_left));
        } else {
            fired = false;

            tmat4_translate(this->inner[a].M, -.425f, -.025f, -.1f);
            tmat4_translate(this->inner[b].M, -.425f, -.025f, .07f);
        }
    } else {
        tmat4_translate(this->inner[a].M, -.425f, -.025f, -.1f);
        tmat4_translate(this->inner[b].M, -.425f, -.025f, .07f);
    }

    tmat4_rotate(this->inner[0].M, -90, 0, 0, -1);
    tmat4_rotate(this->inner[1].M, -90, 0, 0, -1);
    tmat3_copy_mat4_sub3x3(this->inner[0].N, this->inner[0].M);
    tmat3_copy_mat4_sub3x3(this->inner[1].N, this->inner[1].M);

    //tmat4_translate(this->inner[0].M, -.425f, -.025f, -.1f); // in lower chamber
    //tmat4_translate(this->inner[0].M, -.425f, -.025f, .07f); // in upper chamber
    //tmat4_translate(this->inner[0].M, -.425f, .5f, -.1f); // behind lower chamber (in ammo box)
}

robot_parts::railgun::railgun(creature *c)
    : weapon(c)
{
    this->terror = 0.05f;
    this->do_update_effects = true;

    this->cooldown = 1000 * 1000;
    this->max_range = 20.f;
    this->active = 0.f;
    this->num_points = 0;
    this->handler = new robot_parts::railgun::cb_handler(this);
    this->fired = false;

    this->set_material(&m_colored);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ARM_RAILGUN));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);
}

void
robot_parts::railgun::step()
{
    weapon::step();

    if (!this->c) return;

    if (this->num_points > 0) {
        this->active -= .04f * G->get_time_mul();
    }

    if (this->fired) {
        float _x = RAILGUN_REACH;
        float _y = 0;

        float a = this->c->get_angle() + M_PI*1.5f + this->c->look_dir*this->get_arm_angle() * M_PI;
        float sn,cs;

        tmath_sincos(a, &sn, &cs);
        b2Vec2 pt1 = this->c->local_to_world(ROBOT_ARM_POS, 0)/* + this->c->look_dir * b2Vec2(-.01f * sn, .5f * cs)*/;
        b2Vec2 pt2;

        pt2.x = _x*cs - _y*sn;
        pt2.y = _x*sn + _y*cs;
        pt2 += pt1;

        b2Vec2 dir = pt2-pt1;
        dir *= 1.f/(dir.Length());
        this->from = pt1;
        this->num_points = 0;
        this->reflected = false;

        while (this->num_points < RAILGUN_MAX_POINTS) {
            this->result_fx = 0;

            W->b2->RayCast(this->handler, pt1, pt2);

            if (result_fx) {
                p_entity *e = static_cast<p_entity*>(result_fx->GetUserData());

                this->points[num_points] = result_pt;
                this->num_points ++;

                if (e) {
                    if (e->g_id == O_LASER_BOUNCER) {

                        b2Vec2 reflection = dir - b2Dot(dir, result_nor)*2.f*result_nor;

                        pt1 = result_pt;
                        pt2 = pt1 + RAILGUN_REACH * reflection;
                        dir = reflection;
                        this->reflected = true;
                        continue;
                    }

                    if (W->is_playing()) {
                        float dmg = 50.f * this->c->get_attack_damage_modifier();

                        if (e->is_creature()) {
                            ((creature*)e)->damage(dmg, this->result_fx, DAMAGE_TYPE_PLASMA, DAMAGE_SOURCE_BULLET, this->c->id);
                        } else if (e->g_id == O_LAND_MINE || e->g_id == O_BOMB) {
                            ((explosive*)e)->damage(dmg);
                        }
                    }
                }
            } else {
                this->points[num_points] = pt2;
                this->num_points ++;
            }

            break;
        }
        this->fired = false;
    }

}

void
robot_parts::railgun::attack(int add_cooldown/*=0*/)
{
    if (this->cooldown_timer <= 0) {
        tms_debugf("attacked!");
        this->active = 1.25f;
        this->fired = true;
        this->last_z = (this->c->i_dir > 0 ? .45f : -.45f)  + this->c->get_layer();
        //this->from = this->c->local_to_world()get_position();
        //this->active = true;

        b2Vec2 p = this->c->get_position();
        G->play_sound(SND_RAILGUN_SHOOT, p.x, p.y, 0, 1.f);

        this->cooldown_timer = this->cooldown * this->c->cooldown_multiplier;
        this->cooldown_timer += add_cooldown;

        this->on_attack();
    }
}

void
robot_parts::railgun::update_effects()
{
    if (!this->c) return;
    if (this->active < 0.0001f) {
        this->num_points = 0;
        return;
    }

    //float t = powf(this->active, 4.);
    float t = powf(this->active, .75f);//, .75);
    float z = this->c->get_layer() * LAYER_DEPTH;
    float a = tclampf(t, 0.f, 1.f);

    //b2Vec2 last = this->local_to_world(b2Vec2(0.f, -.35f), 0);
    //b2Vec2 last = this->c->get_position();
    b2Vec2 last = this->from;

    tvec3 rgb = (tvec3){1.f, 0.0f, 0.7f};

    for (int x=0; x<this->num_points; x++) {
        linebuffer::add(
                last.x, last.y, this->last_z,
                this->points[x].x, this->points[x].y, this->last_z,
                rgb.r*3.f+1.f, rgb.g*3.f+1.f, rgb.b*3.f+1.f, a,
                rgb.r*3.f+1.f, rgb.g*3.f+1.f, rgb.b*3.f+1.f, a,//*a*.25f,
                .1f, .1f);

        last = this->points[x];
    }

    spritebuffer::add(last.x, last.y, this->last_z, 1.f, 1.f, 1.f, a, .3f, .3f, 1, cos((double)(_tms.last_time + rand()%100000)/100000.) * .25f);
}

float32
robot_parts::railgun::cb_handler::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    p_entity *r = static_cast<p_entity*>(f->GetUserData());

    if (f->IsSensor()) {
        return -1.f;
    }

    if (!world::fixture_in_layer(f, self->c->get_layer(), 6)) {
        return -1.f;
    }

    if (r) {
        if (this->self->reflected == false && this->self->c == r) return -1.f;

        if (r->is_creature()) {
            creature *c = static_cast<creature*>(r);

            if (c->is_foot_fixture(f)) {
                return -1.f;
            }
        }
    }

    self->result_nor = nor;
    self->result_pt = pt;
    self->result_fx = f;

    return fraction;
}

robot_parts::weapon*
robot_parts::weapon::make(int weapon_id, creature *c)
{
    robot_parts::weapon *w;

    switch (weapon_id) {
        case WEAPON_ARM_CANNON: w = new robot_parts::arm_cannon(c); break;
        case WEAPON_SHOTGUN: w = new robot_parts::shotgun(c); break;
        case WEAPON_RAILGUN: w = new robot_parts::railgun(c); break;
        case WEAPON_ROCKET: w = new robot_parts::rocket_launcher(c); break;
        case WEAPON_BOMBER: w = new robot_parts::bomber(c); break;
        case WEAPON_TESLA: w = new robot_parts::teslagun(c); break;
        case WEAPON_PLASMAGUN: w = new robot_parts::plasmagun(c); break;
        case WEAPON_MEGABUSTER: w = new robot_parts::megabuster(c); break;
        case WEAPON_TRAINING_SWORD: w = new robot_parts::training_sword(c); break;
        case WEAPON_WAR_HAMMER: w = new robot_parts::war_hammer(c); break;
        case WEAPON_SIMPLE_AXE: w = new robot_parts::simple_axe(c); break;
        case WEAPON_CHAINSAW: w = new robot_parts::chainsaw(c); break;
        case WEAPON_SPIKED_CLUB: w = new robot_parts::spiked_club(c); break;
        case WEAPON_STEEL_SWORD: w = new robot_parts::steel_sword(c); break;
        case WEAPON_BASEBALLBAT: w = new robot_parts::baseballbat(c); break;
        case WEAPON_SPEAR: w = new robot_parts::spear(c); break;
        case WEAPON_WAR_AXE: w = new robot_parts::war_axe(c); break;
        case WEAPON_PIXEL_SWORD: w = new robot_parts::pixel_sword(c); break;
        case WEAPON_SERPENT_SWORD: w = new robot_parts::serpent_sword(c); break;
        case WEAPON_PICKAXE: w = new robot_parts::pickaxe(c); break;
        default:
            tms_errorf("unhandled weapon %d", weapon_id);
            return 0;
    }

    return w;
}

robot_parts::tool*
robot_parts::tool::make(int tool_id, creature *c)
{
    robot_parts::tool *t;

    switch (tool_id) {
        case TOOL_BUILDER:  t = new robot_parts::builder(c); break;
        case TOOL_ZAPPER:   t = new robot_parts::miner(c); break;
        case TOOL_TIMECTRL: t = new robot_parts::timectrl(c); break;
        case TOOL_DEBUGGER: t = new robot_parts::debugger(c); break;
        case TOOL_PAINTER:  t = new robot_parts::painter(c); break;
        case TOOL_FACTION_WAND: t = new robot_parts::faction_wand(c); break;
        case TOOL_COMPRESSOR:   t = new robot_parts::compressor(c); break;
        default:
            tms_errorf("Unhandled tool id %d", tool_id);
            return 0;
    }

    return t;
}

robot_parts::tool::tool(creature *c)
    : arm(c)
{
    adventure::bars[BAR_TOOL_CD].color = (tvec3){.7f, .7f, .7f};
}

void
robot_parts::tool::step()
{
    if (this->cooldown_timer > 0) {
        this->cooldown_timer -= G->timemul(WORLD_STEP) * this->c->cooldown_multiplier;
        float a = (float)this->cooldown_timer;
        float b = (float)this->cooldown;
        float v = a/b;
        if (this->c && this->c->is_player()) {
            adventure::bars[BAR_TOOL_CD].value = v;
            adventure::bars[BAR_TOOL_CD].time = 2.f;
        }
    }
}

robot_parts::nulltool::nulltool(creature *c)
    : tool(c)
{
    this->set_material(&m_robot_arm);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_DRAGARM));
    this->set_uniform("~color", .9f, .9f, .9f, 1.f);
}

int
robot_parts::nulltool::action(uint32_t type, uint64_t pointer_id, tvec2 pos)
{
    return EVENT_CONT;
}

robot_parts::builder::builder(creature *c)
    : tool(c)
{
    this->set_material(&m_weapon_nospecular);
    this->set_mesh(mesh_factory::get_mesh(MODEL_BUILDER));
    this->set_uniform("~color", .9f, .9f, .9f, 1.f);
}

int
robot_parts::builder::action(uint32_t type, uint64_t pointer_id, tvec2 pos)
{
    /* due to general spaghetti, the code is still in game.cc */
    return EVENT_CONT;
}

void
robot_parts::builder::stop()
{
    if (this->c->is_player()) {
        G->drop_interacting();
    }
    tool::stop();
}

robot_parts::faction_wand::faction_wand(creature *c)
    : tool(c)
{
    this->controlling_id = 0;
    this->effect = 0;

    this->set_material(&m_item_shiny);
    this->set_mesh(mesh_factory::get_mesh(MODEL_FACTIONWAND));

    this->do_update_effects = true;
}

void
robot_parts::faction_wand::update_effects()
{
    if (!W->is_paused() && this->controlling_id != 0) {
        p_entity *e = W->get_entity_by_id(this->controlling_id);

        if (e) {
            if (this->effect) {
                this->effect->update_pos(e->get_position(), e->get_layer()+1);
            }
        } else {
            if (this->effect) {
                this->effect->continuous = false;
                this->effect = 0;
            }
        }
    } else if (this->effect) {
        this->effect->continuous = false;
        this->effect = 0;
    }
}

void
robot_parts::faction_wand::stop()
{
    tms_warnf("faction_wand::stop needs implementing!");
}

int
robot_parts::faction_wand::action(uint32_t type, uint64_t pointer_id, tvec2 pos)
{
#ifdef TMS_BACKEND_PC
    if (pointer_id != 0) {
        return EVENT_CONT;
    }
#endif

    switch (type) {
        case TMS_EV_POINTER_DOWN:
            {
                tvec3 tproj;
                W->get_layer_point(G->cam, (int)pos.x, (int)pos.y, this->c->get_layer(), &tproj);

                p_entity *e = 0;
                b2Body *b;
                tvec2 offs;
                uint8_t frame;
                b2Fixture *fx = 0;
                W->query(G->cam, (int)pos.x, (int)pos.y, &e, &b, &offs, &frame, G->layer_vis, false, &fx);
                /* Check if we're trying to set a new control target */
                if (e && tvec2_dist(tvec2b(this->c->get_position()), tvec2b(e->get_position())) < 2.f && e->is_robot() && this->c->is_friend(e)) {
                    creature *control = static_cast<creature*>(e);

                    /*if (!control->is_roaming()) {
                        control->roam = true;
                    }*/
                    this->controlling_id = e->id;
                    if (!this->effect) {
                        tms_debugf("emitted effect!");
                        this->effect = new magic_effect(e->get_position(), e->get_layer(), 10);
                        this->effect->continuous = true;
                        this->effect->color = tvec4f(0.5f, 1.f, 1.f, 1.f);
                        this->effect->speed_mod = 2.f;
                        G->emit(this->effect);
                    }
                } else if (this->controlling_id) {
                    creature *control = static_cast<creature*>(W->get_entity_by_id(this->controlling_id));

                    if (control) {
                        tms_debugf("set target to something!");

                        if (e && e->g_id != O_CHUNK) {
                            if (e != control) {
                                control->roam_unset_target();
                                control->roam_target_id = e->id;

                                magic_effect *f = new magic_effect(e->get_position(), e->get_layer(), 10);
                                f->color = tvec4f(1.0f, 0.2f, 0.2f, 1.f);
                                f->speed_mod = 1.f;
                                G->emit(f);
                            }
                        } else {
                            control->roam_unset_target();

                            control->roam_target_type = TARGET_POSITION;
                            control->roam_target_pos.x = tproj.x;
                            control->roam_target_pos.y = tproj.y;
                            tms_debugf("set target to pos!");

                            magic_effect *f = new magic_effect(control->roam_target_pos, 1, 10);
                            f->color = tvec4f(0.2f, 1.0f, 0.2f, 1.f);
                            f->speed_mod = 1.f;
                            G->emit(f);
                        }
                    }
                }
            }
            break;
    }

    return EVENT_CONT;
}

struct tms_mesh **compressor_light_meshes[COMPRESSOR_NUM_ITEMS] = {
    &mesh_factory::models[MODEL_COMPRESSOR_LAMP1].mesh,
    &mesh_factory::models[MODEL_COMPRESSOR_LAMP2].mesh,
    &mesh_factory::models[MODEL_COMPRESSOR_LAMP3].mesh,
    &mesh_factory::models[MODEL_COMPRESSOR_LAMP4].mesh,
};

robot_parts::compressor::compressor(creature *c)
    : tool(c)
    , emit_timer(0)
    , item_index(0)
{
    this->do_update_effects = true;

    this->set_material(&m_item);
    this->set_mesh(mesh_factory::get_mesh(MODEL_COMPRESSOR));

    memset(this->storage, 0, sizeof(this->storage));

    for (int x=0; x<COMPRESSOR_NUM_ITEMS; ++x) {
        struct tms_entity &light = this->lights[x];

        tms_entity_init(&light);
        tms_entity_set_mesh(&light, *compressor_light_meshes[x]);
        tms_entity_set_material(&light, &m_pv_rgba);
        tms_entity_set_uniform4f(&light, "~color", 1.f, 1.f, 1.f, 1.f);

        tms_entity_add_child(this, &light);
    }
}

int
robot_parts::compressor::action(uint32_t type, uint64_t pointer_id, tvec2 pos)
{
#ifdef TMS_BACKEND_PC
    if (pointer_id != 0) {
        return EVENT_CONT;
    }
#endif

    switch (type) {
        case TMS_EV_POINTER_DOWN:
            this->emit_timer = 0;
            this->active = true;
        case TMS_EV_POINTER_DRAG:
            {
                if (this->active) {
                    tvec3 tproj;
                    W->get_layer_point(G->cam, (int)pos.x, (int)pos.y, this->c->get_layer(), &tproj);

                    b2Vec2 creature_pos = this->c->get_position();

                    b2Vec2 oo = b2Vec2(tproj.x, tproj.y);
                    oo -= creature_pos;

                    float a = atan2f(oo.y, oo.x);

                    a -= this->c->get_angle();

                    this->set_arm_angle(a);

                    p_entity *e = 0;
                    b2Body *b;
                    tvec2 offs;
                    uint8_t frame;
                    b2Fixture *fx = 0;

                    W->query(G->cam, (int)pos.x, (int)pos.y, &e, &b, &offs, &frame, G->layer_vis, false, &fx);

                    if (e && b2Distance(creature_pos, e->get_position()) <= 3.f && e->is_compressable() && item_index < COMPRESSOR_NUM_ITEMS) {
                        b2Body *b = e->get_body(0);
                        if (b) {
                            float x = -3.f * oo.x;
                            float y = -3.f * oo.y;
                            b->SetLinearVelocity(b2Vec2(x, y) + this->c->get_body(0)->GetLinearVelocity());
                        }

                        float sn,cs;
                        tmath_sincos(this->c->get_angle() + M_PI*1.5f + this->c->look_dir*this->get_arm_angle() * M_PI, &sn, &cs);
                        b2Vec2 absorb_point = b2Vec2(0.f+cs*.5f, .33f+sn*.5f);

                        G->lock();
                        if (G->absorb(e, false, this->c, absorb_point, 0)) {

                            G->finished_tt(TUTORIAL_PICKUP_EQUIPMENT);
                            G->close_tt(TUTORIAL_TEXT_PICKUP_EQUIPMENT);

                            G->play_sound(SND_COMPRESSOR, tproj.x, tproj.y, 0, 1.f);
                            struct compressor_item &ci = this->storage[this->item_index ++];

                            ci.g_id = e->g_id;
                            ci.sub_id = e->get_sub_id();

                            if (this->c->is_robot()) {
                                robot_base *rob = static_cast<robot_base*>(c);
                                rob->consume_timer = 1.f;
                            }
                        }
                        this->emit_timer = 0;
                        G->unlock();

                        this->active = false;
                    }
                }

                if (this->active) {
                    if (type == TMS_EV_POINTER_DRAG) {
                        return EVENT_DONE;
                    }
                }
            }
            break;

        case TMS_EV_POINTER_UP:
            {
                this->active = false;
            }
            break;
    }

    return EVENT_CONT;
}

void
robot_parts::compressor::step()
{
    if (this->active) {
        if (this->item_index == 0) {
            this->active = false;
            return;
        }

        this->emit_timer += G->timemul(WORLD_STEP);

        float time = 0.3f;

        if (this->emit_timer >= COMPRESSOR_EMIT_TIME) {
            time = 0.f;
            this->active = false;

            this->emit_timer = 0;
            tms_infof("EMIT!!!!!!!");

            struct compressor_item &ci = this->storage[-- this->item_index];

            p_entity *result = 0;

            switch (ci.g_id) {
                case O_ITEM:
                    {
                        item *i = static_cast<item*>(of::create(O_ITEM));
                        i->set_item_type(ci.sub_id);

                        result = i;
                    }
                    break;

                default:
                    tms_errorf("Unhandled compressor item emit: %u - %u",
                            ci.g_id, ci.sub_id);
                    break;
            }

            if (result) {
                float a = this->c->get_angle() + M_PI*1.5f + this->c->look_dir*this->get_arm_angle() * M_PI;
                float sn,cs;
                tmath_sincos(a, &sn, &cs);

                b2Vec2 _v = b2Vec2(cs, sn);

                b2Vec2 pos = this->c->local_to_world(b2Vec2(this->c->look_dir*1.f+cs*.5f, .25f+sn*.6f), 0);
                result->set_position(pos);
                result->set_layer(this->c->get_layer());
                G->lock();
                G->play_sound(SND_COMPRESSOR_REVERSE, pos.x, pos.y, 0, 1.f);
                G->emit(result, this->c, 4.f*_v + this->c->get_body(0)->GetLinearVelocity());
                G->unlock();
            }

            ci.g_id = 0;
            ci.sub_id = 0;
        }

        G->add_hp(this->c, (float)this->emit_timer / COMPRESSOR_EMIT_TIME, TV_HP_COMPRESSOR, time);
    }
}

void
robot_parts::compressor::stop()
{
    this->active = false;
    this->emit_timer = 0;
}

void
robot_parts::compressor::update()
{
    tool::update();

    for (int x=0; x<COMPRESSOR_NUM_ITEMS; ++x) {
        struct tms_entity &light = this->lights[x];

        tmat4_copy(light.M, this->M);
        tmat3_copy(light.N, this->N);

        float hl = .5f;
        float r = .1f;

        hl += r + sin((double)_tms.last_time * .000004)*r;

        if (this->item_index > x) {
            tms_entity_set_uniform4f(&light, "~color", 0.f+hl, .5f+1.f*hl, 0.f+hl, 1.75f);
        } else {
            tms_entity_set_uniform4f(&light, "~color", 0.f+hl, 0.f+hl, 0.f+hl, 1.75f);
        }
    }
}

void
robot_parts::compressor::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    lb->ensure(sizeof(uint32_t)*2*COMPRESSOR_NUM_ITEMS);

    for (int x=0; x<COMPRESSOR_NUM_ITEMS; ++x) {
        const struct compressor_item &ci = this->storage[x];
        lb->w_uint32(ci.g_id);
        lb->w_uint32(ci.sub_id);
    }
}

void
robot_parts::compressor::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    for (int x=0; x<COMPRESSOR_NUM_ITEMS; ++x) {
        this->storage[x].g_id = lb->r_uint32();
        this->storage[x].sub_id = lb->r_uint32();
    }
}

robot_parts::miner::miner(creature *c)
    : tool(c)
{
    this->set_material(&m_weapon_nospecular);
    this->set_mesh(mesh_factory::get_mesh(MODEL_MINER));
    this->set_damage(2.f);
}

void
robot_parts::miner::step()
{
    if (adventure::mining) {
        tvec3 tproj;
        W->get_layer_point(G->cam, adventure::last_mouse_x, adventure::last_mouse_y, this->c->get_layer(), &tproj);
        adventure::update_mining_pos(tproj.x, tproj.y);
    }
}

void
robot_parts::miner::set_damage(float new_damage)
{
    if (new_damage > MINER_MAX_DAMAGE) new_damage = MINER_MAX_DAMAGE;
    this->damage = new_damage;

    float m = this->damage / MINER_MAX_DAMAGE;
    this->set_uniform("~color",
                      0.3f + MINER_BASE_R * m,
                      0.3f + MINER_BASE_G * m,
                      0.3f + MINER_BASE_B * m,
                      MINER_BASE_A);
}

void
robot_parts::miner::stop()
{
    if (this->c->id == G->state.adventure_id) {
        for (int x=0; x<MAX_P; x++) {
            G->mining[x] = false;
        }
        adventure::end_mining();
    }

    tool::stop();
}

int
robot_parts::miner::action(uint32_t type, uint64_t pointer_id, tvec2 pos)
{
    switch (type) {
        case TMS_EV_POINTER_DOWN:
            {
                p_entity *e = 0;
                b2Body *b;
                tvec2 offs;
                uint8_t frame;
                b2Fixture *fx = 0;
                W->query(G->cam, (int)pos.x, (int)pos.y, &e, &b, &offs, &frame, G->layer_vis, false, &fx);

                if (!e) {
                    return EVENT_CONT;
                }

                /*
                if (e->get_layer() != this->c->get_layer()) {
                    return EVENT_CONT;
                }
                */

                if (e->g_id == O_TPIXEL
                        || e->g_id == O_CHUNK
                        || e->g_id == O_PLANT
                        || e->g_id == O_ITEM
                        || e->is_creature()
                        || e->is_zappable()
                   ) {
                    tvec3 tproj;
                    W->get_layer_point(G->cam, (int)pos.x, (int)pos.y, this->c->get_layer(), &tproj);
                    adventure::update_mining_pos(tproj.x, tproj.y);
                    adventure::begin_mining();
                    G->mining[pointer_id] = true;
                    tms_debugf("set mining to true");
                    return EVENT_DONE;
                }
            }
            break;

        case TMS_EV_POINTER_DRAG:
            {
                if (G->mining[pointer_id]) {
                    tvec3 tproj;
                    W->get_layer_point(G->cam, (int)pos.x, (int)pos.y, this->c->get_layer(), &tproj);
                    adventure::update_mining_pos(tproj.x, tproj.y);

                    return EVENT_DONE;
                }
            }
            break;

        case TMS_EV_POINTER_UP:
            {
                if (G->mining[pointer_id]) {
                    G->mining[pointer_id] = false;
                    adventure::end_mining();
                    return EVENT_DONE;
                }
            }
            break;
    }
    return EVENT_CONT;
}

robot_parts::timectrl::timectrl(creature *c)
    : tool(c)
{
    this->is_down = false;
    this->chargeup_timer = 0;
    this->fire_timer = 0;
    this->cooldown = TIMECTRL_COOLDOWN;

    this->set_material(&m_robot_arm);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_DRAGARM));
    this->set_uniform("~color", .1f, .1f, .9f, 1.f);
}

int
robot_parts::timectrl::action(uint32_t type, uint64_t pointer_id, tvec2 pos)
{
    switch (type) {
        case TMS_EV_POINTER_DOWN:
            if (!this->is_down && this->cooldown_timer <= 0) {
                this->is_down = true;
                this->pointer_id = pointer_id;
                tms_debugf("Time ctrl on pointer down. pid: %" PRIu64, pointer_id);
                this->chargeup_timer = TIMECTRL_CHARGEUP_TIME;

                adventure::bars[BAR_TOOL].color = (tvec3){.05f, .7f, .7f};
                adventure::bars[BAR_TOOL].value = 0.f;
                adventure::bars[BAR_TOOL].time = 2.f;
                return EVENT_DONE;
            }
            break;

        case TMS_EV_POINTER_DRAG:
            if (this->is_down && this->pointer_id == pointer_id) {
                return EVENT_DONE;
            }
            break;

        case TMS_EV_POINTER_UP:
            if (this->is_down && this->pointer_id == pointer_id) {
                tms_debugf("pointer up timectrl");
                this->is_down = false;
                return EVENT_DONE;
            }
            break;
    }

    return EVENT_CONT;
}

#define MAX_FIRE_TIMER 3*1000*1000

void
robot_parts::timectrl::step()
{
    tool::step();

    if (this->is_down && this->chargeup_timer > 0) {
        this->chargeup_timer -= G->timemul(WORLD_STEP);

        float a = (float)this->chargeup_timer;
        float b = (float)TIMECTRL_CHARGEUP_TIME;
        float v = 1.f-a/b;

        adventure::bars[BAR_TOOL].value = v;
        adventure::bars[BAR_TOOL].time = 3.f;
    }

    if (this->is_down && this->chargeup_timer <= 0 && !this->active) {
        this->fire_timer = MAX_FIRE_TIMER;
        //this->is_down = false;
        //robot_base::effect e(EFFECT_TYPE_CD_REDUCTION, EFFECT_METHOD_ADDITIVE, 2.f, 1250*1000);
        creature_effect e(EFFECT_TYPE_CD_REDUCTION, EFFECT_METHOD_MULTIPLICATIVE, 3.f, 1250*1000);
        this->c->apply_effect(EFFECT_TYPE_CD_REDUCTION, e);
        this->active = true;
        this->cooldown_timer = this->cooldown;

        adventure::bars[BAR_TOOL].color = (tvec3){.5f, .7f, .05f};
        adventure::bars[BAR_TOOL].time = 3.f;
    }

    if (this->fire_timer > 0) {
        float a = (float)this->fire_timer;
        float b = (float)MAX_FIRE_TIMER;
        float v = sqrtf(a/b) * .9f;

        adventure::bars[BAR_TOOL].value = v;
        adventure::bars[BAR_TOOL].time = 3.f;

        G->state.time_mul = v;
        this->fire_timer -= G->timemul(WORLD_STEP);
    } else {
        this->active = false;
    }
}

robot_parts::debugger::debugger(creature *c)
    : tool(c)
{
    this->cooldown = TIMECTRL_COOLDOWN;
    this->edev = 0;

    this->set_material(&m_robot_arm);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_DRAGARM));
    this->set_uniform("~color", .9f, .1f, .9f, 1.f);
}

int
robot_parts::debugger::action(uint32_t type, uint64_t pointer_id, tvec2 pos)
{
    switch (type) {
        case TMS_EV_POINTER_DOWN:
            {
                p_entity *e = 0;
                b2Body *b;
                tvec2 offs;
                uint8_t frame;
                W->query(G->cam, (int)pos.x, (int)pos.y, &e, &b, &offs, &frame, G->layer_vis);
                if (e && e->flag_active(ENTITY_IS_EDEVICE)) {
                    this->edev = e;
                    tms_debugf("clicked on edevice %p", e);

                    // point arm at edevice
                    b2Vec2 oo = e->get_position() - this->c->get_position();
                    float a = atan2f(oo.y, oo.x) - this->c->get_angle();
                    this->set_arm_angle(a);
                    this->pointer_id = pointer_id;

                    return EVENT_DONE;
                }
            }
            break;

        case TMS_EV_POINTER_DRAG:
            {
                // don't allow camera movements if we pressed down on an edevice
                if (this->edev) {
                    return EVENT_DONE;
                }
            }
            break;

        case TMS_EV_POINTER_UP:
            {
                if (pointer_id == this->pointer_id) {
                    // reset arm angle
                    // this->arm_angle = 0.f;

                    if (this->edev) {
                        edevice *ed = this->edev->get_edevice();
                        G->ss_edev = ed;
                        tms_debugf("show connection dialog");
                        G->set_mode(GAME_MODE_SELECT_SOCKET);
                    }
                    this->edev = 0;
                }
            }
            break;
    }
    return EVENT_CONT;
}

void
robot_parts::debugger::step()
{
}

robot_parts::painter::painter(creature *c)
    : tool(c)
{
//    this->set_flag(ENTITY_HAS_CONFIG, true); // XXX: Should this be replaced with in-game config?

    this->painting = false;

    /*
    this->set_num_properties(3);
    this->properties[0].type = P_FLT;
    this->properties[1].type = P_FLT;
    this->properties[2].type = P_FLT;
    this->properties[0].v.f = 0.9f; // red
    this->properties[1].v.f = 0.1f; // green
    this->properties[2].v.f = 0.3f; // blue
    */

    this->set_material(&m_robot_arm);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_DRAGARM));
    this->set_uniform("~color", .9f, .1f, .3f, 1.f);
}

int
robot_parts::painter::action(uint32_t type, uint64_t pointer_id, tvec2 pos)
{
    switch (type) {
        case TMS_EV_POINTER_DOWN:
        case TMS_EV_POINTER_DRAG:
            {
                // don't allow camera movements if we are painting
                if (this->painting || type == TMS_EV_POINTER_DOWN) {
                    p_entity *e = 0;
                    b2Body *b;
                    tvec2 offs;
                    uint8_t frame;
                    W->query(G->cam, (int)pos.x, (int)pos.y, &e, &b, &offs, &frame, G->layer_vis);
                    if (e) {
                        // point arm at entity
                        b2Vec2 oo = e->get_position() - this->c->get_position();
                        float a = atan2f(oo.y, oo.x) - this->c->get_angle();
                        this->set_arm_angle(a);
                        this->pointer_id = pointer_id;
                        this->painting = true;

                        //e->set_color4(this->properties[0].v.f, this->properties[1].v.f, this->properties[2].v.f);
                    }

                    return EVENT_DONE;
                }
            }
            break;

        case TMS_EV_POINTER_UP:
            {
                if (pointer_id == this->pointer_id) {
                    // reset arm angle
                    this->arm_angle = 0.f;

                    this->painting = false;
                }
            }
            break;
    }
    return EVENT_CONT;
}

void
robot_parts::painter::update_appearance()
{
    /*
    float r = this->properties[0].v.f;
    float g = this->properties[1].v.f;
    float b = this->properties[2].v.f;

    this->set_uniform("~color", r, g, b, 1.f);
    */
}

void
robot_parts::arm::set_arm_angle(float a, float speed)
{
    a = creature::real_arm_angle(this->c, a);

    this->arm_angle = this->arm_angle*(1.f-speed) + a*speed;
}

void
robot_parts::arm::set_arm_angle_raw(float a)
{
    this->arm_angle = a;
}

robot_parts::arm::arm(creature *c)
{
    this->do_update_effects = false;
    this->used = false;
    this->c = c;
    this->arm_angle = 0.f;
    this->arm_fold = 0.f;
    this->cooldown = 0;
    this->pointer_id = 0;
    this->cooldown_timer = 0;
    this->active = false;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

uint32_t
robot_parts::arm::get_item_id() const
{
    switch (this->get_arm_category()) {
        case ARM_WEAPON: return _weapon_to_item[this->get_arm_type()];
        case ARM_TOOL: return _tool_to_item[this->get_arm_type()];
        default:
            tms_errorf("unhandled arm type: %d", this->get_arm_type());
            break;
    }

    return 0;
}

robot_parts::bomber::bomber(creature *c)
    : weapon(c)
{
    this->terror = 0.1f;
    this->max_range = 9.f;
    this->cooldown = 750 * 1000;
    this->chamber_rotation = -BOMBER_CHAMBER_ROTATION;

    this->set_material(&m_weapon);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ARM_BOMBER));
    this->set_uniform("~color", .3f, .3f, .3f, 1.f);

    tms_entity_init(&this->chamber);
    tms_entity_set_mesh(&this->chamber, mesh_factory::get_mesh(MODEL_ARM_BOMBER_CHAMBER));
    tms_entity_set_material(&this->chamber, &m_weapon);
    tms_entity_set_uniform4f(&this->chamber, "~color", .3f, .3f, .3f, 1.f);

    tms_entity_add_child(this, &this->chamber);

    this->bomb_fired = 0;
}

static void on_bomb_absorbed(entity *self, void *userdata)
{
    if (self->flag_active(ENTITY_IS_ROBOT)) {
        robot_base *r = static_cast<robot_base*>(self);
        robot_parts::weapon *w = r->has_weapon(WEAPON_BOMBER);

        static_cast<robot_parts::bomber*>(w)->bomb_fired = 0;
    }
}

int
robot_parts::bomber::pre_attack()
{
    if (this->bomb_fired) {
        this->bomb_fired->triggered = true;
        this->bomb_fired = 0;

        return EVENT_DONE;
    }

    return EVENT_CONT;
}

void
robot_parts::bomber::attack(int add_cooldown/*=0*/)
{
    if (this->cooldown_timer <= 0) {
        float angle = this->c->get_angle() + M_PI*1.5f + this->c->look_dir*this->get_arm_angle() * M_PI;
        this->chamber_rotation = (this->chamber_rotation + BOMBER_CHAMBER_ROTATION) % 360;

        b2Vec2 _v;
        tmath_sincos(angle, &_v.y, &_v.x);
        _v *= .5f;

        explosive *bullet = new explosive(EXPLOSIVE_TRIGGER);
        bullet->set_layer(this->c->get_layer());
        this->c->subscribe(bullet, ENTITY_EVENT_REMOVE, &on_bomb_absorbed, bullet);
        bullet->_pos = this->c->local_to_world(b2Vec2(0, .125f), 0) + _v;

        b2Vec2 v;
        tmath_sincos(angle, &v.y, &v.x);
        v *= -0.5f;
        this->c->body->ApplyLinearImpulse(v, this->c->body->GetWorldPoint(b2Vec2(0,.25f)));

        b2Vec2 p = this->c->local_to_world(ROBOT_ARM_POS, 0);

        v *= -BULLET_VELOCITY;
        G->lock();
        G->emit(bullet, this->c, v+this->c->get_body(0)->GetWorldVector(this->c->last_ground_speed));
        G->play_sound(SND_ROBOT_BOMB, p.x, p.y, 0, 1.f);
        G->unlock();

        this->cooldown_timer = this->cooldown;
        this->cooldown_timer += add_cooldown;

        this->on_attack();

        this->bomb_fired = bullet;
    }
}

void
robot_parts::bomber::update()
{
    weapon::update();

    tmat4_copy(this->chamber.M, this->M);
    tmat3_copy(this->chamber.N, this->N);

    tmat4_translate(this->chamber.M, -.42737f, .47396, -.02307);

    float cd_left = this->get_cooldown_fraction();

    tmat4_rotate(this->chamber.M,
            this->chamber_rotation + BOMBER_CHAMBER_ROTATION * (cd_left > 0.f ? (1 - cd_left) : 1.f), 0, -1, 0);

    tmat3_copy_mat4_sub3x3(this->chamber.N, this->chamber.M);
}

robot_parts::equipment::equipment(creature *r)
{
    this->r = r;
    this->fx = 0;
    this->layer_mask = 15;

    this->update_method = ENTITY_UPDATE_CUSTOM;

    /* FIXME */
    /* fix what? */
    this->set_material(&m_colored);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_GUNARM));
    this->set_uniform("~color", ROBOT_COLOR, 1.f);
}

uint32_t
robot_parts::equipment::get_item_id()
{
    switch (this->get_equipment_category()) {
        case EQUIPMENT_BACK: return _back_to_item[this->get_equipment_type()];
        case EQUIPMENT_FRONT: return _front_to_item[this->get_equipment_type()];
        case EQUIPMENT_HEAD: return _head_to_item[this->get_equipment_type()];
        case EQUIPMENT_FEET: return _feet_to_item[this->get_equipment_type()];
        case EQUIPMENT_HEADWEAR: return _head_equipment_to_item[this->get_equipment_type()];
    }

    return 0;
}

robot_parts::equipment*
robot_parts::equipment::make(creature *c, int e_category, int e_type)
{
    equipment *h = 0;

    switch (e_category) {
        case EQUIPMENT_FEET:
            switch (e_type) {
                case FEET_BIPED: h = new robot_parts::feet(c); break;
                case FEET_MINIWHEELS: h = new robot_parts::miniwheels(c); break;
                case FEET_QUADRUPED: h = new robot_parts::quadruped(c); break;
                case FEET_MONOWHEEL: h = new robot_parts::monowheel(c); break;
                default:
                     tms_errorf("Unhandled feet: %d", e_type);
                case FEET_NULL:
                     return 0;
            }
            break;

        case EQUIPMENT_HEAD:
            switch (e_type) {
                case HEAD_ROBOT: h = new robot_parts::robot_head(c); break;
                case HEAD_ROBOT_UNCOVERED: h = new robot_parts::robot_head_inside(c); break;
                case HEAD_COW: h = new robot_parts::cow_head(c); break;
                case HEAD_PIG: h = new robot_parts::pig_head(c); break;
                case HEAD_OSTRICH: h = new robot_parts::ostrich_head(c); break;
                case HEAD_DUMMY: h = new robot_parts::dummy_head(c); break;
                default:
                     tms_errorf("Unhandled head: %d", e_type);
                case HEAD_NULL:
                     return 0;
            }
            break;

        case EQUIPMENT_FRONT:
            switch (e_type) {
                case FRONT_EQUIPMENT_ROBOT_FRONT: h = new robot_parts::robot_front(c); break;
                case FRONT_EQUIPMENT_BLACK_ROBOT_FRONT: h = new robot_parts::black_robot_front(c); break;
                case FRONT_EQUIPMENT_PIONEER_FRONT: h = new robot_parts::pioneer_front(c); break;
                default:
                    tms_errorf("unhandled front %d", e_type);
                case FRONT_EQUIPMENT_NULL:
                    return 0;
            }
            break;

        case EQUIPMENT_BACK:
            switch (e_type) {
                case BACK_EQUIPMENT_ROBOT_BACK: h = new robot_parts::robot_back(c); break;
                case BACK_EQUIPMENT_BLACK_ROBOT_BACK: h = new robot_parts::black_robot_back(c); break;
                case BACK_EQUIPMENT_JETPACK: h = new robot_parts::jetpack(c); break;
                case BACK_EQUIPMENT_UPGRADED_JETPACK: h = new robot_parts::upgraded_jetpack(c); break;
                case BACK_EQUIPMENT_ADVANCED_JETPACK: h = new robot_parts::advanced_jetpack(c); break;
                case BACK_EQUIPMENT_PIONEER_BACK: h = new robot_parts::pioneer_back(c); break;
                default:
                    tms_errorf("unhandled back %d", e_type);
                case BACK_EQUIPMENT_NULL:
                    return 0;
            }
            break;

        case EQUIPMENT_HEADWEAR:
            switch (e_type) {
                case HEAD_EQUIPMENT_NINJAHELMET: h = new robot_parts::ninjahelmet(c); break;
                case HEAD_EQUIPMENT_HEISENBERG: h = new robot_parts::heisenberghat(c); break;
                case HEAD_EQUIPMENT_WIZARDHAT: h = new robot_parts::wizardhat(c); break;
                case HEAD_EQUIPMENT_CONICALHAT: h = new robot_parts::conicalhat(c); break;
                case HEAD_EQUIPMENT_POLICEHAT: h = new robot_parts::policehat(c); break;
                case HEAD_EQUIPMENT_TOPHAT: h = new robot_parts::tophat(c); break;
                case HEAD_EQUIPMENT_KINGSCROWN: h = new robot_parts::kingscrown(c); break;
                case HEAD_EQUIPMENT_JESTERHAT: h = new robot_parts::jesterhat(c); break;
                case HEAD_EQUIPMENT_WITCH_HAT: h = new robot_parts::witch_hat(c); break;
                case HEAD_EQUIPMENT_HARD_HAT: h = new robot_parts::hard_hat(c); break;
                case HEAD_EQUIPMENT_VIKING_HELMET: h = new robot_parts::vikinghelmet(c); break;
                default:
                    tms_errorf("unhandled headwear %d", e_type);
                case HEAD_EQUIPMENT_NULL:
                    return 0;
            }
            break;

    }

    return h;
}

void
robot_parts::equipment::separate()
{
    uint32_t item_id = this->get_item_id();

    if (item_id != ITEM_INVALID) {
        b2Vec2 vel;

        item *i = of::create_item(item_id);
        i->set_scale(this->r->get_scale());

        if (this->get_body(0)) {
            i->_pos = this->get_body(0)->GetPosition();
            i->_angle = this->get_body(0)->GetAngle();
            vel = this->get_body(0)->GetLinearVelocity();
        } else if (this->fx) {
            i->_pos = this->r->get_body(0)->GetWorldPoint(((b2PolygonShape*)this->fx->GetShape())->m_centroid);
            i->_angle = this->r->get_body(0)->GetAngle();
            vel = this->r->get_body(0)->GetLinearVelocity();
        } else {
            tms_warnf("no position to separate from");
            i->_pos = this->r->get_body(0)->GetWorldPoint(b2Vec2(0.f, 0.f));
            i->_angle = this->r->get_body(0)->GetAngle();
            vel = this->r->get_body(0)->GetLinearVelocity();
        }

        i->set_layer(this->r->get_layer());
        i->properties[1].v.f = tclampf((this->r->i_dir+1.f)/2.f, 0.f, 1.f); /* rotation */
        G->emit(i, 0/*this->r*/, vel);
    }

    this->r->set_equipment(this->get_equipment_category(), 0);
}

void
robot_parts::equipment::get_shape_for_dir(b2Vec2 *out, int dir)
{
    if (dir == 1) {
        memcpy(out, this->shape.m_vertices, 4*sizeof(b2Vec2));
    } else {
        out[0] = -this->shape.m_vertices[1];
        out[1] = -this->shape.m_vertices[0];
        out[2] = -this->shape.m_vertices[3];
        out[3] = -this->shape.m_vertices[2];
    }
}

void
robot_parts::equipment::add_as_child()
{
    tms_entity_add_child(this->r, this);

    if (this->r->scene) {
        tms_scene_add_entity(this->r->scene, this);
    }
}

void
robot_parts::equipment::remove_as_child()
{
    tms_entity_remove_child(this->r, this);

    if (this->scene) {
        tms_scene_remove_entity(this->scene, this);
    }
}

void
robot_parts::equipment::on_dir_change()
{
    if (this->fx) {
        int i_dir = (int)roundf(this->r->i_dir);

        b2Vec2 verts[4];
        this->get_shape_for_dir(verts, i_dir);
        ((b2PolygonShape*)this->fx->GetShape())->Set(verts, 4);
        ((b2PolygonShape*)this->fx->GetShape())->Scale(this->r->get_scale());
    }
}

void
robot_parts::equipment::add_to_world()
{
    b2PolygonShape sh;

    //tms_debugf("equipment add to world in layer %d", this->r->get_layer());

    int i_dir = (int)roundf(this->r->i_dir);
    if (i_dir != -1 && i_dir != 1) {
        i_dir = 1;
    }

    b2Vec2 verts[4];
    for (int x=0; x<4; x++) {
        verts[x] = i_dir * this->shape.m_vertices[x];
    }

    sh.Set(verts, 4);

    sh.Scale(this->r->get_scale());

    tms_infof("scale: %.2f", this->r->get_scale());

    b2FixtureDef fd;
    fd.shape = &sh;
    fd.restitution = this->get_material()->restitution;
    fd.friction = this->get_material()->friction;
    fd.density = this->get_material()->density;
    fd.filter = world::get_filter_for_layer(this->get_layer(), this->layer_mask);
    this->fx = this->r->get_body(0)->CreateFixture(&fd);
    this->fx->SetUserData(this->r);
}

void
robot_parts::equipment::remove_from_world()
{
    this->r->get_body(0)->DestroyFixture(this->fx);
    this->fx = 0;
}

robot_parts::headwear::headwear(creature *r) : equipment(r)
{
    this->r = r;
}

void
robot_parts::head_base::on_dir_change()
{
    int new_dir = (int)roundf(this->r->i_dir);

    //tms_debugf("dir change head base");

    if (this->r->recreate_head_on_dir_change) {
        this->remove_from_world();
        this->add_to_world();
    }
}

void
robot_parts::head_base::separate()
{
    /* make sure the headwear is also separated */
    if (this->r->equipments[EQUIPMENT_HEADWEAR]) {
        this->r->equipments[EQUIPMENT_HEADWEAR]->separate();
    }

    equipment::separate();
}

void
robot_parts::headwear::update()
{
    if (!this->r->head) {
        /* XXX */
        return;
    }

    tmat4_copy(this->M, this->r->head->M);
    b2Vec2 offs = this->get_offset();

    switch (this->get_attachment_point()) {
        case HEADWEAR_ATTACHMENT_TOP:
            {
                b2Vec2 anchor = this->r->head->get_top_anchor();
                tmat4_translate(this->M, 0, anchor.y, anchor.x);
                tmat4_rotate(this->M, this->r->head->get_top_angle(), -1.f, 0.f, 0.f);
                tmat4_translate(this->M, offs.x, offs.y, 0.f);
            }
            break;

        case HEADWEAR_ATTACHMENT_CENTRE:
            {
                b2Vec2 anchor = this->r->head->get_centre_anchor() + this->get_offset();
                tmat4_translate(this->M, anchor.x, anchor.y, 0.f);
            }
            break;
    }
    tmat3_copy_mat4_sub3x3(this->N, this->M);
}

void
robot_parts::equipment::update()
{
#if 0
    if (this->r->flag_active(ENTITY_IS_ROBOT)) {
        robot_base *r = static_cast<robot_base*>(this->r);
        this->set_uniform("~color", r->faction->color.r, r->faction->color.g, r->faction->color.b, 1.f);
    }
#endif
    tmat4_copy(this->M, this->r->M);
    tmat3_copy(this->N, this->r->N);
}

robot_parts::heisenberghat::heisenberghat(creature *r)
    : headwear(r)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_HAT));
    this->set_material(&m_edev_dark);
}

robot_parts::wizardhat::wizardhat(creature *r)
    : headwear(r)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_WIZARDHAT));
    this->set_material(&m_item);
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);
}

robot_parts::witch_hat::witch_hat(creature *r)
    : headwear(r)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_WITCH_HAT));
    this->set_material(&m_item);
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);
}

robot_parts::tophat::tophat(creature *r)
    : headwear(r)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_TOPHAT));
    this->set_material(&m_item_shiny);
    //this->set_uniform("~color", .2f, .2f, .2f, 1.f);
}

robot_parts::kingscrown::kingscrown(creature *r)
    : headwear(r)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_KINGSCROWN));
    this->set_material(&m_item_shiny);
    //this->set_uniform("~color", .2f, .2f, .2f, 1.f);
}

robot_parts::jesterhat::jesterhat(creature *r)
    : headwear(r)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_JESTERHAT));
    this->set_material(&m_item);
}

robot_parts::conicalhat::conicalhat(creature *r)
    : headwear(r)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_CONICALHAT));
    this->set_material(&m_item);
    //this->set_uniform("~color", .8f, .9f, .5f, 1.f);
}

robot_parts::policehat::policehat(creature *r)
    : headwear(r)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_POLICEHAT));
    this->set_material(&m_item);
}

robot_parts::ninjahelmet::ninjahelmet(creature *r)
    : headwear(r)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_NINJAHELMET));
    this->set_material(&m_edev_dark);
}

robot_parts::hard_hat::hard_hat(creature *r)
    : headwear(r)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_HARD_HAT));
    this->set_material(&m_item_shiny);
}

robot_parts::vikinghelmet::vikinghelmet(creature *r)
    : headwear(r)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_VIKING_HELMET));
    this->set_material(&m_item_shiny);
}

robot_parts::robot_back::robot_back(creature *r)
    : back(r)
{
    this->shape.SetAsBox(.375f/2.f, .75f/2.f, b2Vec2(-.375f/2.f, 0.f), 0);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_BACK));
    this->set_material(&m_robot_tinted);
}

robot_parts::black_robot_back::black_robot_back(creature *r)
    : back(r)
{
    this->shape.SetAsBox(.375f/2.f, .75f/2.f, b2Vec2(-.375f/2.f, 0.f), 0);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_BACK));
    this->set_material(&m_robot_tinted_light);
    this->set_uniform("~color", 0.2f, 0.2f, 0.2f, 1.f);
}

robot_parts::robot_front::robot_front(creature *r)
    : front(r)
{
    this->shape.SetAsBox(.375f/2.f, .75f/2.f, b2Vec2(.375f/2.f, 0.f), 0);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_FRONT));
    this->set_material(&m_robot_tinted);
}

robot_parts::black_robot_front::black_robot_front(creature *r)
    : front(r)
{
    this->shape.SetAsBox(.375f/2.f, .75f/2.f, b2Vec2(.375f/2.f, 0.f), 0);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROBOT_FRONT));
    this->set_material(&m_robot_tinted_light);
    this->set_uniform("~color", 0.2f, 0.2f, 0.2f, 1.f);
}

robot_parts::pioneer_front::pioneer_front(creature *r)
    : front(r)
{
    this->shape.SetAsBox(.375f/2.f, .75f/2.f, b2Vec2(.375f/2.f, 0.f), 0);
    this->set_mesh(mesh_factory::get_mesh(MODEL_PIONEER_FRONT));
    this->set_material(&m_robot_armor);
}

robot_parts::pioneer_back::pioneer_back(creature *r)
    : back(r)
{
    this->shape.SetAsBox(.375f/2.f, .75f/2.f, b2Vec2(-.375f/2.f, 0.f), 0);
    this->set_mesh(mesh_factory::get_mesh(MODEL_PIONEER_BACK));
    this->set_material(&m_robot_armor);
}

robot_parts::base_jetpack::base_jetpack(creature *c)
    : back(c)
{
    this->active = false;

    /* Only enable audio for one of the two flames. */
    this->flames[0] = new flame_effect(c->get_position(), r->get_layer(), 0);
    this->flames[1] = new flame_effect(c->get_position(), r->get_layer(), 0, false);
    this->flames[0]->set_thrustmul(0.f);
    this->flames[1]->set_thrustmul(0.f);
    this->flames[0]->set_z_offset(-0.3f);
    this->flames[1]->set_z_offset(0.3f);

    b2Vec2 verts[4] = {
        b2Vec2(-.1f, .2f),
        b2Vec2(-.4f, .3f),
        b2Vec2(-.4f, -.3f),
        b2Vec2(-.1f, -.2f),
    };
    this->shape.Set(verts, 4);
}

void
robot_parts::base_jetpack::add_to_world()
{
    back::add_to_world();

    if (W->is_playing()) {
        /* We use post_emit whenever an emit is necessary within add_to_world.
         * This prevents the to_be_emitted vector to be invalidated due to
         * changes being made while it's being iterated over. */
        G->post_emit(this->flames[0], this->r, this->r->get_smooth_velocity());
        G->post_emit(this->flames[1], this->r, this->r->get_smooth_velocity());
    }
}

robot_parts::jetpack::jetpack(creature *c)
    : base_jetpack(c)
{
    this->fuel = JETPACK_MAX_FUEL;

    this->set_material(&m_edev_dark);
    this->set_mesh(mesh_factory::get_mesh(MODEL_JETPACK));
}

bool
robot_parts::jetpack::on_jump()
{
    this->active = true;
    this->r->motion = MOTION_BODY_EQUIPMENT;
    return false;
}

bool
robot_parts::jetpack::on_stop_jump()
{
    this->active = false;
    this->r->motion = MOTION_DEFAULT;
    return false;
}

void
robot_parts::jetpack::step()
{
    if (this->r->is_dead()) {
        this->r->motion = MOTION_DEFAULT;
        this->active = false;
    }

    if (this->fuel > JETPACK_MAX_FUEL)
        this->fuel = JETPACK_MAX_FUEL;

    if (this->r && this->r->is_player()) {
        if (this->active || this->fuel < JETPACK_MAX_FUEL) {
            adventure::bars[BAR_ARMOR].value = this->fuel/JETPACK_MAX_FUEL;
            adventure::bars[BAR_ARMOR].color = (tvec3){.5f, .7f, .05f};
            adventure::bars[BAR_ARMOR].time = 2.f;
        }
    }

    if (this->fuel > 0.f && this->active) {
        b2Vec2 force;
        b2Body *b = this->r->get_body(0);
        float angle = this->r->get_angle()+(M_PI/2.f);
        tmath_sincos(angle, &force.y, &force.x);

        force.x *= JETPACK_FORCE_MUL * this->r->get_total_mass();
        force.y *= JETPACK_FORCE_MUL * this->r->get_total_mass();

        b->ApplyForceToCenter(force);

        b2Vec2 p;
        p = this->r->local_to_world(b2Vec2(this->r->i_dir * -.45f, -.25f), 0);
        p.x += -.01f + ((rand()%100) / 100.f) * .02f;
        p.y += -.01f + ((rand()%100) / 100.f) * .02f;
        b2Vec2 v = b->GetWorldVector(b2Vec2(0.f, -1.f)) + .25f*this->r->get_smooth_velocity();

        for (int x=0; x<2; x++) {
            if (this->flames[x]) {
                this->flames[x]->update_pos(p, v);
                this->flames[x]->set_layer(this->r->get_layer());
                this->flames[x]->set_thrustmul((this->fuel/JETPACK_MAX_FUEL) * 3.f);
            }
        }

        this->fuel -= JETPACK_FUEL_CONSUMPTION_RATE * G->get_time_mul();
    } else {
        for (int x=0; x<2; x++) {
            if (this->flames[x]) {
                this->flames[x]->set_thrustmul(0.f);
            }
        }
    }

    this->fuel += JETPACK_FUEL_RECHARGE_RATE * G->get_time_mul();
}

robot_parts::upgraded_jetpack::upgraded_jetpack(creature *r)
    : base_jetpack(r)
{
    this->fuel = UPGRADED_JETPACK_MAX_FUEL;

    this->set_mesh(mesh_factory::get_mesh(MODEL_JETPACK));
}

bool
robot_parts::upgraded_jetpack::on_jump()
{
    this->active = true;
    this->r->motion = MOTION_BODY_EQUIPMENT;
    return false;
}

bool
robot_parts::upgraded_jetpack::on_stop_jump()
{
    this->active = false;
    this->r->motion = MOTION_DEFAULT;
    return false;
}

void
robot_parts::upgraded_jetpack::step()
{
    if (this->r->is_dead()) {
        this->active = false;
        this->r->motion = MOTION_DEFAULT;
    }
    if (this->fuel > UPGRADED_JETPACK_MAX_FUEL)
        this->fuel = UPGRADED_JETPACK_MAX_FUEL;

    if (this->r && this->r->is_player()) {
        if (this->active || this->fuel < UPGRADED_JETPACK_MAX_FUEL) {
            adventure::bars[BAR_ARMOR].value = this->fuel/UPGRADED_JETPACK_MAX_FUEL;
            adventure::bars[BAR_ARMOR].color = (tvec3){.5f, .7f, .05f};
            adventure::bars[BAR_ARMOR].time = 2.f;
        }
    }

    if (this->fuel > 0.f && this->active) {
        b2Vec2 applied_forces(0,0);
        b2Vec2 force;
        b2Body *b = this->r->get_body(0);
        float angle = this->r->get_angle()+(M_PI/2.f);
        tmath_sincos(angle, &force.y, &force.x);

        force.x *= UPGRADED_JETPACK_FORCE_MUL * this->r->get_total_mass();
        force.y *= UPGRADED_JETPACK_FORCE_MUL * this->r->get_total_mass();

        b->ApplyForceToCenter(force);
        //tms_debugf("UP Applying force %.2f/%.2f", force.x, force.y);
        applied_forces = force;

        b2Vec2 p;
        p = this->r->local_to_world(b2Vec2(this->r->i_dir * -.45f, -0.f), 0);
        p.x += -.01f + ((rand()%100) / 100.f) * .02f;
        p.y += -.01f + ((rand()%100) / 100.f) * .02f;
        b2Vec2 v = this->r->get_body(0)->GetWorldVector(b2Vec2(0.f, -1.f)) + .25f*this->r->get_smooth_velocity();

        for (int x=0; x<2; x++) {
            if (this->flames[x]) {
                this->flames[x]->update_pos(p, v);
                this->flames[x]->set_layer(this->r->get_layer());
                this->flames[x]->set_thrustmul((this->fuel/UPGRADED_JETPACK_MAX_FUEL) * 3.f);
            }
        }

        this->fuel -= UPGRADED_JETPACK_FUEL_CONSUMPTION_RATE * G->get_time_mul();
    } else {
        for (int x=0; x<2; x++) {
            if (this->flames[x]) {
                this->flames[x]->set_thrustmul(0.f);
            }
        }
    }

    this->fuel += UPGRADED_JETPACK_FUEL_RECHARGE_RATE * G->get_time_mul();
}

void
robot_parts::feet_base::add_to_world()
{
    tms_assertf(this->r, "no creature set for feet!");

    b2BodyDef bd;
    bd.type = b2_dynamicBody;
    bd.position = this->r->local_to_world(b2Vec2(0.f, 0.f), 0);
    bd.angle = this->r->get_angle();

    this->set_layer(this->r->get_layer());

    this->body = W->b2->CreateBody(&bd);
    this->body->SetAngularDamping(r->angular_damping);
    this->body->SetUserData((void*)1);

    this->create_fixtures();

    if (!this->soft) {
        this->r->create_feet_joint(0);
    }
}
void
robot_parts::feet_base::remove_from_world()
{
    this->r->destroy_feet_joint();

    if (this->body) {
        this->body->GetWorld()->DestroyBody(this->body);
        this->dangle();
    }
}

void
robot_parts::head_base::add_to_world()
{
    int new_dir = (int)roundf(this->r->i_dir);

    b2BodyDef bd;
    bd.type = b2_dynamicBody;
    bd.position = this->r->local_to_world(b2Vec2(this->r->head_pos.x*new_dir*this->r->get_scale(), this->r->head_pos.y*this->r->get_scale()), 0);
    bd.angle = this->r->get_angle();

    this->body = W->b2->CreateBody(&bd);
    this->body->SetAngularDamping(r->angular_damping);
    this->body->SetUserData((void*)5);

    this->create_fixtures();

    this->r->create_head_joint();
}
void
robot_parts::head_base::remove_from_world()
{
    this->r->destroy_head_joint();

    if (this->body) {
        this->body->GetWorld()->DestroyBody(this->body);
        this->dangle();
    }
}

void
robot_parts::feet::reset_angles()
{
    float ga = this->r->get_gravity_angle();

    this->foot_normal[0] = ga;
    this->foot_normal[1] = ga;
    this->gangle[0] = ga;
    this->gangle[1] = ga;
}

robot_parts::feet_base::feet_base(creature *c) :
    equipment(c)
{
    this->soft = false;
    this->disable_sound = false;
    this->body_index = 0;
    this->local_x_offset = 0.f;
    this->stepcount = 0.f;
    this->damage_multiplier = .5f;
    this->damage_sensitivity = 125.f;
    this->dangle();
    this->reset();
    this->do_step = false;
}

void
robot_parts::quadruped::update_fixture()
{
    this->feets[0]->local_x_offset = -(this->x_offset*this->r->get_scale())/2.f;
    this->feets[1]->local_x_offset = +(this->x_offset*this->r->get_scale())/2.f;

    this->feets[0]->update_fixture();
    this->feets[1]->update_fixture();
}

void
robot_parts::monowheel::update_fixture()
{
    const float mod = this->r->get_scale();

    if (this->f) {
        b2CircleShape *shape = static_cast<b2CircleShape*>(this->f->GetShape());
        shape->m_radius = MONOWHEEL_SIZE * .75f;
        shape->m_p = b2Vec2(-.2f + this->local_x_offset, -this->get_offset());
        shape->Scale(mod);
        this->f->Refilter();
    }

    if (this->body) {
        this->body->ResetMassData();
    }
}

void
robot_parts::miniwheels::update_fixture()
{
    float mod = this->r->get_scale();

    tms_debugf("feet update fixture");
    if (this->f0) {
        b2CircleShape *shape = static_cast<b2CircleShape*>(this->f0->GetShape());
        shape->m_radius = BASE_FOOT_SIZE * mod;
        shape->m_p = b2Vec2(-.2f + this->local_x_offset, -this->get_offset());
        shape->Scale(this->r->get_scale());
        this->f0->Refilter();
    }
    if (this->f1) {
        b2CircleShape *shape = static_cast<b2CircleShape*>(this->f1->GetShape());
        shape->m_radius = BASE_FOOT_SIZE * mod;
        shape->m_p = b2Vec2( .2f + this->local_x_offset, -this->get_offset());
        shape->Scale(this->r->get_scale());
        this->f1->Refilter();
    }

    if (this->body) {
        this->body->ResetMassData();
    }
}

void
robot_parts::feet::update()
{
    tmat4_copy(this->M, this->r->M);
    tmat3_copy(this->N, this->r->N);
    //tmat4_scale(this->M, this->r->get_scale(), this->r->get_scale(), this->r->get_scale());
    this->legs[0]->update();
    this->legs[1]->update();
}

void
robot_parts::feet::update_fixture()
{
    tms_debugf("feet update fixture");
    if (this->f0) {
        b2CircleShape *shape = static_cast<b2CircleShape*>(this->f0->GetShape());
        shape->m_radius = BASE_FOOT_SIZE;
        shape->m_p = b2Vec2((-.2f + this->local_x_offset), -this->get_offset());
        shape->Scale(this->r->get_scale());
        this->f0->Refilter();
    }
    if (this->f1) {
        b2CircleShape *shape = static_cast<b2CircleShape*>(this->f1->GetShape());
        shape->m_radius = BASE_FOOT_SIZE;
        shape->m_p = b2Vec2((.2f + this->local_x_offset), -this->get_offset());
        shape->Scale(this->r->get_scale());
        this->f1->Refilter();
    }

    if (this->body) {
        //this->body->ResetMassData();
    }
}

void
robot_parts::feet::create_fixtures()
{
    const int layer = this->get_layer();

    tms_debugf("!!!!!!!!!!!!!!!!!!!!!!!!!!! creating feet in layer %d", layer);

    float creature_scale = this->r ? this->r->get_scale() : 1.f;

    b2CircleShape c1;
    c1.m_radius = BASE_FOOT_SIZE;
    c1.m_p = b2Vec2((-.2f + this->local_x_offset), -this->get_offset());
    c1.Scale(creature_scale);

    b2CircleShape c2;
    c2.m_radius = BASE_FOOT_SIZE;
    c2.m_p = b2Vec2(( .2f + this->local_x_offset), -this->get_offset());
    c2.Scale(creature_scale);

    b2FixtureDef fd_foot1; /* foot 1 */
    fd_foot1.shape = &c1;
    fd_foot1.friction       = m_robot_foot.friction;
    fd_foot1.density        = m_robot_foot.density;
    fd_foot1.restitution    = m_robot_foot.restitution;
    fd_foot1.filter = world::get_filter_for_layer(layer, 15);

    b2FixtureDef fd_foot2; /* foot 2 */
    fd_foot2.shape = &c2;
    fd_foot2.friction       = m_robot_foot.friction;
    fd_foot2.density        = m_robot_foot.density;
    fd_foot2.restitution    = m_robot_foot.restitution;
    fd_foot2.filter = world::get_filter_for_layer(layer, 15);

    (this->f0 = this->body->CreateFixture(&fd_foot1))->SetUserData(this->r);
    (this->f1 = this->body->CreateFixture(&fd_foot2))->SetUserData(this->r);
}

void
robot_parts::monowheel::create_fixtures()
{
    b2CircleShape c1;
    c1.m_radius = MONOWHEEL_SIZE * .75f;
    c1.m_p = b2Vec2(0.f, -this->get_offset());

    c1.Scale(this->r->get_scale());

    b2FixtureDef fd_foot1; /* foot 1 */
    fd_foot1.shape = &c1;
    fd_foot1.friction       = m_robot_foot.friction;
    fd_foot1.density        = m_robot_foot.density;
    fd_foot1.restitution    = m_robot_foot.restitution;
    fd_foot1.filter = world::get_filter_for_layer(this->get_layer(), 15);

    (this->f = this->body->CreateFixture(&fd_foot1))->SetUserData(this->r);
}

void
robot_parts::miniwheels::create_fixtures()
{
    b2CircleShape c1;
    c1.m_radius = .25f;
    c1.m_p = b2Vec2(-.2f, -this->get_offset());

    b2CircleShape c2;
    c2.m_radius = .25f;
    c2.m_p = b2Vec2(.2f, -this->get_offset());

    c1.Scale(this->r->get_scale());
    c2.Scale(this->r->get_scale());

    b2FixtureDef fd_foot1; /* foot 1 */
    fd_foot1.shape = &c1;
    fd_foot1.friction       = m_robot_foot.friction;
    fd_foot1.density        = m_robot_foot.density;
    fd_foot1.restitution    = m_robot_foot.restitution;
    fd_foot1.filter = world::get_filter_for_layer(this->get_layer(), 15);

    b2FixtureDef fd_foot2; /* foot 2 */
    fd_foot2.shape = &c2;
    fd_foot2.friction       = m_robot_foot.friction;
    fd_foot2.density        = m_robot_foot.density;
    fd_foot2.restitution    = m_robot_foot.restitution;
    fd_foot2.filter = world::get_filter_for_layer(this->get_layer(), 15);

    (this->f0 = this->body->CreateFixture(&fd_foot1))->SetUserData(this->r);
    (this->f1 = this->body->CreateFixture(&fd_foot2))->SetUserData(this->r);
}

float
robot_parts::feet_base::get_offset()
{
    float robot_feet_offset = 0.f;
    float robot_scale = 1.f;
    if (this->r) {
        robot_feet_offset = this->r->feet_offset;
        robot_scale = this->r->get_scale();
    }

    return (this->offset + robot_feet_offset) * robot_scale
        + ((1.f-robot_scale) * .125f); /* XXX this should not be done if the feet aren't made of circles with radius .25 */
}

void
robot_parts::monowheel::set_layer(int n)
{
    tms_entity_set_prio_all(static_cast<struct tms_entity*>(this), n);
    if (this->f) this->f->SetFilterData(world::get_filter_for_layer(n, 15));
}

void
robot_parts::miniwheels::set_layer(int n)
{
    tms_entity_set_prio_all(static_cast<struct tms_entity*>(this), n);
    if (this->f0) this->f0->SetFilterData(world::get_filter_for_layer(n, 15));
    if (this->f1) this->f1->SetFilterData(world::get_filter_for_layer(n, 15));
}

void
robot_parts::feet::set_layer(int n)
{
    tms_entity_set_prio_all(static_cast<struct tms_entity*>(this), n);
    if (this->f0) this->f0->SetFilterData(world::get_filter_for_layer(n, 15));
    if (this->f1) this->f1->SetFilterData(world::get_filter_for_layer(n, 15));
}

robot_parts::monowheel::monowheel(creature *c) :
    feet_base(c)
{
    this->offset = 0.2f;//.05f;

    this->damage_sensitivity = 50.f;
    this->damage_multiplier = 0.35f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_WHEEL));
    this->set_material(&m_wheel);

    this->dangle();
    this->reset();
}

void
robot_parts::monowheel::step()
{
    if (this->r->is_walking()) {
        float inc = WORLD_STEP/1000000.f * this->r->get_speed() * G->get_time_mul() * (1.f/this->r->get_scale());
        this->stepcount += this->r->look_dir != this->r->dir ? -inc : inc;
        this->do_step = false;
    }
}

robot_parts::miniwheels::miniwheels(creature *c) :
    feet_base(c)
{
    this->wheels[0] = new wheel(this, -1.f, 1.f);
    this->wheels[1] = new wheel(this, 1.f, 1.f);
    this->wheels[2] = new wheel(this, -1.f, -1.f);
    this->wheels[3] = new wheel(this, 1.f, -1.f);
    this->offset = 0.f;//.05f;

    this->damage_sensitivity = 50.f;
    this->damage_multiplier = 0.75f;

    this->dangle();
    this->reset();
}

void
robot_parts::miniwheels::add_as_child()
{
    tms_entity_add_child(this->r, this->wheels[0]);
    tms_entity_add_child(this->r, this->wheels[1]);
    tms_entity_add_child(this->r, this->wheels[2]);
    tms_entity_add_child(this->r, this->wheels[3]);

    if (this->r->scene) {
        tms_scene_add_entity(this->r->scene, this->wheels[0]);
        tms_scene_add_entity(this->r->scene, this->wheels[1]);
        tms_scene_add_entity(this->r->scene, this->wheels[2]);
        tms_scene_add_entity(this->r->scene, this->wheels[3]);
    }
}

void
robot_parts::miniwheels::remove_as_child()
{
    tms_entity_remove_child(this->r, this->wheels[0]);
    tms_entity_remove_child(this->r, this->wheels[1]);
    tms_entity_remove_child(this->r, this->wheels[2]);
    tms_entity_remove_child(this->r, this->wheels[3]);

    if (this->wheels[0]->scene) {
        tms_scene_remove_entity(this->wheels[0]->scene, this->wheels[0]);
    }
    if (this->wheels[1]->scene) {
        tms_scene_remove_entity(this->wheels[1]->scene, this->wheels[1]);
    }
    if (this->wheels[2]->scene) {
        tms_scene_remove_entity(this->wheels[2]->scene, this->wheels[2]);
    }
    if (this->wheels[3]->scene) {
        tms_scene_remove_entity(this->wheels[3]->scene, this->wheels[3]);
    }
}

void
robot_parts::miniwheels::step()
{
    if (this->do_step) {
        float inc = WORLD_STEP/1000000.f * this->r->get_speed() * G->get_time_mul() * (1.f/this->r->get_scale());
        this->stepcount += this->r->look_dir != this->r->dir ? -inc : inc;
        this->do_step = false;
    }
}

robot_parts::miniwheels::wheel::wheel(miniwheels *parent, float pos, float z)
{
    this->parent = parent;
    this->pos = pos;
    this->z = z;
    this->set_mesh(mesh_factory::get_mesh(MODEL_WHEEL));
    this->set_material(&m_wheel);
}

void
robot_parts::miniwheels::wheel::update()
{
    tmat4_copy(this->M, this->parent->r->M);
    tmat4_translate(this->M, -.02f+(.2f*this->z) + (this->z > 0 ? .05f:0), -.175f/2.f - this->parent->get_offset(), this->pos*.225f);
    tmat4_rotate(this->M, 90 + (this->z > 0 ? 180:0), 0, 1.f, 0);
    tmat4_rotate(this->M, this->parent->stepcount*(180.f/M_PI), 0.f, 0, 1);
    tmat3_copy_mat4_sub3x3(this->N, this->M);

    tmat4_scale(this->M, .3f, .3f, .4f);
}

void robot_parts::monowheel::update()
{
    tmat4_copy(this->M, this->r->M);
    tmat4_translate(this->M,
            -.07f,
            -this->get_offset(),
            0.f);
    tmat4_rotate(this->M, 90, 0, 1.f, 0);
    tmat4_rotate(this->M, this->stepcount*(180.f/M_PI), 0.f, 0, 1);
    tmat3_copy_mat4_sub3x3(this->N, this->M);

    tmat4_scale(this->M, MONOWHEEL_SIZE, MONOWHEEL_SIZE, MONOWHEEL_SIZE*1.75f);
}

void
robot_parts::monowheel::handle_contact(b2Contact *contact, b2Fixture *rf, b2Fixture *o, const b2Manifold *man, float base_tangent, bool rev)
{
    float input_tangent = base_tangent;

    if (!this->on) {
        if (!this->r->is_dead()) {
            contact->SetFriction(0);
            contact->SetTangentSpeed(0);
            contact->SetEnabled(false);
        }
        return;
    }

    this->r->on_ground = CREATURE_GROUND_THRESHOLD;

    if (this->r->is_walking()) {
        if (this->r->is_standing()) {
            if (fabsf(this->r->balance->get_offset()) < .4f) {
                base_tangent += (rev ? 1 : 1 )*1.1f * this->r->dir * (this->r->get_speed() / 10.f);
                this->do_step = true;
            }
        }
    }

    contact->SetTangentSpeed(base_tangent);
    b2WorldManifold wm;
    contact->GetWorldManifold(&wm);

    b2Vec2 vel = o->GetBody()->GetLinearVelocityFromWorldPoint(wm.points[0]);
    this->r->set_ground_speed(
            b2Dot(this->r->get_tangent_vector(1.f), vel)+input_tangent,
            b2Dot(this->r->get_normal_vector(1.f), vel)
            );
}

void
robot_parts::miniwheels::handle_contact(b2Contact *contact, b2Fixture *rf, b2Fixture *o, const b2Manifold *man, float base_tangent, bool rev)
{
    float input_tangent = base_tangent;

    if (!this->on) {
        if (!this->r->is_dead()) {
            contact->SetFriction(0);
            contact->SetTangentSpeed(0);
            contact->SetEnabled(false);
        }
        return;
    }

    this->r->on_ground = CREATURE_GROUND_THRESHOLD;

    if (this->r->is_walking()) {
        if (this->r->is_standing()) {
            if (fabsf(this->r->balance->get_offset()) < .4f) {
                base_tangent += (rev ? 1 : 1 )*1.1f * this->r->dir * (this->r->get_speed() / 10.f);
                this->do_step = true;
            }
        }
    }

    contact->SetTangentSpeed(base_tangent);
    b2WorldManifold wm;
    contact->GetWorldManifold(&wm);

    float mul = b2Distance(o->GetBody()->GetLinearVelocityFromWorldPoint(wm.points[0]), this->r->get_body(0)->GetLinearVelocity());
    mul = tclampf(mul, 1.f, 5.f);
    mul = 1.f/mul;

    b2Vec2 vel = o->GetBody()->GetLinearVelocityFromWorldPoint(wm.points[0]);
    this->r->set_ground_speed(
            b2Dot(this->r->get_tangent_vector(1.f), vel)+input_tangent * mul,
            b2Dot(this->r->get_normal_vector(1.f), vel) * mul
            );
}

robot_parts::quadruped::quadruped(creature *c) :
    feet_base(c)
{
    this->feets[0] = new robot_parts::feet(c);
    this->feets[1] = new robot_parts::feet(c);

    this->feets[0]->foot_scale = .5f;
    this->feets[1]->foot_scale = .5f;

    this->feets[0]->local_x_offset = -c->feet_width/2.f;
    this->feets[1]->local_x_offset = +c->feet_width/2.f;
    this->feets[1]->body_index = 1;

    this->feets[0]->soft = true;
    this->feets[1]->soft = true;

    this->x_offset = c->feet_width;
    this->offset = 0.f;

    this->damage_sensitivity = 75.f;
    this->damage_multiplier = .125f;

    this->dangle();
    this->reset();
}

robot_parts::quadruped::~quadruped()
{
    delete this->feets[0];
    delete this->feets[1];
}

void
robot_parts::quadruped::add_as_child()
{
    tms_entity_add_child(this->r, this->feets[0]);
    tms_entity_add_child(this->r, this->feets[1]);

    if (this->r->scene) {
        tms_scene_add_entity(this->r->scene, this->feets[0]);
        tms_scene_add_entity(this->r->scene, this->feets[1]);
    }
}

void
robot_parts::quadruped::remove_as_child()
{
    tms_entity_remove_child(this->r, this->feets[0]);
    tms_entity_remove_child(this->r, this->feets[1]);

    if (this->feets[0]->scene) {
        tms_scene_remove_entity(this->feets[0]->scene, this->feets[0]);
    }
    if (this->feets[1]->scene) {
        tms_scene_remove_entity(this->feets[1]->scene, this->feets[1]);
    }
}

void
robot_parts::quadruped::update()
{
    this->feets[0]->update();
    this->feets[1]->update();
}

void
robot_parts::quadruped::reset_angles()
{
    this->feets[0]->reset_angles();
    this->feets[1]->reset_angles();
}

b2Body*
robot_parts::quadruped::get_body(uint8_t x)
{
    if (x < this->get_num_bodies()) {
        return this->feets[x]->body;
    }

    return 0;
}

void
robot_parts::quadruped::dangle()
{
    this->feets[0]->dangle();
    this->feets[1]->dangle();
}

void
robot_parts::quadruped::remove_from_world()
{
    this->feets[0]->remove_from_world();
    this->feets[1]->remove_from_world();
}

void
robot_parts::quadruped::step()
{
    this->feets[0]->step();
    this->feets[1]->step();
}

void
robot_parts::quadruped::add_to_world()
{
    this->feets[0]->add_to_world();
    this->feets[1]->add_to_world();

    this->r->create_feet_joint(0);
}

void
robot_parts::quadruped::create_fixtures()
{
    //this->feets[0]->create_fixtures(layer);
    //this->feets[1]->create_fixtures(layer);
}

bool
robot_parts::quadruped::is_foot_fixture(b2Fixture *f)
{

    /*tms_debugf("comparing %p with %p %p %p %p", f,  this->feets[0]->f0, this->feets[0]->f1
            , this->feets[1]->f0, this->feets[1]->f1);*/
    return (f == this->feets[0]->f0 || f == this->feets[0]->f1
            || f == this->feets[1]->f0 || f == this->feets[1]->f1);
}

void
robot_parts::quadruped::set_layer(int l)
{
    tms_entity_set_prio_all(static_cast<struct tms_entity*>(this), l);
    this->feets[0]->set_layer(l);
    this->feets[1]->set_layer(l);
}

void
robot_parts::quadruped::handle_contact(b2Contact *contact, b2Fixture *f, b2Fixture *other, const b2Manifold *man, float base_tangent, bool rev)
{
    if (this->feets[0]->is_foot_fixture(f)) this->feets[0]->handle_contact(contact,f,other,man,base_tangent,rev);
    else this->feets[1]->handle_contact(contact,f,other,man,base_tangent,rev);
}

robot_parts::feet::feet(creature *c) :
    feet_base(c)
{
    this->legs[0] = new robot_parts::leg(0, c, this);
    this->legs[1] = new robot_parts::leg(1, c, this);

    tms_entity_add_child(this, this->legs[0]);
    tms_entity_add_child(this, this->legs[1]);
    this->y = 0.f;

    this->damage_sensitivity = 100.f;
    this->damage_multiplier = .25f;

    this->foot_scale = 0.9f;
    this->sound_accum = 0;
    this->offset = 0;//.45f;
    this->dangle();
    this->reset();

    this->set_mesh(mesh_factory::get_mesh(MODEL_FEET_FRAME));
    this->set_material(&m_robot_tinted);
}

void
robot_parts::feet::handle_contact(b2Contact *contact, b2Fixture *rf, b2Fixture *o, const b2Manifold *man, float base_tangent, bool rev)
{
    float input_tangent = base_tangent;

    if ((o->GetUserData()) && ((p_entity*)(o->GetUserData()))->flag_active(ENTITY_IS_BULLET)) {
        contact->SetEnabled(false);
        return;
    }

    if (!this->on) {
        if (this->r->is_dead()) {
            ;
        } else {
            contact->SetFriction(0);
            contact->SetTangentSpeed(0);
            contact->SetEnabled(false);
        }
        return;
    }

#if 0
    if (!this->r->jumping) {
        if (this->r->j_feet[this->body_index]) {
            if (this->r->j_feet[this->body_index]->GetJointTranslation() < -.25f) {
                contact->SetEnabled(false);
                return;
            }
        }
    }
#endif

    b2WorldManifold wm;
    contact->GetWorldManifold(&wm);
    b2Vec2 n = wm.normal;
    float t = atan2f(n.y, n.x);

    float gangle = t + (rev ? 0.f : -1.f) * M_PI;// + M_PI/2.f;

    float _dd = tmath_adist(gangle, this->r->get_down_angle());
    float dd = fabsf(_dd);

    int gi = 0;
    gi = (rf == this->f0 ? 0 : 1);

    this->foot_blocked[gi] = false;

    if (dd > M_PI*1.5f) {
        contact->SetEnabled(false);
        return;
    }

    if (this->r->jumping == 2 && dd > M_PI/4.f) {
        contact->SetEnabled(false);
        return;
    }

    if (this->r->jumping == 1 && dd > M_PI/8.f) {
        contact->SetTangentSpeed(0.f);
        contact->SetFriction(0.f);
        return;
    }

    b2Vec2 vel = o->GetBody()->GetLinearVelocityFromWorldPoint(wm.points[0]);
    this->r->set_ground_speed(
            (b2Dot(this->r->get_tangent_vector(1.f), vel)+input_tangent),
            b2Dot(this->r->get_normal_vector(1.f), vel)
            );

    this->r->on_ground = CREATURE_GROUND_THRESHOLD;

    if (dd > M_PI/3.35f && gi == ((_dd < 0.f)?1:0)) {
        this->foot_blocked[gi] = true;
        contact->SetTangentSpeed(0.f);
        contact->SetFriction(0.f);
        //contact->SetEnabled(false);
        //contact->SetF
        //contact->SetTangentSpeed(base_tangent);
        return;
    }

    float blend = .15f;

    float diff = tmath_adist(this->gangle[gi], gangle);
    this->gangle[gi] = this->gangle[gi] + diff * blend;
    this->gangle_timer[gi] = .25f;

    diff = tmath_adist(this->foot_normal[gi], gangle);
    this->foot_normal[gi] = this->foot_normal[gi] + diff * blend*2.f;

    float d1 = tmath_adist(this->r->body->GetAngle(), this->gangle[0]);
    float d2 = tmath_adist(this->r->body->GetAngle(), this->gangle[1]);

    //r->balance->target = (this->gangle[fabsf(d1) > fabsf(d2)] + M_PI/2.f)/2.f;
    //r->balance->target = 0.f;

    if (this->r->is_walking()) {
        if (this->foot_blocked[1-gi]) { /* if the other foot is blocked, lower this foot's friction to minimize physics glitching against walls/obstacles */
            contact->SetFriction(.05f);
        }
        if (this->r->is_standing()) {
            if (this->r->flag_active(ENTITY_IS_ROBOT)) {
                if (fabsf(this->r->balance->get_offset()) < .4f) {
                    base_tangent += (rev ? 1 : 1 )*1.1f * this->r->dir * (this->r->get_speed() / 10.f);
                    this->do_step = true;
                }
            } else {
                base_tangent += (rev ? 1 : 1 )*1.1f * this->r->dir * (this->r->get_speed() / 10.f);
                this->do_step = true;
            }
        } else {
        }
    } else {
    }

    contact->SetTangentSpeed(base_tangent);
}

void
robot_parts::feet::step()
{
    for (int x=0; x<2; x++) {
        this->gangle_timer[x] -= WORLD_STEP/1000000.f * G->get_time_mul();
        if (this->gangle_timer[x] <= 0.f) {
            this->gangle_timer[x] = 0.f;
            this->gangle[x] = this->r->get_gravity_angle();

            //float diff = tmath_adist(this->foot_normal[x], this->r->get_gravity_angle()-M_PI/2.5f*(x?1:-1));
            float diff = tmath_adist(this->foot_normal[x], this->r->get_down_angle());
            this->foot_normal[x] = this->foot_normal[x]+diff*.5f;
        }

        b2Fixture *ff = (x==0?this->f0:this->f1);
        if (ff) {
            b2CircleShape *c = (b2CircleShape*)ff->GetShape();
            if (c) {
                float blend = .25f;

                float t;
                if (this->r->on_ground > 0.f)  {
                    //t = (-this->get_offset()+tclampf((tmath_adist(this->r->get_gravity_angle(), foot_normal[x])-1.f*tmath_adist(this->r->get_down_angle(), this->r->get_gravity_angle()))*.2, -.2f, .2f)*(x?1:-1));
                    t = (-this->get_offset()+tclampf((tmath_adist(this->r->get_gravity_angle(), foot_normal[x])-1.f*tmath_adist(this->r->get_down_angle(), this->r->get_gravity_angle()))*.2, -.2f, .2f)*(x?1:-1));
                } else  {
                    t = -this->get_offset();
                }
                c->m_p.y = c->m_p.y * (1.f-blend) + t*blend;
            }
        }
    }

    if (this->do_step) {
        this->y += WORLD_STEP/1000000.f * FOOT_RAISE_SPEED * G->get_time_mul();
        if (this->y > 1.f) this->y = 1.f;

        float inc = WORLD_STEP/1000000.f * this->r->get_speed() * this->r->on_ground*(1.f/CREATURE_GROUND_THRESHOLD) * G->get_time_mul() * (1.f/this->r->get_scale());
        this->stepcount += this->r->look_dir != this->r->dir ? -inc : inc;

        if (!this->disable_sound) {
            this->sound_accum += inc;
            if (this->sound_accum >= M_PI) {
                b2Vec2 p = this->r->get_position();
                G->play_sound(SND_ROBOT, p.x, p.y, rand(), 1.f);
                this->sound_accum -= M_PI;
            }
        }

        this->do_step = false;
    } else if (this->r->is_idle() || this->r->is_action_active()) {
        this->y -= WORLD_STEP/1000000.f * FOOT_RAISE_SPEED * G->get_time_mul();
        if (this->y < 0.f) this->y = 0.f;
    }
}

void
robot_parts::feet_base::set_on(bool on)
{
    if (this->body) {
        if (on) {
            this->body->SetAngularDamping(r?r->angular_damping:CREATURE_DAMPING);
        } else {
            this->body->SetAngularDamping(0.5f);
        }
    }

    this->on = on;
}

robot_parts::advanced_jetpack::advanced_jetpack(creature *r)
    : base_jetpack(r)
{
    this->fuel = ADVANCED_JETPACK_MAX_FUEL;

    this->set_material(&m_edev_dark);
    this->set_mesh(mesh_factory::get_mesh(MODEL_ADVANCED_JETPACK));
}

bool
robot_parts::advanced_jetpack::on_jump()
{
    if (this->active) {
        this->active = false;
        this->r->motion = MOTION_DEFAULT;
        return true;
    } else {
        if (this->r->on_ground <= 0) {
            this->active = true;
            this->r->motion = MOTION_BODY_EQUIPMENT;
            return true;
        }
    }
    return false;
}

void
robot_parts::advanced_jetpack::step()
{
    if (this->r->is_dead()) {
        this->active = false;
        this->r->motion = MOTION_DEFAULT;
    }

    if (this->fuel > ADVANCED_JETPACK_MAX_FUEL)
        this->fuel = ADVANCED_JETPACK_MAX_FUEL;

    if (this->r && this->r->is_player()) {
        if (this->active || this->fuel < ADVANCED_JETPACK_MAX_FUEL) {
            adventure::bars[BAR_ARMOR].value = this->fuel/ADVANCED_JETPACK_MAX_FUEL;
            adventure::bars[BAR_ARMOR].color = (tvec3){.5f, .7f, .05f};
            adventure::bars[BAR_ARMOR].time = 2.f;
        }
    }

    if (this->fuel > 0.f && this->active) {
        b2Vec2 gravity = this->r->get_gravity();
        /* Step 1: Stabilize the robot. */
        for (int x=0; x<this->r->get_num_bodies(); ++x) {
            b2Vec2 force;
            b2Body *b = this->r->get_body(x);
            if (!b) continue;

            force.x = -gravity.x * b->GetMass();
            force.y = -gravity.y * b->GetMass();

            b->ApplyForceToCenter(force);

            b->SetLinearVelocity(.95f*b->GetLinearVelocity());
        }

        b2Vec2 dir =
              (this->r->is_moving_up()) * this->r->get_body(0)->GetWorldVector(b2Vec2(0.f, 1.f))
            + (this->r->is_moving_down()) * this->r->get_body(0)->GetWorldVector(b2Vec2(0.f, -1.f))
            + (this->r->is_moving_left()) * this->r->get_body(0)->GetWorldVector(b2Vec2(-1.f, 0.f))
            + (this->r->is_moving_right()) * this->r->get_body(0)->GetWorldVector(b2Vec2(1.f, 0.f));

        dir *= ADVANCED_JETPACK_FORCE_MUL * this->r->get_total_mass();

        this->r->get_body(0)->ApplyForceToCenter(dir);

        b2Vec2 p;
        p = this->r->local_to_world(b2Vec2(this->r->i_dir * -.45f, -0.25f), 0);
        p.x += -.01f + ((rand()%100) / 100.f) * .02f;
        p.y += -.01f + ((rand()%100) / 100.f) * .02f;
        b2Vec2 v = this->r->get_body(0)->GetWorldVector(b2Vec2(0.f, -1.f)) + .25f*this->r->get_smooth_velocity();

        for (int x=0; x<2; x++) {
            if (this->flames[x]) {
                this->flames[x]->update_pos(p, v);
                this->flames[x]->set_layer(this->r->get_layer());
                this->flames[x]->set_thrustmul((this->fuel/ADVANCED_JETPACK_MAX_FUEL) * 3.f);
            }
        }

        this->fuel -= ADVANCED_JETPACK_FUEL_CONSUMPTION_RATE * G->get_time_mul();
    } else {
        this->r->reset_damping();

        for (int x=0; x<2; x++) {
            if (this->flames[x]) {
                this->flames[x]->set_thrustmul(0.f);
            }
        }
    }

    this->fuel += ADVANCED_JETPACK_FUEL_RECHARGE_RATE * G->get_time_mul();
}

robot_parts::teslagun::teslagun(creature *c)
    : weapon(c)
{
    this->terror = 0.55f;
    this->do_update_effects = true;

    this->max_range = 4.f;

    this->cooldown = 1000 * 1000;
    this->active = false;
    this->num_points = 0;

    this->set_material(&m_weapon);
    this->set_mesh(mesh_factory::get_mesh(MODEL_TESLA_GUN));
    //this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    this->tesla = new tesla_effect(c, b2Vec2(0.f, 0.f), 0);
}

robot_parts::teslagun::~teslagun()
{
    delete this->tesla;
}

void
robot_parts::teslagun::step()
{
    weapon::step();

    if (this->active) {
        float a = this->c->get_angle() + M_PI*1.5f + this->c->look_dir*this->get_arm_angle() * M_PI;
        float sn,cs;
        tmath_sincos(a, &sn, &cs);
        this->tesla->_pos = this->c->local_to_world(b2Vec2(0.f+cs*.5f, .33f+sn*.5f), 0);
        this->tesla->min_angle = a-.25f;
        this->tesla->max_angle = a+.25f;
        this->tesla->prio = this->c->get_layer();
        this->tesla->step();

        G->play_sound(SND_DISCHARGE, this->c->get_position().x, this->c->get_position().y, 0, 0.35f * (std::min(this->tesla->get_num_paths()/2.f, 1.f)), true, this);
    }

    if (this->active) {
    }
}

void
robot_parts::teslagun::attack(int add_cooldown/*=0*/)
{
    this->active = true;

#if 0
    if (this->cooldown_timer <= 0) {
        tms_debugf("attacked!");
        this->last_z = (this->c->i_dir > 0 ? .4f : -.4f)  + this->c->get_layer();
        //this->from = this->c->local_to_world()get_position();
        //this->active = true;

        b2Vec2 p = this->c->get_position();
        //G->play_sound(SND_RAILGUN_SHOOT, p.x, p.y, 0, 1.f);

        tesla_effect *effect = new tesla_effect(this->c, this->c->get_position(), this->c->get_layer());
        G->emit(effect, this, b2Vec2(0,0));

        this->cooldown_timer = this->cooldown * this->c->cooldown_multiplier;
        this->cooldown_timer += add_cooldown;

    }
#endif

    this->on_attack();
}

void
robot_parts::teslagun::attack_stop()
{
    sm::stop(&sm::discharge, this);
    this->active = false;
}

void
robot_parts::teslagun::update_effects()
{
    if (!this->c) return;

    if (this->active) {
        this->tesla->update_effects();
    }
}

/**
 * Plasma Gun
 **/
robot_parts::plasmagun::plasmagun(creature *c)
    : weapon(c)
{
    this->terror = 0.1f;
    this->do_update_effects = true;

    this->cooldown = 90000;
    this->active = false;
    this->highlight = 0.f;

    this->set_material(&m_weapon);
    this->set_mesh(mesh_factory::get_mesh(MODEL_PLASMA_GUN));
    //this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    tms_entity_init(&this->inner);
    tms_entity_set_mesh(&this->inner, mesh_factory::get_mesh(MODEL_PLASMA_GUN_INNER));
    tms_entity_set_material(&this->inner, &m_pv_rgba);
    tms_entity_set_uniform4f(&this->inner, "~color", 1.f, 1.f, 1.f, 1.f);

    tms_entity_add_child(this, &this->inner);
}

robot_parts::plasmagun::~plasmagun()
{
}

void
robot_parts::plasmagun::update()
{
    weapon::update();

    tmat4_copy(this->inner.M, this->M);
    tmat3_copy(this->inner.N, this->N);

    float hl = this->highlight;
    float r = .1f;

    hl += r + sin((double)_tms.last_time * .000004)*r;

    tms_entity_set_uniform4f(&this->inner, "~color", 0.f+hl, .5f+1.f*hl, 0.f+hl, .75f);
}

void
robot_parts::plasmagun::step()
{
    weapon::step();

    if (this->active) {
        if (this->cooldown_timer <= 0) {
            float angle = this->c->get_angle() + M_PI*1.5f + this->c->look_dir*this->get_arm_angle() * M_PI;
            angle += -.05f + (rand()%100 / 100.f)*.1f;
            b2Vec2 p = this->c->local_to_world(ROBOT_ARM_POS, 0);
            b2Vec2 v;
            tmath_sincos(angle, &v.y, &v.x);

            item *bullet = of::create_item(ITEM_PLASMA);
            bullet->set_scale(this->c->get_scale());
            G->timed_absorb(bullet, 7.5);
            bullet->_pos = p + .75f*v;
            bullet->_angle = 0.f;
            bullet->set_layer(this->c->get_layer());

            v *= 15.f;

            G->lock();
            //G->emit(bullet, this->c, v+this->c->get_tangent_vector(this->c->last_ground_speed));//this->c->get_smooth_velocity());
            G->emit(bullet, this->c, v+this->c->get_body(0)->GetWorldVector(this->c->last_ground_speed));
            G->play_sound(SND_PLASMA_SHOOT, p.x, p.y, 0, 1.f);
            G->unlock();

            this->cooldown_timer = this->cooldown;

            this->on_attack();

            this->highlight = 1.f;
        }
    }

    this->highlight -= .055f;
    if (this->highlight < 0.f) this->highlight = 0.f;
}

void
robot_parts::plasmagun::attack(int add_cooldown/*=0*/)
{
    this->active = true;
}

void
robot_parts::plasmagun::attack_stop()
{
    this->active = false;
}

void
robot_parts::plasmagun::update_effects()
{
}

/**
 * Mega Buster
 **/
robot_parts::megabuster::megabuster(creature *c)
    : weapon(c)
{
    this->terror = 0.1f;
    this->do_update_effects = true;

    this->cooldown = 100000;
    this->active = false;
    this->charge = 0;
    this->hl = 0;

    this->set_material(&m_pv_colored);
    this->set_mesh(mesh_factory::get_mesh(MODEL_MEGA_BUSTER));
    this->set_uniform("~color", MEGA_BUSTER_COLOR, 1.f);
}

robot_parts::megabuster::~megabuster()
{
}

void
robot_parts::megabuster::update()
{
    weapon::update();

    double hl = (double)this->charge / (double)MEGABUSTER_CHARGE_MAX;

    this->set_uniform("~color",
            MEGA_BUSTER_COLOR_R+.5f*hl*(.5f+.5f*sin(this->hl)),
            MEGA_BUSTER_COLOR_G+.5f*hl*(.5f+.5f*sin(this->hl*2)),
            MEGA_BUSTER_COLOR_B, 1.f);
}

void
robot_parts::megabuster::step()
{
    weapon::step();

    if (this->active) {
        this->charge += G->timemul(WORLD_STEP);
        this->charge = std::min(this->charge, (uint64_t)MEGABUSTER_CHARGE_MAX);

        double hl = (double)this->charge / (double)MEGABUSTER_CHARGE_MAX;

        double freq = (hl*hl)+.125f;
        this->hl += freq;

        int play = ceilf((this->hl-2.f) / 13.f);
        if (play > this->charge_snd) {
            this->charge_snd = play;
            b2Vec2 p = this->c->local_to_world(ROBOT_ARM_POS, 0);
            G->play_sound(SND_BUSTER_CHARGE, p.x, p.y, freq * 2.f, .4f);
        }

        if (this->c && this->c->is_player()) {
            adventure::bars[BAR_ARMOR].value = hl;
            adventure::bars[BAR_ARMOR].color = (tvec3){.5f, .5f, 1.f};
            adventure::bars[BAR_ARMOR].time = 1.f;
        }
    }
}

void
robot_parts::megabuster::attack(int add_cooldown/*=0*/)
{
    if (!this->active) {
        this->active = true;
        this->charge = 0;
        this->hl = 0;
        this->charge_snd = 0;
    }
}

void
robot_parts::megabuster::attack_stop()
{
    if (this->cooldown_timer <= 0 && this->active) {
        //tms_debugf("shooting with charge %", this->charge);

        if (this->charge < MEGABUSTER_CHARGE_MIN)
            this->charge = 0;
        float angle = this->c->get_angle() + M_PI*1.5f + this->c->look_dir*this->get_arm_angle() * M_PI;
        b2Vec2 p = this->c->local_to_world(ROBOT_ARM_POS, 0);
        b2Vec2 v;
        tmath_sincos(angle, &v.y, &v.x);

        item *bullet = of::create_item(ITEM_SOLAR);
        bullet->set_scale(this->c->get_scale());
        G->timed_absorb(bullet, 7.5);
        bullet->_pos = p + .75f*v;
        bullet->_angle = 0.f;
        bullet->set_layer(this->c->get_layer());

        bullet->data = UINT_TO_VOID(this->charge);
        bullet->set_scale(1.f + ((double)this->charge/(double)MEGABUSTER_CHARGE_MAX));

        v *= 12.f;

        G->lock();
        G->emit(bullet, this->c, v+this->c->get_body(0)->GetWorldVector(this->c->last_ground_speed));
        if (this->charge >= MEGABUSTER_CHARGE_MAX*.7) {
            G->play_sound(SND_BUSTER_SHOOT_MAXCHARGE, p.x, p.y, 0, .4f);
        } else {
            G->play_sound(SND_BUSTER_SHOOT, p.x, p.y, 0, .4f);
        }
        G->unlock();

        this->cooldown_timer = this->cooldown;

        this->on_attack();
        this->charge = 0;
        this->hl = 0;
    }

    this->active = false;
}

robot_parts::melee_weapon::melee_weapon(creature *c)
    : weapon(c)
    , arm_offset(0.5f)
    , pending_arm_angle(0.f)
    , target_arm_angle(0.f)
    , arm_movement(0.f)
    , block_dmg_multiplier(0.f)
    , plant_dmg_multiplier(0.f)
    , strength(1.f)
    , first_hit(true)
    , shape(0)
{
    this->dmg_type = DAMAGE_TYPE_BLUNT;
    this->max_range = 1.25f;
}

void
robot_parts::melee_weapon::set_arm_angle(float a, float speed/*=1.f*/)
{
    a = creature::real_arm_angle(this->c, a);

    this->pending_arm_angle = a - this->arm_offset;
}

void
robot_parts::melee_weapon::on_attack()
{
    weapon::on_attack();

    this->first_hit = true;
    this->active = true;

    this->dmg_multiplier = 1.f;
    this->force_multiplier = 1.f;
}

void
robot_parts::melee_weapon::attack_stop()
{
    this->active = false;
}

void
robot_parts::melee_weapon::raycast(const b2Vec2 &from, const b2Vec2 &to)
{
    /* XXX: ugly and bad */
    if (std::abs((from-to).x) > FLT_EPSILON && std::abs((from-to).y) > FLT_EPSILON) {
        this->dmg_multiplier += 1.5f;
        this->force_multiplier += 0.2f;
        this->hit_static = false;

        W->raycast(this, from, to);
    }
}

float32
robot_parts::melee_weapon::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    if (!world::fixture_in_layer(f, this->c->get_layer(), 2+4)) {
        return -1.f;
    }

    p_entity *e = static_cast<p_entity*>(f->GetUserData());

    if (e) {
        if (e == this->c) {
            return -1.f;
        }

        if (/*e->is_static()*/ /*!e->is_creature()*/
            f->GetBody()->GetMass() > .3f || f->GetBody()->GetMass() <= 0.f
                ) {
            this->hit_static = true;
        }

        b2Body *b = e->get_body(0);

        float damage = this->get_damage() * this->get_strength() * G->get_time_mul();

        if (b) {
            b2Vec2 force(nor);

            force *= -(this->get_force() * this->get_strength() * G->get_time_mul());

            const b2Body *self_body = this->c->get_body(0);

            if (self_body) {
                const b2Vec2 &velocity = self_body->GetLinearVelocity();

                force.x += (this->get_force() * 0.1f * velocity.x * this->get_mass() * G->get_time_mul());
                force.y += (this->get_force() * 0.1f * velocity.y * this->get_mass() * G->get_time_mul());
            }

            b->ApplyForceToCenter(force);
        }

        if (e->is_creature()) {
            ((creature*)e)->damage(damage, f, DAMAGE_TYPE_FORCE, DAMAGE_SOURCE_BULLET/*??*/, this->c->id);
            G->add_highlight(e, HL_PRESET_DEFAULT, 0.25f);

            if (this->first_hit) {
                G->play_sound(SND_SHEET_METAL, pt.x, pt.y, rand(), .325f);

                G->emit(new spark_effect(
                            pt,
                            e->get_layer()
                            ), 0);
                G->emit(new smoke_effect(
                            pt,
                            e->get_layer(),
                            .5f,
                            .5f
                            ), 0);

                this->first_hit = false;
            }
        } else if ((e->is_interactive()
                     || e->g_id == O_CHUNK
                     || e->g_id == O_TPIXEL
                     || e->g_id == O_PLANT
                   ) && W->level.flag_active(LVL_ENABLE_INTERACTIVE_DESTRUCTION)) {
            if (e->g_id == O_CHUNK || e->g_id == O_TPIXEL) {
                damage *= this->block_dmg_multiplier;
            } else if (e->g_id == O_PLANT) {
                damage *= this->plant_dmg_multiplier;

                if (this->dmg_type == DAMAGE_TYPE_HEAVY_SHARP && this->first_hit) {
                    G->play_sound(SND_CHOP_WOOD, pt.x, pt.y, rand(), .65f);
                    this->first_hit = false;
                }
            }

            G->damage_interactive(e, f, f->GetUserData2(), damage, pt, this->dmg_type);
        }

        this->dmg_multiplier = 1.f;
        this->force_multiplier = 1.f;
    }

    return fraction;
}

class shape_tester : public b2QueryCallback
{
  public:
    // IN
    b2Shape *shape;
    b2Transform *transform;
    int layer;
    int sublayer;
    bool keep_going; /* Keep going after hitting the first fixture. */

    // OUT
    std::vector<b2Fixture*> *hit_fixtures;

    bool ReportFixture(b2Fixture *fx);
} shape_tester;

bool
shape_tester::ReportFixture(b2Fixture *fx)
{
    if (fx->IsSensor()) {
        return true;
    }

    if (!world::fixture_in_layer(fx, layer, sublayer)) {
        // XXX
        return false;
    }

    b2Body  *fx_body = fx->GetBody();
    b2Shape *fx_shape = fx->GetShape();

    if (b2TestOverlap(this->shape, 0,
                fx_shape, 0,
                *this->transform,
                fx_body->GetTransform())) {

        this->hit_fixtures->push_back(fx);

        return (this->keep_going ? true : false);
    }

    return true;
}

const std::vector<struct entity_hit>&
robot_parts::melee_weapon::test_shape()
{
    this->m_results.clear();

    if (this->shape) {
        b2AABB aabb;
        b2Transform transform;
        std::vector<b2Fixture*> fixtures_list;

        b2Vec2 arm_pos = this->c->local_to_world(ROBOT_ARM_POS, 0);

        float angle = this->c->get_angle() + this->c->look_dir * (((this->get_arm_angle() + this->arm_offset) * M_PI));

        float sn, cs;
        tmath_sincos(angle, &sn, &cs);

        const b2Vec2 look_shape_offset(this->c->look_dir*this->shape_offset.x, this->shape_offset.y);

        arm_pos.x += look_shape_offset.x*cs - look_shape_offset.y*sn;
        arm_pos.y += look_shape_offset.x*sn + look_shape_offset.y*cs;

        transform.p = arm_pos;
        transform.q.Set(this->c->get_angle());

        shape_tester.shape = this->shape;
        shape_tester.transform = &transform;
        shape_tester.layer = this->c->get_layer();
        shape_tester.sublayer = 2+4;
        shape_tester.keep_going = true;

        shape_tester.hit_fixtures = &fixtures_list;

        this->shape->ComputeAABB(&aabb, transform, 0);

        W->query_aabb(&shape_tester, aabb);

        for (std::vector<b2Fixture*>::const_iterator it = fixtures_list.begin();
                it != fixtures_list.end(); ++it) {
            b2Fixture *fx = *it;

            p_entity *e = static_cast<p_entity*>(fx->GetUserData());

            if (e) {
                this->m_results.push_back(entity_hit(e, fx));
            }
        }
    }

    return this->m_results;
}

void
robot_parts::base_sword::step()
{
    arm::step(); // important!

    const float realdist    = this->arm_movement * this->swing_speed;
    const float absrealdist = std::abs(realdist);
    const float dist        = realdist * G->get_time_mul();

    this->arm_movement -= dist;
    this->arm_angle += dist;

    if (absrealdist <= 0.001f) {
        return;
    }

    const b2Vec2 arm_pos = ROBOT_ARM_POS;
    float angle = this->c->look_dir * (((this->get_arm_angle() + this->arm_offset) * M_PI));

    b2Vec2 from, to;
    float sn, cs;
    tmath_sincos(angle, &sn, &cs);

    from.x = arm_pos.x*cs - arm_pos.y*sn;
    from.y = arm_pos.x*sn + arm_pos.y*cs;

    from *= -1.f;

    b2Vec2 test(0.f, 1.f);

    to.x = test.x*cs - test.y*sn;
    to.y = test.x*sn + test.y*cs;

    to *= -1.f;

    to += from;

    this->raycast(
            this->c->local_to_world(from, 0),
            this->c->local_to_world(to, 0)
            );
}

void
robot_parts::base_sword::attack(int add_cooldown/*=0*/)
{
    if (this->cooldown_timer <= 0) {
        this->target_arm_angle = this->pending_arm_angle + .1f;

        const float a = M_PI * this->arm_angle;
        const float b = M_PI * this->target_arm_angle;

        float adist = tmath_adist(a, b);
        adist += copysignf(.85f, adist);

        this->arm_movement = adist / M_PI;

        G->play_sound(SND_SWISH_BLADE, this->c->get_position().x, this->c->get_position().y, rand(), .4f, false, this->c);

        /* Melee weapons, by default, have no cooldown.
         * However, to make sure they work properly with the AI we need to add
         * a sort of cooldown anyway.
         * the `add_cooldown'-variable is only set by roam_attack. */
        this->cooldown_timer = add_cooldown > 0 ? 300000 : 0;
        this->on_attack();
    }
}

void
robot_parts::base_hammer::step()
{
    arm::step(); // important!

    float swing_speed = this->swing_speed;

    //if (absrealdist <= 0.01f) {
        if (this->hammering && !this->active) {
            this->hammering = false;
            this->target_arm_angle = 0.75f;

            const float a = M_PI * this->arm_angle;
            const float b = M_PI * this->target_arm_angle;

            const float adist = tmath_adist(a, b) / M_PI;

            this->arm_movement = tmath_adist(a, b) / M_PI;
        }

        //return;
    //}

    if (!this->hammering && !this->active) {
        swing_speed *= .25f;
    }

    const float realdist    = this->arm_movement * swing_speed;
    const float absrealdist = std::abs(realdist);
    const float dist        = realdist * G->get_time_mul();

    this->arm_movement -= dist;
    this->arm_angle += dist;

    /* Hammers only do damage on the down-swing. */
    if (realdist >= -0.01f) {
        if (this->active) {
            /*
            this->dmg_multiplier = 0.2f;
            this->force_multiplier = 0.2f;
            */
            return;
        } else {
            return;
        }
    }

    const b2Vec2 arm_pos = ROBOT_ARM_POS;
    float angle = this->c->look_dir * (((this->get_arm_angle() + this->arm_offset) * M_PI));

    b2Vec2 from, to;
    float sn, cs;
    tmath_sincos(angle, &sn, &cs);

    from.x = arm_pos.x*cs - arm_pos.y*sn;
    from.y = arm_pos.x*sn + arm_pos.y*cs;

    from *= -1.f;

    const b2Vec2 test(0.f, 0.90f);

    to.x = test.x*cs - test.y*sn;
    to.y = test.x*sn + test.y*cs;

    to *= -1.f;

    to += from;

    const b2Vec2 cur_dir(this->c->look_dir * 0.5f, 0.f);

    b2Vec2 real_from = to;
    b2Vec2 real_to;

    real_to.x = cur_dir.x*cs - cur_dir.y*sn;
    real_to.y = cur_dir.x*sn + cur_dir.y*cs;

    real_to *= -1.f;

    real_to += real_from;

    this->raycast(
            this->c->local_to_world(real_from, 0),
            this->c->local_to_world(real_to, 0)
            );

    if (this->hit_static) {
        /* If the hammer hits a static objects, we stop and return to our original position. */
        this->hammering = true;
        this->active = false;
    }
}

void
robot_parts::base_hammer::attack(int add_cooldown/*=0*/)
{
    if (this->cooldown_timer <= 0) {
        if (this->c->look_dir == DIR_RIGHT) {
            this->target_arm_angle = -.1f;
        } else if (this->c->look_dir == DIR_LEFT) {
            this->target_arm_angle = -.1f;
        } else {
            return;
        }

        this->hammering = true;

        const float a = M_PI * this->arm_angle;
        const float b = M_PI * this->target_arm_angle;

        const float adist = tmath_adist(a, b) / M_PI;

        this->arm_movement = tmath_adist(a, b) / M_PI;

        this->strength = std::abs(this->arm_movement);

        tms_infof("damage multiplier: %.2f", this->strength);

        G->play_sound(SND_SWISH_HAMMER, this->c->get_position().x, this->c->get_position().y, rand(), 0.65f, false, this->c);

        /* Melee weapons, by default, have no cooldown.
         * However, to make sure they work properly with the AI we need to add
         * a sort of cooldown anyway.
         * the `add_cooldown'-variable is only set by roam_attack. */
        this->cooldown_timer = add_cooldown > 0 ? 300000 : 0;
        this->on_attack();
    }
}

void
robot_parts::base_axe::step()
{
    arm::step(); // important!

    const float realdist    = this->arm_movement * this->swing_speed;
    const float absrealdist = std::abs(realdist);
    const float dist        = realdist * G->get_time_mul();

    this->arm_movement -= dist;
    this->arm_angle += dist;

    if (absrealdist <= 0.001f) {
        return;
    }

    const b2Vec2 arm_pos = ROBOT_ARM_POS;
    float angle = this->c->look_dir * (((this->get_arm_angle() + this->arm_offset) * M_PI));

    b2Vec2 from, to;
    float sn, cs;
    tmath_sincos(angle, &sn, &cs);

    from.x = arm_pos.x*cs - arm_pos.y*sn;
    from.y = arm_pos.x*sn + arm_pos.y*cs;

    from *= -1.f;

    b2Vec2 test(0.f, 1.f);

    to.x = test.x*cs - test.y*sn;
    to.y = test.x*sn + test.y*cs;

    to *= -1.f;

    to += from;

    this->raycast(
            this->c->local_to_world(from, 0),
            this->c->local_to_world(to, 0)
            );
}

void
robot_parts::base_axe::attack(int add_cooldown/*=0*/)
{
    if (this->cooldown_timer <= 0) {
        if (this->c->look_dir == DIR_LEFT) {
            this->target_arm_angle = this->pending_arm_angle - 0.1f;
        } else if (this->c->look_dir == DIR_RIGHT) {
            this->target_arm_angle = this->pending_arm_angle - 0.1f;
        } else {
            return;
        }

        const float a = M_PI * this->arm_angle;
        const float b = M_PI * this->target_arm_angle;

        const float adist = tmath_adist(a, b) / M_PI;

        if (std::abs(adist) < 0.2f) {
            /**
             * TODO: If we perform an attack in a similar angle to las time,
             * perform some offset to the current angle instead of using the diff.
             **/
            this->arm_movement = .75f;
        } else {
            this->arm_movement = tmath_adist(a, b) / M_PI;
        }

        G->play_sound(SND_SWISH_AXE, this->c->get_position().x, this->c->get_position().y, rand(), .55f, false, this->c);

        /* Melee weapons, by default, have no cooldown.
         * However, to make sure they work properly with the AI we need to add
         * a sort of cooldown anyway.
         * the `add_cooldown'-variable is only set by roam_attack. */
        this->cooldown_timer = add_cooldown > 0 ? 300000 : 0;
        this->on_attack();
    }
}

void
robot_parts::base_chainsaw::step()
{
    arm::step(); // important!

    const b2Vec2& pos = this->c->get_position();

    if (this->active) {
        this->arm_angle = this->pending_arm_angle;
    } else if (this->active_timer > 0) {
    }

    if (this->active_timer > 0) {
        if (!this->active) {
            const float sub_mod = 0.98f + (0.01995f * (1.f-G->get_time_mul()));

            this->active_timer *= sub_mod;

            if (this->active_timer <= 1000) {
                this->active_timer = 0;
                sm::stop(&sm::saw_loop, this->c);

                return;
            }
        } else if (this->active_timer <= MAX_CHAINSAW_TIMER) {
            this->active_timer += G->timemul(WORLD_STEP);
        }

        this->dmg_multiplier = this->active_timer / (float)MAX_CHAINSAW_TIMER;

        float dmg = this->get_damage();

        if (dmg < 0.02f) {
            return;
        }

        float vol = (this->active_timer / (float)MAX_CHAINSAW_TIMER);

        G->play_sound(SND_SAW, pos.x, pos.y, 0, vol, true, this->c);

        const std::vector<struct entity_hit> results = this->test_shape();

        for (std::vector<struct entity_hit>::const_iterator it = results.begin();
                it != results.end(); ++it) {
            const struct entity_hit &eh = *it;

            if (eh.e == this->c) {
                // ignore self ;-)
                continue;
            }

            bool ret = G->damage_entity(eh.e, eh.fx, this->get_damage() * G->get_time_mul(), b2Vec2(0,0) /* :S */,
                    DAMAGE_TYPE_FORCE, DAMAGE_SOURCE_BULLET/*??*/, this->c->id,
                    true, false, false, true);

            if (ret) {
                b2Vec2 arm_pos = this->c->local_to_world(ROBOT_ARM_POS, 0);

                float angle = this->c->get_angle() + this->c->look_dir * (((this->get_arm_angle() + this->arm_offset) * M_PI));

                float sn, cs;
                tmath_sincos(angle, &sn, &cs);

                const b2Vec2 look_shape_offset(this->c->look_dir*this->shape_offset.x, this->shape_offset.y);

                arm_pos.x += look_shape_offset.x*cs - look_shape_offset.y*sn;
                arm_pos.y += look_shape_offset.x*sn + look_shape_offset.y*cs;

                if (rand()%10 == 0) {
                    b2Vec2 nearest_point = b2Vec2t(p_entity::get_nearest_point(arm_pos, eh.fx));

                    G->emit(new spark_effect(
                                nearest_point,
                                eh.e->get_layer()
                                ), 0);
                    G->emit(new smoke_effect(
                                nearest_point,
                                eh.e->get_layer(),
                                .5f,
                                .5f
                                ), 0);
                }
            }
        }
    }
}

void
robot_parts::base_chainsaw::attack(int add_cooldown/*=0*/)
{
    if (this->cooldown_timer <= 0) {
        this->arm_angle = this->pending_arm_angle;

        //G->play_sound(SND_SWISH, this->c->get_position().x, this->c->get_position().y, SWISH_BLADE, .4f, false, this->c);

        this->active_timer = 1;

        const b2Vec2 &pos = this->c->get_position();

        /* Melee weapons, by default, have no cooldown.
         * However, to make sure they work properly with the AI we need to add
         * a sort of cooldown anyway.
         * the `add_cooldown'-variable is only set by roam_attack. */
        this->cooldown_timer = add_cooldown > 0 ? 300000 : 0;
        this->on_attack();
    }
}

void
robot_parts::base_spear::step()
{
    arm::step(); // important!

    const b2Vec2& pos = this->c->get_position();

    if (this->active) {
        //this->arm_angle = this->pending_arm_angle;
    } else {
        this->arm_angle = this->pending_arm_angle;
    }

    switch (this->state) {
        case SPEAR_RETRACTING:
            this->state_pos -= this->retract_speed * G->get_time_mul();

            if (this->state_pos < this->min_state_pos) {
                this->state_pos = this->min_state_pos;
                this->state = SPEAR_IDLE;
            }
            break;

        case SPEAR_PROTRACTING:
            this->state_pos += this->protract_speed * G->get_time_mul();

            if (this->state_pos > this->max_state_pos) {
                this->state_pos = this->max_state_pos;

                this->state = SPEAR_FULL_LENGTH;
            }

            {
                b2Vec2 arm_pos = ROBOT_ARM_POS;
                arm_pos.y += this->sting_offset*this->state_pos;
                float angle = this->c->look_dir * (((this->get_arm_angle() + this->arm_offset) * M_PI));

                b2Vec2 from, to;
                float sn, cs;
                tmath_sincos(angle, &sn, &cs);

                from.x = arm_pos.x*cs - arm_pos.y*sn;
                from.y = arm_pos.x*sn + arm_pos.y*cs;

                from *= -1.f;

                b2Vec2 test(0.f, this->sting_length);

                to.x = test.x*cs - test.y*sn;
                to.y = test.x*sn + test.y*cs;

                to *= -1.f;

                to += from;

                this->raycast(
                        this->c->local_to_world(from, 0),
                        this->c->local_to_world(to, 0)
                        );
            }
            break;

        case SPEAR_FULL_LENGTH:
            if (!this->active) {
                this->state = SPEAR_RETRACTING;
            }
            break;

        case SPEAR_IDLE:
            break;
    }
}

void
robot_parts::base_spear::attack(int add_cooldown/*=0*/)
{
    if (this->state == SPEAR_IDLE || this->state == SPEAR_RETRACTING) {
        this->arm_angle = this->pending_arm_angle;

        G->play_sound(SND_SWISH_SPEAR, this->c->get_position().x, this->c->get_position().y, rand(), 1.f, false, this->c);

        this->state = SPEAR_PROTRACTING;

        /* Melee weapons, by default, have no cooldown.
         * However, to make sure they work properly with the AI we need to add
         * a sort of cooldown anyway.
         * the `add_cooldown'-variable is only set by roam_attack. */
        this->cooldown_timer = add_cooldown > 0 ? 300000 : 0;
        this->on_attack();
    }
}

void
robot_parts::base_spear::update()
{
    melee_weapon::update();

    tmat4_translate(this->M, 0.f, -1.f*this->state_pos, 0.f);
}

robot_parts::training_sword::training_sword(creature *c)
    : base_sword(c)
{
    this->set_material(&m_item);
    this->set_mesh(mesh_factory::get_mesh(MODEL_WOODSWORD));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    /* Melee-weapon */
    this->dmg_type = DAMAGE_TYPE_BLUNT;
    this->dmg = 0.125f;
    this->force = 15.f;
    this->mass = 0.25f;
    this->plant_dmg_multiplier = 0.1f;
}

robot_parts::war_hammer::war_hammer(creature *c)
    : base_hammer(c)
{
    this->set_material(&m_item);
    this->set_mesh(mesh_factory::get_mesh(MODEL_HAMMER));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    /* Melee-weapon */
    this->dmg = 1.5f;
    this->force = 70.f;
    this->mass = 1.f;
    this->plant_dmg_multiplier = 0.1f;
    this->block_dmg_multiplier = 1.5f;
}

robot_parts::simple_axe::simple_axe(creature *c)
    : base_axe(c)
{
    this->set_material(&m_item);
    this->set_mesh(mesh_factory::get_mesh(MODEL_SIMPLE_AXE));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    /* Melee-weapon */
    this->dmg = 0.35f;
    this->force = 5.f;
    this->mass = 0.375f;
    this->plant_dmg_multiplier = .8f;
}

robot_parts::chainsaw::chainsaw(creature *c)
    : base_chainsaw(c)
{
    this->set_material(&m_item_shiny);
    this->set_mesh(mesh_factory::get_mesh(MODEL_SAW));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    /* Melee-weapon */
    this->dmg = 0.3f;
    this->force = 2.f;
    this->mass = 0.375f;
    this->plant_dmg_multiplier = 1.2f;

    this->shape = static_cast<b2Shape*>(new b2CircleShape());
    ((b2CircleShape*)this->shape)->m_radius = 0.25f;
    this->shape_offset.Set(-0.1625f, -0.875f);

    tms_entity_init(&this->inner);
    tms_entity_set_mesh(&this->inner, mesh_factory::get_mesh(MODEL_SAW_BLADE));
    tms_entity_set_material(&this->inner, &m_item_shiny);
    tms_entity_set_uniform4f(&this->inner, "~color", 1.f, 1.f, 1.f, 1.f);

    tms_entity_add_child(this, &this->inner);

    /* Chainsaw */
    //this->tick_rate = 10;
}

void
robot_parts::chainsaw::update()
{
    base_chainsaw::update();

    tmat4_copy(this->inner.M, this->M);
    tmat3_copy(this->inner.N, this->N);

    tmat4_translate(this->inner.M, -.445, -.6, -.12);

    this->blade_rot += (this->active_timer / ((float)MAX_CHAINSAW_TIMER/10.f)) * 0.04;

    tmat4_rotate(this->inner.M, this->blade_rot * RADTODEG, -1, 0, 0);

    tmat3_copy_mat4_sub3x3(this->inner.N, this->inner.M);
}

robot_parts::spiked_club::spiked_club(creature *c)
    : base_hammer(c)
{
    this->set_material(&m_item);
    this->set_mesh(mesh_factory::get_mesh(MODEL_SPIKED_CLUB));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    /* Melee-weapon */
    this->dmg = 1.f;
    this->force = 60.f;
    this->mass = 1.f;
    this->plant_dmg_multiplier = 0.1f;
    this->block_dmg_multiplier = 1.5f;
}

robot_parts::steel_sword::steel_sword(creature *c)
    : base_sword(c)
{
    this->set_material(&m_item_shiny);
    this->set_mesh(mesh_factory::get_mesh(MODEL_STEEL_SWORD));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    /* Melee-weapon */
    this->dmg = 0.6f;
    this->force = 30.f;
    this->mass = 0.35f;
    this->plant_dmg_multiplier = 0.5f;
}

robot_parts::baseballbat::baseballbat(creature *c)
    : base_sword(c)
{
    this->set_material(&m_item_shiny);
    this->set_mesh(mesh_factory::get_mesh(MODEL_BASEBALLBAT));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    /* Melee-weapon */
    this->dmg_type = DAMAGE_TYPE_BLUNT;
    this->dmg = 0.15f;
    this->force = 50.f;
    this->mass = 0.35f;
    this->plant_dmg_multiplier = 0.1f;
}

robot_parts::spear::spear(creature *c)
    : base_spear(c)
{
    this->set_material(&m_item_shiny);
    this->set_mesh(mesh_factory::get_mesh(MODEL_SPEAR));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    /* Melee-weapon */
    this->dmg = 1.5f;
    this->force = 90.f;
    this->mass = 0.35f;
    this->plant_dmg_multiplier = 0.1f;
    this->max_range = 2.0f;

    /* Spear */
    this->retract_speed = 0.075f;
    this->protract_speed = 0.175f;
    this->sting_offset = 1.85f;
    this->sting_length = 0.25f;
}

robot_parts::war_axe::war_axe(creature *c)
    : base_axe(c)
{
    this->set_material(&m_item_shiny);
    this->set_mesh(mesh_factory::get_mesh(MODEL_WAR_AXE));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    /* Melee-weapon */
    this->dmg = .9f;
    this->force = 15.f;
    this->mass = 0.5f;
    this->plant_dmg_multiplier = 1.f;
}

robot_parts::pixel_sword::pixel_sword(creature *c)
    : base_sword(c)
{
    this->set_material(&m_item);
    this->set_mesh(mesh_factory::get_mesh(MODEL_PIXEL_SWORD));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    /* Melee-weapon */
    this->dmg_type = DAMAGE_TYPE_BLUNT;
    this->dmg = 0.25f;
    this->force = 15.f;
    this->mass = 0.35f;
    this->plant_dmg_multiplier = 0.2f;
}

robot_parts::serpent_sword::serpent_sword(creature *c)
    : base_sword(c)
{
    this->set_material(&m_item_shiny);
    this->set_mesh(mesh_factory::get_mesh(MODEL_SERPENT_SWORD));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    /* Melee-weapon */
    this->dmg = 0.8f;
    this->force = 35.f;
    this->mass = 0.4f;
    this->plant_dmg_multiplier = 0.9f;
}

robot_parts::pickaxe::pickaxe(creature *c)
    : base_axe(c)
{
    this->set_material(&m_item);
    this->set_mesh(mesh_factory::get_mesh(MODEL_PICKAXE));
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    /* Melee-weapon */
    this->dmg = 0.35f;
    this->force = 5.f;
    this->mass = 0.375f;
    this->plant_dmg_multiplier = .4f;
}
