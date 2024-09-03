#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class decay : public i1o1gate
{
  protected:
    float value;

  public:
    decay();
    edevice* solve_electronics();
    const char *get_name(){return "Decay";};

    const char *get_slider_label(int s) { return "Decay mul"; };
    float get_slider_snap(int s) { return 0.025f; };
    float get_slider_value(int s);
    void on_slider_change(int s, float value);

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl,lb);
        lb->w_s_float(this->value);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->value = lb->r_float();
    }
};
