#include "boundary.hh"
#include "game.hh"

boundary::boundary()
    : value(0.f)
{
    this->num_sliders = 1;

    this->set_num_properties(1);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 0.f;
}

edevice*
boundary::solve_electronics()
{
    if (!this->s_out[0].written()) {
        this->s_out[0].write(this->value);
    }

    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    this->value = this->s_in[0].get_value();

    return 0;
}

float
boundary::get_slider_value(int s)
{
    return this->properties[0].v.f;
}

void
boundary::on_slider_change(int s, float value)
{
    this->properties[0].v.f = value;
    G->show_numfeed(value);
}
