#pragma once

#include <tms/bindings/cpp/cpp.hh>
#include "text.hh"
#include "const.hh"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_STROKER_H
#include FT_GLYPH_H
#include FT_TRUETYPE_IDS_H

#define OUTLINE_MODIFIER 3.0
#define OUTLINE_SIZE 1
#define THICK_OUTLINE_SIZE 2
#define TEXT_ATLAS_CHANNELS 1

#define NUM_SOCKET_SPRITES 24

extern SDL_Color C_WHITE;
extern SDL_Color C_BLACK;

enum {
    S_SLIDER_2,
    S_SLIDER_3,
    S_SLIDER_HANDLE,
    S_IMPORT,
    S_EXPORT,
    S_STAR,
    S_LOCK,
    S_ERROR,
    S_CHECKMARK,
    S_MARKER,
    S_OIL,
    S_MOTOR_AXISROT,
    S_CONVEYOR_AXISROT,
    S_SELECT,
    S_WIP,
    S_WIP_2,
    S_PLUS,
    S_MINUS,
    S_MOUSE,
    S_TPIXEL_MULTI,
    S_RIGHT,
    S_LEFT,
    S_ADVENTURE_LEFTRIGHT,
    S_UP,
    S_DOWN,
    S_DROPDOWN,
    S_ADVUP,
    S_ADVDOWN,
    S_CONFIG,
    S_FLOOR,
    S_NOT_FLOOR,
    S_INFO,
    S_HELP,
    S_DC,
    S_CLOSE,
    S_SEARCH,
    S_PLAY,
    S_ORTHOGRAPHIC,
    S_PAUSE,
    S_MENU,
    S_RADIAL_2,
    S_RADIAL_3,
    S_RADIAL_W_KNOB,
    S_RADIAL_KNOB,
    S_FIELD_2,
    S_FIELD_3,
    S_FIELD_KNOB,
    S_ROUNDED_SQUARE,

    S_UNPLUG,
    S_LAYERVIS_1,
    S_LAYERVIS_2,
    S_LAYERVIS_3,
    S_AXIS,
    S_ATTACH,
    S_ATTACH_RIGID,
    S_ATTACH_ROTARY,

    S_SEP,
    S_ROT,
    S_EMPTY,
    S_MULTISEL,
    S_MULTICABLE,
    S_MULTICONN,
    S_CONNEDIT,
    S_MOVABLE,
    S_CW,
    S_CCW,
    S_BTN_LOCK,
    S_BTN_UNLOCK,

    S_MENU_PLAY,
    S_MENU_CREATE,
    S_BITHACK,

    S_ROUNDED_HELP,

    S_ILAYER0,
    S_ILAYER1,
    S_ILAYER2,

    S_BOXSELECT,
    S_SELECT_THROUGH,

    S_LAYER_INCLUSION,

    S_LAYER0,
    S_LAYER1,
    S_LAYER2,

    S_TPIXEL_MATERIAL0,
    S_TPIXEL_MATERIAL1,
    S_TPIXEL_MATERIAL2,
    S_TPIXEL_MATERIAL3,

    S_INVENTORY_ICONS0,
    S_INVENTORY_ICON_END = S_INVENTORY_ICONS0 + NUM_RESOURCES-1,

    NUM_SPRITES
};

struct sprite_load_data {
    const char *path;
    struct tms_atlas **atlas;
    struct tms_sprite *sprite;
};

class gui_spritesheet
{
  public:
    static struct sprite_load_data sprites[NUM_SPRITES];
    static inline struct tms_sprite *get_sprite(int sprite_id)
    {
        return gui_spritesheet::sprites[sprite_id].sprite;
    }
    static inline struct tms_sprite **get_rsprite(int sprite_id)
    {
        return &gui_spritesheet::sprites[sprite_id].sprite;
    }

    static bool use_cache;
    static void load_cache(void);

    static tms_atlas *tmp_atlas;
    static bool tmp_atlas_modified;
    static bool text_atlas_modified;
    static bool all_fonts_loaded;

    static tms_atlas *atlas;
    static tms_atlas *atlas_edev;
    static tms_atlas *atlas_text;

    static p_text *tx_sock_tag[];

    static p_text *t_test_playing_back; /* Test-playing level, press B to return to sandbox. */
    static p_text *t_get_ready; /* CLICK TO BEGIN */
    static p_text *t_continue; /* Continue >> */
    static p_text *t_win; /* Level completed! */
    static p_text *t_lose; /* Game over */
    static p_text *t_player_death; /* You died */
    static p_text *t_submit_score; /* Submit score */

    static p_text *t_in[NUM_SOCKET_SPRITES];
    static p_text *t_out[NUM_SOCKET_SPRITES];

    static p_text *tx_in[NUM_SOCKET_SPRITES];
    static p_text *tx_out[NUM_SOCKET_SPRITES];

    static float text_factor;

    static void init_atlas(void);
    static void init_loading_font(void);
    static void init_fonts(void);
    static void upload_text_atlas(void);
    static void init(void);
    static void deinit();
    static void add(struct tms_atlas *atlas, struct tms_sprite **sprite, const char *path);
    static void cleanup();

    static FT_Library ft;
};
