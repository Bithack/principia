#pragma once

#include "entity.hh"

class game;

/* TODO: gear damping */

class gear : public entity, public b2QueryCallback
{
  private:
    connection c_back;
    connection c_front;

    entity *q_result;
    int q_dir;
    uint8_t q_frame;
    b2Vec2 q_point;

  public:
    b2RevoluteJoint *joint;
    entity *pending;
    gear();
    b2Fixture *outer_fixture;

    void set_position(float x, float y, uint8_t frame=0);
    void set_angle(float a);
    void add_to_world();
    const char* get_name(){return "Gear";}
    void on_touch(b2Fixture *a, b2Fixture *b);
    void set_anchor_pos(float x, float y);
    void find_pairs();
    int get_num_gear_conns();
    void create_shape();
    float get_slider_value(int s);
    float get_slider_snap(int s);
    void on_slider_change(int s, float value);
    float get_ratio();
    void connection_create_joint(connection *c);
    bool connection_destroy_joint(connection *c);
    void on_load(bool created, bool has_state);

    void step(void);
    void tick(void);

    connection * load_connection(connection &conn);
    void remove_connection(connection *c);
    void destroy_connection(connection *c);
    void disconnect_gears();

    void fix_position(gear *other);

    bool ReportFixture(b2Fixture *f);
    void setup();
    void on_pause();
    friend class connection;
};
