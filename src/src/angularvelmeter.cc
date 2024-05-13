#include "angularvelmeter.hh"
#include "game.hh"

/**
 * Sockets:
 * OUT0 = CCW
 * OUT1 = CW
 *
 * Properties:
 * 0 = float, Sensitivity
 **/
angularvelmeter::angularvelmeter()
{
    this->s_out[0].tag = SOCK_TAG_NONE;
    this->s_out[1].tag = SOCK_TAG_NONE;
}

edevice*
angularvelmeter::solve_electronics()
{
    float av = 0.f;
    float vel = 0.f;

    if (this->get_body(0)) {
        av = this->get_body(0)->GetAngularVelocity();
        vel = tclampf(std::abs(av) / this->properties[0].v.f, 0.f, 1.f);
    }

    if (av > 0.f) {
        this->s_out[0].write(vel);
        this->s_out[1].write(0.f);
    } else {
        this->s_out[0].write(0.f);
        this->s_out[1].write(vel);
    }

    return 0;
}
