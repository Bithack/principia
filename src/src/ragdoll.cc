#include "ragdoll.hh"
#include "world.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"

#include <cstdio>
#include <cstdlib>

#define LEGLEN 1.3f
#define ARMLEN 1.f
#define TORSOLEN .95f
#define FRICTION .1f
#define ENABLE_FRICTION true

static b2Vec2 ipos[9] =
{
#if 0
    b2Vec2(0.f, -TORSOLEN/2.f - .2f), /*torso */

    b2Vec2(0.f, -TORSOLEN/2.f - .2f - TORSOLEN/2.f - LEGLEN/4.f),  /* legs */
    b2Vec2(0.f, -TORSOLEN/2.f - .2f - TORSOLEN/2.f - LEGLEN/4.f - LEGLEN/2.f),
    b2Vec2(0.f, -TORSOLEN/2.f - .2f - TORSOLEN/2.f - LEGLEN/4.f),
    b2Vec2(0.f, -TORSOLEN/2.f - .2f - TORSOLEN/2.f - LEGLEN/4.f - LEGLEN/2.f),

    b2Vec2(0.f, -TORSOLEN/2.f - .2f - .1f + ARMLEN/4.f),  /* arms */
    b2Vec2(0.f, -TORSOLEN/2.f - .2f - .1f + ARMLEN/4.f - ARMLEN/2.f),
    b2Vec2(0.f, -TORSOLEN/2.f - .2f - .1f + ARMLEN/4.f),
    b2Vec2(0.f, -TORSOLEN/2.f - .2f - .1f + ARMLEN/4.f - ARMLEN/2.f),
#endif
b2Vec2(0.003423, -0.671770),
b2Vec2(0.281585, -1.358396),
b2Vec2(0.499958, -1.894983),
b2Vec2(-0.022438, -1.465383),
b2Vec2(-0.266360, -2.051900),
b2Vec2(-0.077386, -0.541481),
b2Vec2(-0.146834, -1.034908),
b2Vec2(0.162758, -0.465837),
b2Vec2(0.534898, -0.468301),
};

static float iangle[9] =
{
0.214114,
1.002892,
0.061540,
-0.034719,
-0.462091,
-0.076837,
0.088618,
0.978577,
2.441211,
};

//static float iangle[9] =

void
update_limb(struct tms_entity *e)
{
    struct limb *l = (struct limb*)e;
    ragdoll *r = (ragdoll*)e->parent;

    b2Body *b = *l->body;

    if (b) {
        tmat4_load_identity(e->M);
        b2Transform t;
        t = b->GetTransform();

        e->M[0] = t.q.c;
        e->M[1] = t.q.s;
        e->M[4] = -t.q.s;
        e->M[5] = t.q.c;
        e->M[12] = t.p.x;
        e->M[13] = t.p.y;
        e->M[14] = e->prio * LAYER_DEPTH - LAYER_DEPTH/2.f + LAYER_DEPTH/4.f * (l->sublayer/4.f)*4.f;

        tmat3_copy_mat4_sub3x3(e->N, e->M);

        tmat4_scale(e->M, l->size.x, l->size.y, l->size.z);
    } else {
        tmat4_load_identity(e->M);
        tmat4_translate(e->M,
                r->get_position().x + l->pos.x,
                r->get_position().y + l->pos.y,
                e->prio * LAYER_DEPTH - LAYER_DEPTH/2.f + LAYER_DEPTH/4.f * (l->sublayer/4.f)*4.f);
        tmat4_rotate(e->M, l->angle * (180.f/M_PI), 0, 0, -1.f);
        tmat3_copy_mat4_sub3x3(e->N, e->M);

        tmat4_scale(e->M, l->size.x, l->size.y, l->size.z);
    }
}

uint32_t
ragdoll::get_num_bodies()
{
    return 10;
}

b2Body*
ragdoll::get_body(uint8_t frame)
{
    switch (frame) {
        case 0: return this->body;
        case 1: return this->torso;
        case 2: case 3: case 4: case 5: return this->leg[frame-2];
        case 6: case 7: case 8: case 9: return this->arm[frame-6];
    }

    return 0;
}

