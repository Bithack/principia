#pragma once

#include "edevice.hh"
#include "i2o0gate.hh"

/**
 * Class representing the Cam Rotator object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Cam_Rotator
 */
class camera_rotator : public i2o0gate {
  public:
    const char *get_name() { return "Cam Rotator"; }

    edevice *solve_electronics();
};
