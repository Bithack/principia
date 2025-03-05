#pragma once

#include "entity.hh"
#include "robot_parts.hh"
#include "activator.hh"

// Define this for verbose robot AI debugging
//#define VERBOSE_AI

#if defined(VERBOSE_AI) && defined(DEBUG)
#define tms_debug_roam_ls(f, ...) fprintf(stdout, "Rls " f "\n", ##__VA_ARGS__); // Layerswitch
#define tms_debug_roam_cb(f, ...) fprintf(stdout, "Rcb " f "\n", ##__VA_ARGS__); // Check blocked
#define tms_debug_roam_w(f, ...) fprintf(stdout, "Rw  " f "\n", ##__VA_ARGS__);  // Walk
#define tms_debug_roam_g(f, ...) fprintf(stdout, "Rg  " f "\n", ##__VA_ARGS__);  // Gather
#define tms_debug_roam_j(f, ...) fprintf(stdout, "Rj  " f "\n", ##__VA_ARGS__);  // Jump
#define tms_debug_roam_ud(f, ...) fprintf(stdout, "Rud " f "\n", ##__VA_ARGS__); // Update direction
#define tms_debug_roam_wa(f, ...) fprintf(stdout, "Rwa " f "\n", ##__VA_ARGS__); // Wander
#define tms_debug_roam(f, ...) fprintf(stdout, "R   " f "\n", ##__VA_ARGS__);    // Roam
#define tms_debug_roam_a(f, ...) fprintf(stdout, "Ra  " f "\n", ##__VA_ARGS__);  // Roam attack
#else
#define tms_debug_roam_ls(...)
#define tms_debug_roam_cb(...)
#define tms_debug_roam_w(...)
#define tms_debug_roam_g(...)
#define tms_debug_roam_j(...)
#define tms_debug_roam_ud(...)
#define tms_debug_roam_wa(...)
#define tms_debug_roam(...)
#define tms_debug_roam_a(...)
#endif

#define CREATURE_JUMP_LEN -.75f
#define CREATURE_FEET_FORCE 50.f
#define CREATURE_FEET_SPEED -3.f

#define CREATURE_BALANCE_THRESHOLD 10.f

#define CREATURE_IDLE 0
#define CREATURE_WALK 1
#define CREATURE_STABILIZE 2
#define CREATURE_DEAD 3

#define ANIMAL_LOGIC_TIMER_MAX   250 * 1000
#define ROBOT_LOGIC_TIMER_MAX    150 * 1000

#define CREATURE_MAX_BODIES (1+4+1)
                            /* body+feet+head */

#define CREATURE_DIRCHANGE_TIME 3000000

/* creature feature flags */
#define CREATURE_FEATURE_HEAD            (1ULL << 0)
#define CREATURE_FEATURE_BACK_EQUIPMENT  (1ULL << 1)
#define CREATURE_FEATURE_WEAPONS         (1ULL << 2)
#define CREATURE_FEATURE_TOOLS           (1ULL << 3)
#define CREATURE_FEATURE_CAN_BE_RIDDEN   (1ULL << 4)
#define CREATURE_FEATURE_FRONT_EQUIPMENT (1ULL << 5)

/* circuits */
#define CREATURE_CIRCUIT_NULL           0
#define CREATURE_CIRCUIT_SOMERSAULT    (1ULL << 0)
#define CREATURE_CIRCUIT_RIDING        (1ULL << 1)
#define CREATURE_CIRCUIT_REGENERATION  (1ULL << 2)
#define CREATURE_CIRCUIT_ZOMBIE        (1ULL << 3)

#define NUM_CIRCUITS 4

