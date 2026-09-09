#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

/**
 * Class representing the Player Activator object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Player_Activator
 */
class player_activator : public i1o1gate_mini {
  public:
    player_activator();
    edevice* solve_electronics();
    const char* get_name() { return "Player Activator"; }
};
