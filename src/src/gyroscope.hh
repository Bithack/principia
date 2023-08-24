#pragma once

#include "edevice.hh"

class gyroscope : public ecomp_multiconnect
{
  public:
    gyroscope();
    edevice* solve_electronics();
    const char *get_name(){return "Gyroscope";};
};
