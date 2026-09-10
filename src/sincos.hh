#pragma once

#include "iomiscgate.hh"

/**
 * Class representing the sincos gate.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Sincos
 */
class esincos : public i1o4gate {
  public:
    esincos();
    edevice* solve_electronics();
    const char *get_name() { return "sincos"; }
};
