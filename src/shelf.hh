#pragma once

#include "entity.hh"

class shelf : public entity
{
  public:
    shelf();
    const char* get_name(){return "Platform";}

    void add_to_world();

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s){return "Size";};
    float update_mesh();
    void recreate_shape();
    void on_load(bool created, bool has_state);
};
