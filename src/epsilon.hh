#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

/**
 * Class representing the Epsilon object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Epsilon
 */
class epsilon : public i1o1gate {
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Epsilon";}

    epsilon();
};
