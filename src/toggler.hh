#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class toggler : public i1o1gate_fifo
{
  private:
    bool value;

  public:
    edevice* solve_electronics();
    const char* get_name(){return "Toggler";}
    void update_effects(void);
    void setup();
    void on_pause();
    toggler();

    void on_slider_change(int s, float value);
    float get_slider_value(int s);
    float get_slider_snap(int s)
    {
        return 1.f;
    }
    const char *get_slider_label(int s)
    {
        return "Initial value";
    }

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl,lb);
        lb->w_s_uint8(this->value);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->value = lb->r_uint8();
    }
};
