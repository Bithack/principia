#pragma once

#include "entity.hh"

class shape_extruder : public entity, public b2QueryCallback {
  public:
    connection c;

    shape_extruder();
    void update();
    const char *get_name() { return "Shape Extruder"; }
    void init();
    bool ReportFixture(b2Fixture *f);
    void find_pairs();
    void add_to_world();
    connection* load_connection(connection &conn);
    void connection_create_joint(connection *c);
    bool connection_destroy_joint(connection *c);
};
