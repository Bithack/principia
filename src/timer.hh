#pragma once

#include "iomiscgate.hh"

#define TIMER_MIN_TIME 16

/**
 * Class representing the Timer object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Timer
 */
class timer : public i2o2gate {
  private:
    int time;
    uint64_t last_tick;
    bool tick;
    bool started;
    uint8_t ticks_left;

  public:
    timer();
    const char *get_name() { return "Timer"; }
    void write_quickinfo(char *out);

    uint64_t refresh_time();

    void on_pause();
    void setup();
    void step();
    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb) {
        entity::write_state(lvl, lb);

        lb->w_s_uint32((uint32_t)this->time);
        lb->w_s_uint8(this->ticks_left);
        lb->w_s_uint8(this->started ? 1 : 0);
        lb->w_s_uint8(this->tick ? 1 : 0);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb) {
        entity::read_state(lvl, lb);

        this->time = (int)lb->r_uint32();
        this->ticks_left = lb->r_uint8();
        this->started = (lb->r_uint8() != 0);
        this->tick = (lb->r_uint8() != 0);
    }

    void restore() {
        entity::restore();

        this->last_tick = _tms.last_time;
    }
};

/**
 * Class representing the IF-else object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/IF-else
 */
class ifelse : public i2o2gate {
  public:
    ifelse();
    edevice* solve_electronics();

    const char *get_name() { return "IF-else"; }
};
