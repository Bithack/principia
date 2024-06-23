#include "angulardamper.hh"
#include "material.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"
#include "object_factory.hh"

angulardamper::angulardamper()
{
    this->set_flag(ENTITY_IS_DEV, true);
    this->set_flag(ENTITY_DO_STEP, true);

    this->width = .5f;
    this->dir = 1.f;
    this->joint = 0;

    this->num_sliders = 2;

    this->set_mesh(mesh_factory::get_mesh(MODEL_ADAMPER));
    this->set_material(&m_heavyedev);

    this->c.init_owned(0, this);
    this->c.type = CONN_CUSTOM;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->set_num_properties(2);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 1.f;
    this->properties[1].type = P_FLT;
    this->properties[1].v.f = 100.f;

    this->layer_mask = 9;
    this->set_as_circle(.5f);
}

bool
angulardamper::ReportFixture(b2Fixture *f)
{
    if (f->IsSensor()) {
        return true;
    }

    entity *e = (entity*)f->GetUserData();
    b2Body *b = f->GetBody();

    if (e && e != this && e->allow_connections()
          && this->get_layer() == e->get_layer()
          && f->TestPoint(this->query_point)) {
        this->query_result = e;
        this->query_frame = (uint8_t)(uintptr_t)b->GetUserData();
        return false;
    }

    return true;
}

float32
angulardamper::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    b2Body *b = f->GetBody();
    entity *e = static_cast<entity*>(f->GetUserData());

    if (e && e != this && e->allow_connections() && this->get_layer() == e->get_layer()) {
        this->query_result = e;
        this->query_frame = (uint8_t)(uintptr_t)b->GetUserData();
        this->query_fraction = fraction;
    }

    return -1;
}

//void angulardamper::on_grab(game *g){};
//void angulardamper::on_release(game *g){};

void
angulardamper::find_pairs()
{
    if (this->c.pending && this->body) {
        b2Vec2 p;
        this->query_point = p = this->local_to_world(b2Vec2(0.f, .0f), 0);
        this->query_result = 0;
        this->query_frame = 0;

        b2AABB aabb;
        aabb.lowerBound.Set(p.x - .05f, p.y - .05f);
        aabb.upperBound.Set(p.x + .05f, p.y + .05f);

        W->b2->QueryAABB(this, aabb);

        if (this->query_result != 0) {
            this->c.o = this->query_result;
            this->c.f[0] = 0;
            this->c.f[1] = this->query_frame;
            this->c.p = p;
            G->add_pair(this, this->query_result, &this->c);
        }
    }
}

connection *
angulardamper::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->layer_mask = 15;
        this->c = conn;
        if (this->fx)
            this->fx->SetFilterData(world::get_filter_for_layer(this->get_layer(), 15));
        return &this->c;
    }

    return 0;
}

void
angulardamper::update_frame(bool hard)
{
    this->c.j = 0;
    this->c.create_joint(0);
}

void
angulardamper::step()
{
    b2RevoluteJoint *j = static_cast<b2RevoluteJoint*>(this->c.j);

    if (!j)
        return;

    float c_angle = j->GetJointAngle();
    float a = 0.f;
    float dist = tmath_adist(c_angle, a);

    float speed = dist * this->properties[0].v.f;
    float torque = this->properties[1].v.f;

    j->SetMotorSpeed(speed);
    j->SetMaxMotorTorque(torque);
    j->EnableMotor(true);
}

bool
angulardamper::connection_destroy_joint(connection *c)
{
    this->layer_mask = 9;

    if (this->fx)
        this->fx->SetFilterData(world::get_filter_for_layer(this->get_layer(), 9));
    else
        tms_infof("no fx");

    return false;
}

void
angulardamper::connection_create_joint(connection *c)
{
    if (c == &this->c) {
        b2RevoluteJointDef rjd;
        rjd.collideConnected = false;
        float aA = c->e->get_body(c->f[0])->GetAngle();
        float aB = c->o->get_body(c->f[1])->GetAngle();
        rjd.referenceAngle = aB - aA;
        tms_infof("reference angle: %.3f", rjd.referenceAngle);
        rjd.bodyA = c->e->get_body(c->f[0]);
        rjd.bodyB = c->o->get_body(c->f[1]);

        rjd.enableLimit = false;
        rjd.localAnchorA = c->e->local_to_body(b2Vec2(0.f, .0f), c->f[0]);
        rjd.localAnchorB = c->o->world_to_body(this->get_position(), c->f[1]);

        if (!W->is_paused()) {
            rjd.maxMotorTorque = 0.f;
            rjd.motorSpeed = 0.f;
            rjd.enableMotor = true;
        }

        tms_infof("%p", c->j);

        if (c->e->get_body(c->f[0]) != c->o->get_body(c->f[1]))
            c->j = W->b2->CreateJoint(&rjd);
        else
            c->j = 0;
    } else {
        tms_infof("another connection was asdasd");
    }
}

void
angulardamper::on_slider_change(int s, float value)
{
    float v;
    if (s == 0) {
        v = (value / 5.f) * 9.f + 0.2f;
    } else if (s == 1) {
        v = (value * 40.f) * 18.f + 40.f;
    }

    if (s == 0 || s == 1) {
        this->set_property(s, v);
        G->show_numfeed(v);
    }
}

