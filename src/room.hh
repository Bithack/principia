#pragma once

#include "composable.hh"

/**
 * Class representing the Background object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Background
 */
class room : public composable {
  public:
    room();
    const char *get_name(){ return "Background"; }

    float get_slider_snap(int s) {
        if (s == 0)
            return 1.f / 4.f;
        else
            return 1.f;
    }

    float get_slider_value(int s) {
        if (s == 0)
            return this->properties[0].v.i / 4.f;
        else
            return this->properties[1].v.i;
    }

    const char *get_slider_label(int s) {
        if (s == 0)
            return "Corners";
        else
            return "Background";
    }

    void on_slider_change(int s, float value);

    void create_sensor();
    void set_layer(int z);
};
