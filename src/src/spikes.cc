#include "spikes.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"
#include "robot.hh"
#include "fxemitter.hh"

spikes::spikes()
{
    this->type = ENTITY_PLANK;
    this->menu_scale = .75f;

    this->set_flag(ENTITY_IS_MAGNETIC, true);

    this->set_mesh(mesh_factory::get_mesh(MODEL_SPIKES));
    this->set_material(&m_spikes);

    this->set_num_properties(0);

    this->num_sliders = 1;

    this->set_num_properties(1);

    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 2.f;

    this->layer_mask = 15;
    this->width = .5f;
    this->height = .4f;
    float qw = this->width/2.f + 0.15f;
    float qh = this->height/2.f + 0.15f;
    this->query_sides[0].SetZero(); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].SetZero(); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */
}

void
spikes::add_to_world()
{
    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = _pos;
    bd.angle = _angle;

    b2PolygonShape box;
    box.SetAsBox(.5f, .4f);

    b2FixtureDef fd;
    fd.shape = &box;
    fd.density = this->get_material()->density;
    fd.friction = this->get_material()->friction;
    fd.restitution = (W->level.version >= 26 ? 0.f : this->get_material()->restitution);
    fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

    b2Body *b = W->b2->CreateBody(&bd);
    (this->base_fx = b->CreateFixture(&fd))->SetUserData(this);

    if (!W->is_paused()) {
        b2PolygonShape sensor_shape;
        sensor_shape.SetAsBox(.5f, .5f, b2Vec2(0.f, .00f), 0.f);

        b2FixtureDef sfd;
        sfd.shape = &sensor_shape;
        sfd.density = .0f;
        sfd.isSensor = true;
        sfd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        (b->CreateFixture(&sfd))->SetUserData(this);
    }

    this->body = b;
}

void
spikes::on_touch(b2Fixture *my, b2Fixture *other)
{
    entity *e;
    if (!other->IsSensor() && (e = static_cast<entity*>(other->GetUserData()))) {
        if (e->flag_active(ENTITY_IS_ROBOT)) {
            robot_base *r = static_cast<robot_base*>(e);
            r->damage(this->properties[0].v.f * 100.f, 0, DAMAGE_TYPE_FORCE, DAMAGE_SOURCE_BULLET, this->id);

            b2Vec2 p = e->get_position() + this->get_position();
            p.x *= .5f;
            p.y *= .5f;

            G->emit(new spark_effect(
                        p,
                        e->get_layer()
                        ), 0);
        }
    }
}

void
spikes::on_slider_change(int s, float value)
{
    float v = value * 2.f;
    this->properties[0].v.f = v;

    G->show_numfeed(v * 100.f);
}