/* creature flags */
#define CREATURE_GODMODE               (1ULL << 0)
#define CREATURE_DISABLE_ACTION        (1ULL << 1)
#define CREATURE_MOVING_LEFT           (1ULL << 2)
#define CREATURE_MOVING_RIGHT          (1ULL << 3)
#define CREATURE_WANDERING             (1ULL << 4)
#define CREATURE_BOX_ME                (1ULL << 5)
#define CREATURE_QUERY_ROBOTS          (1ULL << 6)
#define CREATURE_PANICKED              (1ULL << 7)
#define CREATURE_QUERY_OIL             (1ULL << 8)
#define CREATURE_FOUND_REACHABLE_OIL   (1ULL << 9)
#define CREATURE_FROZEN                (1ULL << 10)
#define CREATURE_CLIMBING_LADDER       (1ULL << 11)
#define CREATURE_MOVING_UP             (1ULL << 12)
#define CREATURE_MOVING_DOWN           (1ULL << 13)
#define CREATURE_LOST_BALANCE          (1ULL << 14)
#define CREATURE_LOST_HEAD             (1ULL << 15)
#define CREATURE_LOST_FEET             (1ULL << 16)
#define CREATURE_LOST_BACK             (1ULL << 17)
#define CREATURE_LOST_FRONT            (1ULL << 18)
#define CREATURE_IS_ZOMBIE             (1ULL << 19)
#define CREATURE_BEING_RIDDEN          (1ULL << 20)
#define CREATURE_LOST_WEAPON           (1ULL << 21)
#define CREATURE_LOST_TOOL             (1ULL << 22)

#define CREATURE_BLOCKED_NONE      0
#define CREATURE_BLOCKED_LAYERMOVE (1 << 0)
#define CREATURE_BLOCKED_JUMP      (1 << 1)
#define CREATURE_BLOCKED_RETARGET  (1 << 2)

#define CREATURE_MAX_FEET_BODIES 4

#define CREATURE_MAX_GROUND_ADIST (M_PI/2.0f)
#define CREATURE_GROUND_THRESHOLD .1f

#define CREATURE_BASE_MAX_HP 100.f
#define CREATURE_BASE_MIN_MAX_HP 20.f
#define CREATURE_BASE_MAX_MAX_HP 15000.f

#define CREATURE_DAMPING 0.f

#define CREATURE_CORPSE_HEALTH 25.f

#define CREATURE_FEET_BODY_RATIO .3f

#define CREATURE_ROAM_AIM_SPEED .25f

#define DIR_LEFT    -1
#define DIR_FORWARD  0
#define DIR_RIGHT    1

#define DIR_BACKWARD_1  2
#define DIR_BACKWARD_2 -2

#define DIR_DOWN 0
#define DIR_UP 2

#define CREATURE_MAX_SPEED 60.f
#define CREATURE_MIN_SPEED 0.f

enum {
    TARGET_ENEMY,
    TARGET_ITEM,
    TARGET_ANCHOR,
    TARGET_FRIEND,
    TARGET_POSITION,

    TARGET_NONE
};

enum {
    MOTION_DEFAULT        = 0,
    MOTION_BODY_EQUIPMENT = 1,
    MOTION_CLIMBING       = 2,
    MOTION_RIDING         = 3,
};

class checkpoint;
class item;

class stabilizer
{
  public:
    b2Body *body;
    float max_force;
    float limit;
    float target;
    float multiplier;

    stabilizer(b2Body *b);
    void apply_forces();
    float get_offset();
};

struct creature_effect {
  public:
    creature_effect(uint8_t type, uint8_t method, float modifier, uint32_t time)
    {
        this->type = type;
        this->method = method;
        this->modifier = modifier;
        this->time = time;
    }

    int32_t time;
    float modifier;
    uint8_t type;
    uint8_t method;
};

class creature : public entity
{
  private:
    float   hp;
    float   max_hp;
    float   attack_damage_modifier; /* multiplies the damage of bullets and other things this creature does to attack enemies */

    float   damage_accum[NUM_DAMAGE_TYPES];
    b2Vec2  velocity;
    uint64_t last_damage_tick;

  protected:
    float armour;
    float max_armour;
    uint64_t inventory[NUM_RESOURCES];

