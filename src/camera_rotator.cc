#include "camera_rotator.hh"
#include "game.hh"

edevice*
camera_rotator::solve_electronics()
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }
    if (!this->s_in[1].is_ready()) {
        return this->s_in[1].get_connected_edevice();
    }

    const float v1 = this->s_in[0].get_value();
    const float v2 = this->s_in[1].get_value();

    if ((bool)roundf(v1)) {
        float val = v2 - 0.5f;
        float angle = (val * (M_PI * 2.f)) - M_PI/2.f;

        float x, y;
        tmath_sincos(angle, &y, &x);

        G->cam->up = tvec3f(x, y, 0.f);
    }

    return 0;
}
