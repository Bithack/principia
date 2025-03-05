#pragma once

#include "creature.hh"
#include "robot_parts.hh"
#include "object_factory.hh"
#include "mood.hh"

class faction_info;

#define ROBOT_DEFAULT_SPEED 18.f

#define ROBOT_DENSITY_MUL 1.f
#define ROBOT_JUMP_FORCE (4.5f*ROBOT_DENSITY_MUL)

#define ROBOT_RELOAD_TIME 100000

#define ROBOT_BALANCE_VELOCITY_MAX 5.f

#define ROBOT_FOOT_FRICTION 4.f

#define ROBOT_MAX_ARMOUR 50.f

#define FOOT_RAISE_SPEED 4.f

#define ROBOT_JUMP_ACTION_NONE      0
#define ROBOT_JUMP_ACTION_MOVE      1
#define ROBOT_JUMP_ACTION_LAYERMOVE 2

#define ROBOT_DAMPING 50.f

#define ROBOT_VISION_DISTANCE   6.f

#define PANIC_ID 200
#define SLOWDOWN_ID 201

#define ROBOT_COLOR .8f, .8f, .8f

class item;

class robot_base : public creature, public b2QueryCallback
{
  public:
    bool ReportFixture(b2Fixture *f);
    float consume_timer;

    faction_info* set_faction(uint8_t faction_id);
    faction_info* set_faction(faction_info *faction);

    faction_info *get_faction()
    {
        return this->faction;
    }
    faction_info *faction;

  protected:
    std::set<entity*> results;

    b2Fixture *f_body;
    b2Fixture *f_sensor;

    void perform_logic();

  private:
    bool        action_active;

    class cb_handler : public b2RayCastCallback
    {
      private:
        robot_base *self;

      public:
        cb_handler(robot_base *s)
        {
            this->self = s;
        };

        float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    };
    class cb_vision_handler : public b2RayCastCallback
    {
      private:
        robot_base *self;

      public:
        bool can_see;
        int test_layer;
        entity *target;

        cb_vision_handler(robot_base *s)
        {
            this->can_see = false;
            this->self = s;
            this->target = 0;
        };

        float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    };
    cb_handler *handler;
    cb_vision_handler *vision_handler;

  public:
    b2Vec2 eye_pos; /* eye position, from where a ray is casted to look for the target */

    int         robot_type;

    mood_data mood;

    robot_base();
    ~robot_base();

    void write_quickinfo(char *out);

    virtual bool consume(item *c, bool silent, bool first=false);
    virtual void action_on();
    virtual void action_off();

    bool can_see(entity *e);

    virtual void create_fixtures();
    virtual void update();
    virtual void update_effects() {};
    virtual void ghost_update();
    virtual void mstep();
    virtual void step();

    void apply_default_motion();
    void apply_climbing_motion();
    void stop_climbing();

    void on_damage(float amount, b2Fixture *f, damage_type dt, uint8_t damage_source, uint32_t attacker_id);
    virtual void on_death();

    void construct();
    void init();
    void setup();
    void restore();
    void on_load(bool created, bool has_state);
    void on_pause();
    void write_state(lvlinfo *lvl, lvlbuf *lb);
    void read_state(lvlinfo *lvl, lvlbuf *lb);

    void on_jump_end();
    virtual bool jump(bool forward_force, float force_mul=1.f);
    void on_jump_begin();

    void set_i_dir(float v)
    {
        this->i_dir = v;
    }

    b2Fixture *get_body_fixture()
    {
        return this->f_body;
    }

    inline bool is_sensor_fixture(b2Fixture *f) { return this->f_sensor == f; }
    inline bool is_body_fixture(b2Fixture *f) { return this->f_body == f; }

    inline b2Fixture *get_sensor_fixture()
    {
        return this->f_sensor;
    }

    inline void finish()
    {
        tms_infof("GOAL");
        this->stop();
        this->set_state(CREATURE_IDLE);
        this->finished = true;
    }

    void reset_limbs();

    int get_default_feet_type(){return (int)this->properties[ROBOT_PROPERTY_FEET].v.i8;};
    int get_default_head_type(){return (int)this->properties[ROBOT_PROPERTY_HEAD].v.i8;};
    int get_default_back_equipment_type(){return (int)this->properties[ROBOT_PROPERTY_BACK].v.i8;};
    int get_default_front_equipment_type(){return (int)this->properties[ROBOT_PROPERTY_FRONT].v.i8;};
    int get_default_head_equipment_type(){return (int)this->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8;};
    int get_default_bolt_set(){return (int)this->properties[ROBOT_PROPERTY_BOLT_SET].v.i8;};

    void roam_gather_sight();
    bool roam_can_target(entity *e, bool must_see=true);
    void roam_set_target_type();
    void roam_set_target(entity *e);

    virtual bool is_friend(entity *e);
    virtual bool is_neutral(entity *e);
    virtual bool is_enemy(entity *e);
    inline bool is_action_active() { return this->action_active; };

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s) {
        if (s == 0) {
            return "Speed";
        } else {
            return "Initial HP";
        }
    };

    void reset_angles();

    virtual void look_for_target();

    static creature_effect *panic_effect;
    static creature_effect *slowdown_effect;

    void reset_friction();

    robot_parts::weapon* equip_weapon(uint8_t weapon_id, bool announce=true);
    bool add_weapon(uint8_t weapon_id);
    robot_parts::weapon *has_weapon(uint8_t weapon_id);

    bool is_roaming();

    bool equip_tool(uint8_t tool_id, bool announce=true);
    bool add_tool(uint8_t tool_id);
    robot_parts::tool *has_tool(uint8_t tool_id);

    int min_box_ticks;

    virtual void refresh_optimal_distance();

    void init_adventure();
    void clear_equipment();
    void refresh_equipment();
    void equip_defaults();

  protected:
    friend class game;
    friend class adventure;
    friend class robot;
    friend class companion;
    friend class spikebot;
    friend class bomber;
    friend class lobber;
    friend class minibot;
    friend class robot_parts::leg;
    friend class robot_parts::leg::foot;
};
