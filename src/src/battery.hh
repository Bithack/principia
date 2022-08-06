#ifndef _BATTERY__H_
#define _BATTERY__H_

#include "edevice.hh"

#define BATTERY_3V 0

class battery : public brcomp_multiconnect
{
    float voltage;
  public:
    battery(int what);

    edevice* solve_electronics();
    const char* get_name(){return "Battery (3V)";};
};

#endif
