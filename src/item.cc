#include "item.hh"
#include "crane.hh"
#include "fxemitter.hh"
#include "game.hh"
#include "gui.hh"
#include "model.hh"
#include "robot_parts.hh"
#include "spritebuffer.hh"
#include "ui.hh"

#define ROCKET_FORCE_MULT 40.f
#define ROCKET_FALLOFF 0.9f

struct item_option item_options[NUM_ITEMS] = {
    item_option("Arm Cannon")
        .set_layer_mask(2+4)
        .set_material(&m_robot_arm)
        .set_mesh(&mesh_factory::models[MODEL_ROBOT_GUNARM].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(ARM_CANNON_COLOR)
        .set_size(.30f, .125f)
        .set_data_id(WEAPON_ARM_CANNON)
        .set_menu_scale(2.f)
        .set_mesh_offset(-.0125f, 0.f)
        .set_magnetic(true)
        .add_worth(RESOURCE_IRON, 18)
        .add_worth(RESOURCE_RUBY, 4)
        ,

    item_option("Builder")
        .set_layer_mask(2+4)
        .set_material(&m_weapon_nospecular)
        .set_mesh(&mesh_factory::models[MODEL_BUILDER].mesh)
        .set_category(ITEM_CATEGORY_TOOL)
        .set_uniform(.9f, .9f, .9f)
        .set_size(.30f, .125f)
        .set_data_id(TOOL_BUILDER)
        .set_menu_scale(2.f)
        .set_mesh_offset(-.0125f, 0.f)
        .set_magnetic(true)
        .add_worth(RESOURCE_IRON, 12)
        .add_worth(RESOURCE_TOPAZ, 2)
        ,

    item_option("Shotgun")
        .set_layer_mask(2+4)
        .set_material(&m_weapon)
        .set_mesh(&mesh_factory::models[MODEL_ARM_SHOTGUN].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.30f, .16f)
        .set_data_id(WEAPON_SHOTGUN)
        .set_menu_scale(2.0f)
        .set_mesh_offset(-.0125f, 0.f)
        .set_magnetic(true)
        .add_worth(RESOURCE_IRON, 22)
        .add_worth(RESOURCE_RUBY, 8)
        .add_worth(RESOURCE_EMERALD, 2)
        ,

    item_option("Railgun")
        .set_layer_mask(2+4)
        .set_material(&m_colored)
        .set_mesh(&mesh_factory::models[MODEL_ARM_RAILGUN].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.65f, .125f)
        .set_data_id(WEAPON_RAILGUN)
        .set_mesh_offset(-.025f, 0.f)
        .set_magnetic(true)
        .add_worth(RESOURCE_IRON, 7)
        .add_worth(RESOURCE_ALUMINIUM, 7)
        .add_worth(RESOURCE_EMERALD, 10)
        ,

    item_option("Oil Barrel")
        .set_effect(new creature_effect(EFFECT_TYPE_HEALTH, EFFECT_METHOD_ADDITIVE, 20.f, 0))
        .set_layer_mask(15)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_OILBARREL].mesh)
        .set_category(ITEM_CATEGORY_POWERUP)
        .set_uniform(.25f, .25f, .25f, 1.f)
        .set_size(.745f/2.f, 1.167f/2.f)
        .set_activator_radius(1.f)
        ,

    item_option("Speed oil")
        .set_effect(new creature_effect(EFFECT_TYPE_SPEED, EFFECT_METHOD_MULTIPLICATIVE, 1.5f, 20*1000*1000))
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_CANISTER_SPEED].mesh)
        .set_category(ITEM_CATEGORY_POWERUP)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.28f, .33f)
        .set_menu_scale(1.25f)
        .set_activator_radius(1.f)
        .add_worth(RESOURCE_EMERALD, 2)
        .add_oil(10.f)
        ,

    item_option("Jump oil")
        .set_effect(new creature_effect(EFFECT_TYPE_JUMP_STRENGTH, EFFECT_METHOD_MULTIPLICATIVE, 1.3f, 10*1000*1000))
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_CANISTER_JUMP].mesh)
        .set_category(ITEM_CATEGORY_POWERUP)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.28f, .33f)
        .set_menu_scale(1.25f)
        .set_activator_radius(1.f)
        .add_worth(RESOURCE_EMERALD, 1)
        .add_worth(RESOURCE_TOPAZ, 1)
        .add_oil(10.f)
        ,

    item_option("Armour oil")
        .set_effect(new creature_effect(EFFECT_TYPE_ARMOUR, EFFECT_METHOD_ADDITIVE, 20.f, 0))
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_CANISTER_ARMOUR].mesh)
        .set_category(ITEM_CATEGORY_POWERUP)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.28f, .33f)
        .set_menu_scale(1.25f)
        .set_activator_radius(1.f)
        .add_worth(RESOURCE_RUBY, 2)
        .add_oil(10.f)
        ,

    item_option("Zapper")
        .set_layer_mask(2+4)
        .set_material(&m_weapon_nospecular)
        .set_mesh(&mesh_factory::models[MODEL_MINER].mesh)
        .set_category(ITEM_CATEGORY_TOOL)
        .set_uniform(0,0,0, 1.f)
        .set_size(.30f, .125f)
        .set_data_id(TOOL_ZAPPER)
        .set_menu_scale(2.0f)
        .set_mesh_offset(-.0125f, 0.f)
        .set_magnetic(true)
        .add_worth(RESOURCE_IRON, 7)
        .add_worth(RESOURCE_ALUMINIUM, 7)
        .add_worth(RESOURCE_EMERALD, 10)
        ,

    item_option("Miner upgrade")
        .set_layer_mask(2+4)
        .set_material(&m_colored)
        .set_mesh(&mesh_factory::models[MODEL_CANISTER].mesh)
        .set_category(ITEM_CATEGORY_POWERUP)
        .set_uniform(.4f, .4f, .8f, 1.f)
        .set_size(.28f, .33f)
        .set_menu_scale(1.25f)
        .add_oil(10.f)
        ,

    item_option("Rocket Launcher")
        .set_layer_mask(2+4)
        .set_material(&m_weapon)
        .set_mesh(&mesh_factory::models[MODEL_ROCKET_LAUNCHER].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.525f, .175f)
        .set_data_id(WEAPON_ROCKET)
        .set_menu_scale(1.25f)
        .set_mesh_offset(-.20f, 0.f)
        .set_magnetic(true)
        .add_worth(RESOURCE_IRON, 25)
        .add_worth(RESOURCE_COPPER, 10)
        .add_worth(RESOURCE_RUBY, 10)
        ,

    item_option("Somersault Circuit")
        .set_layer_mask(2)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_CIRCUIT].mesh)
        .set_category(ITEM_CATEGORY_CIRCUIT)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .3f)
        .set_data_id(CREATURE_CIRCUIT_SOMERSAULT)
        .set_menu_scale(2.25f)
        .set_magnetic(true)
        .add_worth(RESOURCE_COPPER, 15)
        .add_worth(RESOURCE_RUBY, 5)
        .add_worth(RESOURCE_EMERALD, 5)
        .add_worth(RESOURCE_DIAMOND, 3)
        ,

    item_option("Jetpack")
        .set_layer_mask(2+4)
        .set_material(&m_edev_dark)
        .set_mesh(&mesh_factory::models[MODEL_JETPACK].mesh)
        .set_category(ITEM_CATEGORY_BACK)
        .set_uniform(.9f, .2f, .2f, 1.f)
        .set_size(.28f, .36f)
        .set_data_id(BACK_EQUIPMENT_JETPACK)
        .set_can_rotate(true)
        .set_magnetic(true)
        .set_rot_offs(-.175f)
        .set_menu_scale(1.125f)
        .add_worth(RESOURCE_IRON, 15)
        .add_worth(RESOURCE_COPPER, 15)
        .add_worth(RESOURCE_EMERALD, 5)
        ,

    item_option("Upgraded Jetpack")
        .set_layer_mask(2+4)
        .set_material(&m_edev)
        .set_mesh(&mesh_factory::models[MODEL_JETPACK].mesh)
        .set_category(ITEM_CATEGORY_BACK)
        .set_uniform(.2f, .9f, .2f, 1.f)
        .set_size(.28f, .36f)
        .set_data_id(BACK_EQUIPMENT_UPGRADED_JETPACK)
        .set_can_rotate(true)
        .set_magnetic(true)
        .set_rot_offs(-.175f)
        .set_menu_scale(1.125f)
        .add_worth(RESOURCE_IRON, 5)
        .add_worth(RESOURCE_ALUMINIUM, 15)
        .add_worth(RESOURCE_COPPER, 15)
        .add_worth(RESOURCE_EMERALD, 15)
        .add_worth(RESOURCE_TOPAZ, 5)
        ,

    item_option("Advanced Jetpack")
        .set_layer_mask(2+4)
        .set_material(&m_edev_dark)
        .set_mesh(&mesh_factory::models[MODEL_ADVANCED_JETPACK].mesh)
        .set_category(ITEM_CATEGORY_BACK)
        .set_uniform(.2f, .2f, .9f, 1.f)
        .set_size(.28f, .44f)
        .set_data_id(BACK_EQUIPMENT_ADVANCED_JETPACK)
        .set_can_rotate(true)
        .set_magnetic(true)
        .set_rot_offs(-.25f)
        .set_menu_scale(1.f)
        .add_worth(RESOURCE_ALUMINIUM, 25)
        .add_worth(RESOURCE_COPPER, 15)
        .add_worth(RESOURCE_EMERALD, 20)
        .add_worth(RESOURCE_TOPAZ, 15)
        .add_worth(RESOURCE_DIAMOND, 15)
        ,

    item_option("Bomb Launcher")
        .set_layer_mask(2+4)
        .set_material(&m_weapon)
        .set_mesh(&mesh_factory::models[MODEL_ARM_BOMBER].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.50f, .25f)
        .set_data_id(WEAPON_BOMBER)
        .set_menu_scale(1.5f)
        .set_mesh_offset(-.18f, 0.f)
        .set_magnetic(true)
        .add_worth(RESOURCE_IRON, 18)
        .add_worth(RESOURCE_RUBY, 4)
        ,

    item_option("Robot head")
        .set_layer_mask(15)
        .set_material(&m_robot)
        .set_mesh(&mesh_factory::models[MODEL_ROBOT_HEAD].mesh)
        .set_category(ITEM_CATEGORY_LOOSE_HEAD)
        .set_uniform(1.f, 1.f, 1.f, 1.f)
        .set_size(.30f, .225f)
        .set_data_id(HEAD_ROBOT)
        .set_can_rotate(true)
        .set_magnetic(true)
        .set_menu_scale(1.35f)
        .set_menu_scale(1.35f)
        .add_worth(RESOURCE_ALUMINIUM, 7)
        .add_worth(RESOURCE_IRON, 14)
        .add_worth(RESOURCE_DIAMOND, 1)
        ,

    item_option("Cow head")
        .set_layer_mask(15)
        .set_material(&m_animal)
        .set_mesh(&mesh_factory::models[MODEL_COW_HEAD].mesh)
        .set_category(ITEM_CATEGORY_LOOSE_HEAD)
        .set_uniform(1.f, 1.f, 1.f, 1.f)
        .set_size(.26f, .3f)
        .set_data_id(HEAD_COW)
        .set_can_rotate(true)
        .set_magnetic(true)
        .set_menu_scale(1.25f)
        .add_worth(RESOURCE_ALUMINIUM, 2)
        .add_worth(RESOURCE_IRON, 19)
        .add_worth(RESOURCE_TOPAZ, 1)
        ,

    item_option("Pig head")
        .set_layer_mask(15)
        .set_material(&m_animal)
        .set_mesh(&mesh_factory::models[MODEL_PIG_HEAD].mesh)
        .set_category(ITEM_CATEGORY_LOOSE_HEAD)
        .set_uniform(1.f, 1.f, 1.f, 1.f)
        .set_size(.30f, .32f)
        .set_data_id(HEAD_PIG)
        .set_can_rotate(true)
        .set_menu_scale(1.25f)
        .set_magnetic(true)
        .add_worth(RESOURCE_ALUMINIUM, 2)
        .add_worth(RESOURCE_IRON, 24)
        .add_worth(RESOURCE_TOPAZ, 1)
        ,

    item_option("Robot front")
        .set_layer_mask(15)
        .set_material(&m_robot)
        .set_mesh(&mesh_factory::models[MODEL_ROBOT_FRONT].mesh)
        .set_category(ITEM_CATEGORY_FRONT)
        .set_uniform(ROBOT_COLOR, 1.f)
        .set_data_id(FRONT_EQUIPMENT_ROBOT_FRONT)
        .set_size(.375f/2.f, .375f)
        .set_can_rotate(true)
        //.set_mesh_offset(-.375f/4.f,0)
        .set_mesh_offset(0,0)
        .set_magnetic(true)
        .set_rot_offs(.175f)
        .set_menu_scale(1.5f)
        .add_worth(RESOURCE_ALUMINIUM, 10)
        .add_worth(RESOURCE_IRON, 20)
        ,

    item_option("Teslagun")
        .set_layer_mask(2+4)
        .set_material(&m_weapon)
        .set_mesh(&mesh_factory::models[MODEL_TESLA_GUN].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.44f, .25f)
        .set_data_id(WEAPON_TESLA)
        .set_menu_scale(2.0f)
        .set_mesh_offset(-.05f, 0.f)
        .set_magnetic(true)
        .add_worth(RESOURCE_IRON, 2)
        .add_worth(RESOURCE_COPPER, 10)
        .add_worth(RESOURCE_ALUMINIUM, 15)
        .add_worth(RESOURCE_EMERALD, 10)
        .add_worth(RESOURCE_DIAMOND, 1)
        ,

    item_option("Plasma Gun")
        .set_layer_mask(2+4)
        .set_material(&m_weapon)
        .set_mesh(&mesh_factory::models[MODEL_PLASMA_GUN].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.55f, .15f)
        .set_data_id(WEAPON_PLASMAGUN)
        .set_menu_scale(1.5f)
        .set_mesh_offset(-.125f, -0.025f)
        .set_magnetic(true)
        .add_worth(RESOURCE_IRON, 4)
        .add_worth(RESOURCE_COPPER, 10)
        .add_worth(RESOURCE_ALUMINIUM, 5)
        .add_worth(RESOURCE_EMERALD, 15)
        .add_worth(RESOURCE_DIAMOND, 5)
        ,

    item_option("Arm Cannon bullet")
        .set_layer_mask(2+4)
        .set_material(&m_bullet)
        .set_mesh(&mesh_factory::models[MODEL_BULLET].mesh)
        .set_category(ITEM_CATEGORY_BULLET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.125f, 0.f)
        .set_menu_scale(2.5f)
        .set_magnetic(true)
        .set_do_step(true)
        .set_zappable(false)
        ,

    item_option("Shotgun pellet")
        .set_layer_mask(2+4)
        .set_material(&m_bullet)
        .set_mesh(&mesh_factory::models[MODEL_DEBRIS].mesh)
        .set_category(ITEM_CATEGORY_BULLET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.05f, .0f)
        .set_menu_scale(2.5f)
        .set_magnetic(true)
        .set_do_step(true)
        .set_zappable(false)
        ,

    item_option("Plasma Gun plasma")
        .set_layer_mask(2+4)
        .set_material(&m_field)
        .set_category(ITEM_CATEGORY_BULLET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.15f, 0.f)
        .set_menu_scale(2.5f)
        .set_do_step(true)
        .set_do_update_effects(true)
        .set_zappable(false)
        ,

    item_option("Rocket Launcher rocket")
        .set_layer_mask(2+4)
        .set_material(&m_pellet)
        .set_mesh(&mesh_factory::models[MODEL_MISSILE].mesh)
        .set_category(ITEM_CATEGORY_BULLET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.27f, .1f)
        .set_menu_scale(2.5f)
        .set_magnetic(true)
        .set_do_step(true)
        .set_zappable(false)
        ,

    item_option("Heisenberg's Hat")
        .set_layer_mask(15)
        .set_material(&m_edev_dark)
        .set_mesh(&mesh_factory::models[MODEL_HAT].mesh)
        .set_category(ITEM_CATEGORY_HEAD)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.27f, .1f)
        .set_data_id(HEAD_EQUIPMENT_HEISENBERG)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        ,

    item_option("Mega Buster")
        .set_layer_mask(2+4)
        .set_material(&m_pv_colored)
        .set_mesh(&mesh_factory::models[MODEL_MEGA_BUSTER].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(MEGA_BUSTER_COLOR, 1.f)
        .set_size(.3f, .15f)
        .set_data_id(WEAPON_MEGABUSTER)
        .set_menu_scale(1.5f)
        .set_mesh_offset(.0f, -0.025f)
        .set_magnetic(true)
        ,

    item_option("Mega Buster Solar Bullet")
        .set_layer_mask(2+4)
        .set_material(&m_field)
        .set_category(ITEM_CATEGORY_BULLET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.15f, 0.f)
        .set_menu_scale(2.5f)
        .set_do_step(true)
        .set_do_update_effects(true)
        .set_zappable(false)
        ,

    item_option("Feet")
        .set_layer_mask(15)
        .set_material(&m_colored)
        .set_mesh(&mesh_factory::models[MODEL_BOX0].mesh)
        .set_category(ITEM_CATEGORY_FEET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.25f, .25f)
        .set_data_id(FEET_BIPED)
        .set_menu_scale(2.5f)
        .set_mesh_offset(-.125f, -0.025f)
        .set_magnetic(true)
        ,

    item_option("Miniwheels")
        .set_layer_mask(15)
        .set_material(&m_colored)
        .set_mesh(&mesh_factory::models[MODEL_BOX0].mesh)
        .set_category(ITEM_CATEGORY_FEET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.25f, .25f)
        .set_data_id(FEET_MINIWHEELS)
        .set_menu_scale(2.5f)
        .set_mesh_offset(-.125f, -0.025f)
        .set_magnetic(true)
        ,

    item_option("Monowheel")
        .set_layer_mask(15)
        .set_material(&m_colored)
        .set_mesh(&mesh_factory::models[MODEL_BOX0].mesh)
        .set_category(ITEM_CATEGORY_FEET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.25f, .25f)
        .set_data_id(FEET_MONOWHEEL)
        .set_menu_scale(2.5f)
        .set_mesh_offset(-.125f, -0.025f)
        .set_magnetic(true)
        ,

    item_option("Quadruped")
        .set_layer_mask(15)
        .set_material(&m_colored)
        .set_mesh(&mesh_factory::models[MODEL_BOX0].mesh)
        .set_category(ITEM_CATEGORY_FEET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.25f, .25f)
        .set_data_id(FEET_QUADRUPED)
        .set_menu_scale(2.5f)
        .set_mesh_offset(-.125f, -0.025f)
        .set_magnetic(true)
        ,

    item_option("Ninja Helmet")
        .set_layer_mask(15)
        .set_material(&m_edev_dark)
        .set_mesh(&mesh_factory::models[MODEL_NINJAHELMET].mesh)
        .set_category(ITEM_CATEGORY_HEAD)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .27f)
        .set_data_id(HEAD_EQUIPMENT_NINJAHELMET)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .set_magnetic(true)
        ,

    item_option("Black robot front")
        .set_layer_mask(15)
        .set_material(&m_robot_tinted_light)
        .set_mesh(&mesh_factory::models[MODEL_ROBOT_FRONT].mesh)
        .set_category(ITEM_CATEGORY_FRONT)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .36f)
        .set_data_id(FRONT_EQUIPMENT_BLACK_ROBOT_FRONT)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .set_magnetic(true)
        ,

    item_option("Riding Circuit")
        .set_layer_mask(2)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_CIRCUIT].mesh)
        .set_category(ITEM_CATEGORY_CIRCUIT)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .3f)
        .set_data_id(CREATURE_CIRCUIT_RIDING)
        .set_menu_scale(2.25f)
        .set_can_rotate(true)
        .set_magnetic(true)
        .add_worth(RESOURCE_COPPER, 15)
        .add_worth(RESOURCE_RUBY, 5)
        .add_worth(RESOURCE_EMERALD, 5)
        .add_worth(RESOURCE_DIAMOND, 3)
        ,

    item_option("Faction Wand")
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_FACTIONWAND].mesh)
        .set_category(ITEM_CATEGORY_TOOL)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.27f, .1f)
        .set_data_id(TOOL_FACTION_WAND)
        .set_menu_scale(1.5f)
        .set_can_rotate(true)
        .set_magnetic(true)
        ,

    item_option("Wizard Hat")
        .set_layer_mask(15)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_WIZARDHAT].mesh)
        .set_category(ITEM_CATEGORY_HEAD)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.27f, .45f)
        .set_data_id(HEAD_EQUIPMENT_WIZARDHAT)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .set_mesh_offset(0,-.4)
        ,

    item_option("Robot back")
        .set_layer_mask(15)
        .set_material(&m_robot_tinted)
        .set_mesh(&mesh_factory::models[MODEL_ROBOT_BACK].mesh)
        .set_category(ITEM_CATEGORY_BACK)
        .set_uniform(ROBOT_COLOR, 1.f)
        .set_size(.375f/2.f, .375f)
        .set_data_id(BACK_EQUIPMENT_ROBOT_BACK)
        .set_can_rotate(true)
        //.set_mesh_offset(.375f/2.f,0)
        .set_magnetic(true)
        .set_rot_offs(-.175f)
        .set_menu_scale(1.5f)
        .add_worth(RESOURCE_ALUMINIUM, 10)
        .add_worth(RESOURCE_IRON, 20)
        ,

    item_option("Uncovered robot head")
        .set_layer_mask(15)
        .set_material(&m_robot2)
        .set_mesh(&mesh_factory::models[MODEL_ROBOT_HEAD_INSIDE].mesh)
        .set_category(ITEM_CATEGORY_LOOSE_HEAD)
        .set_uniform(1.f, 1.f, 1.f, 1.f)
        .set_size(.2f, .22f)
        .set_data_id(HEAD_ROBOT_UNCOVERED)
        .set_can_rotate(true)
        .set_mesh_offset(0,.1)
        .set_magnetic(true)
        .set_menu_scale(1.5f)
        .add_worth(RESOURCE_ALUMINIUM, 3)
        .add_worth(RESOURCE_IRON, 3)
        .add_worth(RESOURCE_DIAMOND, 1)
        ,

    item_option("Wood Bolt Set")
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_BOLTSET_WOOD].mesh)
        .set_category(ITEM_CATEGORY_BOLT_SET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .23f)
        .set_data_id(BOLT_SET_WOOD)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .add_worth(RESOURCE_WOOD, 15)
        ,

    item_option("Steel Bolt Set")
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_BOLTSET_STEEL].mesh)
        .set_category(ITEM_CATEGORY_BOLT_SET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .23f)
        .set_data_id(BOLT_SET_WOOD)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .add_worth(RESOURCE_IRON, 5)
        .add_worth(RESOURCE_RUBY, 1)
        ,

    item_option("Sapphire Bolt Set")
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_BOLTSET_TITANIUM].mesh)
        .set_category(ITEM_CATEGORY_BOLT_SET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .23f)
        .set_data_id(BOLT_SET_SAPPHIRE)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .add_worth(RESOURCE_IRON, 35)
        .add_worth(RESOURCE_SAPPHIRE, 15)
        ,

    item_option("Diamond Bolt Set")
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_BOLTSET_DIAMOND].mesh)
        .set_category(ITEM_CATEGORY_BOLT_SET)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .23f)
        .set_data_id(BOLT_SET_DIAMOND)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .add_worth(RESOURCE_ALUMINIUM, 35)
        .add_worth(RESOURCE_DIAMOND, 10)
        ,

    item_option("Conical Hat")
        .set_layer_mask(15)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_CONICALHAT].mesh)
        .set_category(ITEM_CATEGORY_HEAD)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.4f, .18f)
        .set_data_id(HEAD_EQUIPMENT_CONICALHAT)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .set_mesh_offset(0,-.14)
        .add_worth(RESOURCE_WOOD, 15)
        ,

    item_option("Ostrich Head")
        .set_layer_mask(15)
        .set_material(&m_animal)
        .set_mesh(&mesh_factory::models[MODEL_OSTRICH_HEAD].mesh)
        .set_category(ITEM_CATEGORY_LOOSE_HEAD)
        .set_uniform(1.f, 1.f, 1.f, 1.f)
        .set_size(.30f, .125f)
        .set_data_id(HEAD_OSTRICH)
        .set_can_rotate(true)
        .set_magnetic(true)
        ,

    item_option("Circuit of Regeneration")
        .set_layer_mask(2)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_CIRCUIT].mesh)
        .set_category(ITEM_CATEGORY_CIRCUIT)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .3f)
        .set_data_id(CREATURE_CIRCUIT_REGENERATION)
        .set_menu_scale(2.25f)
        .set_magnetic(true)
        .add_worth(RESOURCE_COPPER, 15)
        .add_worth(RESOURCE_RUBY, 5)
        .add_worth(RESOURCE_EMERALD, 5)
        .add_worth(RESOURCE_DIAMOND, 3)
        ,

    item_option("Zombie Circuit")
        .set_layer_mask(2)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_CIRCUIT].mesh)
        .set_category(ITEM_CATEGORY_CIRCUIT)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .3f)
        .set_data_id(CREATURE_CIRCUIT_ZOMBIE)
        .set_magnetic(true)
        .set_menu_scale(2.25f)
        .add_worth(RESOURCE_COPPER, 15)
        .add_worth(RESOURCE_RUBY, 5)
        .add_worth(RESOURCE_EMERALD, 5)
        .add_worth(RESOURCE_DIAMOND, 3)
        ,

    item_option("Police Hat")
        .set_layer_mask(15)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_POLICEHAT].mesh)
        .set_category(ITEM_CATEGORY_HEAD)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.27f, .1f)
        .set_data_id(HEAD_EQUIPMENT_POLICEHAT)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .set_mesh_offset(0,-.1)
        ,

    item_option("Black robot back")
        .set_layer_mask(15)
        .set_material(&m_robot_tinted_light)
        .set_mesh(&mesh_factory::models[MODEL_ROBOT_BACK].mesh)
        .set_category(ITEM_CATEGORY_BACK)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.375f/2.f, .375f)
        .set_data_id(BACK_EQUIPMENT_BLACK_ROBOT_BACK)
        .set_can_rotate(true)
        //.set_mesh_offset(.375f/2.f,0)
        .set_magnetic(true)
        .set_rot_offs(-.175f)
        .set_menu_scale(1.5f)
        .add_worth(RESOURCE_ALUMINIUM, 10)
        .add_worth(RESOURCE_IRON, 20)
        ,

    item_option("Top Hat")
        .set_layer_mask(15)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_TOPHAT].mesh)
        .set_category(ITEM_CATEGORY_HEAD)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .35f)
        .set_data_id(HEAD_EQUIPMENT_TOPHAT)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .set_mesh_offset(0,-.25)
        ,

    item_option("Compressor")
        .set_layer_mask(2+4)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_COMPRESSOR].mesh)
        .set_category(ITEM_CATEGORY_TOOL)
        .set_uniform(.9f, .9f, .9f)
        .set_size(.52f, .13f)
        .set_data_id(TOOL_COMPRESSOR)
        .set_menu_scale(2.f)
        .set_mesh_offset(.115f, 0.f)
        .set_magnetic(true)
        .add_worth(RESOURCE_IRON, 7)
        .add_worth(RESOURCE_ALUMINIUM, 7)
        .add_worth(RESOURCE_TOPAZ, 1)
        .add_worth(RESOURCE_EMERALD, 1)
        ,
    item_option("King's Crown")
        .set_layer_mask(15)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_KINGSCROWN].mesh)
        .set_category(ITEM_CATEGORY_HEAD)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .15f)
        .set_data_id(HEAD_EQUIPMENT_KINGSCROWN)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .set_mesh_offset(0.f,-.1f)
        ,

    item_option("Dummy head")
        .set_layer_mask(15)
        .set_material(&m_animal)
        .set_mesh(&mesh_factory::models[MODEL_DUMMY_HEAD].mesh)
        .set_category(ITEM_CATEGORY_LOOSE_HEAD)
        .set_uniform(1.f, 1.f, 1.f, 1.f)
        .set_size(.175f, .25f)
        .set_data_id(HEAD_DUMMY)
        .set_can_rotate(true)
        .set_mesh_offset(0.f,.06f)
        .set_magnetic(true)
        .set_menu_scale(1.25f)
        ,

    item_option("Jester hat")
        .set_layer_mask(15)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_JESTERHAT].mesh)
        .set_category(ITEM_CATEGORY_HEAD)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.5f, .2f)
        .set_data_id(HEAD_EQUIPMENT_JESTERHAT)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .set_mesh_offset(.1f,-.1f)
        ,

    item_option("Training sword")
        .set_layer_mask(2+4)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_WOODSWORD].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .65f)
        .set_data_id(WEAPON_TRAINING_SWORD)
        .set_menu_scale(0.75f)
        .set_mesh_offset(-.0125f, -.5f)
        ,

    item_option("Witch Hat")
        .set_layer_mask(15)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_WITCH_HAT].mesh)
        .set_category(ITEM_CATEGORY_HEAD)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.27f, .45f)
        .set_data_id(HEAD_EQUIPMENT_WITCH_HAT)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .set_mesh_offset(0,-.4)
        ,

    item_option("War hammer")
        .set_layer_mask(2+4)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_HAMMER].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .6f)
        .set_data_id(WEAPON_WAR_HAMMER)
        .set_menu_scale(0.75f)
        .set_mesh_offset(-.0125f, -.5f)
        ,

    item_option("Simple axe")
        .set_layer_mask(2+4)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_SIMPLE_AXE].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .45f)
        .set_data_id(WEAPON_SIMPLE_AXE)
        .set_menu_scale(0.75f)
        .set_mesh_offset(-.0125f, -.3f)
        ,

    item_option("Chainsaw")
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_SAW1].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.53f, .25f)
        .set_data_id(WEAPON_CHAINSAW)
        .set_menu_scale(0.75f)
        .set_mesh_offset(.2f, .1f)
        ,

    item_option("Spiked Club")
        .set_layer_mask(2+4)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_SPIKED_CLUB].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .65f)
        .set_data_id(WEAPON_SPIKED_CLUB)
        .set_menu_scale(0.75f)
        .set_mesh_offset(-.0125f, -.5f)
        ,

    item_option("Steel Sword")
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_STEEL_SWORD].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .7f)
        .set_data_id(WEAPON_STEEL_SWORD)
        .set_menu_scale(0.75f)
        .set_mesh_offset(-.0125f, -.5f)
        ,

    item_option("Baseball bat")
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_BASEBALLBAT].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .6f)
        .set_data_id(WEAPON_BASEBALLBAT)
        .set_menu_scale(0.75f)
        .set_mesh_offset(-.0125f, -.45f)
        ,

    item_option("Spear")
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_SPEAR].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .6f)
        .set_data_id(WEAPON_SPEAR)
        .set_menu_scale(0.75f)
        .set_mesh_offset(0.f, -.45f)
        ,

    item_option("War axe")
        .set_layer_mask(2+4)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_WAR_AXE].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.35f, .7f)
        .set_data_id(WEAPON_WAR_AXE)
        .set_menu_scale(0.75f)
        .set_mesh_offset(.0125f, -.5f)
        ,

    item_option("Pixel sword")
        .set_layer_mask(2+4)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_PIXEL_SWORD].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .65f)
        .set_data_id(WEAPON_PIXEL_SWORD)
        .set_menu_scale(0.75f)
        .set_mesh_offset(-.0125f, -.5f)
        ,

    item_option("Hard hat")
        .set_layer_mask(15)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_HARD_HAT].mesh)
        .set_category(ITEM_CATEGORY_HEAD)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.25f, .2f)
        .set_data_id(HEAD_EQUIPMENT_HARD_HAT)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .set_mesh_offset(0.f,-.1f)
        ,

    item_option("Serpent Sword")
        .set_layer_mask(2+4)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_SERPENT_SWORD].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .7f)
        .set_data_id(WEAPON_SERPENT_SWORD)
        .set_menu_scale(0.75f)
        .set_mesh_offset(-.0125f, -.5f)
        ,

    item_option("Pioneer front")
        .set_layer_mask(15)
        .set_material(&m_robot_armor)
        .set_mesh(&mesh_factory::models[MODEL_PIONEER_FRONT].mesh)
        .set_category(ITEM_CATEGORY_FRONT)
        .set_uniform(ROBOT_COLOR, 1.f)
        .set_size(.3f, .36f)
        .set_data_id(FRONT_EQUIPMENT_PIONEER_FRONT)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .set_magnetic(true)
        ,

    item_option("Pioneer back")
        .set_layer_mask(15)
        .set_material(&m_robot_armor)
        .set_mesh(&mesh_factory::models[MODEL_PIONEER_BACK].mesh)
        .set_category(ITEM_CATEGORY_BACK)
        .set_uniform(ROBOT_COLOR, 1.f)
        .set_size(.375f/2.f, .375f)
        .set_data_id(BACK_EQUIPMENT_PIONEER_BACK)
        .set_can_rotate(true)
        //.set_mesh_offset(.375f/2.f,0)
        .set_magnetic(true)
        .set_rot_offs(-.175f)
        .set_menu_scale(1.5f)
        .add_worth(RESOURCE_ALUMINIUM, 10)
        .add_worth(RESOURCE_IRON, 20)
        ,

    item_option("Viking Helmet")
        .set_layer_mask(15)
        .set_material(&m_item_shiny)
        .set_mesh(&mesh_factory::models[MODEL_VIKING_HELMET].mesh)
        .set_category(ITEM_CATEGORY_HEAD)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .15f)
        .set_data_id(HEAD_EQUIPMENT_VIKING_HELMET)
        .set_menu_scale(2.5f)
        .set_can_rotate(true)
        .set_mesh_offset(0.f,-.1f)
        ,

    item_option("Pickaxe")
        .set_layer_mask(2+4)
        .set_material(&m_item)
        .set_mesh(&mesh_factory::models[MODEL_PICKAXE].mesh)
        .set_category(ITEM_CATEGORY_WEAPON)
        .set_uniform(.2f, .2f, .2f, 1.f)
        .set_size(.3f, .52f)
        .set_data_id(WEAPON_PICKAXE)
        .set_menu_scale(0.75f)
        .set_mesh_offset(-.0125f, -.38f)
        ,
};

