#include "eatan2.hh"
#include "game.hh"

edevice*
eatan2::solve_electronics()
{
    for (int x=0; x<4; x++) {
        if (!this->s_in[x].is_ready()) {
            return this->s_in[x].get_connected_edevice();
        }
    }

    float py = this->s_in[0].get_value();
    float px = this->s_in[1].get_value();
    float ny = this->s_in[2].get_value();
    float nx = this->s_in[3].get_value();

    float x = px > 0.f ? px : -nx;
    float y = py > 0.f ? py : -ny;

    double a = atan2(y,x) + (M_PI*2.f);
    a = fmod(a, M_PI*2.);
    a = fabs(a) /  (M_PI*2.);

    this->s_out[0].write((float)a);

    return 0;
}

