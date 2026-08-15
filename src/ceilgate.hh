#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

/**
 * Class representing the Ceil object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Ceil
 */
class ceilgate : public i1o1gate {
  public:
    ceilgate();
    edevice *solve_electronics();
    const char *get_name() { return "Ceil"; }
};
