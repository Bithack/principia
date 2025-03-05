#pragma once

#include "edevice.hh"
#include "i0o1gate.hh"


/* Makes sure the sawtooth wave behaves consistently */
#define PRECISE_SAWTOOTH


#ifndef PRECISE_SAWTOOTH
# error Hold it right htere! state saving is limited to PRECISE_SAWTOOTH
#endif

class sawtooth : public i0o1gate
{
#ifdef PRECISE_SAWTOOTH
    uint64_t elapsed;
#else
    double elapsed;
#endif

  public:
    sawtooth();
    const char *get_name(){return "Sawtooth";};
    void setup();
    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl,lb);
        lb->w_s_uint64(this->elapsed);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->elapsed = lb->r_uint64();
    }

    float get_slider_value(int s)
    {
        if (s == 0) {
            //return tmath_logstep_position(this->properties[0].v.f, 0.02, 10);
            return this->properties[0].v.f / 4.f;
        } else
            return this->properties[1].v.f;
    };

    float get_slider_snap(int s)
    {
        return .05f;
    };

    const char *get_slider_label(int s)
    {
        if (s == 0)
            return "Frequency Hz";
        else
            return "Phase";
    };
    void on_slider_change(int s, float value);
};