int _head_to_item[NUM_HEAD_TYPES] = {
    0,
    ITEM_ROBOT_HEAD,
    ITEM_COW_HEAD,
    ITEM_PIG_HEAD,
    ITEM_ROBOT_HEAD_INSIDE,
    ITEM_OSTRICH_HEAD,
    ITEM_DUMMY_HEAD,
};

int _bolt_to_item[NUM_BOLT_SETS] = {
    ITEM_BOLT_SET_WOOD,
    ITEM_BOLT_SET_STEEL,
    ITEM_BOLT_SET_SAPPHIRE,
    ITEM_BOLT_SET_DIAMOND,
};

int _feet_to_item[NUM_FEET_TYPES] = {
    0,
    ITEM_BIPED,
    ITEM_MINIWHEELS,
    ITEM_QUADRUPED,
    ITEM_MONOWHEEL,
};

int _back_to_item[NUM_BACK_EQUIPMENT_TYPES] = {
    0,
    ITEM_ROBOT_BACK,
    ITEM_JETPACK,
    ITEM_UPGRADED_JETPACK,
    ITEM_ADVANCED_JETPACK,
    ITEM_BLACK_ROBOT_BACK,
    ITEM_PIONEER_BACK,
};

int _front_to_item[NUM_FRONT_EQUIPMENT_TYPES] = {
    0,
    ITEM_ROBOT_FRONT,
    ITEM_BLACK_ROBOT_FRONT,
    ITEM_PIONEER_FRONT,
};

