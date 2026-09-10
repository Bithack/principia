#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

/**
 * Class representing the Sqrt object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Sqrt
 */
class sqrtgate : public i1o1gate {
  public:
    sqrtgate();
    edevice* solve_electronics();
    const char* get_name() { return "Sqrt"; }
};
