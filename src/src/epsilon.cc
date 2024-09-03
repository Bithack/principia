#include "epsilon.hh"

edevice*
epsilon::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    v = fminf(1.f, v+1e-5);

    this->s_out[0].write(v);

    return 0;
}
