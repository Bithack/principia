#include "weight.hh"
#include "model.hh"
#include "material.hh"
#include "game.hh"

weight::weight()
{
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);
    this->set_flag(ENTITY_IS_MOVEABLE, true);

    this->set_mesh(mesh_factory::get_mesh(MODEL_WEIGHT));
    this->set_material(&m_weight);
    this->set_uniform("~color", .2f, 0.2f, 0.2f, 1.f);

    this->num_sliders = 1;

    this->set_num_properties(1);
    this->properties[0].type = P_FLT; /* Weight multiplier */
    this->properties[0].v.f = 1.f;

    this->query_vec = b2Vec2(0.f, -0.175f);
    this->query_pt  = b2Vec2(0.f, -0.25f);
}

void
weight::add_to_world()
{
    b2Vec2 verts[4] = {
        b2Vec2(0.2268f, .324f),
        b2Vec2(-0.2268f, .324f),
        b2Vec2(-0.506f, -.324f),
        b2Vec2(0.506f, -.324f),
    };
    b2PolygonShape shape;
    shape.Set(verts, 4);

    b2FixtureDef fd;
    fd.shape = &shape;
    fd.density = this->properties[0].v.f * this->get_material()->density;
    fd.friction = this->get_material()->friction;
    fd.restitution = this->get_material()->restitution;
    fd.filter = world::get_filter_for_layer(this->get_layer(), 6);

    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = this->_pos;
    bd.angle = this->_angle;

    this->body = W->b2->CreateBody(&bd);
    (this->body->CreateFixture(&fd))->SetUserData(this);

    this->width = .25f;
    this->height = .25f;
}

float
weight::get_slider_value(int s)
{
    return this->properties[0].v.f - 0.5f;
}

float
weight::get_slider_snap(int s)
{
    return 1.f / 20.f;
}

void
weight::on_slider_change(int s, float value)
{
    float v = (.5f + value*2);
    this->set_property(0, v);
    G->show_numfeed(v);
}
