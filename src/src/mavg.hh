#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class mavg : public i1o1gate_fifo
{
  protected:
    float value;

  public:
    mavg();
    //float get_slider_snap(int s){return 0.f;};
    float get_slider_value(int s){return ((1.f-this->properties[0].v.f) * 8.f);};
    const char *get_slider_label(int s){return "Coefficient";};
    void on_slider_change(int s, float value){this->properties[0].v.f = 1.f-(value / 8.f);};
    edevice* solve_electronics();
    const char* get_name(){return "Moving Average";}
    void update_effects(void);
    void setup()
    {
        this->value = 0.f;
    };

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
