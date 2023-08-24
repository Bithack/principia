#pragma once

#include "entity.hh"

class seesaw : public entity,
               public b2QueryCallback,
               public b2RayCastCallback
{
  private:
    connection c;
    connection c_floor;
    entity *query_result;
    b2Fixture *query_result_fx;
    b2Vec2 query_point;
    float query_fraction;
    uint8_t query_frame;

  public:
    seesaw();
    const char* get_name(){return "Seesaw base";}
    void setup();
    void add_to_world();
    bool ReportFixture(b2Fixture *f);
    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    connection *load_connection(connection &conn);
    void find_pairs();
    void connection_create_joint(connection *c);
};
