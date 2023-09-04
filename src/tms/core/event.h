#ifndef _TMS_EVENT__H_
#define _TMS_EVENT__H_

#include <tms/core/err.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tms_screen;

enum TMS_KEYS {
    TMS_KEY_NONE = 0,
    TMS_KEY_A = 4,
    TMS_KEY_B,
    TMS_KEY_C,
    TMS_KEY_D,
    TMS_KEY_E,
    TMS_KEY_F,
    TMS_KEY_G, /* 10 */
    TMS_KEY_H,
    TMS_KEY_I,
    TMS_KEY_J,
    TMS_KEY_K,
    TMS_KEY_L,
    TMS_KEY_M,
    TMS_KEY_N,
    TMS_KEY_O,
    TMS_KEY_P,
    TMS_KEY_Q, /* 20 */
    TMS_KEY_R,
    TMS_KEY_S,
    TMS_KEY_T,
    TMS_KEY_U,
    TMS_KEY_V,
    TMS_KEY_W,
    TMS_KEY_X,
    TMS_KEY_Y,
    TMS_KEY_Z,
    TMS_KEY_1, /* 30 */
    TMS_KEY_2,
    TMS_KEY_3,
    TMS_KEY_4,
    TMS_KEY_5,
    TMS_KEY_6,
    TMS_KEY_7,
    TMS_KEY_8,
    TMS_KEY_9,
    TMS_KEY_0,
    TMS_KEY_ENTER, /* 40 */
    TMS_KEY_ESC, // ESCAPE?
    TMS_KEY_BACKSPACE,
    TMS_KEY_TAB,
    TMS_KEY_SPACE,
    TMS_KEY_MINUS,
    TMS_KEY_EQUALS,
    TMS_KEY_LBRACKET,
    TMS_KEY_RBRACKET,
    TMS_KEY_BACKSLASH,
    TMS_KEY_SEMICOLON = 51,
    TMS_KEY_APOSTROPHE,
    TMS_KEY_GRAVE,
    TMS_KEY_COMMA,
    TMS_KEY_PERIOD,
    TMS_KEY_FRONTSLASH,
    TMS_KEY_F1 = 58,
    TMS_KEY_F2,
    TMS_KEY_F3, /* 60 */
    TMS_KEY_F4,
    TMS_KEY_F5,
    TMS_KEY_F6,
    TMS_KEY_F7,
    TMS_KEY_F8,
    TMS_KEY_F9,
    TMS_KEY_F10,
    TMS_KEY_F11,
    TMS_KEY_F12,
    TMS_KEY_SCROLLLOCK = 71,
    TMS_KEY_PAUSE,
    TMS_KEY_INSERT,
    TMS_KEY_HOME,
    TMS_KEY_PAGEUP,
    TMS_KEY_DELETE,
    TMS_KEY_END,
    TMS_KEY_PAGEDOWN,
    TMS_KEY_RIGHT,
    TMS_KEY_LEFT, /* 80 */
    TMS_KEY_DOWN,
    TMS_KEY_UP,
    TMS_KEY_MENU = 101,
    TMS_KEY_LEFT_CTRL = 224,
    TMS_KEY_LEFT_SHIFT,
    TMS_KEY_LEFT_ALT,
    TMS_KEY_LEFT_META,
    TMS_KEY_RIGHT_CTRL,
    TMS_KEY_RIGHT_SHIFT,
    TMS_KEY_RIGHT_ALT,
    TMS_KEY_RIGHT_META,

    TMS_KEY__NUM
};

enum TMS_MODS
{
    TMS_MOD_NONE = 0x0000,
    TMS_MOD_LSHIFT = 0x0001,
    TMS_MOD_RSHIFT = 0x0002,
    TMS_MOD_LCTRL = 0x0040,
    TMS_MOD_RCTRL = 0x0080,
    TMS_MOD_LALT = 0x0100,
    TMS_MOD_RALT = 0x0200,
    TMS_MOD_LGUI = 0x0400,
    TMS_MOD_RGUI = 0x0800,
    TMS_MOD_NUM = 0x1000,
    TMS_MOD_CAPS = 0x2000,
    TMS_MOD_MODE = 0x4000,
    TMS_MOD_RESERVED = 0x8000
};

#define TMS_MOD_CTRL   (TMS_MOD_LCTRL|TMS_MOD_RCTRL)
#define TMS_MOD_SHIFT  (TMS_MOD_LSHIFT|TMS_MOD_RSHIFT)
#define TMS_MOD_ALT    (TMS_MOD_LALT|TMS_MOD_RALT)
#define TMS_MOD_GUI    (TMS_MOD_LGUI|TMS_MOD_RGUI)

enum {
    TMS_EV_NONE           = 1 << 0,
    TMS_EV_KEY_DOWN       = 1 << 1,
    TMS_EV_KEY_PRESS      = 1 << 2,
    TMS_EV_KEY_UP         = 1 << 3,
    TMS_EV_POINTER_DOWN   = 1 << 4,
    TMS_EV_POINTER_UP     = 1 << 5,
    TMS_EV_POINTER_DRAG   = 1 << 6,
    TMS_EV_POINTER_MOVE   = 1 << 7,
    TMS_EV_POINTER_SCROLL = 1 << 8,
    TMS_EV_KEY_REPEAT     = 1 << 9,
    TMS_EV_TEXT_INPUT     = 1 << 10,
};

enum {
    TMS_EV_MASK_KEY     = 0x00000e,
    TMS_EV_MASK_POINTER = 0x0000f0,
    TMS_EV_MASK_DOWN    = 0x000016,
    TMS_EV_MASK_PRESS   = 0x000014,
    TMS_EV_MASK_UP      = 0x000028,
    TMS_EV_MASK_ALL     = 0xffffff,
};

struct tms_pointer_motion {
    uint64_t pointer_id;
    float x;
    float y;
};

struct tms_pointer_scroll {
    int mouse_x;
    int mouse_y;
    int32_t x;
    int32_t y;
};

struct tms_pointer_button {
    uint64_t pointer_id;
    float x;
    float y;
    int button;
};

struct tms_key {
    int keycode;
    uint16_t mod;
};

struct tms_text_input {
    char text[32];
};

struct tms_event {
    int  type;
    union {
        struct tms_pointer_motion    motion;
        struct tms_pointer_button    button;
        struct tms_key               key;
        struct tms_pointer_scroll    scroll;
        struct tms_text_input        text;
    } data;
};

int tms_event_push(struct tms_event in);
int tms_event_process_all(struct tms_screen *s);

typedef int (*tms_event_handler)(struct tms_event*);
int tms_event_register_raw(tms_event_handler);

#ifdef __cplusplus
}
#endif

#endif
