#pragma once

#include "composable.hh"

class angulardamper : public composable,
               public b2RayCastCallback,
               public b2QueryCallback
{
  private:
    connection c;
    float dir;

    b2Vec2 query_vec;
    entity *query_result;
    b2Vec2 query_point;
    float query_fraction;
    uint8_t query_frame;

    b2RevoluteJoint *joint;

  public:
    angulardamper();
    const char *get_name(){return "Angular Damper";};

    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    bool ReportFixture(b2Fixture *f);
    connection *load_connection(connection &conn);
    void find_pairs();
    //void on_grab(game *g);
    //void on_release(game *g);

    void step();

    void update_frame(bool hard);
    void connection_create_joint(connection *c);
    bool connection_destroy_joint(connection *c);

    float get_slider_snap(int s)
    {
        switch (s) {
            case 0: return 1.f / 9.f;
            case 1: return 1.f / 18.f;
        }
        return 0.f;
    };
    float get_slider_value(int s)
    {
        switch (s) {
            case 0: return (this->properties[0].v.f - 0.2f) * 5.f / 9.f;
            case 1: return (this->properties[1].v.f - 40.f) / 40.f / 18.f;
        }
        return 0.f;
    };

    const char *get_slider_label(int s)
    {
        switch (s) {
            case 0: return "Max Speed";
            case 1: return "Torque";
        }
        return "";
    };
    void on_slider_change(int s, float value);
};
