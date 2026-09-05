#pragma once

#include "edevice.hh"
#include "i0o1gate.hh"

/**
 * Class representing the Random object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Random
 */
class erandom : public i0o1gate {
  public:
    edevice* solve_electronics();
    const char *get_name() { return "Random"; }
};
