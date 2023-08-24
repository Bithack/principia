#pragma once

#include "entity.hh"
#include "robot_base.hh"

#define ITEM_MAX_HEALTH 15.f

#define UNLOCKED_ITEM_LEVEL_ID 1275

struct item_option {
    const char *name;
    creature_effect *ef;
    int layer_mask;
    tms::material *material;
    struct tms_mesh **mesh;
    uint32_t category;
    tvec4 uniform;
    tvec2 size;
    uint32_t data_id;
    float menu_scale;
    bool can_rotate; /* if the item can have an arbitrary rotation, like heads and bodies */
    tvec2 mesh_offset;
    bool magnetic;
    bool step;
    bool update_effects;
    float rot_offs;
    struct worth worth;
    bool zappable;
    float activator_radius;

    struct tms_sprite image;
    struct tms_sprite *name_spr;

    item_option(const char *_name)
        : name(_name)
    {
        this->zappable = true;
        this->can_rotate = false;
        this->magnetic = false;
        this->step = false;
        this->update_effects = false;
        this->rot_offs = 0.f;
        this->layer_mask = 2+4;
        this->data_id = 0;
        this->ef = 0;
        this->menu_scale = 1.f;

        this->mesh = 0;

        this->mesh_offset.x = 0.f;
        this->mesh_offset.y = 0.f;

        this->activator_radius = -1.f;
    }

    struct item_option&
    set_effect(creature_effect *ef)
    {
        this->ef = ef;

        return *this;
    }

    struct item_option&
    set_zappable(bool val)
    {
        this->zappable = val;

        return *this;
    }

    struct item_option&
    set_layer_mask(int v)
    {
        this->layer_mask = v;

        return *this;
    }

    struct item_option&
    set_material(tms::material *material)
    {
        this->material = material;

        return *this;
    }

    struct item_option&
    set_mesh(struct tms_mesh **mesh)
    {
        this->mesh = mesh;

        return *this;
    }

    struct item_option&
    set_category(uint32_t category)
    {
        this->category = category;

        return *this;
    }

    struct item_option&
    set_uniform(float r, float g, float b, float a=1.f)
    {
        this->uniform = tvec4f(r, g, b, a);

        return *this;
    }

    struct item_option&
    set_size(float w, float h)
    {
        this->size.w = w;
        this->size.h = h;

        return *this;
    }

    struct item_option&
    set_data_id(uint32_t data_id)
    {
        this->data_id = data_id;

        return *this;
    }

    struct item_option&
    set_menu_scale(float scale)
    {
        this->menu_scale = scale;

        return *this;
    }

    struct item_option&
    set_mesh_offset(float x, float y)
    {
        this->mesh_offset.x = x;
        this->mesh_offset.y = y;

        return *this;
    }

    struct item_option&
    set_magnetic(bool val)
    {
        this->magnetic = val;

        return *this;
    }

    struct item_option&
    set_do_step(bool val)
    {
        this->step = val;

        return *this;
    }

    struct item_option&
    set_activator_radius(float val)
    {
        this->activator_radius = val;

        return *this;
    }

    struct item_option&
    set_do_update_effects(bool val)
    {
        this->update_effects = val;

        return *this;
    }

    struct item_option&
    set_can_rotate(bool val)
    {
        this->can_rotate = val;

        return *this;
    }

    struct item_option&
    set_rot_offs(float val)
    {
        this->rot_offs = val;

        return *this;
    }

    /* Add num resources */
    struct item_option& add_worth(uint8_t resource_type, uint32_t num)
    {
        this->worth.add(resource_type, num);

        return *this;
    }

    /* Add oil */
    struct item_option& add_oil(float val)
    {
        this->worth.add_oil(val);

        return *this;
    }

    /* Add resources and oil from another worth object */
    struct item_option& add_worth(const struct worth& _worth)
    {
        this->worth.add(_worth);

