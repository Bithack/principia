#ifndef _GRID__H_
#define _GRID__H_

#include "entity.hh"

class grid : public entity
{
  public:
    grid();
    const char *get_name() { return "Grid"; }
    void add_to_world(){};
};

#endif
