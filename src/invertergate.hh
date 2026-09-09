#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

/**
 * Class representing the Inverter object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Inverter
 */
class invertergate : public i1o1gate {
  public:
    invertergate();
    edevice* solve_electronics();
    const char* get_name(){return "Inverter";}
};
