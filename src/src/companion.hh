#pragma once

#include "robot_base.hh"

class companion : public robot_base
{
  public:
    companion();
    const char *get_name() { return "Companion"; };

    void setup(){robot_base::setup(); this->initialize_interactive();};
    void init_body();
    void init_properties();

    void roam_update_dir();
};
