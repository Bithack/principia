#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class player_activator : public i1o1gate_mini
{
  public:
    player_activator();
    edevice* solve_electronics();
    const char* get_name(){return "Player Activator";}
};
