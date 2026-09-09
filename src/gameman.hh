#pragma once

#include "entity.hh"
#include "edevice.hh"

/**
 * Class representing the Game Manager object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Game_Manager
 */
class gameman : public brcomp_multiconnect {
  public:
    gameman();
    edevice* solve_electronics();
    const char *get_name() { return "Game Manager"; }
};
