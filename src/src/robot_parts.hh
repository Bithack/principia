#pragma once

#include "entity.hh"
#include "types.hh"
#include <vector>

class magic_effect;
class creature;
class robot_base;
class robot;
class explosive;
class flame_effect;
class tesla_effect;

struct entity_hit
{
    p_entity *e;
    b2Fixture *fx;

    entity_hit(p_entity *_e, b2Fixture *_fx)
        : e(_e)
        , fx(_fx)
    { }
};

#define BULLET_LIFE 5000000
#define BULLET_VELOCITY 22.f

#define RAILGUN_MAX_POINTS 40
#define RAILGUIN_MIN_LEN .05f
#define RAILGUN_REACH 200.f

#define TIMECTRL_CHARGEUP_TIME 100*1000
#define TIMECTRL_COOLDOWN 500*1000

#define MINER_BASE_R .4f
#define MINER_BASE_G .4f
#define MINER_BASE_B .9f
#define MINER_BASE_A 1.f
#define MINER_MAX_DAMAGE 5.f

#define JETPACK_MAX_FUEL 30.f
#define JETPACK_FUEL_RECHARGE_RATE      .1f
#define JETPACK_FUEL_CONSUMPTION_RATE   .45f
#define JETPACK_FORCE_MUL 20.f

#define UPGRADED_JETPACK_MAX_FUEL 100.f
#define UPGRADED_JETPACK_FUEL_RECHARGE_RATE         .4f
#define UPGRADED_JETPACK_FUEL_CONSUMPTION_RATE      .7f
#define UPGRADED_JETPACK_FUEL_SIDE_CONSUMPTION_RATE .2f
#define UPGRADED_JETPACK_FORCE_MUL 20.f
#define UPGRADED_JETPACK_SIDE_FORCE_MUL 8.f

#define ADVANCED_JETPACK_MAX_FUEL 500.f
#define ADVANCED_JETPACK_FUEL_RECHARGE_RATE         .5f
#define ADVANCED_JETPACK_FUEL_CONSUMPTION_RATE      .7f
#define ADVANCED_JETPACK_FORCE_MUL 20.f

#define BASE_FOOT_SIZE .25f

#define ROBOT_ARM_POS b2Vec2(0.f, 0.25f)

#define MEGABUSTER_CHARGE_MAX 2200000llu
#define MEGABUSTER_CHARGE_MIN  500000llu
#define MEGABUSTER_CHARGE_DAMAGE 100.f

#define ROCKET_LAUNCHER_MISSILES 2

#define MEGA_BUSTER_COLOR_R .4f
#define MEGA_BUSTER_COLOR_G .5f
#define MEGA_BUSTER_COLOR_B .86f
#define MEGA_BUSTER_COLOR MEGA_BUSTER_COLOR_R, MEGA_BUSTER_COLOR_G, MEGA_BUSTER_COLOR_B

#define ARM_CANNON_COLOR .3f, .3f, .3f

#define COMPRESSOR_NUM_ITEMS_1_5 4

#define COMPRESSOR_NUM_ITEMS COMPRESSOR_NUM_ITEMS_1_5
#define COMPRESSOR_EMIT_TIME 1000000

enum {
    EQUIPMENT_HEAD,
    EQUIPMENT_FEET,
    EQUIPMENT_BACK,
    EQUIPMENT_FRONT,
    EQUIPMENT_HEADWEAR,

    NUM_EQUIPMENT_TYPES,
};

enum {
    ARM_WEAPON,
    ARM_TOOL,

    NUM_ARM_TYPES
};

#define NUM_EQUIPMENT_TYPES_1_5 NUM_EQUIPMENT_TYPES

extern int _equipment_required_features[NUM_EQUIPMENT_TYPES];
extern uint64_t _equipment_destruction_flags[NUM_EQUIPMENT_TYPES];

enum {
    WEAPON_NULL,
    WEAPON_ARM_CANNON,
    WEAPON_SHOTGUN,
    WEAPON_RAILGUN,
    WEAPON_ROCKET,
    WEAPON_BOMBER,
    WEAPON_TESLA,
    WEAPON_PLASMAGUN,
    WEAPON_MEGABUSTER, /* 1.5 */
    WEAPON_TRAINING_SWORD,
    WEAPON_WAR_HAMMER,
    WEAPON_SIMPLE_AXE,
    WEAPON_CHAINSAW,
    WEAPON_SPIKED_CLUB,
    WEAPON_STEEL_SWORD,
    WEAPON_BASEBALLBAT,
    WEAPON_SPEAR,
    WEAPON_WAR_AXE,
    WEAPON_PIXEL_SWORD,
    WEAPON_SERPENT_SWORD,
    WEAPON_PICKAXE,

    NUM_WEAPONS
};

#define NUM_WEAPONS_1_5 NUM_WEAPONS

enum {
    TOOL_NULL,
    TOOL_ZAPPER,
    TOOL_TIMECTRL,
    TOOL_DEBUGGER,
    TOOL_PAINTER,
    TOOL_BUILDER,
    TOOL_FACTION_WAND,
    TOOL_COMPRESSOR,

    NUM_TOOLS
};

#define NUM_TOOLS_1_5 NUM_TOOLS

enum {
    FEET_NULL       = 0,
    FEET_BIPED      = 1,
    FEET_MINIWHEELS = 2,
    FEET_QUADRUPED  = 3,
    FEET_MONOWHEEL  = 4,

    NUM_FEET_TYPES
};

enum {
    HEAD_NULL  = 0,
    HEAD_ROBOT = 1,
    HEAD_COW   = 2,
    HEAD_PIG   = 3,
    HEAD_ROBOT_UNCOVERED   = 4,
    HEAD_OSTRICH = 5,
    HEAD_DUMMY = 6,

