#pragma once

#include "wplug.hh"

class socket;

class mini_transmitter : public wireless_plug
{
  public:
    float pending_value; /* this should be gotten through the connected edevice */
    uint64_t edev_step;

    mini_transmitter();
    edevice *get_edevice(){return (edevice*)this;};
    entity *get_entity(){return (entity*)this;};
    const char *get_name(){return "Mini transmitter";};

    void begin()
    {
        this->step_count = ~edev_step_count;
        this->edev_step = ~edev_step_count;
    }

    int get_dir(){return CABLE_OUT;}

    void setup();
    void on_pause();

    edevice* solve_electronics();

    inline float get_value()
    {
        return this->pending_value;
    };

    inline void set_value(float v)
    {
        this->pending_value = v;
    }

    plug_base *get_other()
    {
        return 0;
    }

    inline void update_color()
    {
        float v = .7f+(this->pending_value/8.f);
        this->set_uniform("~color", v, v, v, 1.f);
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

    void restore()
    {
        entity::restore();

        this->reconnect();
    }
};
