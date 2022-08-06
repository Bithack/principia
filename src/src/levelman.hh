#ifndef _LEVELMAN__H_
#define _LEVELMAN__H_

#include "edevice.hh"

class levelman : public brcomp_multiconnect
{
  public:
    levelman();
    const char *get_name() { return "Level Manager"; }
    edevice *solve_electronics();
};

#endif
