#include "elimit.hh"
#include "game.hh"

elimit::elimit()
{
    this->counter = 0;
    this->num_sliders = 1;
    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_EMPTY));

    this->set_num_properties(1);
    this->properties[0].type = P_INT;
    this->properties[0].v.i = 5;

    this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;
    this->s_in[0].tag = SOCK_TAG_GENERIC_BOOL;
}

edevice*
elimit::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    bool v = ((bool)(int)roundf(this->s_in[0].get_value()) && this->counter < this->properties[0].v.i);

    if (v) this->counter ++;

    this->s_out[0].write(v);

    return 0;
}

float
elimit::get_slider_value(int s)
{
    return this->properties[0].v.i / 10.f;
}

void
elimit::on_slider_change(int s, float value)
{
    int v = roundf(value * 10.f);
    this->properties[0].v.i = v;
    G->show_numfeed(v);
}