void
ragdoll::update(void)
{
    for (int x=0; x<9; x++) {
        update_limb(&this->limbs[x].super);
    }

    entity_fast_update(this);
}

void
ragdoll::on_grab(game *g)
{

}

void
ragdoll::on_release(game *g)
{

}

ragdoll::ragdoll()
{
    this->width = .5f;
    this->menu_pos = b2Vec2(0.f, 1.f);
    this->menu_scale = .5f;

    this->ji = new joint_info(JOINT_TYPE_RAGDOLL, this);
    this->ji->should_destroy = false;

    for (int x=0; x<9; x++) {
        this->joints[x] = 0;
        this->joint_states[x] = true;
    }

    this->num_sliders = 2;

    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->set_mesh(mesh_factory::get_mesh(MODEL_SPHERE));

    this->set_material(&m_wood);
    //this->set_uniform("~color", 1.f, 1.f, 1.f, 1.f);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->torso = 0;
    for (int x=0; x<4; x++) {
        this->arm[x] = 0;
        this->leg[x] = 0;
    }

    this->update_method = ENTITY_UPDATE_CUSTOM;
    this->curr_update_method = ENTITY_UPDATE_CUSTOM;

    for (int x=0; x<9; x++) {
        tms_entity_init((struct tms_entity*)&this->limbs[x]);
        tms_entity_set_mesh((struct tms_entity*)&this->limbs[x], mesh_factory::get_mesh(MODEL_LIMB));
        tms_entity_set_material((struct tms_entity*)&this->limbs[x], &m_colored);
        /* TODO: specific shader */
        tms_entity_set_uniform4f((struct tms_entity*)&this->limbs[x], "~color", 0.7f, 0.7f, 0.9f, 1.f);
        //tms_entity_set_update_fn((struct tms_entity *)&this->limbs[x], update_limb);
        tms_entity_add_child(this, (struct tms_entity *)&this->limbs[x]);
        this->limbs[x].super.prio = 0;
        this->limbs[x].size = (tvec3){.1f, .25f, .1f};
        this->limbs[x].pos = ipos[x];
        this->limbs[x].angle = iangle[x];

        if (x == 0) {
            this->limbs[x].body = &this->torso;
            strcpy(this->limbs[x].name, "Torso");
        } else if (x < 5) {
            this->limbs[x].body = &this->leg[x-1];

            sprintf(this->limbs[x].name, "Leg %d", x);
        } else {
            this->limbs[x].body = &this->arm[x-5];

            sprintf(this->limbs[x].name, "Arm %d", x-4);
        }
    }

    this->limbs[0].sublayer = 2.f;
    this->limbs[0].size = (tvec3){1.5f, TORSOLEN, 1.5f};

    this->limbs[1].sublayer = 1.f;
    this->limbs[1].size = (tvec3){1.1f, LEGLEN/2.f, 1.1f};
    this->limbs[2].sublayer = 1.f;
    this->limbs[2].size = (tvec3){1.f, LEGLEN/2.f, 1.f};
    this->limbs[3].sublayer = 2.f;
    this->limbs[3].size = (tvec3){1.1f, LEGLEN/2.f, 1.1f};
    this->limbs[4].sublayer = 2.f;
    this->limbs[4].size = (tvec3){1.f, LEGLEN/2.f, 1.f};

    this->limbs[5].sublayer = 0.f;
    this->limbs[5].size = (tvec3){.9f, ARMLEN/2.f, .9f};
    this->limbs[6].sublayer = 0.f;
    this->limbs[6].size = (tvec3){.9f, ARMLEN/2.f, .9f};
    this->limbs[7].sublayer = 3.f;
    this->limbs[7].size = (tvec3){.9f, ARMLEN/2.f, .9f};
    this->limbs[8].sublayer = 3.f;
    this->limbs[8].size = (tvec3){.9f, ARMLEN/2.f, .9f};

    this->set_num_properties(9*3+2);
    for (int x=0; x<9; x++) {
        this->properties[x*3].type = P_FLT;
        this->properties[x*3].v.f = ipos[x].x;

        this->properties[x*3+1].type = P_FLT;
        this->properties[x*3+1].v.f = ipos[x].y;

        this->properties[x*3+2].type = P_FLT;
        this->properties[x*3+2].v.f = 0.f;
    }
    this->properties[9*3].type = P_FLT; /* Durability */
    this->properties[9*3].v.f = 50.f;    /* 1 - 100, 100 being indestructible */
    this->properties[9*3+1].type = P_INT; /* Head size */
    this->properties[9*3+1].v.i = 0;  /* 1 - 4, 4 being largest */

    this->destructable_joints.clear();
}