    NUM_HEAD_TYPES
};

enum {
    BACK_EQUIPMENT_NULL             = 0,
    BACK_EQUIPMENT_ROBOT_BACK       = 1,
    BACK_EQUIPMENT_JETPACK          = 2,
    BACK_EQUIPMENT_UPGRADED_JETPACK = 3,
    BACK_EQUIPMENT_ADVANCED_JETPACK = 4,
    BACK_EQUIPMENT_BLACK_ROBOT_BACK = 5,
    BACK_EQUIPMENT_PIONEER_BACK  = 6,

    NUM_BACK_EQUIPMENT_TYPES
};

enum {
    FRONT_EQUIPMENT_NULL              = 0,
    FRONT_EQUIPMENT_ROBOT_FRONT       = 1,
    FRONT_EQUIPMENT_BLACK_ROBOT_FRONT = 2,
    FRONT_EQUIPMENT_PIONEER_FRONT  = 3,

    NUM_FRONT_EQUIPMENT_TYPES
};

enum {
    HEAD_EQUIPMENT_NULL       = 0,
    HEAD_EQUIPMENT_NINJAHELMET = 1,
    HEAD_EQUIPMENT_HEISENBERG  = 2,
    HEAD_EQUIPMENT_WIZARDHAT  = 3,
    HEAD_EQUIPMENT_CONICALHAT = 4,
    HEAD_EQUIPMENT_POLICEHAT = 5,
    HEAD_EQUIPMENT_TOPHAT = 6,
    HEAD_EQUIPMENT_KINGSCROWN = 7,
    HEAD_EQUIPMENT_JESTERHAT = 8,
    HEAD_EQUIPMENT_WITCH_HAT = 9,
    HEAD_EQUIPMENT_HARD_HAT = 10,
    HEAD_EQUIPMENT_VIKING_HELMET = 11,

    NUM_HEAD_EQUIPMENT_TYPES
};

enum {
    BOLT_SET_WOOD     = 0,
    BOLT_SET_STEEL    = 1,
    BOLT_SET_SAPPHIRE = 2,
    BOLT_SET_DIAMOND  = 3,

    NUM_BOLT_SETS
};

enum {
    HEADWEAR_ATTACHMENT_TOP    = 0,
    HEADWEAR_ATTACHMENT_CENTRE = 1,
};

extern int _head_to_item[NUM_HEAD_TYPES];
extern int _bolt_to_item[NUM_BOLT_SETS];
extern int _back_to_item[NUM_BACK_EQUIPMENT_TYPES];
extern int _front_to_item[NUM_FRONT_EQUIPMENT_TYPES];
extern int _head_equipment_to_item[NUM_HEAD_EQUIPMENT_TYPES];
extern int _tool_to_item[NUM_TOOLS];
extern int _weapon_to_item[NUM_WEAPONS];
extern int _feet_to_item[NUM_FEET_TYPES];

uint32_t _circuit_flag_to_item(uint32_t circuit_id);

namespace robot_parts
{
    /**
     * Base equipment class, adds and removes fixtures from the creature's body
     **/
    class equipment : public entity /* TODO: use tms::entity instead, this one's big! */
    {
      public:
        creature        *r;
        b2Fixture       *fx;
        b2PolygonShape   shape;

        equipment(creature *r);

        void update();
        void add_to_world();
        void remove_from_world();

        virtual void separate();
        virtual void on_dir_change();
        void get_shape_for_dir(b2Vec2 *out, int dir);

        virtual void add_as_child();
        virtual void remove_as_child();
        virtual int get_equipment_category()=0;
        virtual int get_equipment_type()=0;
        uint32_t get_item_id();
        void write_state(lvlinfo *lvl, lvlbuf *lb){};
        void read_state(lvlinfo *lvl, lvlbuf *lb){};

        virtual float get_adjusted_damage(float amount, b2Fixture *f, uint8_t damage_type, uint8_t damage_source, uint32_t attacker_id)=0;

        const char *get_name(){return "Equipment";};

        static equipment *make(creature *c, int e_category, int e_type);
    };

    class head_base : public equipment
    {
      public:
        head_base(creature *c);
        tvec2 local_offset;
        tvec2 model_offset;

        float get_adjusted_damage(float amount, b2Fixture *f, uint8_t damage_type, uint8_t damage_source, uint32_t attacker_id)
        {
            return amount;
        };

        inline b2Body* get_body(uint8_t fr) { return this->body; }

        virtual int get_equipment_type()=0;
        virtual int get_equipment_category(){return EQUIPMENT_HEAD;};

        void separate();

        virtual b2Vec2 get_centre_anchor(){return b2Vec2(0.f, 0.f);};
        virtual float get_centre_angle(){return 0.f;};
        virtual b2Vec2 get_top_anchor(){return b2Vec2(0.f, .2f);};
        virtual float get_top_angle(){return 0.f;};
        virtual float get_width()=0;
        virtual float get_height()=0;

        void on_dir_change();

        void add_to_world();
        void remove_from_world();

        virtual void create_fixtures() = 0;
        virtual void dangle();
        virtual void set_layer(int layer);
        virtual void update();
    };

    class robot_head : public head_base
    {
      public:
        robot_head(creature *c);

        int get_equipment_type(){return HEAD_ROBOT;};
        void create_fixtures();

        virtual float get_width(){return .600f;};
        virtual float get_height(){return .536f;};
    };

    class robot_head_inside : public robot_head
    {
      public:
        robot_head_inside(creature *c);
        int get_equipment_type(){return HEAD_ROBOT_UNCOVERED;};
        
        b2Vec2 get_top_anchor(){return b2Vec2(0.f, .107f);};
    };