int _head_equipment_to_item[NUM_HEAD_EQUIPMENT_TYPES] = {
    0,
    ITEM_NINJAHELMET,
    ITEM_HAT,
    ITEM_WIZARDHAT,
    ITEM_CONICALHAT,
    ITEM_POLICEHAT,
    ITEM_TOPHAT,
    ITEM_KINGSCROWN,
    ITEM_JESTERHAT,
    ITEM_WITCH_HAT,
    ITEM_HARD_HAT,
    ITEM_VIKING_HELMET,
};

int _tool_to_item[NUM_TOOLS] = {
    0,
    ITEM_ZAPPER,
    0,
    0,
    0,
    ITEM_BUILDER,
    ITEM_FACTION_WAND,
    ITEM_COMPRESSOR,
};

int _weapon_to_item[NUM_WEAPONS] = {
    0,
    ITEM_ARM_CANNON,
    ITEM_SHOTGUN,
    ITEM_RAILGUN,
    ITEM_ROCKET_LAUNCHER,
    ITEM_BOMBER,
    ITEM_TESLAGUN,
    ITEM_PLASMAGUN,
    ITEM_MEGABUSTER,
    ITEM_TRAINING_SWORD,
    ITEM_HAMMER,
    ITEM_SIMPLE_AXE,
    ITEM_CHAINSAW,
    ITEM_SPIKED_CLUB,
    ITEM_STEEL_SWORD,
    ITEM_BASEBALLBAT,
    ITEM_SPEAR,
    ITEM_WAR_AXE,
    ITEM_SERPENT_SWORD,
    ITEM_PICKAXE,
};

