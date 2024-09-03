#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class fifo : public i1o1gate_fifo
{
  private:
    float queue[8];
    int ptr;

  public:
    fifo();
    edevice* solve_electronics();
    void setup();
    const char* get_name(){return "FIFO queue";}
    void update_effects(void);

    void read_state(lvlinfo *lvl, lvlbuf *lb);
    void write_state(lvlinfo *lvl, lvlbuf *lb);
};
