#ifndef _GYROSCOPE__H_
#define _GYROSCOPE__H_

#include "edevice.hh"

class gyroscope : public ecomp_multiconnect
{
  public:
    gyroscope();
    edevice* solve_electronics();
    const char *get_name(){return "Gyroscope";};
};

#endif
