#pragma once

#include <tms/bindings/cpp/cpp.hh>

#define M_DENSITY 1.f

#ifndef GLSL
#define GLSL(...) #__VA_ARGS__
#endif

//#define AMBIENT ".08"
//#define AMBIENT ".08"
#define AMBIENT ".55"
#define AMBIENT_M ".10"

#define TYPE_WOOD        1
#define TYPE_METAL       2
#define TYPE_SHEET_METAL 4
#define TYPE_PLASTIC     8
#define TYPE_RUBBER      16
#define TYPE_WOOD2       32
#define TYPE_METAL2      64
#define TYPE_STONE       128

#define C_WOOD                  (TYPE_WOOD)
#define C_WOOD_METAL            (TYPE_WOOD | TYPE_METAL)
#define C_WOOD_SHEET_METAL      (TYPE_WOOD | TYPE_SHEET_METAL)
#define C_WOOD_PLASTIC          (TYPE_WOOD | TYPE_PLASTIC)
#define C_WOOD_RUBBER           (TYPE_WOOD | TYPE_RUBBER)
#define C_WOOD_METAL2           (TYPE_WOOD | TYPE_METAL2)
#define C_WOOD_WOOD2            (TYPE_WOOD | TYPE_WOOD2)
#define C_WOOD_STONE            (TYPE_WOOD | TYPE_STONE)

#define C_METAL                 (TYPE_METAL)
#define C_METAL_SHEET_METAL     (TYPE_METAL | TYPE_SHEET_METAL)
#define C_METAL_PLASTIC         (TYPE_METAL | TYPE_PLASTIC)
#define C_METAL_RUBBER          (TYPE_METAL | TYPE_RUBBER)
#define C_METAL_METAL2          (TYPE_METAL | TYPE_METAL2)
#define C_METAL_WOOD2           (TYPE_METAL | TYPE_WOOD2)
#define C_METAL_STONE           (TYPE_METAL | TYPE_STONE)

#define C_SHEET_METAL           (TYPE_SHEET_METAL)
#define C_SHEET_METAL_PLASTIC   (TYPE_SHEET_METAL | TYPE_PLASTIC)
#define C_SHEET_METAL_RUBBER    (TYPE_SHEET_METAL | TYPE_RUBBER)
#define C_SHEET_METAL_METAL2    (TYPE_SHEET_METAL | TYPE_METAL2)
#define C_SHEET_METAL_WOOD2     (TYPE_SHEET_METAL | TYPE_WOOD2)
#define C_SHEET_METAL_STONE     (TYPE_SHEET_METAL | TYPE_STONE)

#define C_PLASTIC               (TYPE_PLASTIC)
#define C_PLASTIC_RUBBER        (TYPE_PLASTIC | TYPE_RUBBER)
#define C_PLASTIC_METAL2        (TYPE_PLASTIC | TYPE_METAL2)
#define C_PLASTIC_WOOD2         (TYPE_PLASTIC | TYPE_WOOD2)
#define C_PLASTIC_STONE         (TYPE_PLASTIC | TYPE_STONE)

#define C_RUBBER                (TYPE_RUBBER)
#define C_RUBBER_METAL2         (TYPE_RUBBER | TYPE_METAL2)
#define C_RUBBER_WOOD2          (TYPE_RUBBER | TYPE_WOOD2)
#define C_RUBBER_STONE          (TYPE_RUBBER | TYPE_STONE)

#define C_METAL2                (TYPE_METAL2)
#define C_METAL2_WOOD2          (TYPE_METAL2 | TYPE_WOOD2)
#define C_METAL2_STONE          (TYPE_METAL2 | TYPE_STONE)

#define C_WOOD2                 (TYPE_WOOD2)
#define C_WOOD2_STONE           (TYPE_WOOD2 | TYPE_STONE)

#define C_STONE                 (TYPE_STONE)
#define C_STONE_STONE           (TYPE_STONE | TYPE_STONE_STONE)

