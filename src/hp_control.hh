#pragma once

#include "i2o1gate.hh"
#include "robot_base.hh"

class hp_control : public i2o1gate_empty {
  public:
    hp_control();

    void setup();
    void restore();
    edevice* solve_electronics();
    const char* get_name() { return "HP Control"; }

    robot_base *target;
};
