#pragma once

#include "entity.hh"

class grid : public entity
{
  public:
    grid();
    const char *get_name() { return "Grid"; }
    void add_to_world(){};
};
