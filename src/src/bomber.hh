#pragma once

#include "robot_base.hh"

#define BOMBER_RELOAD_TIME 200000

class bomber : public robot_base
{
    b2Fixture *f_outer;

  public:
    bomber();
    const char *get_name() { return "Bomber"; };

    float aim_angle;

    void set_layer(int l);

    void roam_aim();
    void roam_attack();

    void aim(float a) { };
    void modify_aim(float da) { this->aim_angle = this->aim_angle + da; };
    float get_aim() { return this->aim_angle; };

    void attack(int add_cooldown=0);
    void step();

    int attack_timer;
};