uint32_t _circuit_flag_to_item(uint32_t circuit_id)
{
    switch (circuit_id) {
        case CREATURE_CIRCUIT_SOMERSAULT: return ITEM_SOMERSAULT_CIRCUIT;
        case CREATURE_CIRCUIT_RIDING: return ITEM_RIDING_CIRCUIT;
        case CREATURE_CIRCUIT_REGENERATION: return ITEM_REGENERATION_CIRCUIT;
        case CREATURE_CIRCUIT_ZOMBIE: return ITEM_ZOMBIE_CIRCUIT;

        default: return ITEM_INVALID;
    }
}

item::item(int32_t initial_item_id/*=-1*/)
    : activator(ATTACHMENT_NONE)
    , has_hit_enemy(false)
{
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_HAS_CONFIG,           true);
    this->set_flag(ENTITY_DO_TICK,              true);
    this->set_flag(ENTITY_CAN_BE_COMPRESSED,    true);
    this->set_flag(ENTITY_DYNAMIC_UNLOADING,    true);

    this->dialog_id = DIALOG_ITEM;

    this->update_method = ENTITY_UPDATE_CUSTOM;
    this->item_category = 0;
    this->ef = 0;
    this->do_recreate_shape = false;
    this->wep = false;

    this->health = ITEM_MAX_HEALTH;
    this->last_damage_tick = 0;

    this->data = 0;

    this->set_num_properties(2);
    this->properties[0].type = P_INT;
    if (initial_item_id < 0) {
        initial_item_id = rand()%NUM_ITEMS;
    }
    this->set_item_type(initial_item_id);

    this->properties[1].type = P_FLT; /* rotation, for heads, bodies, etc */
    this->properties[1].v.f = .5f;
}

