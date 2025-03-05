#pragma once

#include "edevice.hh"

class vendor : public brcomp_multiconnect
{
  public:
    vendor();
    const char *get_name() { return "Vendor"; }

    void init();
    void setup();

    void on_touch(b2Fixture *my, b2Fixture *other);

    float get_slider_snap(int s){return 1.f / 14.f;};
    float get_slider_value(int s){ return tclampf((this->properties[2].v.i-1) / 14.f, 0.f, 1.f); };
    const char *get_slider_label(int s){return "Amount";};
    void on_slider_change(int s, float value);

    void step();
    edevice* solve_electronics();

    b2Fixture *absorb_sensor;

    /* State variables */
    uint32_t deposited;
    uint32_t num_bought;
    bool active;
    bool do_reset;
    bool last_bought;

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_uint32(this->deposited);
        lb->w_s_uint32(this->num_bought);
        lb->w_s_uint8(this->active ? 1 : 0);
        lb->w_s_uint8(this->do_reset ? 1 : 0);
        lb->w_s_uint8(this->last_bought ? 1 : 0);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->deposited = lb->r_uint32();
        this->num_bought = lb->r_uint32();
        this->active = (lb->r_uint8() != 0);
        this->do_reset = (lb->r_uint8() != 0);
        this->last_bought = (lb->r_uint8() != 0);
    }
};
