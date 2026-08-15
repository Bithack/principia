#pragma once

#include "edevice.hh"
#include "mavg.hh"

/**
 * Class representing the 0-Reset M. Avg object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/0-Reset_M._Avg
 */
class cavg : public mavg {
  public:
    edevice *solve_electronics();
    const char *get_name() { return "0-Reset M. Avg"; }
};
