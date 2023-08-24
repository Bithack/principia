#pragma once

#include "entity.hh"

#define FLUID_MAX_SIZE 5.f

class fluid : public entity
{
  public:
    fluid();
    void add_to_world();
    const char *get_name(){return "Fluid";};

    void update();

    inline float get_width()
    {
        return this->properties[0].v.f = tclampf(this->properties[0].v.f, .5f, FLUID_MAX_SIZE);
    }
    inline float get_height()
    {
        return this->properties[1].v.f = tclampf(this->properties[1].v.f, .5f, FLUID_MAX_SIZE);
    }
    const char *get_slider_label(int s){return s==0?"Width":"Height";};
    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
};