item::~item()
{

}

void
item::tick()
{
    if (this->do_recreate_shape) {
        this->recreate_shape();
        this->do_recreate_shape = false;
    }
}

void
item::init()
{
    switch (this->properties[0].v.i) {
        case ITEM_ROCKET:
            {
            }
            break;
    }
}

void
item::setup()
{
    this->set_item_type(this->properties[0].v.i);
}

void
item::on_load(bool created, bool has_state)
{
    this->set_item_type(this->properties[0].v.i);
}

void
item::on_pause()
{
    this->setup();
}

void
item::recreate_shape()
{
    uint32_t ct = this->get_item_type();
    if (ct >= NUM_ITEMS) this->set_item_type(ct = (NUM_ITEMS - 1));

    if (this->body) {
        while (this->body->GetFixtureList()) {
            this->body->DestroyFixture(this->body->GetFixtureList());
        }
    }

    const item_option &co = item_options[ct];
    if (co.mesh) {
        this->set_mesh(*co.mesh);
        this->set_uniform("~color", co.uniform.r, co.uniform.g, co.uniform.b, co.uniform.a);
    } else {
        this->set_mesh((tms::mesh*)0);
    }
    this->set_material(co.material);

    if (co.size.y <= 0.f) {
        this->create_circle(this->get_dynamic_type(), co.size.x, this->material);
    } else {
        this->create_rect(this->get_dynamic_type(), co.size.x, co.size.y, this->material);
    }

    if (ct == ITEM_PLASMA || ct == ITEM_SOLAR) {
        /* plasma bullets are not effected by gravity */
        this->body->SetGravityScale(0.f);
    }

    this->menu_scale = co.menu_scale;

    if (W->is_playing() && co.activator_radius > 0.f) {
        b2Vec2 p = this->local_to_body(b2Vec2(0, 0), 0);

        b2CircleShape c;
        c.m_radius = 1.f;
        c.m_p = p;

        b2FixtureDef fd;
        fd.isSensor = true;
        fd.restitution = 0.f;
        fd.friction = FLT_EPSILON;
        fd.density = 0.00001f;
        fd.filter = world::get_filter_for_layer(this->get_layer(), this->layer_mask);
        fd.shape = &c;

        (this->fx_sensor = this->body->CreateFixture(&fd))->SetUserData(this);
    }
}

