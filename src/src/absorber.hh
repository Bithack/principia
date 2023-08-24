#pragma once

#include "edevice.hh"
#include "i1o0gate.hh"

#include <list>

class absorber : public edev_multiconnect
{
  private:
    int size; /* 0 = mini, 1 = big */
    std::list<b2Fixture*> pending_fixtures;

    /* State-variables */
    bool absorbed;
    bool do_accumulate;
    int state;
    uint64_t time;

    uint64_t absorb_interval;
    float absorber_w;
    float absorber_h;
    float frame_w;
    float frame_h;

    tms_entity *field;
    float field_life;

  public:
    absorber(int size);
    const char *get_name(){if (this->size == 0)return "Mini absorber"; else return "Absorber";};

    void add_to_world();

    void setup();
    void init();
    void step();

    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s){return "Absorb Interval";};

    edevice* solve_electronics();

    bool can_handle(entity *e);
    void update_effects();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_uint8(this->absorbed ? 1 : 0);
        lb->w_s_uint8(this->do_accumulate ? 1 : 0);
        lb->w_s_uint32(this->state);
        lb->w_s_uint64(this->time);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->absorbed = (lb->r_uint8() != 0);
        this->do_accumulate = (lb->r_uint8() != 0);
        this->state = lb->r_uint32();
        this->time = lb->r_uint64();
    }
};

class autoabsorber : public i1o0gate
{
  public:
    autoabsorber();
    const char *get_name(){return "Auto Absorber";};
    edevice* solve_electronics();
};

class autoprotector : public i1o0gate
{
  public:
    const char *get_name(){return "Auto Protector";};
    edevice* solve_electronics();
};
