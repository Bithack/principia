#include "ctrlbase.hh"
#include "model.hh"
#include "material.hh"
#include "ifdevice.hh"

ctrlfplus::ctrlfplus()
{
    this->num_s_in = 4;
    this->num_s_out = 4;

    this->scaleselect = true;

    this->s_out[0].ctype = CABLE_BLUE;
    this->s_out[0].lpos = b2Vec2(-1.f, 0.f);
    this->s_out[0].angle = M_PI/2.f;

    this->s_in[0].ctype = CABLE_BLACK;
    this->s_in[0].lpos = b2Vec2(1.15f, 0.f);
    this->s_in[0].angle = -M_PI/2.f;

    this->s_in[1].ctype = CABLE_RED; /* speed ctl */
    this->s_in[1].lpos = b2Vec2(.35f, 0.f);
    this->s_in[1].angle = -M_PI/2.f;
    this->s_in[1].tag = SOCK_TAG_SPEED;

    this->s_in[2].ctype = CABLE_RED; /* reverse ctl */
    this->s_in[2].lpos = b2Vec2(.35f+.25f, 0.f);
    this->s_in[2].angle = -M_PI/2.f;
    this->s_in[2].tag = SOCK_TAG_REVERSE;

    this->s_in[3].ctype = CABLE_RED; /* tradeoff ctl */
    this->s_in[3].lpos = b2Vec2(.35f+.5f, 0.f);
    this->s_in[3].angle = -M_PI/2.f;
    this->s_in[3].tag = SOCK_TAG_TRADEOFF;

    this->s_out[1].ctype = CABLE_RED;
    this->s_out[1].lpos = b2Vec2(-.55f, 0.f);
    this->s_out[1].angle = M_PI/2.f;
    this->s_out[1].tag = SOCK_TAG_FD_SPEED;

    this->s_out[2].ctype = CABLE_RED;
    this->s_out[2].lpos = b2Vec2(-.55f+.3f, 0.f);
    this->s_out[2].angle = M_PI/2.f;
    this->s_out[2].tag = SOCK_TAG_FD_FORCE;

    this->s_out[3].ctype = CABLE_RED;
    this->s_out[3].lpos = b2Vec2(-.55f+.6f, 0.f);
    this->s_out[3].angle = M_PI/2.f;
    this->s_out[3].tag = SOCK_TAG_FD_ERROR;

    this->set_mesh(mesh_factory::get_mesh(MODEL_CTRLFPLUS));
    this->set_material(&m_misc);

    this->set_as_rect(2.175f*.5f, .313f*.5f);
}

edevice*
ctrlfplus::solve_electronics(void)
{
    if (!this->s_out[0].p) {
        this->s_out[1].write(0.f);
        this->s_out[2].write(0.f);
        this->s_out[3].write(0.f);
        return 0;
    }

    if (!this->s_out[1].written()
        || !this->s_out[2].written()
        || !this->s_out[3].written()
        ) {
        iffeed feed;
        feed.speed = 0.f;
        feed.torque = 0.f;
        feed.error = 0.f;

        ifdevice *i = this->s_out[0].p->find_ifdevice();
        if (i) {
            i->ifget(&feed);

            //tms_infof("%.2f/%.2f/%.2f", feed.speed, feed.torque, feed.error);
            this->s_out[1].write(feed.speed);
            this->s_out[2].write(feed.torque);
            this->s_out[3].write(feed.error);
        }

    }

    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();
    if (!this->s_in[2].is_ready())
        return this->s_in[2].get_connected_edevice();
    if (!this->s_in[3].is_ready())
        return this->s_in[3].get_connected_edevice();

    float voltage = this->s_in[0].get_value();
    float speed = this->s_in[1].p?this->s_in[1].get_value():1.f;

    if (this->s_in[2].p) {
        if ((bool)roundf(this->s_in[2].get_value()))
            speed *= -1.f;
    }

    bool has_tradeoff = (bool)this->s_in[3].p;
    float tradeoff = this->s_in[3].get_value();

    ifdevice *i = this->s_out[0].p->find_ifdevice();
    if (i) {
        i->ifstep(voltage, speed, 0.f, tradeoff, false, has_tradeoff);
    }

    return 0;
}