void
item::add_to_world()
{
    this->recreate_shape();
}

void
item::update()
{
    item_option co = item_options[this->properties[0].v.i];

    switch (this->item_category) {
        case ITEM_CATEGORY_TOOL:
            {
                tmat4_load_identity(this->M);
                b2Vec2 p = this->get_position();
                float a = this->get_angle();

                tmat4_translate(this->M, p.x, p.y, this->get_layer()*LAYER_DEPTH);
                tmat4_rotate(this->M, a * (180.f/M_PI) + 180, 0.f, 0.f, -1.f);

                /* the mesh is rotated weirdly to fit the robot better, so we have to rotate it back */
                tmat4_rotate(this->M, -90, 0, 0, 1.);
                tmat4_rotate(this->M, -90, 0,1,0);
                tmat4_rotate(this->M, 180, 1,0,0);
                tmat4_translate(this->M, -.35f, co.mesh_offset.x, co.mesh_offset.y);
                //tmat4_rotate(this->M, 90, 0,1,0);

                tmat3_copy_mat4_sub3x3(this->N, this->M);
                this->wep = true;
            }
            break;

        case ITEM_CATEGORY_WEAPON:
            {
                tmat4_load_identity(this->M);
                b2Vec2 p = this->get_position();
                float a = this->get_angle();

                tmat4_translate(this->M, p.x, p.y, this->get_layer()*LAYER_DEPTH);
                tmat4_rotate(this->M, a * (180.f/M_PI) + 180, 0.f, 0.f, -1.f);

                /* the mesh is rotated weirdly to fit the robot better, so we have to rotate it back */
                tmat4_rotate(this->M, -90, 0, 0, 1.);
                tmat4_rotate(this->M, -90, 0,1,0);
                tmat4_rotate(this->M, 180, 1,0,0);
                tmat4_translate(this->M, .35f, co.mesh_offset.x, co.mesh_offset.y);
                //tmat4_rotate(this->M, 90, 0,1,0);

                tmat3_copy_mat4_sub3x3(this->N, this->M);
                this->wep = true;
            }
            break;

        case ITEM_CATEGORY_LOOSE_HEAD:
        case ITEM_CATEGORY_HEAD:
        case ITEM_CATEGORY_BACK:
        case ITEM_CATEGORY_FRONT:
            {
                tmat4_load_identity(this->M);
                b2Vec2 p = this->get_position();
                tmat4_translate(this->M, p.x, p.y, this->get_layer() * LAYER_DEPTH);
                tmat4_rotate(this->M, this->get_angle() * (180.f/M_PI), 0, 0, -1);
                tmat4_translate(this->M, co.mesh_offset.x, co.mesh_offset.y, 0.f);
                tmat4_translate(this->M, 0.f, 0.f, co.rot_offs);
                tmat4_rotate(this->M, (this->properties[1].v.f-.5f) * 180.f, 0, -1, 0);
                tmat4_translate(this->M, 0.f, 0.f, -co.rot_offs);
                tmat3_copy_mat4_sub3x3(this->N, this->M);
                tmat4_scale(this->M, this->get_scale(), this->get_scale(), this->get_scale());
                this->wep = true;
            }
            break;

        default:
            {
                if (this->wep) {
                    // reset matrices from being rendered as a weapon/tool
                    tmat4_load_identity(this->M);
                    this->wep = false;
                }

                entity_fast_update(this);
            }
            break;
    }
}

#define INTERACTIVE_DAMAGE_MODIFIER 0.05f

void
item::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::read_state(lvl, lb);

    this->set_scale(lb->r_float());

    switch (this->properties[0].v.i) {
        case ITEM_SOLAR:
            this->data = (void*)(uintptr_t)lb->r_uint32();
            break;
    }
}

void
item::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::write_state(lvl, lb);

    lb->w_s_float(this->get_scale());

    switch (this->properties[0].v.i) {
        case ITEM_SOLAR:
            lb->w_s_uint32((uint32_t)(uintptr_t)(this->data));
            break;
    }
}