    class cow_head : public head_base
    {
      public:
        cow_head(creature *c);

        int get_equipment_type(){return HEAD_COW;};
        float get_width(){return .75f;};
        float get_height(){return .656f;};
        float get_top_angle(){return -50.f;};
        b2Vec2 get_top_anchor(){return b2Vec2(-.1f, .2f);};
        void create_fixtures();
    };

    class pig_head : public head_base
    {
      public:
        pig_head(creature *c);

        int get_equipment_type(){return HEAD_PIG;};
        float get_width(){return .46f;};
        float get_height(){return .65f;};
        float get_top_angle(){return -50.f;};
        b2Vec2 get_top_anchor(){return b2Vec2(-.1f, .2f);};
        void create_fixtures();
    };

    class ostrich_head : public head_base
    {
      public:
        ostrich_head(creature *c);

        int get_equipment_type(){return HEAD_OSTRICH;};
        float get_width(){return .46f;};
        float get_height(){return .65f;};
        float get_top_angle(){return -30.f;};
        b2Vec2 get_top_anchor(){return b2Vec2(.07f, 1.10f);};
        void create_fixtures();
    };
    
    class dummy_head : public head_base
    {
      public:
        dummy_head(creature *c);

        int get_equipment_type(){return HEAD_DUMMY;};
        void create_fixtures();

        virtual float get_width(){return .600f;};
        virtual float get_height(){return .536f;};
        
        float get_top_angle(){return -25.f;};
        b2Vec2 get_top_anchor(){return b2Vec2(-.05f, .105f);};
    };

    class feet_base : public equipment
    {
      public:
        bool        soft;
        bool        on;
        bool        do_step;
        float       stepcount;
        b2Body     *body;
        int         body_index;
        float       offset;
        float       local_x_offset;
        float       damage_sensitivity; /* impact impulse required before damage is taken */
        float       damage_multiplier;  /* damage multiplier, so strong feet might have a damage multiplier of .5 to reduce damage taken */
        bool        disable_sound;

        feet_base(creature *c);
        virtual void step()=0;

        float get_adjusted_damage(float amount, b2Fixture *f, uint8_t damage_type, uint8_t damage_source, uint32_t attacker_id)
        {
            return amount*.5f;
        };

        void on_dir_change(){};

        float get_offset();

        void add_to_world();
        void remove_from_world();

        virtual b2Body *get_body(uint8_t x){if (x == 0) return this->body; else return 0;};
        virtual b2Vec2 get_body_offset(int x){return b2Vec2(0.f, 0.f);};
        virtual uint32_t get_num_bodies(){return 1;};

        virtual bool can_climb_ladder(){return false;};

        virtual void update_fixture() = 0;
        virtual void create_fixtures()=0;

        virtual bool is_foot_fixture(b2Fixture *f)=0;
        virtual void reset_angles(){};
        virtual void reset(){this->set_on(true);};

        virtual float get_tilt(){return 0.f;};
        void set_on(bool on);

        int get_equipment_category()
        {
            return EQUIPMENT_FEET;
        };

        virtual void dangle() { body=0; };
        virtual void set_layer(int n)=0;
        virtual void handle_contact(b2Contact *contact, b2Fixture *f, b2Fixture *other, const b2Manifold *man, float base_tangent, bool rev)=0;
    };

    class monowheel : public feet_base
    {
      public:
        b2Fixture *f;
        monowheel(creature *c);

        int get_equipment_type(){return FEET_MONOWHEEL;};

        void dangle()
        {
            f = 0;
            feet_base::dangle();
        };

        bool is_foot_fixture(b2Fixture *f)
        {
            return f == this->f;
        }
        void set_layer(int n);
        void update_fixture();
        void create_fixtures();
        void step();
        void handle_contact(b2Contact *contact, b2Fixture *f, b2Fixture *other, const b2Manifold *man, float base_tangent, bool rev);

        void update();
    };

    class miniwheels : public feet_base
    {
        class wheel : public tms::entity
        {
          public:
            float pos;
            float z;
            miniwheels *parent;
            wheel(miniwheels *parent, float pos, float z);
            void update();
        } *wheels[4];

      public:
        b2Fixture  *f0, *f1;
        miniwheels(creature *c);
        void add_as_child();
        void remove_as_child();

        int get_equipment_type(){return FEET_MINIWHEELS;};

        void dangle()
        {
            f0 = 0;
            f1 = 0;
            feet_base::dangle();
        };

        bool is_foot_fixture(b2Fixture *f)
        {
            return f == f0 || f == f1;
        }
        void set_layer(int n);
        void update_fixture();
        void create_fixtures();
        void step();
        void handle_contact(b2Contact *contact, b2Fixture *f, b2Fixture *other, const b2Manifold *man, float base_tangent, bool rev);

        void update(){
            this->wheels[0]->update();
            this->wheels[1]->update();
            this->wheels[2]->update();
            this->wheels[3]->update();
        };
    };

    class feet;

    class leg : public tms::entity
    {
      private:
        float transform[16];
        int d;
        int body_index;
        float length;
        creature *c;
        feet *f;

      public:
        float cc;
        leg(int d, creature* c, feet *f);
        class foot : public tms::entity
        {
          private:
            leg *l;
            int d;

          public:
            foot(int d, leg* l);
            virtual void update();
        };

        virtual void update();
        foot *myfoot;
    };

    class quadruped : public feet_base
    {
      public:
        feet *feets[2];
        float x_offset;

        quadruped(creature *c);
        ~quadruped();
        void add_as_child();
        void remove_as_child();

        b2Vec2 get_body_offset(int x){return b2Vec2(0.f,0.f);}
        uint32_t get_num_bodies(){ return 2; };
        b2Body *get_body(uint8_t x);

