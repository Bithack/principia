#pragma once

#include "iomiscgate.hh"

/**
 * Class representing the atan2 gate.
 *
 * Player Wiki ref: https://principia-web.se/wiki/atan2
 */
class eatan2 : public i4o1gate {
  public:
    edevice* solve_electronics();
    const char *get_name() { return "atan2"; }
};
