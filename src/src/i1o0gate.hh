#ifndef _I1O0GATE__H_
#define _I1O0GATE__H_

#include "edevice.hh"

class i1o0gate : public brcomp_multiconnect
{
  public:
    i1o0gate();
};

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

#endif