  public:
    creature();
    ~creature();

    virtual const char *get_name(void) = 0;

    std::map<uint8_t, creature_effect> effects;

    float states[CREATURE_MAX_BODIES][3];

    std::set<activator*> activators;
    activator           *cur_activator;
    creature            *cur_riding;
    b2WeldJoint         *activator_joint;

    b2Vec2  gravity_forces;
    float   shock_forces;

    int                      motion;

    int                      ladder_time; /* time since we last was on a ladder */
    uint32_t                 ladder_id; /* id of the ladder */

    uint32_t                 last_attacker_id; /* ID of the last creature which did damage to us. */

    float                    damage_multiplier; /* sensitivity to receiving damage */
    tvec2                    neck_pos;
    tvec2                    head_pos;
    bool                     recreate_head_on_dir_change;

    int                      bolt_set;

    robot_parts::head_base  *head;
    robot_parts::feet_base  *feet;

    b2Joint                 *j_head;
    b2Joint                 *j_climb;

    float                    feet_offset;
    float                    feet_width;
    b2PrismaticJoint        *j_feet[CREATURE_MAX_FEET_BODIES];
    int                      j_feet_count;

    checkpoint              *current_checkpoint;

    robot_parts::equipment  *equipments[NUM_EQUIPMENT_TYPES];

    robot_parts::tool       *tool;
    robot_parts::weapon     *weapon;

    robot_parts::weapon     *weapons[NUM_WEAPONS];
    robot_parts::tool       *tools[NUM_TOOLS];

    uint16_t num_weapons;
    uint16_t num_tools;

    stabilizer *balance;
    float angular_damping;

    b2Shape *body_shape;

    /* fixtures for the layersensors */
    b2Fixture *f_lback;
    b2Fixture *f_lfront;

    b2Vec2 default_pos;
    float default_angle;
    float default_layer;
    float layer_new, layer_blend, layer_old;

    b2Vec2      last_ground_speed;
    int         jump_time;
    int         jumping;
    int         _state;
    bool        fixed_dir;
    int         look_dir;
    int         dir;
    int         new_dir;
    float       i_dir;
    float       last_i_dir;
    float       on_ground;
    bool        finished;

    uint64_t    features;
    uint64_t    creature_flags;

    uint32_t    circuits;
    uint32_t    circuits_compat; /* compatible circuits, creature-type specific */

    int         logic_timer;
    int         logic_timer_max;
    uint32_t    death_step;
    bool        gives_score;

    int         layer_dir;
    int         dir_timer;
    int         layermove_timer;
    int         layermove_pretimer;
    int         jump_action_time;
    int         jump_action;

    float       target_dist;
    int         target_layer;
    int         target_side;
    float       target_side_accum;

    entity     *roam_target;
    uint32_t    previous_roam_target_id;
    uint32_t    roam_target_id;
    b2Vec2      roam_target_pos;
    int         roam_target_type;
    float       roam_target_aim;
    float       roam_optimal_big_distance;
    float       roam_optimal_small_distance;

    bool _mstep_layermove;
    int  _mstep_jump; /* 1 low jump, 2 high jump */

    /* Variables related to querying the world */
    int     query_layer;
    int     found_ground;
    entity *found_entity;

    int lback_count; /* num objects in back-layer */
    int lfront_count; /* num objects in front-layer */
    int real_lback_count;
    int real_lfront_count;
    int lfront_tpixel_count;
    int real_lfront_tpixel_count;
    int lback_tick;
    int lfront_tick;
    int lfront_tpixel_tick;

    /* variables set via various callbacks */
    bool shoot_target;
    b2Vec2 ground_pt;
    b2Vec2 ground_nor;

    float base_cooldown_multiplier; /* the default cooldown multiplier for this creature, cooldown returns to this after any effects has ended */
    float cooldown_multiplier;

    float base_speed;
    float speed;
    float speed_modifier;

