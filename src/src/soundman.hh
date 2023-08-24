#pragma once

#include "i1o1gate.hh"

class soundman : public i1o1gate
{
  private:
    float volume;
    bool busy; /* do not state save the busy variable,
                  no sounds will be playing on state load or open
                  of a level */

  public:
    soundman();
    const char *get_name(){return "Sound Manager";};

    void init();
    void setup();
    void remove_from_world();

    edevice* solve_electronics(void);

    inline bool is_busy()
    {
        return this->properties[1].v.i == 0 && this->busy;
    }

    inline float get_volume()
    {
        return this->volume;
    }

    inline void enable()
    {
        this->busy = true;
    }

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);
        lb->w_s_float(this->volume);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);
        this->volume = lb->r_float();
    }

    static void* translate(uint32_t sound_id);
};
