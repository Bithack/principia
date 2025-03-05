#pragma once

#include "entity.hh"

class oilrig : public entity_simpleconnect
{
  private:
    float oil_accum;

    float size;

  public:
    bool active;
    oilrig();
    void update_effects();

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity_simpleconnect::read_state(lvl, lb);

        this->oil_accum = lb->r_float();
        this->active = (bool)lb->r_uint8();
    }

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity_simpleconnect::write_state(lvl, lb);
        lb->w_s_float(this->oil_accum);
        lb->w_s_uint8((uint8_t)this->active);
    }

    void step();
    void setup();
    void add_to_world();
    const char *get_name(){return "Oil Rig";};
};