    float base_jump_strength;
    float jump_strength;
    float jump_strength_multiplier;

    int balance_regain;

    virtual void remove_from_world();
    void on_load(bool created, bool has_state);
    void init();
    void setup();
    void restore();
    void on_pause();

    void write_state(lvlinfo *lvl, lvlbuf *lb);
    void read_state(lvlinfo *lvl, lvlbuf *lb);

    bool drop_resource(uint8_t resource_type, uint64_t amount, b2Vec2 relative_pos, float vel=0.f);
    void add_resource(uint8_t resource_type, int amount);
    inline uint64_t get_num_resources(uint8_t resource_type) const
    {
        return (resource_type < NUM_RESOURCES ? this->inventory[resource_type] : 0);
    }

    void activate_activator(activator *act);
    void activate_closest_activator(int offset=0);

    virtual void respawn();
    void apply_damages();
    virtual void apply_destruction();
    void drop_item(uint32_t item_id);
    virtual void on_death();

    virtual void init_adventure(){};

    virtual void on_touch(b2Fixture *my, b2Fixture *other);
    virtual void on_untouch(b2Fixture *my, b2Fixture *other);

    uint32_t get_num_bodies()
    {
        return CREATURE_MAX_BODIES;
    }

    virtual b2Body* get_body(uint8_t fr);

    virtual void perform_logic() = 0;

    virtual b2Fixture *get_body_fixture()=0;

    b2Fixture *get_head_fixture()
    {
        if (this->head && this->head->parent == this) {
            return this->head->fx;
        }

        return 0;
    }
    b2Body *get_head_body()
    {
        if (this->head && this->head->parent == this) {
            return this->head->body;
        }

        return 0;
    }

    void recalculate_mass_distribution();

    bool is_standing()
    {
        float ref_angle = this->get_gravity_angle();
        float _a = this->get_down_angle();

        return fabsf(tmath_adist(_a, ref_angle)) <= CREATURE_MAX_GROUND_ADIST;
    }

    virtual void create_head_joint() {};
    virtual void destroy_head_joint();

    virtual void on_jump_end(){};
    virtual void on_jump_begin(){};

    void destroy_layermove_sensors();
    void create_layermove_sensors();

    class cb_ground_handler : public b2RayCastCallback
    {
      public:
        creature *self;
        float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    } ground_handler;

    struct gather_data {
        bool found_ground;
        bool found_ground_v[3];
        bool found_obstacle[3];
        entity* obstacle[3];
    } gather;

    struct blocked_data {
        bool        is_blocked;
        b2Vec2      pt;

        /* stage 0: move through all layers and try to find a path
         * to move or to jump over something
         *
         * stage 1: check all layers for something that we can destroy
         * to move on.
         *
         * stage 2: wait for a miracle to happen
         **/
        int         stage;
        bool        attempted_layers[2][3];
        bool        enable_walk;
        bool        done; /* whether we're done attempting the current layer */
        blocked_data()
        {
            this->stage=0;
            this->pt.SetZero();

            this->unset();
        }
        void next_stage(){this->stage++;done=false;}
        void previous_stage(int layer){this->stage--;this->attempted_layers[this->stage][layer] = false;done = false;};
        void set(){
            memset(attempted_layers, 0, sizeof(attempted_layers));
            done = false;
            is_blocked = true;
            stage = 0;
        };
        void unset(){
            is_blocked = false;
            for (int s=0;s<2;++s) {
                for (int l=0;l<3;++l) {
                    this->attempted_layers[s][l]=false;
                }
            }
        };
    } blocked;

    b2Vec2 get_gravity();
    float get_gravity_angle();
    float get_down_angle();
    virtual float get_feet_speed(){return 1.f;};

    void set_jump_state(int s, float force_mul=1.f, b2Vec2 dir_bias=b2Vec2(0.f,0.f));

