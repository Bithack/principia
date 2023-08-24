#pragma once

#include "robot_base.hh"

class spikebot : public robot_base
{
  private:
    tms::entity *e_body;
    float body_angle;
    bool angry;

  public:
    spikebot(float scale=1.f);
    const char *get_name() { return "Spikebot"; };

    void action_on();
    void action_off();

    void setup();
    void step();
    void update();
    void on_death();

    void roam_attack();
    void roam_update_dir();

    float get_damage();

    inline bool is_spikebot() { return true; }
};

class mini_spikebot : public spikebot
{
  public:
    mini_spikebot()
        : spikebot(0.33f)
    {
    }

    const char *get_name() { return "Mini Spikebot"; };
};
