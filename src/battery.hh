#pragma once

#include "edevice.hh"

/**
 * Class representing the Battery (3V) object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Battery_(3V)
 */
class battery : public brcomp_multiconnect {
    float voltage;
  public:
    battery();

    edevice* solve_electronics();
    const char* get_name() { return "Battery (3V)"; }
};