    void deactivate_feet();
    void activate_feet();
    /* mode 0 = default, mode 1 = jump, mode 2 = fold */
    void create_feet_joint(int mode, b2Vec2 bias=b2Vec2(0,0));
    void destroy_feet_joint();
    void detach_feet();

    b2Vec2 get_roam_target_pos()
    {
        if (this->roam_target) {
            return this->roam_target->get_position();
        } else if (this->roam_target_type == TARGET_POSITION) {
            return this->roam_target_pos;
        } else {
            return b2Vec2(0,0);
        }
    }

    virtual void roam_on_target_absorbed();
    virtual void roam_look();
    virtual void roam_retarget();
    virtual void roam_aim();
    virtual void roam_attack();
    virtual void roam_update_dir();
    virtual int get_optimal_walking_dir(float tangent_dist); // part of roam_update_dir
    virtual void roam_jump();
    virtual void roam_gather();
    virtual void roam_gather_sight();
    virtual void roam_walk();
    virtual void roam_layermove();
    virtual void roam_check_blocked();
    virtual void roam_wander();
    virtual void roam_set_target(entity *e);
    virtual bool roam_can_target(entity *e, bool must_see=true);
    virtual bool roam_neglect();
    virtual void roam_set_target_type();
    virtual void roam_unset_target();
    virtual void roam_perform_target_actions();
    void roam_setup_target();

    virtual bool can_jump(){return true;};
    virtual bool jump(bool forward_force, float force_mul=1.f);
    virtual void stop_jump();

    virtual void move(int dir, bool force=false);
    virtual bool layermove(int dir);
    virtual bool can_layermove() { return true; }

    virtual void set_position(float x, float y, uint8_t fr=0);
    void set_layer(int l);
    virtual void set_fixture_layer(int l);

    virtual void step();
    virtual void pre_step();
    virtual void mstep();
    virtual void update();
    void look(int dir, bool force=false);

    double get_tangent_speed();
    double get_normal_speed();
    b2Vec2 get_normal_vector(float mag);
    float get_tangent_distance(b2Vec2 p);
    b2Vec2 get_tangent_vector(float mag);

    virtual bool is_action_active() { return false; };

    void set_ground_speed(float tangent, float normal);

    inline void set_speed(float speed, bool recalculate=true)
    {
        this->base_speed = speed;

        if (this->base_speed < CREATURE_MIN_SPEED) this->base_speed = CREATURE_MIN_SPEED;
        else if (this->base_speed > CREATURE_MAX_SPEED) this->base_speed = CREATURE_MAX_SPEED;

        if (recalculate) this->recalculate_effects();
    }

    inline float get_speed()
    {
        float s = this->speed * (this->speed_modifier < 0.f ? 1.f : this->speed_modifier);

        if (this->creature_flag_active(CREATURE_WANDERING))
            s *= .5f;

        return s;
    }
    inline float get_jump_strength()
    {
        return this->jump_strength + ((this->jump_strength_multiplier * this->jump_strength));
    }

    float get_upper_mass()
    {
        float mass = 0.f;
        b2Body *b;

        for (int x=0; x<this->get_num_bodies(); x++) {
            if (x < 1 || x > 4) {
                if ((b = this->get_body(x))) {
                    mass += b->GetMass();
                }
            }
        }

        return mass;
    }

    virtual void reset_limbs() { }

    virtual bool is_friend(entity *e) { return true; }
    virtual bool is_neutral(entity *e) { return false; }
    virtual bool is_enemy(entity *e) { return false; }

    /* Returns true if the creature is currently attached with an activator.
     * This requires there to be a joint between the creature and the activator. */
    virtual bool is_attached_to_activator() { return (this->cur_activator != 0); }

    /* Returns true if the creature is connected via a joint to another entity */
    bool has_attachment();