#define SL_SHARED       (1UL << 0)

struct shader_load_data {
    uint32_t flags;
    const char *name;
    tms::shader **shader;
    tms::shader **fallback;
};

class material_factory {
  public:
    material_factory()
    {
        material_factory::background_id = 0;
    };

    static void upload_all();
    static void free_shaders();
    static void init();
    static void init_shaders();
    static void init_materials();
    static void load_bg_texture(bool soft=false);

    static int background_id;
};

class m : public tms::material
{
  public:
    float friction;
    float restitution;
    float density;

    uint8_t type;

    /* TODO: add bitfield here */

    m()
    {
        friction = .5f;
        restitution = .5f;
        density = 1.f * M_DENSITY;
        type = TYPE_WOOD;
    };
};

extern m m_colored;
extern m m_gem;
extern m m_pv_colored;
extern m m_pv_rgba;
extern m m_wood;
extern m m_tpixel;
extern m m_grass;
extern m m_cavemask;
extern m m_weight;
extern m m_metal;
extern m m_angulardamper;
extern m m_rocket;
extern m m_plastic;
extern m m_pellet;
extern m m_bullet;
extern m m_rope;
extern m m_gen;
extern m m_battery;
extern m m_wmotor;
extern m m_gear;
extern m m_gear_ao;
extern m m_sticky;
extern m m_breadboard;
extern m m_ladder;
extern m m_room;
extern m m_wheel;
extern m m_cup;
extern m m_rackhouse;
extern m m_rack;
extern m m_i2o1;
extern m m_i1o1;
extern m m_magnet;
extern m m_factory;
extern m m_iomisc;
extern m m_iron;
extern m m_rail;
extern m m_ledbuf;
extern m m_digbuf;
extern m m_charbuf;
extern m m_charbuf2;
extern m m_fluidbuf;
extern m m_spritebuf;
extern m m_spritebuf2;
extern m m_linebuf;
extern m m_linebuf2;
extern m m_bigpanel;
extern m m_misc;
extern m m_mpanel;
extern m m_smallpanel;
extern m m_motor;
extern m m_leaves;
extern m m_robot;
extern m m_animal;
extern m m_robot_skeleton;
extern m m_robot_tinted;
extern m m_robot_head;
extern m m_robot_arm;
extern m m_robot_leg;
extern m m_robot_foot;
extern m m_weapon;
extern m m_weapon_nospecular;
extern m m_cable;
extern m m_pixel;
extern m m_bg;
extern m m_bg2;
extern m m_bg_fixed;
extern m m_grid;
extern m m_bg_colored;
extern m m_border;
extern m m_rubber;
extern m m_bedrock;
extern m m_bark;
extern m m_field;
extern m m_conveyor;
extern m m_cpad;
extern m m_rubberband;
extern m m_edev;
extern m m_edev_dark;
extern m m_spikes;
extern m m_interactive;
extern m m_heavyedev;
extern m m_conn;
extern m m_conn_no_ao;
extern m m_cable_red;
extern m m_cable_black;
extern m m_cable_blue;
extern m m_red;
extern m m_item;
extern m m_item_shiny;
extern m m_chest;
extern m m_chest_shiny;
extern m m_repairstation;
extern m m_robot2;
extern m m_stone;
extern m m_decoration;
extern m m_robot_tinted_light;
extern m m_robot_armor;

extern tms::texture *tex_bg;
extern tms::texture *tex_bedrock;

/*
extern tms::shader *shader_colored;
extern tms::shader *shader_pv_colored;
extern tms::shader *shader_pv_textured;
extern tms::shader *shader_textured;
extern tms::shader *shader_gi;
extern tms::shader *shader_ao;
extern tms::shader *shader_pv_textured_ao;
*/

extern const char *available_bgs[];
extern const int num_bgs;

extern const int colored_bgs[];
