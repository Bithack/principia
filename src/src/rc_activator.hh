#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class rcactivator : public i1o1gate_mini
{
  public:
    rcactivator();
    edevice* solve_electronics();
    const char* get_name(){return "RC Activator";}
};