ragdoll::~ragdoll()
{
    for (int x=0; x<9; x++) {
        tms_entity_uninit(&this->limbs[x].super);
    }
}

void
ragdoll::set_layer(int z)
{
    struct tms_scene *scene = this->scene;
    int head_layer = 6;

    switch (this->properties[9*3+1].v.i) {
        case 2: head_layer = 6; break;
        case 3: head_layer = 15; break;
        case 4: head_layer = 15; break;
        default: case 1: head_layer = 6; break;
    }

    if (scene)
        tms_scene_remove_entity(scene, this);

    this->prio = z;

    for (int x=0; x<9; x++) {
        this->limbs[x].super.prio = z;
    }

    if (this->body) {
        this->body->GetFixtureList()->SetFilterData(world::get_filter_for_layer(this->prio, head_layer));
        this->torso->GetFixtureList()->SetFilterData(world::get_filter_for_layer(this->prio, 6));
        for (int x=0; x<2; x++) {
            for (int y=0; y<2; y++) {
                this->arm[x*2+y]->GetFixtureList()->SetFilterData(world::get_filter_for_layer(this->prio,
                            x == 0 ? 1 : 8));
                this->leg[x*2+y]->GetFixtureList()->SetFilterData(world::get_filter_for_layer(this->prio,
                            x == 0 ? 2 : 4));
            }

        }
    }

    if (scene)
        tms_scene_add_entity(scene, this);
}

b2Vec2
ragdoll::get_limb_pos(int n)
{
    return b2Vec2(this->properties[n*3].v.f, this->properties[n*3+1].v.f) + this->get_position();
}

float
ragdoll::get_limb_angle(int n)
{
    return this->properties[n*3+2].v.f + this->get_angle();
}

void
ragdoll::pre_write(void)
{
    for (int x=0; x<9; x++) {
        b2Vec2 p = (*this->limbs[x].body)->GetPosition() - this->get_position();
        float a = (*this->limbs[x].body)->GetAngle() - this->get_angle();
        this->properties[x*3+0].v.f = p.x;
        this->properties[x*3+1].v.f = p.y;
        this->properties[x*3+2].v.f = a;
    }

    entity::pre_write();
}

