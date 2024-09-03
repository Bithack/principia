#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class valueshift : public i1o1gate
{
  public:
    valueshift();
    edevice* solve_electronics();
    const char* get_name(){return "Value shift";}

    float get_slider_value(int s)
    {
        return this->properties[s].v.f;
    };

    float get_slider_snap(int s)
    {
        return 0.05f;
    }
    const char *get_slider_label(int s)
    {
        return "Value";
    }
    void on_slider_change(int s, float value);
};
