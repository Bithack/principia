#pragma once

#include "edevice.hh"

class levelman : public brcomp_multiconnect
{
  public:
    levelman();
    const char *get_name() { return "Level Manager"; }
    edevice *solve_electronics();
};
