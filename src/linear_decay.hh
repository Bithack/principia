#pragma once

#include "edevice.hh"
#include "decay.hh"

class ldecay : public decay
{
  public:
    ldecay();
    edevice* solve_electronics();
    const char *get_name(){return "Linear Decay";};

    const char *get_slider_label(int s) { return "Decay rate"; };
    float get_slider_snap(int s) { return 0.02f; };
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
};

