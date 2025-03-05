#pragma once

#include "composable.hh"

class splank : public composable, public b2RayCastCallback
{
  private:
    int q_dir;
    entity *q_result;
    b2Fixture *q_fx;
    b2Vec2 q_point;
    uint8_t q_frame;
    b2Vec2 q_vec;

    connection c[2];

  public:
    splank();

    const char* get_name(){return "Sublayer plank";}
    void find_pairs();
    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);

    void toggle_axis_rot();
    struct tms_sprite *get_axis_rot_sprite();
    const char *get_axis_rot_tooltip();
    /* Regardless of what axis rotation state we are in, we always want the
     * axis rotation image to be "active" (i.e. not faded) */
    bool get_axis_rot(){return true;};

    void on_load(bool created, bool has_state);

    connection *load_connection(connection &conn);
    float get_slider_snap(int s){return 1.f / 3.f;};
    float get_slider_value(int s){return this->properties[0].v.i / 3.f;};
    const char *get_slider_label(int s){return "Sublayer";};
    void on_slider_change(int s, float value);
};
