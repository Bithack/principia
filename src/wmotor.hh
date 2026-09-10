#pragma once

#include "entity.hh"

/**
 * Class representing the Wall Pivot object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Wall_Pivot
 */
class wmotor : public entity {
  private:
    connection c;

  public:
    wmotor();
    void add_to_world();
    void update();
    const char *get_name() { return "Wall Pivot"; }
    void find_pairs();
    void connection_create_joint(connection *c);
    connection* load_connection(connection &conn);
};
