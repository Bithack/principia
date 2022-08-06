#ifndef _RESISTOR__H_
#define _RESISTOR__H_

#include "i2o1gate.hh"

class resistor : public i2o1gate
{
  public:
    resistor();
    const char *get_name(){return "EC Resistor";};

    edevice* solve_electronics();
};

#endif
