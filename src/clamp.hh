#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class clamp : public i1o1gate_mini
{
  private:
    float prev_value;

  public:
    edevice* solve_electronics();
    const char* get_name(){return "Clamp";}

    const char *get_slider_label(int s) { switch (s) { case 0: return "Min Value"; case 1: return "Max Value"; } return ""; };
    float get_slider_snap(int s) { return 0.05f; };
    float get_slider_value(int s) { return this->properties[s].v.f; };
    void on_slider_change(int s, float value);

    clamp();
};
