#pragma once

#include "entity.hh"

/**
 * Class representing the Rack object.
 *
 * This object is unused and has no Wiki entry.
 */
class rack : public entity {
  private:
    b2PrismaticJoint *joint;
    b2Body *rackbody;
    tms_entity *rackent;

  public:
    float limits[2];

    rack();
    virtual const char* get_name() { return "Rack"; }
    void add_to_world();
    void remove_from_world();
    void create_shape();
    void update();
    bool point_in_range(b2Vec2 p);

    void set_position(float x, float y, uint8_t frame=0);
    void set_angle(float a);
    void pre_write();

    void update_limits();
    float get_projection(b2Vec2 p);

    friend class connection;
};
