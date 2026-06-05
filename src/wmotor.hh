#pragma once

#include "entity.hh"

class wmotor : public entity
{
  private:
    connection c;

  public:
    wmotor();
    void add_to_world();
    void update();
    const char *get_name(){return "Wall pivot";};
    void find_pairs();
    void connection_create_joint(connection *c);
    connection* load_connection(connection &conn);
};