        void step();
        void update_fixture();
        void create_fixtures();

        void add_to_world();
        void remove_from_world();

        void reset_angles();
        bool is_foot_fixture(b2Fixture *f);
        void set_layer(int l);
        void dangle();
        void update();
        void handle_contact(b2Contact *contact, b2Fixture *f, b2Fixture *other, const b2Manifold *man, float base_tangent, bool rev);

        int get_equipment_type(){return FEET_QUADRUPED;};
    };

    class feet : public feet_base
    {
      public:
        leg *legs[2];
        float y;
        b2Fixture  *f0, *f1;
        float sound_accum;
        float gangle_timer[2];
        float foot_normal[2];
        float gangle[2];
        bool  foot_blocked[2];
        float foot_scale;

        feet(creature *c);

        bool can_climb_ladder(){return true;};

        float get_tilt()
        {
            float tilt = cos(this->stepcount);
            tilt *= .05f;
            return tilt;
        };

        void update();

        void reset_angles();
        void reset()
        {
            feet_base::reset();

            float ga = -M_PI/2.f;
            this->foot_normal[0] = ga;
            this->foot_normal[1] = ga;
            this->gangle[0] = ga;
            this->gangle[1] = ga;
            this->sound_accum = 0;
            this->gangle_timer[0] = 0.f;
            this->gangle_timer[1] = 0.f;
            this->foot_blocked[0] = false;
            this->foot_blocked[1] = false;
        }

        void dangle()
        {
            this->f0 = 0;
            this->f1 = 0;
            feet_base::dangle();
        }

        bool is_foot_fixture(b2Fixture *f)
        {
            return f == f0 || f == f1;
        }
        void set_layer(int n);
        void update_fixture();
        void create_fixtures();
        void step();
        void handle_contact(b2Contact *contact, b2Fixture *f, b2Fixture *other, const b2Manifold *man, float base_tangent, bool rev);

        int get_equipment_type(){return FEET_BIPED;};
    };

    class arm : public tms::entity
    {
      protected:
        creature *c;
        float arm_angle;
        float arm_fold;
        int cooldown;
        bool used;
        bool active;

      public:
        bool do_update_effects;
        arm(creature *c);
        void setup() { this->reset_settings(); }
        void update();
        virtual const char *get_name()=0;
        virtual void step();
        virtual void on_attack();
        void reset_settings()
        {
            this->arm_angle = 0.f;
            this->arm_fold = 0.f;
            this->cooldown_timer = 0;
            this->used = false;
        }

        uint32_t get_item_id() const;

        virtual int get_arm_type() const = 0;
        virtual int get_arm_category() const = 0;

        inline int get_weapon_type() const { return this->get_arm_type(); }
        inline int get_tool_type() const { return this->get_arm_type(); }

        /* By default, weapons and tools are dropped on death. */
        virtual bool dropped_on_death() const
        {
            return true;
        }

        inline bool is_active() const
        {
            return this->active;
        }

        virtual void set_arm_angle(float a, float speed=1.f);
        void set_arm_angle_raw(float a);
        float get_arm_angle();
        virtual float get_angle_diff(float a)
        {
            return std::abs(this->get_arm_angle() - a);
        }

        void set_arm_fold(float v);
        float get_arm_fold();

        float get_cooldown_fraction();

        virtual void update_effects(){};

        void read_state(lvlinfo *lvl, lvlbuf *lb){};
        void write_state(lvlinfo *lvl, lvlbuf *lb){};

        uint64_t pointer_id;
        int cooldown_timer;
        bool fired;
    };

    class tool : public arm
    {
      public:
        tool(creature *c);
        virtual int action(uint32_t type, uint64_t pointer_id, tvec2 pos) = 0;
        virtual void step();

        static tool *make(int tool_id, creature *c);

        virtual void stop(){this->active = false;};

        int get_arm_category() const { return ARM_TOOL; }
    };

    class nulltool : public tool
    {
      public:
        nulltool(creature *c);
        const char* get_name() { return "Nulltool"; }

        int action(uint32_t type, uint64_t pointer_id, tvec2 pos);

        int get_arm_type() const { return TOOL_NULL; }
    };

    class builder : public tool
    {
      public:
        builder(creature *c);
        const char* get_name() { return "Builder"; }

        int action(uint32_t type, uint64_t pointer_id, tvec2 pos);
        void stop();

        int get_arm_type() const { return TOOL_BUILDER; }
        bool dropped_on_death() const { return false; }
    };

    class miner : public tool
    {
      public:
        miner(creature *c);
        const char* get_name() { return "Zapper"; }

        void step();
        int action(uint32_t type, uint64_t pointer_id, tvec2 pos);

        void set_damage(float new_damage);
        float damage;
        void stop();

        int get_arm_type() const { return TOOL_ZAPPER; }
        bool dropped_on_death() const { return false; }
    };

    class faction_wand : public tool
    {
      private:
        uint32_t controlling_id;
        magic_effect *effect;

      public:
        faction_wand(creature *c);
        const char* get_name() { return "Faction Wand"; }

        int action(uint32_t type, uint64_t pointer_id, tvec2 pos);
        void update_effects();
        void stop();

        int get_arm_type() const { return TOOL_FACTION_WAND; }
    };

    class compressor : public tool
    {
      private:
        struct compressor_item {
            uint32_t g_id;
            uint32_t sub_id;
        } storage[COMPRESSOR_NUM_ITEMS];

        struct tms_entity lights[COMPRESSOR_NUM_ITEMS];

        int32_t emit_timer;

      public:
        uint8_t item_index;
        compressor(creature *c);
        const char* get_name() { return "Compressor"; }

        int action(uint32_t type, uint64_t pointer_id, tvec2 pos);
        void step();
        void stop();