    bool is_player();
    virtual bool is_roaming() { return 0; }
    inline bool is_dead() { return this->state_active(CREATURE_DEAD); }
    inline bool is_alive() { return !this->is_dead(); }
    inline bool is_idle() { return this->state_active(CREATURE_IDLE); }
    inline bool is_walking() { return this->state_active(CREATURE_WALK); }
    inline bool is_moving_left() { return this->creature_flag_active(CREATURE_MOVING_LEFT); }
    inline bool is_moving_right() { return this->creature_flag_active(CREATURE_MOVING_RIGHT); }
    inline bool is_climbing_ladder() { return this->creature_flag_active(CREATURE_CLIMBING_LADDER) && (this->ladder_time <= 8000/*WORLD_STEP*/); }
    inline bool is_currently_climbing() { return this->motion == MOTION_CLIMBING; }
    inline bool is_panicked() { return this->creature_flag_active(CREATURE_PANICKED); }
    inline bool is_frozen() { return this->creature_flag_active(CREATURE_FROZEN); }
    inline bool is_wandering() { return this->creature_flag_active(CREATURE_WANDERING); }
    inline bool is_moving_up() { return this->creature_flag_active(CREATURE_MOVING_UP); }
    inline bool is_moving_down() { return this->creature_flag_active(CREATURE_MOVING_DOWN); }
    virtual inline bool is_spikebot() { return false; }

    inline bool has_circuit(uint32_t circuit) const
    {
        return this->circuits & circuit;
    }

    inline void set_has_circuit(uint32_t circuit, bool v)
    {
        if (v) {
            this->circuits |= circuit;
        } else {
            this->circuits &= ~circuit;
        }
    }

    inline bool has_feature(uint64_t flag) const
    {
        return this->features & flag;
    }

    inline bool creature_flag_active(uint64_t flag) const
    {
        return this->creature_flags & flag;
    }

    bool last_attacker_was_player() const;

    inline void set_creature_flag(uint64_t flag, bool v)
    {
        if (v) {
            this->creature_flags |= flag;
        } else {
            this->creature_flags &= ~flag;
        }
    }

    robot_parts::equipment *get_equipment_by_fixture(b2Fixture *f)
    {
        for (int x=0; x<NUM_EQUIPMENT_TYPES; x++) {
            if (this->equipments[x] && this->equipments[x]->fx == f) {
                return this->equipments[x];
            }
        }

        return 0;
    }

    inline bool is_foot_fixture(b2Fixture *f)
    {
        if (!f) return false;
        if (!this->feet || !this->feet->is_foot_fixture(f)) return false;
        return true;
    }

    virtual b2Fixture* get_sensor_fixture() { return 0; }

    virtual float get_damage_multiplier(b2Fixture *f)
    {
        if (this->is_foot_fixture(f)) {
            return this->feet->damage_multiplier;
        } else
            return 1.f;
    };

    virtual float get_damage_sensitivity(b2Fixture *f)
    {
        if (this->is_foot_fixture(f)) {
            return this->feet->damage_sensitivity;
        } else
            return 600.f;
    };

    inline int get_state() { return this->_state; }
    inline bool state_active(int state) { return this->get_state() == state; }
    void set_state(int new_state);

    inline void go()
    {
        this->set_state(CREATURE_WALK);
    }

    inline void stop()
    {
        if (this->finished) return;

        this->set_state(CREATURE_IDLE);
    }

    virtual void reset_damping();
    virtual void set_damping(float v);

    virtual void reset_friction()=0;
    virtual void set_friction(float v);

    void lose_balance();
    void try_regain_balance();
    void on_dir_change();

    float get_z();
    void damage(float amount, b2Fixture *f, damage_type damage_type, uint8_t damage_source, uint32_t attacker_id);
    virtual void on_damage(float amount, b2Fixture *f, damage_type damage_type, uint8_t damage_source, uint32_t attacker_id);
    virtual float get_adjusted_damage(float amount, b2Fixture *f, damage_type damage_type, uint8_t damage_source, uint32_t attacker_id);

