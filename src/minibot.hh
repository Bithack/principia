#pragma once

#include "robot_base.hh"

class miniwheel;

class minibot : public robot_base
{
  public:
    minibot();
    ~minibot();
    const char *get_name() { return "Cleaner Bot"; };

    void roam_set_target_type();
    void roam_attack();
    void roam_update_dir();
    bool roam_neglect();
    bool roam_can_target(entity *e, bool must_see=true);
    void look_for_target();

    bool can_jump(){return false;};

    void on_load(bool created, bool has_state);
    void eat(entity *e);
};
