#pragma once

#include "entity.hh"

/**
 * Class representing the Goal object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Goal
 */
class goal : public entity {
  public:
    goal();

    void on_touch(b2Fixture *my, b2Fixture *other);
    void add_to_world();

    const char *get_name() { return "Goal"; }
};
