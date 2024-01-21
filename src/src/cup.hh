#pragma once

#include "entity.hh"

/* TODO: use special shadowing to prevent artifacts */

class cup : public entity
{
  protected:
    void create_fixtures();

  public:
    cup();

    void add_to_world();
    const char* get_name(){return "Plastic Cup";}
};