    virtual robot_parts::tool* get_tool() const { return 0; }
    int get_tool_type() const
    {
        return this->get_tool() ? this->get_tool()->get_arm_type() : -1;
    }
    virtual robot_parts::weapon* get_weapon() const { return 0; }
    int get_weapon_type() const
    {
        return this->get_weapon() ? this->get_weapon()->get_arm_type() : -1;
    }

    float get_armour(){return this->armour;};
    float get_max_armour(){return this->max_armour;};
    float get_max_hp(){return this->max_hp;};
    float get_hp(){return this->hp;};
    void set_hp(float f){this->hp = f;};
    bool increase_max_hp(float m)
    {
        float cur_max_hp = this->max_hp;
        float new_max_hp = cur_max_hp + m;

        if (new_max_hp > CREATURE_BASE_MAX_MAX_HP)
            new_max_hp = CREATURE_BASE_MAX_MAX_HP;
        else if (new_max_hp < CREATURE_BASE_MIN_MAX_HP)
            new_max_hp = CREATURE_BASE_MIN_MAX_HP;

        this->max_hp = new_max_hp;

        if (this->hp > this->max_hp) this->hp = this->max_hp;

        return new_max_hp != cur_max_hp;
    }

    void attach_to(entity *e);
    void detach();

    inline void
    unset_attached()
    {
        if (this->body) {
            //this->body->SetAngularDamping(this->angular_damping);
        }

        if (this->balance) {
            this->balance->max_force = .8f;
            this->balance->multiplier = 1.f;
            this->balance->limit = 100.f;
            //this->balance->limit = .4f;
        }

        this->fixed_dir = false;
    }

    inline void
    set_attached(int dir)
    {
        //this->body->SetAngularDamping(20.f);
        //this->balance->limit = 500.f;
        //this->balance->max_force = 4.f;
        //this->balance->multiplier = 20.f;
        //this->balance->limit = 20.f;
        this->balance->limit = 100.f;
        this->fixed_dir = true;
        this->look_dir = dir;
    }

    void set_checkpoint(checkpoint *c);

    /* adventure mode */
    void stop_moving(int dir);

    bool can_climb_ladder()
    {
        return this->feet && this->feet->can_climb_ladder();
    }

    void on_release_playing();

    virtual bool consume(item *c, bool silent, bool first=false) {return false;};
    virtual void attack(int add_cooldown=0) {};
    virtual void attack_stop(){};
    void tool_stop(){
        if (this->get_tool()) {
            this->get_tool()->stop();
        }
    };
    virtual void action_on() {};
    virtual void action_off() {};

    virtual int get_default_feet_type(){return FEET_BIPED;};
    virtual int get_default_head_type(){return HEAD_ROBOT;};
    virtual int get_default_head_equipment_type(){return HEAD_EQUIPMENT_NULL;};
    virtual int get_default_front_equipment_type(){return FRONT_EQUIPMENT_NULL;};
    virtual int get_default_back_equipment_type(){return BACK_EQUIPMENT_NULL;};
    virtual int get_default_bolt_set(){return BOLT_SET_STEEL;};

    bool set_equipment(int e_category, int e_type);
    bool set_bolt_set(int type);
    bool set_tool(int tool_id, robot_parts::tool *t = 0);
    bool set_weapon(int weapon_id, robot_parts::weapon *t = 0);

    void remove_weapon(robot_parts::weapon *w);
    void remove_tool(robot_parts::tool *t);

    void add_to_world();
    virtual void create_fixtures()=0;

    virtual void aim(float a){};
    virtual void modify_aim(float da){};
    virtual float get_aim() { return 0.f; };

    static float real_arm_angle(creature *c, float a);

    b2Vec2 get_smooth_velocity();

    bool apply_effect(uint8_t item_type, const creature_effect &e);
    void recalculate_effects();

    inline void set_attack_damage_modifier(float v) { this->attack_damage_modifier = v; }
    inline float get_attack_damage_modifier() { return this->attack_damage_modifier; }

    friend class robot_base;
    friend class animal;
    friend class game;
};
