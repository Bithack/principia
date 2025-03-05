#include "linear_decay.hh"
#include "game.hh"

ldecay::ldecay()
{
    this->properties[0].v.f = 0.1f; /* decay rate */
}

edevice*
ldecay::solve_electronics()
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    float v = this->s_in->get_value();

    this->value += v;

    this->value = tclampf(this->value, 0.f, 1.f);
    this->value -= this->properties[0].v.f;

    this->value = tclampf(this->value, 0.f, 1.f);

    this->s_out[0].write(this->value);

    return 0;
}

float
ldecay::get_slider_value(int s)
{
    return this->properties[0].v.f * 10.f;
}

void
ldecay::on_slider_change(int s, float value)
{
    float v = value / 10.f;
    this->properties[0].v.f = v;
    G->show_numfeed(v, 3);
}
