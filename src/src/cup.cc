#include "cup.hh"

#include "material.hh"
#include "model.hh"
#include "world.hh"

cup::cup()
{
    this->width = 1.25f;
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);

    this->menu_scale = 1.f/1.25f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_CUP));
    this->set_material(&m_cup);
    //this->set_uniform("~color", .7f, .7f, 1.f, 1.f);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

void
cup::create_fixtures()
{
    tms_assertf(this->body, "Plastic Cup create fixtures called with no body.");

    b2PolygonShape s_left, s_right, s_bottom;
    s_left.SetAsBox(.05f, .75f, b2Vec2(-.5f, 0), .1f);
    s_right.SetAsBox(.05f, .75f, b2Vec2(.5f, 0), -.1f);
    s_bottom.SetAsBox(.5f, .05f, b2Vec2(0.f, -.75f), 0.f);

    s_left.Scale(this->get_scale());
    s_right.Scale(this->get_scale());
    s_bottom.Scale(this->get_scale());

    b2FixtureDef fd;
    fd.density = 1.f; /* XXX */
    fd.friction = .5f;
    fd.restitution = .5f;

    fd.shape = &s_left;
    (this->body->CreateFixture(&fd))->SetUserData(this);
    fd.shape = &s_right;
    (this->body->CreateFixture(&fd))->SetUserData(this);
    fd.shape = &s_bottom;
    (this->body->CreateFixture(&fd))->SetUserData(this);
}

void
cup::add_to_world()
{
    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = this->_pos;
    bd.angle = this->_angle;

    this->body = W->b2->CreateBody(&bd);

    this->create_fixtures();

    this->set_layer(this->prio);
}
