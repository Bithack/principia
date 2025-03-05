#pragma once

#include "composable.hh"

class cylinder : public composable
{
  public:
    connection c_back;
    connection c_front;

    /**
     * 0 = regular cylinder
     * 1 = interactive cylinder
     **/
    int obj_type;

    cylinder(int type);

    const char* get_name(){return obj_type==0?"Cylinder":"Interactive Cylinder";}
    void find_pairs();

    connection* load_connection(connection &conn);

    void setup();

    float get_slider_snap(int s);
    float get_slider_value(int s);
    const char *get_slider_label(int s){return "Size";};
    void on_slider_change(int s, float value);
    void on_load(bool created, bool has_state);
};
