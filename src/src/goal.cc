#include "goal.hh"
#include "world.hh"
#include "creature.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"
#include "adventure.hh"

goal::goal()
{
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);
    this->set_mesh(mesh_factory::get_mesh(MODEL_CPAD+14));
    this->set_material(&m_cpad);

    this->update_method = ENTITY_UPDATE_STATIC;
}

void
goal::on_touch(b2Fixture *my, b2Fixture *other)
{
    entity *o = (entity*)other->GetUserData();

    if (o && o->is_creature()) {
        creature *c = static_cast<creature*>(o);

        if (c->get_sensor_fixture() == other && (!W->is_adventure() || c == adventure::player)) {
            G->finish(true);
        }
    }
}

void
goal::add_to_world()
{
    b2BodyDef bd;
    bd.type = b2_staticBody;
    bd.position = _pos;
    bd.angle = _angle;

    b2PolygonShape box;
    box.SetAsBox(.5f, .25f);

    b2FixtureDef fd;
    fd.shape = &box;
    fd.density = 1.f;
    fd.friction = .5f;
    fd.restitution = .3f;
    fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

    b2Body *b = W->b2->CreateBody(&bd);
    this->body = b;
    (b->CreateFixture(&fd))->SetUserData(this);

    if (W->is_playing()) {
        b2PolygonShape sensor_shape;
        sensor_shape.SetAsBox(.15f, .15f, b2Vec2(0, .55f), 0);

        b2FixtureDef sfd;
        sfd.shape = &sensor_shape;
        sfd.density = .1f;
        sfd.isSensor = true;
        sfd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        (b->CreateFixture(&sfd))->SetUserData(this);
    }
}
