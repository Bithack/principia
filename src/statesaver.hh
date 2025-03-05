#pragma once

#include "edevice.hh"
#include "i1o0gate.hh"

class statesaver : public i1o0gate
{
    bool last_in; /* for sparsifier thing */
  public:
    void setup(){this->last_in=false;};
    const char *get_name(){return "State Saver";};
    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb);
    void read_state(lvlinfo *lvl, lvlbuf *lb);
};
