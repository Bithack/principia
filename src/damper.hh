#pragma once

#include "composable.hh"

class damper : public composable,
               public b2RayCastCallback,
               public b2QueryCallback
{
  public:
    connection c;
    float dir;

    entity *query_result;
    b2Fixture *query_result_fx;
    b2Vec2 query_point;
    float query_fraction;
    uint8_t query_frame;

    damper();

    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    bool ReportFixture(b2Fixture *f);
    connection *load_connection(connection &conn);
    void find_pairs();
    void on_grab(game *g);
    void on_release(game *g);
};

/* the "main" part of the damper */
class damper_1 : public damper
{
  public:
    b2PrismaticJoint *joint;
    connection dconn;

    damper_1();
    ~damper_1();
    void construct();
    void step(void);

    void set_layer(int z);
    connection *load_connection(connection &conn);
    void connection_create_joint(connection *c);
    void update_frame(bool hard);
    void set_moveable(bool moveable);
    const char *get_name(void){return "Damper";};
    float get_slider_snap(int s){ return .05f; };
    float get_slider_value(int s){
        return this->properties[1+s].v.f / 20.f;
    };

    const char *get_slider_label(int s) {
        if (s == 0) return "Pressure";
        else return "Max speed";
    }
    void on_slider_change(int s, float value);
};

class damper_2 : public damper
{
  public:
    damper_1 *d1;
    damper_2();

    void set_layer(int z);
    entity *get_property_entity(){return d1?(entity*)d1:(entity*)this;};
    const char* get_name(){return "Damper (part)";}
    void update_frame(bool hard);
};
