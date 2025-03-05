#pragma once

#include "edevice.hh"
#include "i1o0gate.hh"

#define GRAVITY_MANAGER 0
#define GRAVITY_SETTER 1

#define LOCALGRAVITY_MAX_MASS 2500.f

class gravityman : public ecomp_multiconnect
{
  private:
    int _type;

  public:
    gravityman(int _type);
    const char* get_name(){
        switch (this->_type) {
            case GRAVITY_MANAGER: return "Gravity manager";
            case GRAVITY_SETTER: return "Gravity setter";
        }

        return "";
    };

    const char *get_slider_label(int s){
        switch (this->_type) {
            case GRAVITY_MANAGER:
                switch (s) {
                    case 0: return "Fallback angle";
                    case 1: return "Fallback force";
                }
                break;

            case GRAVITY_SETTER:
                switch (s) {
                    case 0: return "Gravity X";
                    case 1: return "Gravity Y";
                }
                break;
        }

        return "";
    };
    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);

    edevice* solve_electronics();
};

class localgravity : public i1o0gate
{
  private:
    float mul;

  public:
    localgravity();
    b2BodyType get_dynamic_type();
    void step();
    const char *get_name(){return "Artificial Gravity";};
    const char *get_slider_label(int s){
        switch (s) {
            case 0: return "Gravity";
        }

        return "";
    };
    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);

    edevice* solve_electronics();

    void toggle_axis_rot();
    struct tms_sprite *get_axis_rot_sprite();
    const char *get_axis_rot_tooltip();
    bool get_axis_rot(){return true;};

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_float(this->mul);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->mul = lb->r_float();
    }
};
