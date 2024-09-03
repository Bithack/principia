#pragma once

#include "edevice.hh"
#include "i0o1gate.hh"

class sinewave : public i0o1gate
{
  private:
    double elapsed;

  public:
    sinewave();
    const char *get_name(){return "Sine wave";};
    void setup();
    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        /* XXX XXX we're losing precision here, maybe we should store the tick count
         * instead and just multiply the tick count with 0.008 */
        entity::write_state(lvl,lb);
        lb->w_s_float((float)this->elapsed);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->elapsed = (double)lb->r_float();
    }

    float get_slider_value(int s)
    {
        if (s == 0)
            return this->properties[0].v.f / 4.f;
        else
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