float
item::get_damage()
{
    float damage = 0.f;

    switch (this->properties[0].v.i) {
        case ITEM_BULLET: damage = 5.f; break;
        case ITEM_SHOTGUN_PELLET:
            damage = 7.f;
            damage += this->get_body(0) ? this->get_body(0)->GetLinearVelocity().LengthSquared() / 30.f : 0.f;
            break;
        case ITEM_PLASMA: damage = 8.f; break;
        case ITEM_ROCKET: damage = 80.f; break;
        case ITEM_SOLAR:
            damage = 15.f;
            damage += ((int)(uintptr_t)this->data)/MEGABUSTER_CHARGE_MAX * MEGABUSTER_CHARGE_DAMAGE;

            break;
    }

    entity *attacker = W->get_entity_by_id(this->emitted_by);
    if (attacker && attacker->flag_active(ENTITY_IS_CREATURE)) {
        damage *= static_cast<creature*>(attacker)->get_attack_damage_modifier();
    }

    return damage;
}

damage_type
item::get_damage_type()
{
    switch (this->properties[0].v.i) {
        case ITEM_PLASMA:
        case ITEM_SOLAR:
            return DAMAGE_TYPE_PLASMA;
        default:
            return DAMAGE_TYPE_FORCE;
    }
}

bool
item::on_collide(b2Fixture *f, b2Vec2 v, float imp)
{
    if (this->flag_active(ENTITY_IS_ABSORBED)) {
        return false;
    }

    entity *e = static_cast<entity*>(f->GetUserData());
    b2Vec2 p = this->get_position();
    item *i = 0;
    if (e && e->g_id == O_ITEM) {
        i = static_cast<item*>(e);
    }

    switch (this->get_item_type()) {
        default: return true;
        case ITEM_SHOTGUN_PELLET:
            if (e) {
                if (i && e->flag_active(ENTITY_IS_BULLET)) {
                    tms_debugf("shotgun bullet colliding with other bullet");
                    if (i->get_item_type() == ITEM_SHOTGUN_PELLET &&
                        this->emitted_by == i->emitted_by) {
                        tms_debugf("ours!");
                        return false;
                    } else {
                        tms_debugf("NOT ours!");
                    }
                }
                tms_debugf("proceed");
                if (e->flag_active(ENTITY_IS_MAGNETIC)
                        || (e->g_id == O_CHUNK && f->GetUserData2() && ((tpixel_desc*)f->GetUserData2())->material > 1)) {
                    if (imp > 10.f && rand()%3 == 0) {
                        G->lock();
                        G->emit(new spark_effect(
                                    this->get_position(),
                                    this->get_layer()
                                    ), 0);
                        G->unlock();
                    }
                }
            }
        case ITEM_BULLET:
            if (i && (i->get_item_type() == ITEM_PLASMA || i->get_item_type() == ITEM_SOLAR)) {
                /*G->lock();
                G->absorb(this);
                G->unlock();*/
                return false;
            }
            break;

        case ITEM_ROCKET:
        case ITEM_SOLAR:
        case ITEM_PLASMA:
            break;
    }

    if (e) {
        float damage = this->get_damage();
        damage_type dt = this->get_damage_type();

        if (e->is_creature()) {
            creature *c = static_cast<creature*>(e);

            switch (this->get_item_type()) {
                case ITEM_BULLET:
                case ITEM_SHOTGUN_PELLET:
                    if (c->is_dead()) {
                        return false;
                    }
                    break;
            }

            if (c->is_foot_fixture(f)) {
                return false;
            }

            if (this->has_hit_enemy) {
                return false;
            }

            this->has_hit_enemy = true;

            G->lock();

            if (c->is_alive()) {
                if (!W->level.flag_active(LVL_DISABLE_ROBOT_HIT_SCORE)) {
                    if (!c->is_player() && c->gives_score && this->emitted_by == G->state.adventure_id) {
                        G->add_score(10);
                    }
                }
            }

            if (c->id != this->emitted_by || (this->get_item_type() != ITEM_SHOTGUN_PELLET
                                              && this->get_item_type() != ITEM_BULLET))
            c->damage(damage, f, dt, DAMAGE_SOURCE_BULLET, this->emitted_by);

            switch (this->properties[0].v.i) {
                case ITEM_BULLET:
                    G->emit(new spark_effect(
                                this->get_position(),
                                this->get_layer()
                                ), 0);
                case ITEM_SHOTGUN_PELLET:
                    G->absorb(this);
                    break;
            }

            G->unlock();
        } else if ((e->is_interactive() || e->g_id == O_PLANT)
                && W->level.flag_active(LVL_ENABLE_INTERACTIVE_DESTRUCTION)
                && imp >= 2.5f) {
            G->lock();
            if (G->absorb(this)) {
                tms_debugf("damage interactive");
                G->damage_interactive(e, f, f->GetUserData2(), damage * INTERACTIVE_DAMAGE_MODIFIER, v, this->get_damage_type());
            }
            G->unlock();
        } else if (e->g_id == O_RESOURCE) {
            //contact->SetEnabled(false);
        } else if (e->g_id == O_CRANE) {
            ((crane*)e)->hit = true;
        }
    }

    switch (this->properties[0].v.i) {
        case ITEM_SOLAR:
        case ITEM_PLASMA:
            if (i && i->item_category == ITEM_CATEGORY_BULLET) {
                return false;
            }

            {
                entity *effect = new plasma_explosion_effect(p, this->get_layer(), false, (this->get_scale() * this->get_scale()));
                G->lock();
                G->emit(effect, this, b2Vec2(0,0));
                G->absorb(this);
                G->unlock();
                return false;
            }
            break;

        case ITEM_ROCKET:
            {
                entity *effect = new explosion_effect(p, this->get_layer(), false, .75f);
                W->explode(this, p, this->get_layer(), 10, 100.f, .5f, .5f);

                G->lock();
                G->emit(effect, this, b2Vec2(0,0));
                G->absorb(this);
                G->unlock();
            }
            break;
    }

    return true;
}

void
item::remove_from_world()
{
    if (this->get_item_type() == ITEM_ROCKET) {
        if (((flame_effect*)this->data))
            ((flame_effect*)this->data)->done = true;
    }

    entity::remove_from_world();
}

void
item::set_item_type(uint32_t item_type)
{
    if (item_type >= NUM_ITEMS) item_type = NUM_ITEMS-1;

    this->properties[0].v.i = item_type;

    struct tms_scene *s = this->scene;

    const struct item_option &co = item_options[item_type];

    if (co.can_rotate) {
        this->num_sliders = 1;
    } else {
        this->num_sliders = 0;
    }

    this->set_flag(ENTITY_MUST_BE_DYNAMIC,      true);
    this->set_flag(ENTITY_IS_MAGNETIC,          co.magnetic);
    this->set_flag(ENTITY_DO_STEP,              co.step);
    this->set_flag(ENTITY_DO_MSTEP,             co.category != ITEM_CATEGORY_BULLET);
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS,    co.update_effects);
    this->set_flag(ENTITY_HAS_ACTIVATOR,        co.activator_radius > 0.f);

    this->set_flag(ENTITY_TRIGGER_EXPLOSIVES,   false);
    this->set_flag(ENTITY_IS_BULLET,            false);
    this->set_flag(ENTITY_FADE_ON_ABSORB,       true);

    switch (co.category) {
        case ITEM_CATEGORY_BULLET:
            this->set_flag(ENTITY_CAN_BE_COMPRESSED, false);
            break;

        default:
            this->set_flag(ENTITY_CAN_BE_COMPRESSED, true);
            break;
    }

    this->layer_mask = co.layer_mask;

    /* set up item-specific flags */
    switch (item_type) {
        case ITEM_SOLAR:
        case ITEM_PLASMA:
        case ITEM_ROCKET:
        case ITEM_SHOTGUN_PELLET:
            this->set_flag(ENTITY_FADE_ON_ABSORB,       false);
        case ITEM_BULLET:
            this->set_flag(ENTITY_TRIGGER_EXPLOSIVES,   true);
            this->set_flag(ENTITY_IS_BULLET,            true);
            //this->layer_mask = 2+4;
            break;
    }

    if (s) {
        G->u_effects.erase(this);
        W->stepable.erase(this);
        W->mstepable.erase(this);

        if (co.update_effects) {
            G->u_effects.insert(this);
        }

        if (co.step) {
            W->stepable.insert(this);
        }

        if (co.category != ITEM_CATEGORY_BULLET) {
            W->mstepable.insert(this);
        }
    }

    this->item_category = co.category;
    this->data_id = co.data_id;
    this->ef = co.ef;
}