        return *this;
    }
};

enum {
    ITEM_ARM_CANNON             = 0,
    ITEM_BUILDER                = 1,
    ITEM_SHOTGUN                = 2,
    ITEM_RAILGUN                = 3,
    ITEM_OIL                    = 4,
    ITEM_SPEED_OIL              = 5,
    ITEM_JUMP_OIL               = 6,
    ITEM_ARMOUR_OIL             = 7,
    ITEM_ZAPPER                 = 8,
    ITEM_MINER_UPGRADE          = 9,
    ITEM_ROCKET_LAUNCHER        = 10,
    ITEM_SOMERSAULT_CIRCUIT     = 11,
    ITEM_JETPACK                = 12,
    ITEM_UPGRADED_JETPACK       = 13,
    ITEM_ADVANCED_JETPACK       = 14,
    ITEM_BOMBER                 = 15,
    ITEM_ROBOT_HEAD             = 16,
    ITEM_COW_HEAD               = 17,
    ITEM_PIG_HEAD               = 18,
    ITEM_ROBOT_FRONT            = 19,
    ITEM_TESLAGUN               = 20,
    ITEM_PLASMAGUN              = 21,
    ITEM_BULLET                 = 22,
    ITEM_SHOTGUN_PELLET         = 23,
    ITEM_PLASMA                 = 24,
    ITEM_ROCKET                 = 25,
    ITEM_HAT                    = 26,
    ITEM_MEGABUSTER             = 27,
    ITEM_SOLAR                  = 28,
    ITEM_BIPED                  = 29,
    ITEM_MINIWHEELS             = 30,
    ITEM_MONOWHEEL              = 31,
    ITEM_QUADRUPED              = 32,
    ITEM_NINJAHELMET            = 33,
    ITEM_BLACK_ROBOT_FRONT      = 34,
    ITEM_RIDING_CIRCUIT         = 35,
    ITEM_FACTION_WAND           = 36,
    ITEM_WIZARDHAT              = 37,
    ITEM_ROBOT_BACK             = 38,
    ITEM_ROBOT_HEAD_INSIDE      = 39,
    ITEM_BOLT_SET_WOOD          = 40,
    ITEM_BOLT_SET_STEEL         = 41,
    ITEM_BOLT_SET_SAPPHIRE      = 42,
    ITEM_BOLT_SET_DIAMOND       = 43,
    ITEM_CONICALHAT             = 44,
    ITEM_OSTRICH_HEAD           = 45,
    ITEM_REGENERATION_CIRCUIT   = 46,
    ITEM_ZOMBIE_CIRCUIT         = 47,
    ITEM_POLICEHAT              = 48,
    ITEM_BLACK_ROBOT_BACK       = 49,
    ITEM_TOPHAT                 = 50,
    ITEM_COMPRESSOR             = 51,
    ITEM_KINGSCROWN             = 52,
    ITEM_DUMMY_HEAD             = 53,
    ITEM_JESTERHAT              = 54,
    ITEM_TRAINING_SWORD         = 55,
    ITEM_WITCH_HAT              = 56,
    ITEM_HAMMER                 = 57,
    ITEM_SIMPLE_AXE             = 58,
    ITEM_CHAINSAW               = 59,
    ITEM_SPIKED_CLUB            = 60,
    ITEM_STEEL_SWORD            = 61,
    ITEM_BASEBALLBAT            = 62,
    ITEM_SPEAR                  = 63,
    ITEM_WAR_AXE                = 64,
    ITEM_PIXEL_SWORD            = 65,
    ITEM_HARD_HAT               = 66,
    ITEM_SERPENT_SWORD          = 67,
    ITEM_PIONEER_FRONT          = 68,
    ITEM_PIONEER_BACK           = 69,
    ITEM_VIKING_HELMET          = 70,
    ITEM_PICKAXE                = 71,
    
