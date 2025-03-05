#pragma once

#include "robot_base.hh"

class item;

class robot : public robot_base
{
  public:
    bool pending_action_off;

    robot();
    ~robot();
    const char *get_name() { return "Robot"; };

    bool can_layermove()
    {
        return !this->is_action_active();
    }

    void init();
    void setup();
    void on_load(bool created, bool has_state);
    void on_pause();
    void step();
    void update();
    void update_effects();

    void aim(float a);

    void modify_aim(float da);

    bool can_jump()
    {
        return !this->action_active;
    }

    float get_aim()
    {
        return this->weapon->get_arm_angle();
    };

    void attack(int add_cooldown=0);
    void attack_stop();
    void action_on();
    void action_off();

    void roam_aim();
    void roam_gather_sight();
    int get_optimal_walking_dir(float tangent_dist); // part of roam_update_dir
    void roam_attack();

    void set_position(float x, float y, uint8_t fr=0);

    void on_death();

    void create_head_joint();

    float get_adjusted_damage(float amount, b2Fixture *f, damage_type damage_type, uint8_t damage_source, uint32_t attacker_id);

  public:
    float target_aim_angle;

    robot_parts::tool* get_tool() const { return this->tool; }
    robot_parts::weapon* get_weapon() const { return this->weapon; }
};
