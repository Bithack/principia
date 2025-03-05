#pragma once

#include "edevice.hh"

class switcher : public brcomp_multiconnect
{
  public:
    int n_out;

    switcher();
    void setup();
    void update_effects();
    edevice* solve_electronics(void);
    const char* get_name(){return "Switch";};

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl,lb);
        lb->w_s_uint8(this->n_out);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->n_out = lb->r_uint8();
    }
};
