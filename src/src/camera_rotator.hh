#pragma once

#include "edevice.hh"
#include "i2o0gate.hh"

class camera_rotator : public i2o0gate
{
  public:
    const char *get_name() { return "Cam Rotator"; }

    edevice *solve_electronics();
};
