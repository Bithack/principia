#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class esub : public i1o1gate_mini
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "sub";}
    void write_quickinfo(char *out);

    const char *get_slider_label(int s) { return "sub"; };
    float get_slider_snap(int s) { return 0.01f; };
    float get_slider_value(int s) { return this->properties[s].v.f; };
    void on_slider_change(int s, float value);

    esub();
};
