#include "timectrl.hh"
#include "game.hh"

timectrl::timectrl()
    : last_set(-1.f)
{

}

edevice*
timectrl::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();

    if (this->last_set != v) {
        G->state.time_mul = v;
        this->last_set = v;
    }

    return 0;
}
