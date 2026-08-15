#pragma once

#include "robot_base.hh"

/**
 * Class representing the Companion object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Companion
 */
class companion : public robot_base {
  public:
    companion();
    const char *get_name() { return "Companion"; }

    void setup();
    void init_body();
    void init_properties();

    void roam_update_dir();
};
