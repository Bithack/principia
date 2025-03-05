#include "cavg.hh"

edevice*
cavg::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();

    float f = this->properties[0].v.f;

    if (v <= 0.f) {
        this->value = 0.f;
    } else {
        this->value = f * this->value + (1.f - f)*v;
    }

    this->s_out[0].write(tclampf(this->value, 0.f, 1.f));

    return 0;
}
