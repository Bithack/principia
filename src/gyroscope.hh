#pragma once

#include "edevice.hh"

/**
 * Class representing the Gyroscope object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Gyroscope
 */
class gyroscope : public ecomp_multiconnect {
  public:
    gyroscope();
    edevice* solve_electronics();
    const char *get_name() { return "Gyroscope"; }
};
