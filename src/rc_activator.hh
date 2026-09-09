#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

/**
 * Class representing the RC Activator object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/RC_Activator
 */
class rcactivator : public i1o1gate_mini {
  public:
    rcactivator();
    edevice* solve_electronics();
    const char* get_name() { return "RC Activator"; }
};
