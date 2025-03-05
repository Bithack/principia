#pragma once

#include "edevice.hh"

enum {
    COMMAND_STOP,
    COMMAND_STARTSTOP,
    COMMAND_LEFT,
    COMMAND_RIGHT,
    COMMAND_LEFTRIGHT,
    COMMAND_JUMP,
    COMMAND_AIM,
    COMMAND_ATTACK,
    COMMAND_LAYERUP,
    COMMAND_LAYERDOWN,
    COMMAND_INCRSPEED,
    COMMAND_DECRSPEED,
    COMMAND_SETSPEED,
    COMMAND_HEALTH,

    NUM_COMMANDS
};

class robot_base;

/** 
 * A command "plate" which directs the robot_base when it steps on it
 **/
class command : public edev
{
  private:
    float last_apply;
    bool do_apply;
    bool active;
    robot_base *target;

    int cmd;

    bool apply_command(robot_base *r);

  public:
    command();
    const char *get_name(void){return "Command pad";};
    void write_quickinfo(char *out);

    void on_load(bool created, bool has_state);

    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);
    void add_to_world();
    void step();

    edevice* solve_electronics();

    void set_command(int cmd, bool reset_property=true);
    inline int get_command() {
        return this->cmd;
    }

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s)
    {
        switch (this->cmd) {
            case COMMAND_ATTACK: return "Bullet Force";
            case COMMAND_AIM: return "Arm Angle";
            case COMMAND_JUMP: return "Jump Strength";
            case COMMAND_INCRSPEED: return "Increase By";
            case COMMAND_DECRSPEED: return "Decrease By";
            case COMMAND_SETSPEED: return "New Speed";
        }
        return "";
    };

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_float(this->last_apply);
        lb->w_s_bool(this->active);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->last_apply = lb->r_float();
        this->active = lb->r_bool();
    }
};
