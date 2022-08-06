#ifndef _ANGULARVELMETER__H_
#define _ANGULARVELMETER__H_

#include "velmeter.hh"

class angularvelmeter : public velmeter
{
  public:
    angularvelmeter();
    const char *get_name(){return "Angular vel. meter";};

    edevice* solve_electronics();
};

#endif
