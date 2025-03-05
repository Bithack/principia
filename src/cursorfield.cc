#include "cursorfield.hh"
#include "ui.hh"
#include "world.hh"

cursorfield::cursorfield()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_CURSORFIELD;

    this->scaleselect = true;
    this->scalemodifier = 6.5f;

    this->set_num_properties(4);

    this->properties[0].type = P_FLT;
    this->properties[0].v.f = .5f;
    this->properties[1].type = P_FLT;
    this->properties[1].v.f = .5f;
    this->properties[2].type = P_FLT;
    this->properties[2].v.f = -.5f;
    this->properties[3].type = P_FLT;
    this->properties[3].v.f = -.5f;
}

void
cursorfield::init()
{
    this->pressed = 0;
    this->hover = 0;
    this->dragged = 0;

    if (this->get_body(0)) {
        b2Body *b = this->get_body(0);

        b2PolygonShape sh;

        b2Vec2 vertices[4] = {
            this->local_to_body(b2Vec2(this->properties[0].v.f, this->properties[1].v.f), 0),
            this->local_to_body(b2Vec2(this->properties[2].v.f, this->properties[1].v.f), 0),
            this->local_to_body(b2Vec2(this->properties[2].v.f, this->properties[3].v.f), 0),
            this->local_to_body(b2Vec2(this->properties[0].v.f, this->properties[3].v.f), 0)
        };

        sh.Set(vertices, 4);

        b2FixtureDef fd;
        fd.isSensor = true;
        fd.shape = &sh;
        fd.density = 0.00000001f;
        fd.friction = FLT_EPSILON;
        fd.restitution = 0.f;
        fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        b->CreateFixture(&fd)->SetUserData(this);
    }
}

edevice*
cursorfield::solve_electronics(void)
{
    this->s_out[0].write(this->pressed >0 ? 1.f : 0.f);
    this->s_out[1].write(this->dragged >0 ? 1.f : 0.f);
    this->s_out[2].write(this->hover >0 ? 1.f : 0.f);
    return 0;
}
