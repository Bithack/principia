#pragma once

#include "edevice.hh"
#include "i0o1gate.hh"

class erandom : public i0o1gate
{
  public:
    edevice* solve_electronics(void);
    const char *get_name(){return "Random";};
};
