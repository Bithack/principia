#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

#define SEQUENCER_MAX_LENGTH   2048
#define SEQUENCER_MIN_TIME 16 /* in milliseconds, must be below 1000 */

class sequencer : public i1o1gate_mini
{
  private:
    uint32_t time;
    uint32_t cur_step;
    uint16_t num_steps;
    uint8_t sequence[SEQUENCER_MAX_LENGTH];
    bool started;

  public:
    sequencer();
    const char *get_name(){return "Sequencer";};

    void on_load(bool created, bool has_state);
    void on_pause() {this->setup();};
    void setup() {this->cur_step=0;this->time = 0; this->started = false;};

    inline uint16_t get_num_steps(){return this->num_steps;};

    void refresh_sequence();
    void step();
    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_uint32(this->time);
        lb->w_s_uint32(this->cur_step);
        lb->w_s_uint8(this->started ? 1 : 0);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->time = lb->r_uint32();
        this->cur_step = lb->r_uint32();
        this->started = (lb->r_uint8() != 0);
    }
};
