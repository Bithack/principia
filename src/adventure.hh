#pragma once

#include "creature.hh"
#include "entity.hh"
#include "text.hh"

#define BAR_SIDE_LEFT  0
#define BAR_SIDE_RIGHT 1
#define ADVENTURE_NUM_FOCUSED 9

#define ADVENTURE_BTN_WEAPON_UP  1
#define ADVENTURE_BTN_TOOL_UP    2
#define ADVENTURE_BTN_LAYER_UP   3
#define ADVENTURE_BTN_LAYER_DOWN 4
#define ADVENTURE_BTN_AIM        5
#define ADVENTURE_BTN_ACTION     6
#define ADVENTURE_BTN_JUMP       7
#define ADVENTURE_BTN_ATTACK     8

enum {
    ADVENTURE_FOCUSED_WEAPON_UP,
    ADVENTURE_FOCUSED_TOOL_UP,
    ADVENTURE_FOCUSED_LEFT,
    ADVENTURE_FOCUSED_RIGHT,
    ADVENTURE_FOCUSED_LAYER_UP,
    ADVENTURE_FOCUSED_LAYER_DOWN,
    ADVENTURE_FOCUSED_AIM,
    ADVENTURE_FOCUSED_ACTION,
    ADVENTURE_FOCUSED_JUMP,
};

enum {
    BAR_TOOL,
    BAR_TOOL_CD,
    BAR_ARMOR,

    NUM_BARS
};

class factory;
class creature;

struct bar
{
    float value; /** value between 0.f and 1.f, how much of the bar should be filled */
    float time; /** time to stay visible */
    tvec3 color;
    int8_t side;
};

class adventure
{
  public:
    static void init();

    static int handle_input_playing(tms::event *ev, int action);

    static bool widgets_active;
    static bool focused(int w);

    static void refresh_widgets();
    static void init_widgets();
    static void hide_left_widgets();
    static void show_left_widgets();
    static void clear_widgets();

    static void update_mining_pos(float x, float y);
    static void begin_mining();
    static void end_mining();
    static bool is_player_alive();
    static void on_player_die();
    static void respawn();

    static void set_player(creature *c, bool snap_camera=true, bool set_rc=true);
    static creature *player;

    static int pending_layermove;
    static int layermove_attempts;
    static int last_mouse_x;
    static int last_mouse_y;

    static void checkpoint_activated(checkpoint *cp);
    static void step();

    static joint_info   *ji;

    static uint64_t mining_timer;
    static bool first_mine; /* whether this is a single-click mine or held down */
    static int mining_zap_type;
    static bool mining;
    static b2Vec2 mining_pos;
    static bool dead;
    static uint32_t death_step;
    static int64_t death_wait;

    static factory *current_factory;

    static int kill_player_counter;
    static bool kill_player;

    static float highlight_inventory[NUM_RESOURCES];

    static int last_picked_up_resource;

    static bool key_down[ADVENTURE_NUM_FOCUSED];

    static void reset_tools();

    static bar bars[NUM_BARS];

    static p_text *current_tool;

    static int num_weapons;
    static int num_tools;
    static tvec2 weapon_icon_pos[NUM_WEAPONS];
    static tvec2 tool_icon_pos[NUM_WEAPONS];

    static void reset();
    static void setup();

    static void render();

    static void tool_changed();

    static tms_wdg *w_weapon_up;
    static tms_wdg *w_tool_up;
    static tms_wdg *w_move_slider;
    static tms_wdg *w_layer_up;
    static tms_wdg *w_layer_down;
    static tms_wdg *w_aim;
    static tms_wdg *w_action;
    static tms_wdg *w_jump;
};
