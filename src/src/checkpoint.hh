#pragma once

#include "edevice.hh"

#define CHECKPOINT_COLOR_INACTIVE .3f, .4f, .3f, 1.f
#define CHECKPOINT_COLOR_ACTIVE .4f, .7f, .4f, 1.f

class checkpoint : public edev_multiconnect
{
  public:
    bool spawned;
    bool activated;

    checkpoint();
    void add_to_world();
    const char *get_name(){return "Checkpoint";};
    edevice* solve_electronics(void);
    void on_touch(b2Fixture *my, b2Fixture *other);

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_bool(this->spawned);
        lb->w_s_bool(this->activated);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->spawned = lb->r_bool();
        this->activated = lb->r_bool();
    }
};
