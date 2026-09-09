#pragma once

#include "edevice.hh"
#include "i1o0gate.hh"

/**
 * Class representing the Artificial Gravity object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Artificial_Gravity
 */
class artificialgravity : public i1o0gate {
  private:
    float mul;

  public:
    artificialgravity();
    b2BodyType get_dynamic_type();
    void step();
    const char *get_name() { return "Artificial Gravity"; }
    const char *get_slider_label(int s) {
        return "Gravity";
    }
    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);

    edevice* solve_electronics();

    void toggle_axis_rot();
    struct tms_sprite *get_axis_rot_sprite();
    const char *get_axis_rot_tooltip();
    bool get_axis_rot() { return true; }

    void write_state(lvlinfo *lvl, lvlbuf *lb) {
        entity::write_state(lvl, lb);
        lb->w_s_float(this->mul);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb) {
        entity::read_state(lvl, lb);
        this->mul = lb->r_float();
    }
};