        void update();

        void write_state(lvlinfo *lvl, lvlbuf *lb);
        void read_state(lvlinfo *lvl, lvlbuf *lb);

        int get_arm_type() const { return TOOL_COMPRESSOR; }
    };

    class timectrl : public tool
    {
      public:
        timectrl(creature *c);
        const char *get_name() { return "Time control"; }

        int action(uint32_t type, uint64_t pointer_id, tvec2 pos);
        void step();

        bool is_down;
        int chargeup_timer;
        int fire_timer;

        int get_arm_type() const { return TOOL_TIMECTRL; }
    };

    class debugger : public tool/*, public edevice*/
    {
      public:
        debugger(creature *c);
        const char *get_name() { return "Debugger"; }

        int action(uint32_t type, uint64_t pointer_id, tvec2 pos);
        void step();

        /*
        edevice *get_edevice(){return (edevice*)this;};
        entity *get_entity(){return (entity*)this;};
        */

        ::entity *edev;

        int get_arm_type() const { return TOOL_DEBUGGER; }
    };

    class painter : public tool
    {
      public:
        painter(creature *c);
        const char *get_name() { return "Painter"; }

        int action(uint32_t type, uint64_t pointer_id, tvec2 pos);
        void update_appearance();

        bool painting;

        int get_arm_type() const { return TOOL_PAINTER; }
    };

    class weapon : public arm
    {
      protected:
        float max_range; /* TODO: add optimal range */

      public:
        weapon(creature *c)
            : arm(c)
        {
            this->max_range = 9.f;
        }

        virtual int pre_attack() {return EVENT_CONT;};
        virtual void attack(int add_cooldown = 0) = 0;
        virtual void attack_stop(){};

        inline float get_max_range()
        {
            return this->max_range;
        }

        float terror;

        int get_arm_category() const { return ARM_WEAPON; }

        static weapon *make(int weapon_id, creature *c);

        virtual inline bool is_melee()
        {
            return false;
        }
    };

    class melee_weapon : public weapon, public b2RayCastCallback
    {
      protected:
        float arm_offset;
        float pending_arm_angle;
        float target_arm_angle;
        bool hit_static;

        float arm_movement;

        float dmg;
        float dmg_multiplier; /* Multiply the damage amount */
        float block_dmg_multiplier;
        float plant_dmg_multiplier;

        float force;
        float force_multiplier; /* Multiply the force amount */

        float strength; /* Multiplier used to modify the damage _AND_ force amount */

        float swing_speed;
        float mass;

        damage_type dmg_type;

        bool first_hit;

        b2Shape *shape;
        b2Vec2   shape_offset;

        inline float get_strength()
        {
            return this->strength;
        }

        inline float get_damage()
        {
            return this->dmg * this->dmg_multiplier;
        }

        inline float get_force()
        {
            return this->force * this->force_multiplier;
        }

        inline float get_mass()
        {
            return this->mass;
        }

        std::vector<struct entity_hit> m_results;

        const std::vector<struct entity_hit>& test_shape();

        inline bool is_melee()
        {
            return true;
        }

      public:
        melee_weapon(creature *c);

        virtual void step() = 0;
        virtual void attack(int add_cooldown=0) = 0;

        void set_arm_angle(float a, float speed=1.f);
        virtual float get_angle_diff(float a) { return 0.f; }

        void on_attack();
        void attack_stop();
        void raycast(const b2Vec2 &from, const b2Vec2 &to);
        float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    };

    class base_sword : public melee_weapon
    {
      public:
        base_sword(creature *c)
            : melee_weapon(c)
        {
            this->swing_speed = 0.25f;
            this->dmg_type = DAMAGE_TYPE_SHARP;
        }
        void step();
        void attack(int add_cooldown=0);
    };

    class base_hammer : public melee_weapon
    {
      protected:
        /* A state to determine whether the hammer is currently in its downwards arc.
         * This can be used to determine if it should do damage, or if it's time to
         * snap back to its original position. */
        bool hammering;

      public:
        base_hammer(creature *c)
            : melee_weapon(c)
        {
            /* By default, hammers set hammering to true.
             * This is an easy method for us to make sure the hammer gets to
             * the proper initial position at the beginning. */
            this->hammering = true;
            this->dmg_type = DAMAGE_TYPE_BLUNT;

            this->swing_speed = 0.2f;
        }
        void step();
        void attack(int add_cooldown=0);
    };
    
    class base_axe : public melee_weapon
    {
      public:
        base_axe(creature *c)
            : melee_weapon(c)
        {
            this->swing_speed = 0.25f;
            this->dmg_type = DAMAGE_TYPE_HEAVY_SHARP;
        }
        void step();
        void attack(int add_cooldown=0);
    };
    
    class base_chainsaw : public melee_weapon
    {
      protected:
        int32_t active_timer;
        float blade_rot;

      public:
        base_chainsaw(creature *c)
            : melee_weapon(c)
            , active_timer(0)
            , blade_rot(0.f)
        {
            this->arm_offset = 0.f;
            this->swing_speed = 0.25f;
            this->dmg_type = DAMAGE_TYPE_HEAVY_SHARP;
        }
        void step();
        void attack(int add_cooldown=0);
    };

    enum spear_state {
        SPEAR_IDLE,
        SPEAR_RETRACTING,
        SPEAR_PROTRACTING,
        SPEAR_FULL_LENGTH,
    };
    
    class base_spear : public melee_weapon
    {
      protected:
        /* REQUIRED */
        float retract_speed;
        float protract_speed;
        float sting_offset;
        float sting_length;

        enum spear_state state;

        float min_state_pos;
        float max_state_pos;
        float state_pos;

