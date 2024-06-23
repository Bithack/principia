#include "wplug.hh"
#include "world.hh"
#include "game.hh"
#include "settings.hh"
#include "textbuffer.hh"

float
wplug::get_angle()
{
    if (this->is_connected())
        return this->plugged_edev->get_entity()->get_angle()+this->s->angle;

    return M_PI/2.f;
}

b2Vec2
wplug::get_position()
{
    if (this->is_connected()) {
        return this->plugged_edev->get_entity()->local_to_world(
                this->s->lpos/* + b2Vec2(cosf(this->s->angle)*.15f, sinf(this->s->angle)*.15f)*/
                , 0);
    } else {
        return entity::get_position();
    }
}

int
wplug::connect(edevice *e, isocket *s)
{
    if (s->ctype != CABLE_RED) {
        tms_warnf("incompatible cable types");
        return 3;
    }

    this->plugged_edev = e;
    this->s = s;

    s->p = this;

    this->set_prio(e->get_entity()->get_layer());

    if (this->body) {
        this->body->GetWorld()->DestroyBody(this->body);
        this->body = 0;
    }

    this->set_property(1, (uint32_t)e->get_entity()->id);
    this->set_property(2, this->plug_base::get_socket_index());

    return T_OK;
}

void
wplug::disconnect()
{
    if (this->is_connected()) {
        /* displace the plug a little not visualize the disconnect */
        float cs = cosf(this->s->angle);
        float sn = sinf(this->s->angle);

        this->_pos = this->get_position() + b2Vec2(cs*.5f,sn*.5f);
        this->_angle = this->get_angle();

        if (this->plugged_edev) {
            this->plugged_edev = 0;
            this->s->p = 0;
            this->s = 0;
        }

        this->create_body();
        this->set_property(1, (uint32_t)0);
    } else {
        tms_warnf("disconnect called, but not connected.");
    }
}

void
wplug::reconnect()
{
    if (!this->is_connected() && this->properties[1].v.i != 0) {
        entity *e = W->get_entity_by_id(this->properties[1].v.i);
        uint8_t s = this->properties[2].v.i;

        if (!e) {
            tms_errorf("object we were connected to doesn't exist anymore");
            return;
        }

        edevice *ed = e->get_edevice();

        if (!ed) {
            tms_errorf("object we're trying to connect to isn't even an edevice.");
            return;
        }

        isocket *ss;

        if (this->socket_dir == CABLE_IN) {
            if (s > 127) {
                tms_errorf("can't connect with out sockets :(");
                return;
            }

            ss = &ed->s_in[s];
        } else {
            if (s < 128) {
                tms_errorf("can't connect with in sockets :(");
                return;
            }

            ss = &ed->s_out[s-128];
        }

        this->connect(ed, ss);
    }
}

void
wplug::create_body()
{
    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.fixedRotation = true;
    bd.angle = this->_angle;
    bd.position = this->_pos;

    b2CircleShape c;
    c.m_radius = .25f;
    c.m_p = b2Vec2(0,0);

    b2FixtureDef fd;
    fd.shape = &c;
    fd.density = 0.2f;
    fd.friction = .5f;
    fd.restitution = .2f;
    fd.isSensor = true;

    b2Body *b = W->b2->CreateBody(&bd);
    (b->CreateFixture(&fd))->SetUserData(this);
    this->body = b;
    this->set_layer(this->prio);
    this->update_color();
}

void
wireless_plug::write_quickinfo(char *out)
{
    sprintf(out, "%s (f:%u)", this->get_name(), this->properties[0].v.i);
}

void
wireless_plug::update_effects()
{
    float z = this->get_layer()*LAYER_DEPTH + .65f;
    b2Vec2 p = this->get_position();

    if (W->is_paused() && G->state.sandbox && G->get_zoom() < 9.f && settings["display_wireless_frequency"]->v.b) {
        char val_str[64];
        sprintf(val_str, "%u", this->properties[0].v.i);
        textbuffer::add_text(val_str, font::medium,
                p.x, p.y, z,
                0.f, 0.f, 0.f, 1.f,
                0.005f);
    }

}

bool
wireless_plug::compatible_with(entity *o)
{
    return (this->num_properties == o->num_properties &&
            (o->g_id == O_RECEIVER || o->g_id == O_MINI_TRANSMITTER));
}
