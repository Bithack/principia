#pragma once

#include "edevice.hh"

/**
 * Class representing the Level Manager object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Level_Manager
 */
class levelman : public brcomp_multiconnect {
  public:
    levelman();
    const char *get_name() { return "Level Manager"; }
    edevice *solve_electronics();
};
