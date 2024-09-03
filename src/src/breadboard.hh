#pragma once

#include "composable.hh"

#define BR_MAX_SIZE 10.f

class breadboard : public composable, public b2RayCastCallback, public b2QueryCallback
{
    connection c[4];

    entity *q_result;
    float q_fraction;
    uint8_t q_frame;

  public:
    breadboard();

    void set_layer(int z);
    void update_size(void);
    void update();
    void on_load(bool created, bool has_state);

    const char *get_slider_label(int s){ switch (s) { case 0: return "Width"; case 1: return "Height"; } return ""; }
    float get_slider_snap(int s){return 1.f/10.f;};
    float get_slider_value(int s){return this->properties[s].v.f / BR_MAX_SIZE;};
    void on_slider_change(int s, float value);

    connection *load_connection(connection &conn);
    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    bool ReportFixture(b2Fixture *f);
    void find_pairs();

    const char* get_name(void) {return "Breadboard";};
};