    //ITEM_TIMECTRL,
    //ITEM_PAINTER,
    //ITEM_KEVLAR,
    //ITEM_DEBUGGER,

    NUM_ITEMS,
};

#define ITEM_INVALID 0xffffffff

extern struct item_option item_options[NUM_ITEMS];

enum {
    ITEM_CATEGORY_GENERIC       = 0,
    ITEM_CATEGORY_WEAPON        = 1,
    ITEM_CATEGORY_HEAD          = 2,
    ITEM_CATEGORY_BACK          = 3,
    ITEM_CATEGORY_FEET          = 4,
    ITEM_CATEGORY_POWERUP       = 5,
    ITEM_CATEGORY_TOOL          = 6,
    ITEM_CATEGORY_CIRCUIT       = 7,
    ITEM_CATEGORY_LOOSE_HEAD    = 8,
    ITEM_CATEGORY_FRONT         = 9,
    ITEM_CATEGORY_BULLET        = 10,
    ITEM_CATEGORY_BOLT_SET      = 11,

    NUM_ITEM_CATEGORIES,
};

class item : public entity, public activator
{
  private:
    float health;
    uint64_t last_damage_tick;

    /* Extra data for bullets. This is used to make sure a bullet does not hit things twice. */
    bool has_hit_enemy;

  public:
    item(int32_t initial_item_id=-1);
    ~item();

    uint32_t         item_category;
    uint32_t         data_id;
    creature_effect *ef;
    float            z; /* some items such as bullet have dynamic sublayer z */

    bool             do_recreate_shape;
    bool             wep;

    void            *data; /* extra data per item type, for a rocket this is the flame effect, etc
                              mega buster solar bullet uses it for the charge
                           */

    void damage(float dmg);
    inline float get_health()
    {
        return this->health;
    }

    bool is_zappable()
    {
        return item_options[this->properties[0].v.i].zappable;
    }

    const char* get_name() { return "Item"; }
    const char *get_real_name()
    {
        return item_options[this->properties[0].v.i].name;
    }
    void write_quickinfo(char *out);

    float get_damage();
    damage_type get_damage_type();
    void step();
    void mstep();
    void init();
    void update_effects();
    void tick();
    void setup();
    void on_load(bool created, bool has_state);
    void on_pause();
    void recreate_shape();
    void add_to_world();
    void remove_from_world();

    void read_state(lvlinfo *lvl, lvlbuf *lb);
    void write_state(lvlinfo *lvl, lvlbuf *lb);

    void write_tooltip(char *out)
    {
        strcpy(out, item_options[this->get_item_type()].name);
    }

    bool on_collide(b2Fixture *f, b2Vec2 pt, float i);

    void update();

    inline uint32_t get_item_type()
    {
        uint32_t t = this->properties[0].v.i;
        return t >= NUM_ITEMS ? NUM_ITEMS-1 : t;
    }

    uint32_t get_sub_id()
    {
        return this->get_item_type();
    }

    void set_item_type(uint32_t item_type);

    static void _init();
    static void unlock(uint32_t item_type, bool signal=true);
    static const char *get_ui_name(uint32_t item_type);

    /* for heads, bodies, etc, that can rotate */
    float get_slider_snap(int s){return .1f;};
    float get_slider_value(int s){return this->properties[1].v.f;};
    const char *get_slider_label(int s){return "Rotation";};
    void on_slider_change(int s, float value){this->properties[1].v.f = value;};

    void drop_worth();

    /* activator stuff */
    b2Fixture *fx_sensor;
    activator *get_activator() { return this; }
    entity *get_activator_entity() { return this; }
    float get_activator_radius()
    {
        return item_options[this->get_item_type()].activator_radius;
    }
    b2Vec2 get_activator_pos() { return this->get_position(); }
    void activate(creature *by);
    void on_touch(b2Fixture *my, b2Fixture *other);
    void on_untouch(b2Fixture *my, b2Fixture *other);
};
