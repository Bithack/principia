#include "ctrlbase.hh"
#include "model.hh"
#include "material.hh"
#include "ifdevice.hh"

ctrlservo::ctrlservo()
{
    this->num_s_in = 2;
    this->num_s_out = 3;

    this->s_out[0].ctype = CABLE_BLUE;
    this->s_out[0].lpos = b2Vec2(-.6f, 0.f);
    this->s_out[0].angle = -M_PI/2.f;

    this->s_in[0].ctype = CABLE_BLACK;
    this->s_in[0].lpos = b2Vec2(.75f, 0.f);
    this->s_in[0].angle = M_PI/2.f;

    this->s_in[1].ctype = CABLE_RED;
    this->s_in[1].lpos = b2Vec2(.45f, 0.f);
    this->s_in[1].angle = M_PI/2.f;
    this->s_in[1].tag = SOCK_TAG_STATE;

    this->s_out[1].ctype = CABLE_RED;
    this->s_out[1].lpos = b2Vec2(-.15f, 0.f);
    this->s_out[1].angle = M_PI/2.f;
    this->s_out[1].tag = SOCK_TAG_FD_STATE;

    this->s_out[2].ctype = CABLE_RED;
    this->s_out[2].lpos = b2Vec2(.15f, 0.f);
    this->s_out[2].angle = M_PI/2.f;
    this->s_out[2].tag = SOCK_TAG_FD_FORCE;

    this->set_mesh(mesh_factory::get_mesh(MODEL_CTRLSERVO));
    this->set_material(&m_misc);

    this->set_as_rect(1.875f*.5f, .313f*.5f);
}

edevice*
ctrlservo::solve_electronics(void)
{
    if (!this->s_out[0].p)
        return 0;

    if (!this->s_out[1].written() || !this->s_out[2].written()) {
        iffeed feed;
        feed.angle = 0.f;
        feed.torque = 0.f;

        ifdevice *i = this->s_out[0].p->find_ifdevice();

        if (i) {
            i->ifget(&feed);

            this->s_out[1].write(feed.angle);
            this->s_out[2].write(feed.torque);
        }

    }

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    float voltage = this->s_in[0].get_value();
    float angle = this->s_in[1].p ? this->s_in[1].get_value() : 1.f;

    ifdevice *i = this->s_out[0].p->find_ifdevice();
    if (i) {
        i->ifstep(voltage, 1.f, angle, 0.f, true, false);
    }

    return 0;
}

