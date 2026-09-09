#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class sqrtgate : public i1o1gate
{
  public:
    sqrtgate();
    edevice* solve_electronics();
    const char* get_name(){return "Sqrt";}
};
