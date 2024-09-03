#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class snapgate : public i1o1gate_mini
{
  public:
    snapgate();
    edevice* solve_electronics();
    const char* get_name(){return "Snap";}

    const char *get_slider_label(int s) { return "Steps"; }
    float get_slider_snap(int s) { return 1.f/20.f; }
    float get_slider_value(int s) { return (this->properties[s].v.f-1.f) / 20.f; }
    void on_slider_change(int s, float value);
};

