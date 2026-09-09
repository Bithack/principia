#pragma once

#include "entity.hh"

/**
 * Background grid entity used for the orthographic view
 */
class grid : public entity {
  public:
    grid();
    const char *get_name() { return "Grid"; }
    void add_to_world() {}
};
