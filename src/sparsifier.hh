#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

/**
 * Class representing the Sparsifier object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Sparsifier
 */
class sparsifier : public i1o1gate {
  public:
    edevice* solve_electronics();
    const char* get_name() { return "Sparsifier"; }
    void setup() { this->last = false; }

    sparsifier();

    void read_state(lvlinfo *lvl, lvlbuf *lb);
    void write_state(lvlinfo *lvl, lvlbuf *lb);

  private:
    bool last;
};
