#pragma once

#include "edevice.hh"

#define CRANE_MIN_LENGTH  .5
#define CRANE_MAX_LENGTH  10.

#define CRANE_PULL_SPEED 0.5f

#define CRANE_MAX_FORCE 1500.f
#define CRANE_MAX_SPEED .5f

class crane_pulley;

class crane : public edev, public b2QueryCallback
{
  public:
    connection pc;
    connection rc;

  private:
    connection c_side[4];

    entity *q_result;
    b2Vec2 q_point;

    crane_pulley *pulley;
    bool go_up;

    b2Body *rev;

  public:
    crane();
    const char *get_name(void){return "Crane";};

    void add_to_world();
    void remove_from_world();
    void set_position(float x, float y, uint8_t frame=0);

    void step();

    void setup();
    void on_pause();
    connection* load_connection(connection &conn);

    void find_pairs();
    bool ReportFixture(b2Fixture *f);

    void connection_create_joint(connection *c);

    edevice* solve_electronics();

    void on_absorb();
    void update_effects();
    void set_layer(int z);

    b2PrismaticJoint *pjoint;
    b2RevoluteJoint *rjoint;
    float desired_pos;
    bool dragging;
    bool hit;
};

class crane_pulley : public entity
{
  private:
    connection c_side[4];

  public:
    crane_pulley(crane *parent);
    const char *get_name(){ return "Crane"; }

    connection* load_connection(connection &conn);
    void find_pairs();
    void add_to_world();

    void on_grab_playing();
    void on_release_playing();
    void set_layer(int z);
    void connection_create_joint(connection *c);

    friend class crane;
};
