#pragma once

#include "edevice.hh"

class wmotor : public entity
{
  private:
    connection c;

  public:
    wmotor();
    void add_to_world();
    //bool solve_electronics();
    void update();
    const char *get_name(){return "Wall pivot";};
    void find_pairs();
    void connection_create_joint(connection *c);
    connection* load_connection(connection &conn);

    //float get_slider_snap(int s){return .05f;};
    //float get_slider_value(int s){return this->properties[0].v.f;};
    //void on_slider_change(int s, float value){this->properties[0].v.f=value;};
};
