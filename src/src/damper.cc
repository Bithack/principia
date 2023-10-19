#include "damper.hh"
#include "material.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"
#include "object_factory.hh"

damper::damper()
{
    this->width = .5f;

    this->c.init_owned(0, this);
    this->c.type = CONN_GROUP;
    this->c.angle = M_PI/2.f;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

void damper::on_grab(game *g){};
void damper::on_release(game *g){};

damper_1::damper_1()
{
    this->dir = 1.f;
    this->menu_scale = .5f;
    this->menu_pos = b2Vec2(0.f, .5f);
    this->joint = 0;
    this->set_flag(ENTITY_DO_STEP, true);

    this->num_sliders = 2;

    this->set_mesh(mesh_factory::get_mesh(MODEL_DAMPER_0));
    this->set_material(&m_metal);

    this->dconn.init_owned(1, this);
    this->dconn.fixed = true;
    this->dconn.type = CONN_CUSTOM;

    this->set_num_properties(3);

    /* translation of the second body */
    /* TODO: remove */
    this->properties[0].type = P_FLT;
    this->set_property(0, 0.f);

    /* spring force coefficient */
    this->properties[1].type = P_FLT;
    this->set_property(1, 10.f);

    /* spring speed */
    this->properties[2].type = P_FLT;
    this->set_property(2, 2.f);

    this->set_as_rect(.125f, .5f);
}

damper_1::~damper_1()
{
    /*
    if (this->dconn.o) {
        ((damper_2*)this->dconn.o)->d1 = 0;
    }
    */
}

damper_2::damper_2()
{
    this->d1 = 0;
    this->dir = -1.f;
    this->set_mesh(mesh_factory::get_mesh(MODEL_DAMPER_1));
    this->set_material(&m_colored);
    this->set_uniform("~color", .18f, .18f, .18f, 1.f);

    this->set_as_rect(.125f, .5f);
}

void
damper_1::update_frame(bool hard)
{
    if (hard) this->dconn.j = 0;
    if (this->dconn.o) this->dconn.create_joint(0);
}

void
damper_2::update_frame(bool hard)
{
    if (this->d1) {
        this->d1->update_frame(hard);
    }
}

/* create the prismatic connection */
void
damper_1::connection_create_joint(connection *c)
{
    if (c == &this->dconn) {
        ((damper_2*)c->o)->d1 = this;

        b2PrismaticJointDef rjd;
        if (W->level.version >= 26 && !W->is_paused())
            rjd.tolerance = W->level.prismatic_tolerance;
        rjd.collideConnected = false;
        rjd.bodyA = c->e->get_body(0);
        rjd.bodyB = c->o->get_body(0);
        rjd.enableLimit = true;
        rjd.lowerTranslation = .25f;
        rjd.upperTranslation = 1.f;
        //rjd.localAxisA = b2Vec2(0.f, -1.f);
        rjd.localAxisA = c->e->local_vector_to_body(b2Vec2(0.f, -1.f), 0);
        rjd.localAnchorA = c->e->local_to_body(b2Vec2(0.f, .0f), 0);
        rjd.localAnchorB = c->o->local_to_body(b2Vec2(0.f, .0f),0);

        rjd.referenceAngle = (rjd.bodyB->GetAngle() - rjd.bodyA->GetAngle())
            - (c->o->get_angle() - c->e->get_angle());

        if (!W->is_paused()) {
            rjd.enableMotor = true;
            rjd.maxMotorForce = 100.f;
            rjd.motorSpeed = 0.f;
        }

        c->j = W->b2->CreateJoint(&rjd);
    }
}

void
damper_1::set_moveable(bool moveable)
{
    tms_infof("damper set moveable");
    this->set_flag(ENTITY_IS_MOVEABLE, moveable);
    entity *other = this->dconn.get_other(this);
    if (other) {
        other->set_moveable(moveable);
    }
    tms_infof("other: %p", other);
    //(this->dconn.get_other(this))->set_moveable(moveable);
}

void
damper_1::construct()
{
    damper_2 *d2 = (damper_2*)of::create(67);

    d2->_pos = this->_pos;
    d2->_pos -= b2Vec2(0.f, 1.f);
    W->add(d2);
    G->add_entity(d2);

    this->dconn.o = d2;
    this->dconn.fixed = true;

    G->apply_connection(&this->dconn, -1);
}

bool
damper::ReportFixture(b2Fixture *f)
{
    if (f->IsSensor()) {
        return true;
    }

    entity *e = (entity*)f->GetUserData();
    b2Body *b = f->GetBody();

    if (e && e->allow_connections() && abs(e->get_layer() - this->get_layer()) == 1
        && f->TestPoint(this->query_point)) {
        this->query_result = e;
        this->query_result_fx = f;
        this->query_frame = (uint8_t)(uintptr_t)b->GetUserData();
        return false;
    }

    return true;
}

float32
damper::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    b2Body *b = f->GetBody();
    entity *e = (entity*)f->GetUserData();

    if (e && e->allow_connections() && e != this
        && e->get_layer() == this->get_layer()) {
        this->query_result = e;
        this->query_fraction = fraction;
        this->query_result_fx = f;
        this->query_frame = (uint8_t)(uintptr_t)b->GetUserData();
    }

    return -1;
}

