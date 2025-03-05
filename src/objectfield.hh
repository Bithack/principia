#pragma once

#include "edevice.hh"

enum {
    OBJECT_FIELD_ID,
    OBJECT_FIELD_OBJECT,
    OBJECT_FIELD_TARGET_SETTER,
};

class objectfield : public edev_multiconnect
{
  private:
    void set_size(int length, int height, bool add_to_world);
    int counter;
    int object_type;

  public:
    objectfield(int _object_type);

    const char* get_name(){
        switch (this->object_type) {
            case OBJECT_FIELD_ID: return "ID field";
            case OBJECT_FIELD_OBJECT: return "Object field";
            case OBJECT_FIELD_TARGET_SETTER: return "Target setter";
        }
        return "";
    };

    float get_slider_snap(int s);
    float get_slider_value(int s);
    const char *get_slider_label(int s);
    void on_slider_change(int s, float value);

    void add_to_world();
    void remove_from_world();

    void on_load(bool created, bool has_state);
    void on_pause();
    void setup();
    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);

    edevice* solve_electronics();

    b2PolygonShape box_shape;

    /* stores the sensor shape */
    b2PolygonShape sensor_shape;
};
