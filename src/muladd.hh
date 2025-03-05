#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class muladd : public i1o1gate_mini
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "muladd";}

    const char *get_slider_label(int s) { switch (s) { case 0: return "multiply"; case 1: return "add"; } return ""; };
    float get_slider_snap(int s) { return 0.025f; };
    float get_slider_value(int s) {
        float v = this->properties[s].v.f;
        if (s == 0) { /* mul */
            v /= 2.0f;
        }
        return v;
    };
    void on_slider_change(int s, float value);

    muladd();
};
