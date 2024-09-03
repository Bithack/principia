#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class elimit : public i1o1gate
{
    int counter;
  public:
    elimit();
    edevice* solve_electronics();
    const char *get_name(){return "Limit";};
    void setup() {this->counter = 0;};
    void on_pause() {this->counter = 0;};

    const char *get_slider_label(int s) { return "Max ticks"; };
    float get_slider_snap(int s) { return 0.1f; };
    float get_slider_value(int s);
    void on_slider_change(int s, float value);

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl,lb);
        lb->w_s_uint32(this->counter);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->counter = lb->r_uint32();
    }
};