      public:
        base_spear(creature *c)
            : melee_weapon(c)
            , state(SPEAR_IDLE)
            , min_state_pos(0.f)
            , max_state_pos(1.f)
            , state_pos(min_state_pos)
        {
            this->arm_offset = 0.f;
            this->swing_speed = 0.0f;
            this->dmg_type = DAMAGE_TYPE_SHARP;
        }
        void step();
        void attack(int add_cooldown=0);

        void update();
    };

    class training_sword : public base_sword
    {
      public:
        training_sword(creature *c);
        const char *get_name() { return "Training sword"; }
        int get_arm_type() const { return WEAPON_TRAINING_SWORD; }
        bool dropped_on_death() const { return false; }
    };

    class war_hammer : public base_hammer
    {
      public:
        war_hammer(creature *c);
        const char *get_name() { return "War hammer"; }
        int get_arm_type() const { return WEAPON_WAR_HAMMER; }
        bool dropped_on_death() const { return false; }
    };
    
    class simple_axe : public base_axe
    {
      public:
        simple_axe(creature *c);
        const char *get_name() { return "Simple axe"; }
        int get_arm_type() const { return WEAPON_SIMPLE_AXE; }
        bool dropped_on_death() const { return false; }
    };
    
    class chainsaw : public base_chainsaw
    {
      protected:
        struct tms_entity inner;

      public:
        chainsaw(creature *c);
        const char *get_name() { return "Chainsaw"; }
        int get_arm_type() const { return WEAPON_CHAINSAW; }
        bool dropped_on_death() const { return false; }

        void update();
    };
    
    class spiked_club : public base_hammer
    {
      public:
        spiked_club(creature *c);
        const char *get_name() { return "Spiked Club"; }
        int get_arm_type() const { return WEAPON_SPIKED_CLUB; }
        bool dropped_on_death() const { return false; }
    };
    
    class steel_sword : public base_sword
    {
      public:
        steel_sword(creature *c);
        const char *get_name() { return "Steel Sword"; }
        int get_arm_type() const { return WEAPON_STEEL_SWORD; }
        bool dropped_on_death() const { return false; }
    };
    
    class baseballbat : public base_sword
    {
      public:
        baseballbat(creature *c);
        const char *get_name() { return "Baseball bat"; }
        int get_arm_type() const { return WEAPON_BASEBALLBAT; }
        bool dropped_on_death() const { return false; }
    };
    
    class spear : public base_spear
    {
      public:
        spear(creature *c);
        const char *get_name() { return "Spear"; }
        int get_arm_type() const { return WEAPON_SPEAR; }
        bool dropped_on_death() const { return false; }
    };
    
    class war_axe : public base_axe
    {
      public:
        war_axe(creature *c);
        const char *get_name() { return "War axe"; }
        int get_arm_type() const { return WEAPON_WAR_AXE; }
        bool dropped_on_death() const { return false; }
    };
    
    class pixel_sword : public base_sword
    {
      public:
        pixel_sword(creature *c);
        const char *get_name() { return "Pixel sword"; }
        int get_arm_type() const { return WEAPON_PIXEL_SWORD; }
        bool dropped_on_death() const { return false; }
    };
    
    class serpent_sword : public base_sword
    {
      public:
        serpent_sword(creature *c);
        const char *get_name() { return "Serpent Sword"; }
        int get_arm_type() const { return WEAPON_SERPENT_SWORD; }
        bool dropped_on_death() const { return false; }
    };
    
    class pickaxe : public base_axe
    {
      public:
        pickaxe(creature *c);
        const char *get_name() { return "Pickaxe"; }
        int get_arm_type() const { return WEAPON_PICKAXE; }
        bool dropped_on_death() const { return false; }
    };

    class arm_cannon : public weapon
    {
      public:
        arm_cannon(creature *c);
        void attack(int add_cooldown=0);
        const char* get_name() { return "Arm Cannon"; }

        bool dropped_on_death() const { return false; }
        int get_arm_type() const { return WEAPON_ARM_CANNON; }
    };

    class shotgun : public weapon
    {
      private:
        bool played_reload_sound;

      public:
        shotgun(creature *c);
        const char* get_name() { return "Shotgun"; }
        void attack(int add_cooldown=0);
        void step();

        int get_arm_type() const { return WEAPON_SHOTGUN; }
    };

    class railgun : public weapon
    {
      private:
        class cb_handler : public b2RayCastCallback
        {
          private:
            railgun *self;

          public:
            cb_handler(railgun *s)
            {
                this->self = s;
            };

            float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
        };

        cb_handler *handler;
        bool first_try;
        bool result;
        b2Vec2 points[RAILGUN_MAX_POINTS];
        uint16_t num_points;

        b2Fixture *result_fx;
        b2Vec2     result_nor;
        b2Vec2     result_pt;

        float active;
        float last_z;
        bool fired;
        b2Vec2 from;
        bool reflected;

      public:
        railgun(creature *c);
        ~railgun()
        {
            delete this->handler;
        }
        const char* get_name() { return "Railgun"; }
        void step();
        void attack(int add_cooldown=0);
        void update_effects();

        int get_arm_type() const { return WEAPON_RAILGUN; }
    };

    class megabuster : public weapon
    {
      private:
        bool active;
        uint64_t charge;
        float hl;
        int charge_snd;

      public:
        megabuster(creature *c);
        ~megabuster();

        const char* get_name() { return "Mega Buster"; }
        void step();
        void attack(int add_cooldown=0);
        void attack_stop();
        void update();

        int get_arm_type() const { return WEAPON_MEGABUSTER; }
    };

    class plasmagun : public weapon
    {
      private:
        bool active;
        float highlight;
        struct tms_entity inner;