void
ragdoll::add_to_world()
{
    {
        /* create torso */
        b2BodyDef bd;
        bd.type = this->get_dynamic_type();
        bd.position = this->get_limb_pos(0);
        bd.angle = this->get_limb_angle(0);

        b2PolygonShape shape;
        shape.SetAsBox(.125f, TORSOLEN/2.f);

        b2FixtureDef fd;
        fd.shape = &shape;
        fd.friction = m_wood.friction;
        fd.density = m_wood.density;
        fd.restitution = m_wood.restitution;
        fd.filter = world::get_filter_for_layer(0, 6);

        this->torso = W->b2->CreateBody(&bd);
        this->torso->CreateFixture(&fd)->SetUserData(this);
        this->torso->SetUserData((void*)1);
    }

    /* create legs */
    for (int x=0; x<2; x++) {
        for (int y=0; y<2; y++) {

            //b2Vec2 p = b2Vec2(0.f, -.25f*y);

            b2BodyDef bd;
            bd.type = this->get_dynamic_type();
            bd.position = this->get_limb_pos(1+x*2+y);
            bd.angle = this->get_limb_angle(1+x*2+y);

            b2PolygonShape shape;
            shape.SetAsBox(.125f, LEGLEN/4.f);

            b2FixtureDef fd;
            fd.shape = &shape;
            fd.friction = m_wood.friction;
            fd.density = m_wood.density;
            fd.restitution = m_wood.restitution;
            fd.filter = world::get_filter_for_layer(0, x == 0 ? 2 : 4);

            this->leg[x*2+y] = W->b2->CreateBody(&bd);
            this->leg[x*2+y]->CreateFixture(&fd)->SetUserData(this);
            this->leg[x*2+y]->SetUserData((void*)(intptr_t)(2+x*2+y));
        }

        /* connect leg parts */
        if (this->joint_states[1+x*2]) {
            b2RevoluteJointDef rjd;
            rjd.referenceAngle = 0.f;
            rjd.collideConnected = false;
            rjd.enableLimit = true;
            rjd.lowerAngle = -2.5f;
            rjd.upperAngle = -0.1f;
            rjd.enableMotor = ENABLE_FRICTION;
            rjd.maxMotorTorque = FRICTION;
            rjd.bodyA = this->leg[x*2+0];
            rjd.bodyB = this->leg[x*2+1];
            rjd.localAnchorA = b2Vec2(0, -LEGLEN/4.f);
            rjd.localAnchorB = b2Vec2(0, LEGLEN/4.f);
            this->joints[1+x*2] = W->b2->CreateJoint(&rjd);
            this->joints[1+x*2]->SetUserData(this->ji);
            this->destructable_joints.insert(this->joints[1+x*2]);
        } else
            this->joints[1+x*2] = 0;

        /* connect leg to torso */
        if (this->joint_states[1+x*2+1]) {
            b2RevoluteJointDef rjd;
            rjd.referenceAngle = 0.f;
            rjd.collideConnected = false;
            rjd.enableLimit = true;
            rjd.lowerAngle = -.5f;
            rjd.upperAngle = 2.5f;
            rjd.enableMotor = ENABLE_FRICTION;
            rjd.maxMotorTorque = FRICTION;
            rjd.bodyA = this->torso;
            rjd.bodyB = this->leg[x*2+0];
            rjd.localAnchorA = b2Vec2(0, -TORSOLEN/2.f);
            rjd.localAnchorB = b2Vec2(0, LEGLEN/4.f);
            this->joints[1+x*2+1] = W->b2->CreateJoint(&rjd);
            this->joints[1+x*2+1]->SetUserData(this->ji);
            this->destructable_joints.insert(this->joints[1+x*2+1]);
        } else
            this->joints[1+x*2+1] = 0;
    }

    /* create arms */
    for (int x=0; x<2; x++) {
        for (int y=0; y<2; y++) {

            //b2Vec2 p = b2Vec2(0.f, -.25f*y);

            b2BodyDef bd;
            bd.type = this->get_dynamic_type();
            bd.position = this->get_limb_pos(5+x*2+y);
            bd.angle = this->get_limb_angle(5+x*2+y);

            b2PolygonShape shape;
            shape.SetAsBox(.125f, ARMLEN/4.f);

            b2FixtureDef fd;
            fd.shape = &shape;
            fd.friction = m_wood.friction;
            fd.density = m_wood.density;
            fd.restitution = m_wood.restitution;
            fd.filter = world::get_filter_for_layer(0, x == 0 ? 1 : 8);

            this->arm[x*2+y] = W->b2->CreateBody(&bd);
            this->arm[x*2+y]->CreateFixture(&fd)->SetUserData(this);
            this->arm[x*2+y]->SetUserData((void*)(intptr_t)(6+x*2+y));
        }

        /* connect arm parts */
        if (this->joint_states[5+x*2]) {
            b2RevoluteJointDef rjd;
            rjd.referenceAngle = 0.f;
            rjd.collideConnected = false;
            rjd.enableLimit = true;
            rjd.lowerAngle = 0.2f;
            rjd.upperAngle = 2.5f;
            rjd.enableMotor = ENABLE_FRICTION;
            rjd.maxMotorTorque = FRICTION;
            rjd.bodyA = this->arm[x*2+0];
            rjd.bodyB = this->arm[x*2+1];
            rjd.localAnchorA = b2Vec2(0, -ARMLEN/4.f);
            rjd.localAnchorB = b2Vec2(0, ARMLEN/4.f);
            this->joints[5+x*2] = W->b2->CreateJoint(&rjd);
            this->joints[5+x*2]->SetUserData(this->ji);
            this->destructable_joints.insert(this->joints[5+x*2]);
        } else
            this->joints[5+x*2] = 0;

        /* connect arm to torso */
        if (this->joint_states[5+x*2+1]) {
            b2RevoluteJointDef rjd;
            rjd.referenceAngle = 0.f;
            rjd.collideConnected = false;
            rjd.enableLimit = true;
            rjd.lowerAngle = -3.14f/2.f;
            rjd.upperAngle = 3.14f;
            rjd.enableMotor = ENABLE_FRICTION;
            rjd.maxMotorTorque = FRICTION;
            rjd.bodyA = this->torso;
            rjd.bodyB = this->arm[x*2+0];
            rjd.localAnchorA = b2Vec2(0, TORSOLEN/2.f - .1f);
            rjd.localAnchorB = b2Vec2(0, ARMLEN/4.f);
            this->joints[5+x*2+1] = W->b2->CreateJoint(&rjd);
            this->joints[5+x*2+1]->SetUserData(this->ji);
            this->destructable_joints.insert(this->joints[5+x*2+1]);
        } else
            this->joints[5+x*2+1] = 0;
    }

    this->recreate_head();
    this->recreate_head_joint(false);

    this->set_layer(this->prio);
}

