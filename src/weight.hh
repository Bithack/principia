#pragma once

#include "entity.hh"

class weight : public entity_simpleconnect
{
  private:

  public:
    weight();
    const char *get_name(void){return "Weight";};

    void add_to_world();

    float get_slider_snap(int s);
    float get_slider_value(int s);
    const char *get_slider_label(int s){return "Weight multiplier";};
    void on_slider_change(int s, float value);
};
