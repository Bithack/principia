#pragma once

#include "composable.hh"

class rubberband : public composable, public b2QueryCallback
{
  private:
    //connection c_back;
    connection c_front;
    connection c_side[4];

    entity *q_result;
    b2Fixture *q_fx;
    uint8_t q_frame;
    int q_dir;
    b2Vec2 q_point;

  public:
    rubberband();

    void on_grab(game *g);
    void on_release(game *g);
    void setup();

    connection* load_connection(connection &conn);

    bool ReportFixture(b2Fixture *f);
    void find_pairs();
};

class rubberband_1: public rubberband
{
  public:
    connection dconn;

    rubberband_1();
    void construct();
    void step(void);
    void update_effects();

    connection* load_connection(connection &conn);
    void set_layer(int z);
    void connection_create_joint(connection *c);
    void update_frame(bool hard);
    const char *get_name(void){return "Rubberband";};
    float get_slider_snap(int s){ return .05f; };
    float get_slider_value(int s){
        if (s == 0) {
            return (this->properties[0].v.f - 1.f)/5.f;
        }
        return (this->properties[1].v.f-.5f)/400.f;
    };

    const char *get_slider_label(int s) {
        if (s == 0) return "Reaction length";
        else return "Coefficient";
    }
    void on_slider_change(int s, float value);
};

class rubberband_2 : public rubberband
{
  public:
    rubberband_1 *d1;

    rubberband_2();

    void set_layer(int z);
    entity *get_property_entity(){return d1?(entity*)d1:(entity*)this;};
    const char* get_name(){return "Rubberband (part)";}
    void update_frame(bool hard);
};
