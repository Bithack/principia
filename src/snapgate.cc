#include "snapgate.hh"
#include "game.hh"

snapgate::snapgate()
{
    this->num_sliders = 1;

    this->set_num_properties(1);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 2.f;
}

edevice*
snapgate::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    float snap = 1.f / (this->properties[0].v.f);

    v /= snap;
    v = roundf(v);
    v *= snap;

    this->s_out[0].write(tclampf(v, 0.f, 1.f));

    return 0;
}

void
snapgate::on_slider_change(int s, float value)
{
    value = 1.f + value * 20.f;
    this->properties[s].v.f = value;
    G->show_numfeed(value);
}