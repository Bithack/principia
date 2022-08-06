#ifndef _CONNECTION__H_
#define _CONNECTION__H_

#include "entity.hh"

class connection_entity;

class connection_entity : public entity
{
  public:
    connection *conn;
    connection_entity(connection *c, int type);
    const char *get_name() { return "Connection entity"; }
    void add_to_world(){};
};

#endif
