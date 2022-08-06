#include "sincos.hh"
#include "model.hh"
#include "main.hh"
#include "game.hh"

/** 
 * OUT0: Positive Y
 * OUT1: Positive X
 * OUT2: Negative Y
 * Out3: Negative X
 **/
esincos::esincos()
{
    this->s_in[0].set_description("Angle");

    this->s_out[0].set_description("Positive Y");
    this->s_out[1].set_description("Positive X");
    this->s_out[2].set_description("Negative Y");
    this->s_out[3].set_description("Negative X");
}

edevice*
esincos::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    float v = this->s_in[0].get_value();
    double a = v * M_PI * 2.0;

    double x = cos(a);
    double y = sin(a);

    int sx = x > 0.0;
    int sy = y > 0.0;

    /* Y */
    this->s_out[2*!sy].write(std::abs(y));
    this->s_out[2*sy].write(0.f);

    /* X */
    this->s_out[2*!sx+1].write(std::abs(x));
    this->s_out[2*sx+1].write(0.f);

    return 0;
}
