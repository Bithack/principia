#pragma once

#include "i2o1gate.hh"

class condenser : public i2o1gate_empty
{
  protected:
    float value;

  public:
    condenser();
    edevice* solve_electronics();
    const char* get_name(){return "Condenser";}

    void setup();
    const char *get_slider_label(int s) {
        if (s == 0)
            return "Max value";
        else // s == 1
            return "Initial fraction";
    }
    float get_slider_snap(int s) {
        if (s == 0)
            return 1/31.f;
        else // s == 1
            return 1/20.f;
    }
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

class wrapcondenser : public condenser
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Wrap condenser";}
};
