#pragma once

#include "edevice.hh"

#define ROBOTMAN_SOCK_NUMCOL 8

// outputs
enum {
    /* OUT0  */ RMAN_WEAPON_ARM_ANGLE,
    /* OUT1  */ RMAN_TOOL_ARM_ANGLE,
    /* OUT2  */ RMAN_ON_WEAPON_FIRE,
    /* OUT3  */ RMAN_ON_TOOL_USE,
    /* OUT4  */ RMAN_CURRENT_LAYER,
    /* OUT5  */ RMAN_HEAD_REMOVED,
    /* OUT6  */ RMAN_MOVING_LEFT,
    /* OUT7  */ RMAN_MOVING_RIGHT,
    /* OUT8  */ RMAN_I_DIR,
    /* OUT9  */ RMAN_DIR,
    /* OUT10 */ RMAN_ACTION_ACTIVE,
    /* OUT11 */ RMAN_IS_ATTACHED,
    /* OUT12 */ RMAN_FB_LEFT,
    /* OUT13 */ RMAN_FB_RIGHT,
    /* OUT14 */ RMAN_FB_JUMP,
    /* OUT15 */ RMAN_FB_AIM,
    /* OUT16 */ RMAN_FB_LAYERUP,
    /* OUT17 */ RMAN_FB_LAYERDOWN,
    /* OUT18 */ RMAN_FB_ACTION,

    RMAN_NUM_OUT
};

// inputs
enum {
    /* IN0  */ RMAN_ENABLE_GODMODE,
    /* IN1  */ RMAN_SPEED_MODIFIER,
    /* IN2  */ RMAN_DISABLE_ACTION,
    /* IN3  */ RMAN_JUMP_STRENGTH_MODIFIER,
    /* IN4  */ RMAN_HP_INCREASE,
    /* IN5  */ RMAN_HP_DECREASE,
    /* IN6  */ RMAN_MAX_HP_INCREASE,
    /* IN7  */ RMAN_MAX_HP_DECREASE,
    /* IN8  */ RMAN_WEAPON_DAMAGE_MULTIPLIER,
    /* IN9  */ RMAN_TOGGLE_ACTION,
    /* IN10 */ RMAN_WALK_LEFT,
    /* IN11 */ RMAN_WALK_RIGHT,
    /* IN12 */ RMAN_STOP_MOVEMENT,
    /* IN13 */ RMAN_JUMP,
    /* IN14 */ RMAN_AIM,
    /* IN15 */ RMAN_ATTACK,
    /* IN16 */ RMAN_ATTACH_NEAREST,
    /* IN17 */ RMAN_DEATTACH,
    /* IN18 */ RMAN_RESPAWN,
    /* IN19 */ RMAN_FREEZE,
    /* IN20 */ RMAN_TOGGLE_ROAM,
    /* IN21 */ RMAN_CYCLE_WEAPONS,
    /* IN22 */ RMAN_CYCLE_FACTIONS,

    RMAN_NUM_IN
};

class robot_base;

class robotman : public brcomp_multiconnect
{
  private:
    float previous_values[RMAN_NUM_IN];
    float values[RMAN_NUM_IN];
    robot_base *target;

  public:
    robotman();
    const char *get_name(){return "Robot Manager";};

    void init();
    void setup();

    void step();

    edevice* solve_electronics(void);

    void write_state(lvlinfo *lvl, lvlbuf *lb);
    void read_state(lvlinfo *lvl, lvlbuf *lb);

    inline robot_base *get_target()
    {
        return this->target;
    }

    inline void set_target(robot_base *new_target)
    {
        this->target = new_target;
    }
};

void on_robotman_target_absorbed(entity *self, void *userdata);
