#include "squaregate.hh"

edevice*
squaregate::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = tclampf(this->s_in[0].get_value(), 0.f, 1.f);
    v *= v;

    this->s_out[0].write(v);

    return 0;
}
