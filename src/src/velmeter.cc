#include "velmeter.hh"
#include "game.hh"

/**
 * Sockets:
 * OUT0 = Angle
 * OUT1 = Velocity
 *
 * Properties:
 * 0 = float, Sensitivity
 **/
velmeter::velmeter()
{
    this->num_sliders = 1;

    this->s_out[0].tag = SOCK_TAG_ANGLE;
    //this->s_out[1].tag = SOCK_TAG_SPEED;

    this->set_num_properties(1);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 5.f;
}

edevice*
velmeter::solve_electronics()
{
    float force;
    b2Vec2 vel(0,0);

    if (this->get_body(0)) {
        vel = this->get_body(0)->GetLinearVelocityFromLocalPoint(this->local_to_body(b2Vec2(0.f,0.f), 0));
    }

    if (vel.x == 0.f) vel.x = .00000001f;

    force = vel.Length();
    float f = tclampf(std::abs(force) / this->properties[0].v.f, 0.f, 1.f);

    if (f > 0.0000001f) {
        float il = 1.f/vel.Length();
        vel.y *= il;
        vel.x *= il;

        float angle = tmath_atan2add(vel.y, vel.x);
        float a = fabsf(fmodf(angle, M_PI*2.f)) / (M_PI*2.f);

        this->s_out[0].write(a);
        this->s_out[1].write(f);
    } else {
        this->s_out[0].write(0.f);
        this->s_out[1].write(f);
    }

    return 0;
}

void
velmeter::on_slider_change(int s, float value)
{
    this->properties[0].v.f = roundf(value * 89.f) + 1.f;
    G->show_numfeed(this->properties[0].v.f);
}
