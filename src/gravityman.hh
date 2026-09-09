#pragma once

#include "edevice.hh"

#define GRAVITY_MANAGER 0
#define GRAVITY_SETTER 1

#define LOCALGRAVITY_MAX_MASS 2500.f

/**
 * Class representing the Gravity Manager and Gravity Setter objects.
 *
 * Player Wiki ref:
 * - https://principia-web.se/wiki/Gravity_Manager
 * - https://principia-web.se/wiki/Gravity_Setter
 */
class gravityman : public ecomp_multiconnect {
  private:
    int _type;

  public:
    gravityman(int _type);
    const char *get_name() {
        switch (this->_type) {
            case GRAVITY_MANAGER: return "Gravity Manager";
            case GRAVITY_SETTER: return "Gravity Setter";
            default: return "";
        }
    }

    const char *get_slider_label(int s){
        if (this->type == GRAVITY_MANAGER) {
            switch (s) {
                case 0: return "Fallback angle";
                case 1: return "Fallback force";
            }
        } else if (this->type == GRAVITY_SETTER) {
            switch (s) {
                case 0: return "Gravity X";
                case 1: return "Gravity Y";
            }
        }

        return "";
    }
    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);

    edevice* solve_electronics();
};