void
damper_1::set_layer(int z)
{
    if (this->body) {
        if (this->dconn.o) this->dconn.o->entity::set_layer(z);
    }
    entity::set_layer(z);
}

void
damper_2::set_layer(int z)
{
    if (this->body) {
        if (this->d1) this->d1->set_layer(z);
    } else
        entity::set_layer(z);
}

void
damper::find_pairs()
{
    if (this->c.pending) {
        this->query_result = 0;
        this->query_frame = 0;

        float h = .8f*this->dir;

        W->b2->RayCast(this,
                this->local_to_world(b2Vec2(0.f, h/2.f), 0),
                this->local_to_world(b2Vec2(0, h), 0));

        if (this->query_result) {
            this->c.o = this->query_result;
            this->c.f[0] = 0;
            this->c.f[1] = this->query_frame;
            this->c.tolerant = false;
            this->c.o_data = this->query_result->get_fixture_connection_data(this->query_result_fx);
            b2Vec2 vv = b2Vec2(0.f, h/2.f);
            vv *= this->query_fraction;
            vv.y += h/2.f;
            this->c.p = this->local_to_world(vv, 0);

            /* XXX */
            //this->c[x].p = this->local_to_world(b2Vec2(0.f, 0.f), x);

            G->add_pair(this, this->query_result, &this->c);
        } else {
            this->query_point = this->local_to_world(b2Vec2(0.f, h*.5f), 0);
            b2AABB aabb;
            aabb.lowerBound.Set(this->query_point.x - .05f, this->query_point.y - .05f);
            aabb.upperBound.Set(this->query_point.x + .05f, this->query_point.y + .05f);
            W->b2->QueryAABB(this, aabb);

            if (this->query_result) {
                this->c.o = this->query_result;
                this->c.f[0] = 0;
                this->c.f[1] = this->query_frame;
                this->c.p = this->query_point;
                this->c.o_data = this->query_result->get_fixture_connection_data(this->query_result_fx);
                //this->c[x].p = this->local_to_world(b2Vec2(0.f, 0.f), x);
                //this->c[x].tolerant = true;
                G->add_pair(this, this->query_result, &this->c);
            }
        }
    }
}

connection *
damper::load_connection(connection &conn)
{
    if (conn.o_index < 1) {
        this->c = conn;
    //    this->c[conn.o_index].tolerant = (conn.type == CONN_WELD);
        this->c.tolerant = false;
        return &this->c;
    }

    return 0;
}

connection *
damper_1::load_connection(connection &conn)
{
    if (conn.o_index == 1) {
        this->dconn = conn;
        this->dconn.fixed = true;
        damper_2 *d2 = (damper_2*)conn.o;
        d2->d1 = this;

        return &this->dconn;
    }

    return damper::load_connection(conn);
}

void damper_1::on_slider_change(int s, float value)
{
    this->properties[1+s].v.f=value * 20.f;
    G->show_numfeed(this->properties[1+s].v.f * (s == 0 ? 120.f : .5f));
}

void damper_1::step(void)
{
    if (this->dconn.j) {
        b2PrismaticJoint *j = (b2PrismaticJoint*)this->dconn.j;
        float offs = j->GetJointTranslation();

        offs -= .25f;
        offs *= 1.f/.75f;

        offs = 1.f - tclampf(offs, 0.f, 1.f);

        j->SetMaxMotorForce(this->properties[1].v.f*120.f/4.f + 120.f * offs * this->properties[1].v.f);
        j->SetMotorSpeed(.5f * this->properties[2].v.f * offs);
    }
}

