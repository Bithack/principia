#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class sparsifier : public i1o1gate
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Sparsifier";}
    void setup() {this->last = false;}

    sparsifier()
        : last(false)
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_SPARSIFY));
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb);
    void write_state(lvlinfo *lvl, lvlbuf *lb);

  private:
    bool last;
};
