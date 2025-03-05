#pragma once

#include "edevice.hh"

class transmitter : public brcomp_multiconnect
{
  private:
    float    pending_value;

    int      is_broadcaster;
    uint32_t range_min;
    uint32_t range_max;

    uint32_t (*frequency_solver)(uint32_t base, float extra);

  public:
    transmitter(int _is_broadcaster);
    const char *get_name(void){return this->is_broadcaster?"Broadcaster":"Transmitter";};
    void write_quickinfo(char *out);

    void construct();
    void on_pause();
    void setup();

    edevice* solve_electronics();

    inline float get_value()
    {
        return this->pending_value;
    }

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_float(this->pending_value);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->pending_value = lb->r_float();
    }
};