void
ragdoll::remove_from_world()
{
    for (int x=0; x<9; x++) {
        /* unset joint userdata so the destruction listener wont act as if the joint is destroyed dynamically */
        if (this->joints[x]) {
            this->joints[x]->SetUserData(0);
            this->joints[x] = 0;
        }
    }

    if (this->torso)
        W->b2->DestroyBody(this->torso);

    this->torso = 0;

    for (int x=0; x<4; ++x) {
        if (this->arm[x])
            W->b2->DestroyBody(this->arm[x]);
        if (this->leg[x])
            W->b2->DestroyBody(this->leg[x]);

        this->arm[x] = 0;
        this->leg[x] = 0;
    }

    entity::remove_from_world();

    std::set<b2Joint*>::iterator it;

    /* Remove any traces of the previously desctructable joints */
    for (it = this->destructable_joints.begin(); it != this->destructable_joints.end(); ++it) {
        W->destructable_joints.erase(*it);
    }

    this->destructable_joints.clear();
}

void
ragdoll::on_pause()
{
    this->destructable_joints.clear();
}

void
ragdoll::setup()
{
    /* XXX: These variables require tweaking */
    float base_max_force = 5500.f;
    float durability_multiplier = 0.02f;

    if (this->properties[9*3].v.f < 100.f) {
        /* TODO: max force needs to scale with durability */
        for (std::set<b2Joint*>::iterator i = this->destructable_joints.begin();
                i != this->destructable_joints.end(); i++)
            G->add_destructable_joint(*i,
                    base_max_force * (this->properties[9*3].v.f * durability_multiplier));
    }
}

void
ragdoll::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    lb->ensure(3*9*sizeof(float) + 9*sizeof(uint8_t));

    tms_debugf("ragdoll write state");

    for (int x=0; x<9; x++) {
        b2Body *b = this->get_body(x);
        b2Vec2 velocity = b ? b->GetLinearVelocity() : b2Vec2(0.f, 0.f);
        float avel = b ? b->GetAngularVelocity() : 0.f;
        lb->w_float(velocity.x);
        lb->w_float(velocity.y);
        lb->w_float(avel);

        lb->w_uint8(this->joints[x] ? 1 : 0);
        tms_debugf("write is destr: %p", this->joints[x]);
    }
}

void
ragdoll::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    for (int x=0; x<9; x++) {
        b2Body *b = this->get_body(x);
        b2Vec2 velocity = b ? b->GetLinearVelocity() : b2Vec2(0.f, 0.f);
        this->body_states[x][0] = lb->r_float();
        this->body_states[x][1] = lb->r_float();
        this->body_states[x][2] = lb->r_float();
        this->joint_states[x] = lb->r_uint8() ? true : false;
    }
}

