#pragma once

#include "edevice.hh"

#define CONVEYOR_STATIC 0
#define CONVEYOR_DYNAMIC 1

class conveyor : public edev
{
    float speed_mul;
    bool invert;
  public:
    conveyor();
    const char* get_name(){return "Conveyor";}
    void setup();
    void tick();

    void recreate_shape();
    void add_to_world();

    void toggle_axis_rot();
    struct tms_sprite *get_axis_rot_sprite();
    bool get_axis_rot(){return !this->flag_active(ENTITY_AXIS_ROT);};

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s){if (s == 0)return "Size";else return "Speed";};
    float get_tangent_speed();

    edevice* solve_electronics();
    bool do_recreate_shape;

    connection c_back[2];
    connection c_front[2];
    void find_pairs();
    connection* load_connection(connection &conn);

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);
        lb->w_s_float(this->speed_mul);
        lb->w_s_uint8(this->invert ? 0 : 1);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);
        this->speed_mul = lb->r_float();
        this->invert = (lb->r_uint8() != 0);
    }
};
