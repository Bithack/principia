#include "ctrlbase.hh"
#include "model.hh"
#include "material.hh"
#include "ifdevice.hh"

ctrlmini::ctrlmini()
{
    this->num_s_in = 3;
    this->num_s_out = 1;

    this->s_out[0].ctype = CABLE_BLUE;
    this->s_out[0].lpos = b2Vec2(-.45f, .0f);
    this->s_out[0].angle = M_PI/2.f;

    this->s_in[0].ctype = CABLE_BLACK;
    this->s_in[0].lpos = b2Vec2(.575f, 0.f);
    this->s_in[0].angle = 0;

    this->s_in[1].ctype = CABLE_RED;
    this->s_in[1].lpos = b2Vec2(.0f, .0f);
    this->s_in[1].angle = 0;
    this->s_in[1].tag = SOCK_TAG_SPEED;

    this->s_in[2].ctype = CABLE_RED;
    this->s_in[2].lpos = b2Vec2(.27f, .0f);
    this->s_in[2].angle = 0;
    this->s_in[2].tag = SOCK_TAG_REVERSE;

    this->set_mesh(mesh_factory::get_mesh(MODEL_CTRLMINI));
    this->set_material(&m_misc);

    this->set_as_rect(1.25f*.5f, .313f*.5f);
}

edevice*
ctrlmini::solve_electronics(void)
{
    if (!this->s_out[0].p)
        return 0;

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();
    if (!this->s_in[2].is_ready())
        return this->s_in[2].get_connected_edevice();

    float voltage = this->s_in[0].get_value();
    float speed = this->s_in[1].p ? this->s_in[1].get_value() : 1.f;

    if (this->s_in[2].p) {
        if ((bool)roundf(this->s_in[2].get_value()))
            speed *= -1.f;
    }

    ifdevice *i = this->s_out[0].p->find_ifdevice();
    if (i) {
        i->ifstep(voltage, speed, 0.f, 0.f, false, false);
    }

    return 0;
}