      public:
        plasmagun(creature *c);
        ~plasmagun();

        const char* get_name() { return "Plasma Gun"; }
        void step();
        void attack(int add_cooldown=0);
        void attack_stop();
        void update();
        void update_effects();

        int get_arm_type() const { return WEAPON_PLASMAGUN; }
    };

    class teslagun : public weapon
    {
      private:
        tesla_effect *tesla;
        bool first_try;
        bool result;
        b2Vec2 points[RAILGUN_MAX_POINTS];
        uint16_t num_points;

        b2Fixture *result_fx;
        b2Vec2     result_nor;
        b2Vec2     result_pt;

        bool active;

      public:
        teslagun(creature *c);
        ~teslagun();

        const char* get_name() { return "Tesla Gun"; }
        void step();
        void attack(int add_cooldown=0);
        void attack_stop();
        void update_effects();

        int get_arm_type() const { return WEAPON_TESLA; }
    };

    class rocket_launcher : public weapon
    {
      private:
        bool played_reload_sound;
        struct tms_entity inner[ROCKET_LAUNCHER_MISSILES];
        bool fired;
        bool missile_switch;

      public:
        rocket_launcher(creature *c);
        const char* get_name() { return "Rocket Launcher"; }
        void attack(int add_cooldown=0);
        void step();
        void update();

        int get_arm_type() const { return WEAPON_ROCKET; }
    };

    class bomber : public weapon
    {
      private:
        struct tms_entity chamber;
        int chamber_rotation;

      public:
        bomber(creature *c);
        const char* get_name() { return "Bomber"; }
        int pre_attack();
        void attack(int add_cooldown=0);
        void update();

        explosive* bomb_fired;

        int get_arm_type() const { return WEAPON_BOMBER; }
    };

    class headwear : public equipment
    {
      protected:
        creature *r;

      public:
        headwear(creature *r);
        void update();
        void setup()
        {
            this->reset_settings();
        };
        void reset_settings()
        {

        };

        float get_adjusted_damage(float amount, b2Fixture *f, uint8_t damage_type, uint8_t damage_source, uint32_t attacker_id)
        {
            return amount*.5f;
        };

        virtual int get_attachment_point(){return HEADWEAR_ATTACHMENT_CENTRE;};

        void add_to_world(){};
        void remove_from_world(){};
        void on_dir_change(){};

        int get_equipment_category(){return EQUIPMENT_HEADWEAR;};

        virtual b2Vec2 get_offset()=0;
        virtual void step() {};
    };

    class heisenberghat : public headwear
    {
      public:
        heisenberghat(creature *r);
        const char *get_name() { return "Heisenberg hat"; }
        b2Vec2 get_offset()
        {
            return b2Vec2(0.f, .2f/2.f);
        };
        int get_attachment_point(){return HEADWEAR_ATTACHMENT_TOP;};
        int get_equipment_type(){return HEAD_EQUIPMENT_HEISENBERG;};
    };

    class wizardhat : public headwear
    {
      public:
        wizardhat(creature *r);
        const char *get_name() { return "Wizard Hat"; }
        b2Vec2 get_offset()
        {
            return b2Vec2(0.f, .1f);
        };
        int get_attachment_point(){return HEADWEAR_ATTACHMENT_TOP;};
        int get_equipment_type(){return HEAD_EQUIPMENT_WIZARDHAT;};
    };
    
    class witch_hat : public headwear
    {
      public:
        witch_hat(creature *r);
        const char *get_name() { return "Witch hat"; }
        b2Vec2 get_offset()
        {
            return b2Vec2(0.f, .1f);
        };
        int get_attachment_point(){return HEADWEAR_ATTACHMENT_TOP;};
        int get_equipment_type(){return HEAD_EQUIPMENT_WITCH_HAT;};
    };
    
    class kingscrown : public headwear
    {
      public:
        kingscrown(creature *r);
        const char *get_name() { return "King's Crown"; }
        b2Vec2 get_offset()
        {
            return b2Vec2(0.f, .1f);
        };
        int get_attachment_point(){return HEADWEAR_ATTACHMENT_TOP;};
        int get_equipment_type(){return HEAD_EQUIPMENT_KINGSCROWN;};
    };
    
    class tophat : public headwear
    {
      public:
        tophat(creature *r);
        const char *get_name() { return "Top Hat"; }
        b2Vec2 get_offset()
        {
            return b2Vec2(0.f, .1f);
        };
        int get_attachment_point(){return HEADWEAR_ATTACHMENT_TOP;};
        int get_equipment_type(){return HEAD_EQUIPMENT_TOPHAT;};
    };

    class conicalhat : public headwear
    {
      public:
        conicalhat(creature *r);
        const char *get_name() { return "Conical Hat"; }
        b2Vec2 get_offset()
        {
            return b2Vec2(0.f, 0.f);
        };
        int get_attachment_point(){return HEADWEAR_ATTACHMENT_TOP;};
        int get_equipment_type(){return HEAD_EQUIPMENT_CONICALHAT;};
    };

    class policehat : public headwear
    {
      public:
        policehat(creature *r);
        const char *get_name() { return "Police Hat"; }
        b2Vec2 get_offset()
        {
            return b2Vec2(0.f, 0.f);
        };
        int get_attachment_point(){return HEADWEAR_ATTACHMENT_TOP;};
        int get_equipment_type(){return HEAD_EQUIPMENT_POLICEHAT;};
    };

    class ninjahelmet : public headwear
    {
      public:
        ninjahelmet(creature *r);
        const char *get_name() { return "Ninja Helmet"; }
        b2Vec2 get_offset()
        {
            return b2Vec2(0.f, .0f);
        };
        int get_attachment_point(){return HEADWEAR_ATTACHMENT_CENTRE;};
        virtual int get_equipment_type(){return HEAD_EQUIPMENT_NINJAHELMET;};
    };
    
