#pragma once

#include "edevice.hh"
#include "mavg.hh"

class cavg : public mavg
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "0-Reset M. Avg";}
};
