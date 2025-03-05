#pragma once

#include "composable.hh"
#include "activator.hh"

class backpack : public composable_simpleconnect, public activator
{
  public:
    backpack();
    const char *get_name(void){ return "Backpack"; }
    activator *get_activator() { return this; }

    float get_sensor_radius()
    {
        return 1.f;
    }
    b2Vec2 get_sensor_offset()
    {
        return b2Vec2(0.f, 0.f);
    }
    entity *get_activator_entity() { return this; }
    float get_activator_radius() { return this->get_sensor_radius(); }
    b2Vec2 get_activator_pos() { return this->get_position() + this->get_sensor_offset(); }
    void activate(creature *by)
    {
    }

    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);
};