    class jesterhat : public headwear
    {
      public:
        jesterhat(creature *r);
        const char *get_name() { return "Jester hat"; }
        b2Vec2 get_offset()
        {
            return b2Vec2(0.f, .1f);
        };
        int get_attachment_point(){return HEADWEAR_ATTACHMENT_TOP;};
        int get_equipment_type(){return HEAD_EQUIPMENT_JESTERHAT;};
    };
    
    class hard_hat : public headwear
    {
      public:
        hard_hat(creature *r);
        const char *get_name() { return "Hard hat"; }
        b2Vec2 get_offset()
        {
            return b2Vec2(0.f, .1f);
        };
        int get_attachment_point(){return HEADWEAR_ATTACHMENT_TOP;};
        int get_equipment_type(){return HEAD_EQUIPMENT_HARD_HAT;};
    };
    
    class vikinghelmet : public headwear
    {
      public:
        vikinghelmet(creature *r);
        const char *get_name() { return "Viking Helmet"; }
        b2Vec2 get_offset()
        {
            return b2Vec2(0.f, .1f);
        };
        int get_attachment_point(){return HEADWEAR_ATTACHMENT_TOP;};
        int get_equipment_type(){return HEAD_EQUIPMENT_VIKING_HELMET;};
    };

    class back : public equipment
    {
      public:
        back(creature *r) : equipment(r) {};
        int get_equipment_category(){return EQUIPMENT_BACK;};

        float get_adjusted_damage(float amount, b2Fixture *f, uint8_t damage_type, uint8_t damage_source, uint32_t attacker_id)
        {
            return amount*.5f;
        };

        virtual bool on_jump(){return false;};
        virtual bool on_stop_jump(){return false;};
    };

    class front : public equipment
    {
      public:
        front(creature *r) : equipment(r) {};
        int get_equipment_category(){return EQUIPMENT_FRONT;};

        float get_adjusted_damage(float amount, b2Fixture *f, uint8_t damage_type, uint8_t damage_source, uint32_t attacker_id)
        {
            return amount*.5f;
        };

        virtual bool on_jump(){return false;};
        virtual bool on_stop_jump(){return false;};
    };

    class robot_front : public front
    {
      public:
        robot_front(creature *r);
        int get_equipment_type(){return FRONT_EQUIPMENT_ROBOT_FRONT;};
        const char *get_name(){return "Robot front";};
    };

    class robot_back : public back
    {
      public:
        robot_back(creature *r);
        int get_equipment_type(){return BACK_EQUIPMENT_ROBOT_BACK;};
        const char *get_name(){return "Robot back";};
    };
    
    class black_robot_back : public back
    {
      public:
        black_robot_back(creature *r);
        int get_equipment_type(){return BACK_EQUIPMENT_BLACK_ROBOT_BACK;};
        const char *get_name(){return "Black robot back";};
    };

    class black_robot_front : public front
    {
      public:
        black_robot_front(creature *r);
        const char *get_name() { return "Black robot front"; }

        void add_to_world(){};
        void remove_from_world(){};

        int get_equipment_type(){return FRONT_EQUIPMENT_BLACK_ROBOT_FRONT;};
    };
    
    class pioneer_front : public front
    {
      public:
        pioneer_front(creature *r);
        int get_equipment_type(){return FRONT_EQUIPMENT_PIONEER_FRONT;};
        const char *get_name(){return "Robot front test";};
    };
    
    class pioneer_back : public back
    {
      public:
        pioneer_back(creature *r);
        int get_equipment_type(){return BACK_EQUIPMENT_PIONEER_BACK;};
        const char *get_name(){return "Robot back test";};
    };

    class base_jetpack : public back
    {
      protected:
        float fuel;
        bool active;
        flame_effect *flames[2];

      public:
        base_jetpack(creature *r);

        virtual void add_to_world();

        virtual void step() = 0;

        void write_state(lvlinfo *lvl, lvlbuf *lb)
        {
            back::write_state(lvl, lb);

            lb->w_s_float(this->fuel);
        }

        void read_state(lvlinfo *lvl, lvlbuf *lb)
        {
            back::read_state(lvl, lb);

            this->fuel = lb->r_float();
        }
    };

    class jetpack : public base_jetpack
    {
      public:
        jetpack(creature *r);
        const char *get_name() { return "Jetpack"; }
        void step();

        bool on_jump();
        bool on_stop_jump();
        int get_equipment_type(){return BACK_EQUIPMENT_JETPACK;};
    };

    class upgraded_jetpack : public base_jetpack
    {
      public:
        upgraded_jetpack(creature *r);
        const char *get_name() { return "Upgraded Jetpack"; }
        void step();

        bool on_jump();
        bool on_stop_jump();
        int get_equipment_type(){return BACK_EQUIPMENT_UPGRADED_JETPACK;};
    };

    class advanced_jetpack : public base_jetpack
    {
      public:
        b2Vec2 _pos;
        advanced_jetpack(creature *c);
        const char *get_name() { return "Advanced Jetpack"; }
        void step();

        bool on_jump();
        int get_equipment_type(){return BACK_EQUIPMENT_ADVANCED_JETPACK;};
    };

    inline float arm::get_arm_angle()
    {
        return this->arm_angle;
    }

    inline void arm::set_arm_fold(float v)
    {
        this->arm_fold = v;
    }

    inline float arm::get_arm_fold()
    {
        return this->arm_fold;
    }

    inline float arm::get_cooldown_fraction()
    {
        float v = (float)this->cooldown_timer / this->cooldown;

        return tclampf(v, 0.f, 1.f);
    }
};
