#ifndef _EATAN2__H_
#define _EATAN2__H_

#include "iomiscgate.hh"

class eatan2 : public i4o1gate
{
  public:
    edevice* solve_electronics();
    const char *get_name(){return "atan2";};
};

#endif
