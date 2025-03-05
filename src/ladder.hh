#pragma once

#include "entity.hh"
#include "ud2.hh"

class ladder : public entity, b2QueryCallback, b2RayCastCallback
{
  private:
    connection c_back;

    connection c_vert[2];
    connection c_hori[2];

    entity *q_result;
    b2Fixture *q_result_fx;
    uint8_t q_frame;
    b2Vec2 q_point;
    float q_fraction;
    bool q_ask_for_permission;

    b2Fixture *ladder_step;

    struct ud2_info ladder_ud2;
    struct ud2_info ladder_step_ud2;

  public:
    ladder();
    const char *get_name(){return "Ladder";};
    void add_to_world();

    connection* load_connection(connection &conn);
    bool ReportFixture(b2Fixture *f);
    void find_pairs();
    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);

    bool enjoys_connection(uint32_t g_id);
};

class ladder_step : public entity, b2QueryCallback
{
  private:

    entity *q_result;
    b2Fixture *q_result_fx;
    uint8_t q_frame;
    b2Vec2 q_point;

    struct ud2_info ladder_step_ud2;

  public:
    bool has_pair;
    connection c_back;

    ladder_step();
    const char *get_name(){return "Ladder Step";};
    void add_to_world();
    connection* load_connection(connection &conn);
    bool ReportFixture(b2Fixture *f);
    void find_pairs();
};