void
item::step()
{
    switch (this->properties[0].v.i) {
        case ITEM_BULLET:
        case ITEM_SHOTGUN_PELLET:
        case ITEM_PLASMA:
        case ITEM_SOLAR:
            /*
            this->life += G->timemul(WORLD_STEP);
            if (this->life >= BULLET_LIFE) {
                if (this->type == BULLET_ROCKET) {
                    ((flame_effect*)this->flames)->done = true;
                }

                G->absorb(this);
            }*/
            break;

        case ITEM_ROCKET:
            {
                if (this->data == 0) {
                    flame_effect *f = (new flame_effect(this->get_position(), this->get_layer(), 0));
                    this->data = (void*)f;
                    G->emit(f, this, b2Vec2(0.f, 0.f), false);
                }
                //if (this->collided) {
                    //return;
                //}
                /* apply force on the rocket */
                b2Body *b = this->get_body(0);
                b2Vec2 force;

                float angle = this->get_angle();

                tmath_sincos(angle, &force.y, &force.x);

                force.x *= ROCKET_FORCE_MULT * b->GetMass();
                force.y *= ROCKET_FORCE_MULT * b->GetMass();

                force.x -= W->get_gravity().x * b->GetMass() * ROCKET_FALLOFF;
                force.y -= W->get_gravity().y * b->GetMass() * ROCKET_FALLOFF;

                b->ApplyForceToCenter(force);

                /* handle flames */
                b2Vec2 p;

                p = this->local_to_world(b2Vec2(-.3f, 0.f), 0);
                p.x += -.01f + ((rand()%100) / 100.f) * .02f;
                p.y += -.01f + ((rand()%100) / 100.f) * .02f;
                b2Vec2 v = b2Vec2(0.f, 0.f);

                flame_effect *flames = (flame_effect*)this->data;

                if (flames) {
                    flames->update_pos(p, v);
                }

                //sm::play(&sm::thruster, this->get_position().x, this->get_position().y, 0, tclampf(1.3f, 0.f, 1.f), true, this);
            }
            break;

        default:
            tms_warnf("unhandled step(), item %u with type %u", this->id, this->properties[0].v.i);
            break;
    }
}

void
item::mstep()
{
    uint64_t timediff = _tms.last_time - this->last_damage_tick;

    if (this->health < ITEM_MAX_HEALTH && timediff > 1000000) {
        float mul = tclampf((timediff-1000000)/500000.f, 0.f, 2.f);
        this->health += 0.15f * G->get_time_mul() * mul;

        G->add_hp(this, this->health / ITEM_MAX_HEALTH, TV_HP_GRAY);

        if (this->health > ITEM_MAX_HEALTH) {
            this->health = ITEM_MAX_HEALTH;
        }
    }
}

void
item::update_effects()
{
    b2Vec2 p = this->get_position();

    switch (this->properties[0].v.i) {
        case ITEM_SOLAR:
            spritebuffer::add(p.x, p.y, this->get_layer()*LAYER_DEPTH,
                    1.0f, 1.f, 0.8f, 1.f,
                    0.2f*(this->get_scale()*this->get_scale()), 0.2f*(this->get_scale()*this->get_scale()), 3, 0);

            spritebuffer::add2(p.x, p.y, this->get_layer()*LAYER_DEPTH,
                    1.0f, 1.f, 1.0f, 0.1f,
                    0.6f*(this->get_scale()*this->get_scale()), 0.6f*(this->get_scale()*this->get_scale()), 1,
                    cos((double)(_tms.last_time + rand()%100000)/100000.) * .25f);
            break;

        case ITEM_PLASMA:
            spritebuffer::add(p.x, p.y, this->get_layer()*LAYER_DEPTH,
                    0.3f, 1.f, 0.3f, 1.f,
                    0.2f, 0.2f, 3, 0);

            spritebuffer::add2(p.x, p.y, this->get_layer()*LAYER_DEPTH,
                    0.7f, 1.f, 0.7f, 0.1f,
                    0.6f, 0.6f, 1,
                    cos((double)(_tms.last_time + rand()%100000)/100000.) * .25f);
            break;

        default:
            tms_warnf("unhandled update_effects, item %u with type %u", this->id, this->properties[0].v.i);
            break;
    }
}

void
item::write_quickinfo(char *out)
{
    sprintf(out, "%s", item_options[this->get_item_type()].name);
}

void
item::_init()
{
    static struct tms_sprite sss;
    sss.tr=(tvec2){0.f,0.f};
    sss.bl=(tvec2){0.f,0.f};
    sss.width=0.f;
    sss.height=0.f;

    int ierr;

    for (int x=0; x<NUM_ITEMS; ++x) {
        struct item_option *io = &item_options[x];

        io->name_spr = G->text_small->add_to_atlas(G->texts, io->name);

        if (!io->name_spr) {
            io->name_spr = &sss;
        } else {
            io->name_spr->width *= gui_spritesheet::text_factor;
            io->name_spr->height *= gui_spritesheet::text_factor;
        }
    }
}

const char*
item::get_ui_name(uint32_t item_type)
{
    const struct item_option &io = item_options[item_type];
    return io.name;
}

void
item::damage(float dmg)
{
    this->health -= dmg;

    G->add_hp(this, this->health / ITEM_MAX_HEALTH, TV_HP_GRAY);

    if (this->health <= 0.f) {
        G->lock();
        this->drop_worth();
        G->absorb(this);
        G->unlock();
    }

    this->last_damage_tick = _tms.last_time;
}

#define RECOVER_RATE_MIN 0.45
#define RECOVER_RATE_MAX 1.0

void
item::drop_worth()
{
    const struct worth &w = item_options[this->get_sub_id()].worth;

    for (int x=0; x<NUM_RESOURCES; ++x) {
        uint32_t num = roundf(w.resources[x] * trandf(RECOVER_RATE_MIN, RECOVER_RATE_MAX));

        if (num) {
            uint32_t split = 1;

            if (num > 5) {
                split = 15;
            }

            while (num > 0) {
                uint32_t out = std::min(split, num);

                resource *r = static_cast<resource*>(of::create(O_RESOURCE));
                if (r) {
                    r->set_resource_type(x);
                    r->set_amount(out);
                    r->set_position(this->get_position().x, this->get_position().y);
                    r->set_layer(this->get_layer());
                    G->emit(r, this, b2Vec2(((rand()%100)-50)/100.f, ((rand()%100)-50)/100.f));
                }

                num -= out;
            }
            tms_infof("DROP %d!!!!!!!", num);
        }
    }
}

void
item::activate(creature *by)
{
    G->lock();
    if (this->item_category == ITEM_CATEGORY_POWERUP) {
        by->consume(this, true);
        G->absorb(this);
    }
    G->unlock();
}

void
item::on_touch(b2Fixture *my, b2Fixture *other)
{
    if (my == this->fx_sensor) {
        this->activator_touched(other);
    }
}

void
item::on_untouch(b2Fixture *my, b2Fixture *other)
{
    if (my == this->fx_sensor) {
        this->activator_untouched(other);
    }
}
