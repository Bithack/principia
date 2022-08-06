#ifndef _SINCOS__H_
#define _SINCOS__H_

#include "iomiscgate.hh"

class esincos : public i1o4gate
{
  public:
    esincos();
    edevice* solve_electronics();
    const char *get_name(){return "sincos";};
};

#endif
