#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

/**
 * Class representing the Floor object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Floor
 */
class integergate : public i1o1gate {
  public:
    integergate();
    edevice* solve_electronics();
    const char* get_name() { return "Floor"; }
};
