#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

/**
 * Class representing the Square object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Square
 */
class squaregate : public i1o1gate {
  public:
    squaregate();
    edevice* solve_electronics();
    const char* get_name(){return "Square";}
};

