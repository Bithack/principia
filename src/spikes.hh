#pragma once

#include "entity.hh"

class spikes_ray_cb;

class spikes : public entity_multiconnect
{
  private:
    entity *result;
    b2Vec2 result_point;
    b2Vec2 vec;
    int dir;
    b2Fixture *base_fx;

    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);

  public:
    spikes();

    const char* get_name(){return "Spikes";}

    void add_to_world();

    void on_touch(b2Fixture *my, b2Fixture *other);

    const char *get_slider_label(int s) { return "Damage"; };
    float get_slider_snap(int s) { return 0.05f; };
    float get_slider_value(int s) { return this->properties[s].v.f / 2.f; };
    void on_slider_change(int s, float value);
};
