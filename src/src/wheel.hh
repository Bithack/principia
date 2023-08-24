#pragma once

#include "composable.hh"

class wheel : public composable, public b2QueryCallback
{
  private:
    m rubber_material;

  public:
    wheel();

    connection c_back;
    connection c_front;

    entity *q_result;
    b2Fixture *q_fx;
    int q_dir;
    uint8_t q_frame;
    b2Vec2 q_point;

    void update(void);
    void toggle_axis_rot(void);
    const char* get_name(){return "Wheel";}

    float get_slider_snap(int s);
    float get_slider_value(int s);
    const char *get_slider_label(int s){return "Size";};
    void on_slider_change(int s, float value);
    void tick();

    bool ReportFixture(b2Fixture *f);
    void find_pairs();
    void on_load(bool created, bool has_state);
    void setup();
    connection * load_connection(connection &conn);

    bool do_update_fixture;
};
