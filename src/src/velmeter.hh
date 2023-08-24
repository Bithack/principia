#pragma once

#include "iomiscgate.hh"

class velmeter : public i0o2gate
{
  public:
    velmeter();
    const char *get_name(){return "Velocity meter";};

    edevice* solve_electronics();

    const char *get_slider_label(int s)
    {
        return "Speed threshold";
    }
    float get_slider_snap(int s)
    {
        return 1.f / 30.f;
    }
    float get_slider_value(int s)
    {
        return (this->properties[0].v.f - 1.f) / 89.f;
    }
    void on_slider_change(int s, float value);
};
