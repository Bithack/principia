#pragma once

#include "edevice.hh"

class pointer : public ecomp_multiconnect
{
  private:
    tms::entity *arrow;
    float arrow_angle;

  public:
    pointer();
    void update();

    void setup() {this->arrow_angle = 0.f;};
    edevice* solve_electronics();
    const char *get_name(){return "Pointer";};

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl,lb);
        lb->w_s_float(this->arrow_angle);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->arrow_angle = lb->r_float();
    }
};
