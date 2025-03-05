#pragma once

#include "entity.hh"

class dragfield : public entity_multiconnect
{
  private:
    connection c;
    int num_inside;
    struct tms_entity *lamp;

    void set_size(float radius);

  public:
    dragfield();
    ~dragfield();

    void init();

    const char* get_name(){return "Dragfield";};

    float get_slider_snap(int s);
    float get_slider_value(int s);
    const char *get_slider_label(int s){return "Field Radius";};
    void on_slider_change(int s, float value);

    void add_to_world();
    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);

    void update(void);
    void update_effects(void);

    b2CircleShape sensor_shape;
};
