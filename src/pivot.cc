#include "pivot.hh"
#include "model.hh"
#include "material.hh"
#include "world.hh"
#include "game.hh"
#include "object_factory.hh"

pivot::pivot()
{
    this->width = 4.f;

    this->set_flag(ENTITY_IS_MAGNETIC, true);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

void pivot::on_grab(game *g){};
void pivot::on_release(game *g){};

pivot_1::pivot_1()
{
    this->query_vec = b2Vec2(0.f, -1.f);
    this->menu_scale = 1.f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_PIVOT));
    this->set_material(&m_metal);

    this->dconn.init_owned(1, this);
    this->dconn.fixed = true;
    this->dconn.type = CONN_CUSTOM;

    this->set_as_rect(.25f, .25f);
}

pivot_2::pivot_2()
{
    this->p1 = 0;
    this->query_vec = b2Vec2(0.f, 1.f);
    /* TODO: use proper matierla */
    this->set_mesh(mesh_factory::get_mesh(MODEL_ANCHOR));
    this->set_material(&m_colored);
    this->set_uniform("~color", .18f, .18f, .18f, 1.f);

    this->set_as_rect(.25f, .25f);
}

void
pivot_1::update_frame(bool hard)
{
    if (hard) this->dconn.j = 0;
    if (this->dconn.o) this->dconn.create_joint(0);
}

void
pivot_2::update_frame(bool hard)
{
    if (this->p1) {
        this->p1->update_frame(hard);
    }
}

void
pivot_1::set_layer(int z)
{
    if (this->body) {
        if (this->dconn.o) this->dconn.o->entity::set_layer(z);
    }
    entity::set_layer(z);
}

void
pivot_2::set_layer(int z)
{
    if (this->body) {
        if (this->p1) this->p1->set_layer(z);
    } else
        entity::set_layer(z);
}

connection *
pivot_1::load_connection(connection &conn)
{
    if (conn.o_index == 1) {
        tms_infof("loaded dconn");
        this->dconn = conn;
        this->dconn.fixed = true;
        return &this->dconn;
    }

    return composable_simpleconnect::load_connection(conn);
}

void
pivot_1::connection_create_joint(connection *c)
{
    if (c == &this->dconn) {
        ((pivot_2*)c->o)->p1 = this;

        b2PivotJointDef rjd;
        rjd.bodyA = c->e->get_body(0);
        rjd.bodyB = c->o->get_body(0);
        rjd.tolerance = .05f;

        rjd.collideConnected = (c->e->body == 0 || c->o->body == 0) ;

        rjd.enableLimit = true;
        rjd.lowerAngle = -M_PI/1.5f;
        rjd.upperAngle = M_PI/1.5f;
        rjd.localAnchorA = c->e->local_to_body(b2Vec2(0.f, .5f), 0);
        rjd.localAnchorB = c->o->local_to_body(b2Vec2(0.f, -.5f), 0);
        /* TODO: reference angle? */
        rjd.referenceAngle = (rjd.bodyB->GetAngle() - rjd.bodyA->GetAngle())
            - (c->o->get_angle() - c->e->get_angle());
        c->j = W->b2->CreateJoint(&rjd);
    }
}

void
pivot_1::construct()
{
    pivot_2 *p2 = (pivot_2*)of::create(69);

    p2->_pos = this->_pos;
    p2->_pos -= b2Vec2(0.f, -1.f);

    W->add(p2);
    G->add_entity(p2);

    this->dconn.o = p2;
    this->dconn.fixed = true;

    G->apply_connection(&this->dconn, -1);
}

