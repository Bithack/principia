#pragma once

#include "bomber.hh"

#define LOBBER_RELOAD_TIME 500000

/**
 * Class representing the Lobber object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Lobber
 */
class lobber : public bomber {
  public:
    lobber();
    const char *get_name() { return "Lobber"; }
    void roam_aim();
    void attack(int add_cooldown=0);
};
