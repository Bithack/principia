#pragma once

#include "edevice.hh"

#include <list>

class proximitysensor : public edev_multiconnect
{
  private:
    b2PolygonShape sensor_shape;
    std::list<b2Fixture*> objects_detected;

    void calculate_sensor();

  public:
    proximitysensor();
    const char *get_name(void){return "Proximity sensor";};

    void on_load(bool created, bool has_state);
    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);
    void add_to_world();

    edevice* solve_electronics();

    float get_slider_snap(int s);
    float get_slider_value(int s);
    const char *get_slider_label(int s){if (s == 0)return "Length"; else return "Range";};
    void on_slider_change(int s, float value);

    const b2PolygonShape& get_sensor_shape() const
    {
        return this->sensor_shape;
    }
};
