#pragma once

#include "entity.hh"

class goal : public entity
{
  public:
    goal();

    void on_touch(b2Fixture *my, b2Fixture *other);
    void add_to_world();

    const char *get_name(void){return "Goal";};
};
