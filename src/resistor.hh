#pragma once

#include "i2o1gate.hh"

/**
 * Class representing the EC Resistor object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/EC_Resistor
 */
class resistor : public i2o1gate {
  public:
    resistor();
    const char *get_name() { return "EC Resistor"; }

    edevice* solve_electronics();
};
