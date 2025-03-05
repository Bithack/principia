#pragma once

#include "bomber.hh"

#define LOBBER_RELOAD_TIME 500000

class lobber : public bomber
{
  public:
    lobber();
    const char *get_name() { return "Lobber"; };

    void roam_aim();

    void attack(int add_cooldown=0);
};
