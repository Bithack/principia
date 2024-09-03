#include "decay.hh"
#include "game.hh"

decay::decay()
    : value(0.f)
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_EMPTY));

    this->num_sliders = 1;

    this->set_num_properties(1);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 0.95f; /* decay multiplier */
}

edevice*
decay::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in->get_value();

    this->value += v;

    if (this->value > 1.f) this->value = 1.f;
    this->value *= this->properties[0].v.f;

    if (this->value < 0.000001f) this->value = 0.f;

    this->s_out[0].write(this->value);

    return 0;
}

float
decay::get_slider_value(int s)
{
    return (this->properties[0].v.f - 0.79f) * 5.f;
}

void
decay::on_slider_change(int s, float value)
{
    float v = value * 0.2f + 0.79f;
    this->properties[0].v.f = v;
    G->show_numfeed(v);
}
