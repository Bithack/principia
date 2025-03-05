#pragma once

#include "edevice.hh"
#include "i0o1gate.hh"

class eventlistener : public i0o1gate
{
  public:
    int triggered;
    int event_id;

    eventlistener();
    void setup();
    edevice* solve_electronics(void);
    const char *get_name(){return "Event Listener";};

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_uint32(this->triggered);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->triggered = (int)lb->r_uint32();
    }

    void restore();
};
