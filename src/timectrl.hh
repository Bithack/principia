#pragma once

#include "i1o0gate.hh"

class timectrl : public i1o0gate
{
  private:
    float last_set;
  public:
    timectrl();
    const char *get_name(){return "Time Ctrl";};
    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_float(this->last_set);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->last_set = lb->r_float();
    }
};
