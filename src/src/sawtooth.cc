#include "sawtooth.hh"
#include "game.hh"

#define STEPS_PER_FREQUENCY 125

sawtooth::sawtooth()
{
#ifdef PRECISE_SAWTOOTH
    this->elapsed = 0;
#else
    this->elapsed = 0.f;
#endif

    this->num_sliders = 2;
    this->set_num_properties(2);

    this->properties[0].type = P_FLT; /* Frequency */
    this->properties[0].v.f = 1.0f;

    this->properties[1].type = P_FLT; /* Phase */
    this->properties[1].v.f = 0.f;
}

void
sawtooth::setup()
{
#ifdef PRECISE_SAWTOOTH
    double x = roundf(this->properties[1].v.f * (STEPS_PER_FREQUENCY / this->properties[0].v.f));
    this->elapsed = (int)x;
#else
    this->elapsed = this->properties[1].v.f;
#endif
}

edevice*
sawtooth::solve_electronics(void)
{
    float v;

#ifdef PRECISE_SAWTOOTH
    int step = ((int)roundf(STEPS_PER_FREQUENCY / this->properties[0].v.f));
    int a = this->elapsed % step;
    v = (float)a / (float)step;

    this->elapsed += 1;
#else
    float inv = 1.f/this->properties[0].v.f;
    v = fmod(this->elapsed, inv) / inv;

    this->elapsed += WORLD_STEP/1000000.;
#endif

    this->s_out[0].write(v);

    return 0;
}

void
sawtooth::on_slider_change(int s, float value)
{
    if (s == 0) {
        //if (value == 0) value = 0.1f/4.f;
        //this->properties[s].v.f = tmath_logstep(value, 0.01, 10.01);
        this->properties[s].v.f = value * 4.f;
    } else
        this->properties[s].v.f = value;

    G->show_numfeed(this->properties[s].v.f);
}