void
ragdoll::restore()
{
    this->setup();

    for (int x=0; x<9; x++) {
        b2Body *b = this->get_body(x);

        if (b) {
            b->SetAngularVelocity(this->body_states[x][2]);
            b->SetLinearVelocity(b2Vec2(this->body_states[x][0], this->body_states[x][1]));
        }
    }
}

void
ragdoll::set_position(float x, float y, uint8_t frame)
{
    b2Body *b = this->get_body(frame);
    if (b) {
        b->SetTransform(b2Vec2(x,y), b->GetAngle());
    } else {
        this->_pos = b2Vec2(x, y);
    }
}

b2Vec2
ragdoll::local_to_world(b2Vec2 p, uint8_t frame)
{
    return this->get_body(frame)->GetWorldPoint(p);
}

void
ragdoll::recreate_head()
{
    uint32_t head_size = this->properties[9*3+1].v.i;
    if (head_size > 1) head_size = 1;
    b2FixtureDef fd;
    this->layer_mask = 15;

    fd.filter = world::get_filter_for_layer(this->prio, 2+4);
    this->set_mesh(mesh_factory::get_mesh(MODEL_SPHERE+head_size));

    if (head_size == 0) {
        this->layer_mask = 1;
    } else {
        this->layer_mask = 15;
    }

    /* XXX: a more appropriate head size might be .125f + (head_size * 0.125f)? */
    this->head_shape.m_radius = .25f * (1.f+head_size);

    if (!this->body) {
        b2BodyDef bd;
        bd.type = this->get_dynamic_type();
        bd.position = this->_pos;
        bd.angle = this->_angle;
        this->body = W->b2->CreateBody(&bd);
    } else {
        while (this->body->GetFixtureList()) {
            this->body->DestroyFixture(this->body->GetFixtureList());
        }
    }

    fd.shape = &this->head_shape;
    fd.friction = m_wood.friction;
    fd.density = m_wood.density;
    fd.restitution = m_wood.restitution;

    this->body->CreateFixture(&fd)->SetUserData(this);
}

void
ragdoll::recreate_head_joint(bool destroy)
{
    if (!this->torso) return;

    if (destroy && this->joints[0]) {
        //return;
        W->b2->DestroyJoint(this->joints[0]);
        this->joints[0] = 0;
    }

    if (this->joint_states[0]) {
        /* Connect head with torso */
        b2RevoluteJointDef rjd;
        rjd.referenceAngle = 0.f;
        rjd.collideConnected = false;
        rjd.enableLimit = true;
        rjd.lowerAngle = -.8f;
        rjd.upperAngle = .3f;
        rjd.enableMotor = ENABLE_FRICTION;
        rjd.maxMotorTorque = FRICTION;
        rjd.bodyA = this->body;
        rjd.bodyB = this->torso;
        rjd.localAnchorA = b2Vec2(0, -.25f*(this->properties[9*3+1].v.i+1.f));
        rjd.localAnchorB = b2Vec2(0, TORSOLEN/2.f);
        this->joints[0] = W->b2->CreateJoint(&rjd);
        this->joints[0]->SetUserData(this->ji);
    } else
        this->joints[0] = 0;
}

float
ragdoll::get_slider_snap(int s)
{
    switch (s) {
        case 0: return 1.f / 99.f;
        case 1: return 1.f;
    }
    return 0.f;
}

float
ragdoll::get_slider_value(int s)
{
    switch (s) {
        case 0:
            {
                float v = this->properties[9*3].v.f - 1.f;
                return v / 99.f;
            }
        case 1:
            {
                float v = this->properties[9*3+1].v.i;
                if (v < 0.f) v = 0.f;
                if (v > 1.f) v = 1.f;
                return v;
            }
    }

    return 0.f;
}

void
ragdoll::on_slider_change(int s, float value)
{
    if (s == 0) {
        float durability = (value * 99.f) + 1.f;
        this->properties[9*3].v.f = durability;
        G->show_numfeed(durability);
    } else if (s == 1) {
        uint32_t size = (uint32_t)value;
        this->properties[9*3+1].v.i = size;

        this->recreate_head();
        this->recreate_head_joint(true);
    }
}
