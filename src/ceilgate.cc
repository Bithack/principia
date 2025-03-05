#include "ceilgate.hh"

edevice*
ceilgate::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = ceilf(this->s_in[0].get_value());

    this->s_out[0].write(tclampf(v, 0.f, 1.f));

    return 0;
}
