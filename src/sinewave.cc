#include "sinewave.hh"
#include "game.hh"
#include "world.hh"

sinewave::sinewave()
{
    this->elapsed = 0.f;
    this->num_sliders = 2;
    this->set_num_properties(2);

    this->properties[0].type = P_FLT; /* Frequency */
    this->properties[0].v.f = 1.0f;

    this->properties[1].type = P_FLT; /* Phase */
    this->properties[1].v.f = 0.f;
}

void
sinewave::setup()
{
    this->elapsed = this->properties[1].v.f*M_PI;
}

edevice*
sinewave::solve_electronics(void)
{
    float v = sin((float)(this->elapsed*this->properties[0].v.f * M_PI * 2.)) * .5f + .5f;

    this->s_out[0].write(v);

    this->elapsed += WORLD_STEP/1000000.;
    return 0;
}

void
sinewave::on_slider_change(int s, float value)
{
    if (s == 0) {
        this->properties[s].v.f = value*4.f;
    } else
        this->properties[s].v.f = value;

    G->show_numfeed(this->properties[s].v.f);
}
