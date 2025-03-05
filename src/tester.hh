#pragma once

#include "entity.hh"
#include "edevice.hh"

class tester : public brcomp_multiconnect
{
  public:
    struct tms_entity lamp;

    tester();
    ~tester();

    edevice* solve_electronics();
    const char *get_name(){return "Debugger";};

    void set_shape();
    void update();

    void on_load(bool created, bool has_state);
    float get_slider_snap(int s){return 1.f;};
    float get_slider_value(int s){return ((float)this->properties[0].v.i);};
    const char *get_slider_label(int s){return "Size";};
    void on_slider_change(int s, float value);
};
