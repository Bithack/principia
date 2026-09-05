#pragma once

#include "entity.hh"

/* TODO: use special shadowing to prevent artifacts */

/**
 * Class representing the Plastic Cup object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Plastic_Cup
 */
class cup : public entity {
  protected:
    void create_fixtures();

  public:
    cup();

    void add_to_world();
    const char* get_name() { return "Plastic Cup"; }
};
