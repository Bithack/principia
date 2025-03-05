#pragma once

#include "wplug.hh"

class socket;

class receiver_base
{
  public:
    float pending_value;
    bool  no_broadcast;

    receiver_base()
        : pending_value(0.f)
        , no_broadcast(false)
    { }

    void reset_recv_value()
    {
        this->pending_value = 0.f;
        this->no_broadcast = false;
    }
};

class receiver : public wireless_plug, public receiver_base
{
  public:
    receiver();
    edevice *get_edevice(){return (edevice*)this;};
    entity *get_entity(){return (entity*)this;};
    const char *get_name(){return "Receiver";};

    int get_dir(){return CABLE_IN;}

    void setup();
    void on_pause();

    edevice* solve_electronics();

    void remove_from_world();

    inline float get_value()
    {
        return this->pending_value;
    };

    inline void update_color()
    {
        float v = .5f+(this->pending_value/8.f);
        this->set_uniform("~color", v, v-.1f, v, 1.f);
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

    void restore();
};
