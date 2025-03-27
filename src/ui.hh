#pragma once

#include "main.hh"

#include <stdint.h>

#define DIALOG_IGNORE          -1
#define DIALOG_SANDBOX_MENU     1
#define DIALOG_QUICKADD         100
#define DIALOG_BEAM_COLOR       101
#define DIALOG_SAVE             102
#define DIALOG_OPEN             103
#define DIALOG_NEW_LEVEL        104
#define DIALOG_SET_FREQUENCY    105
#define DIALOG_PIXEL_COLOR      106
#define DIALOG_CONFIRM_QUIT     107
#define DIALOG_SET_COMMAND      108
#define DIALOG_STICKY           109
#define DIALOG_FXEMITTER        110
#define DIALOG_CAMTARGETER      111
#define DIALOG_SET_FREQ_RANGE   112
#define DIALOG_OPEN_OBJECT      113
#define DIALOG_EXPORT           114
#define DIALOG_SET_PKG_LEVEL    115
#define DIALOG_ROBOT            116
#define DIALOG_MULTIEMITTER     117
#define DIALOG_PUZZLE_PLAY      118
#define DIALOG_TIMER            119
#define DIALOG_EVENTLISTENER    120
#define DIALOG_SETTINGS         121
#define DIALOG_SAVE_COPY        122
#define DIALOG_LEVEL_PROPERTIES 123
#define DIALOG_LEVEL_INFO       124
#define DIALOG_DIGITALDISPLAY   125
#define DIALOG_PLAY_MENU        126
#define DIALOG_OPEN_AUTOSAVE    127
#define DIALOG_COMMUNITY        128
#define DIALOG_PROMPT_SETTINGS  129
#define DIALOG_PROMPT           130
#define DIALOG_SFXEMITTER       131
#define DIALOG_VARIABLE         132
#define DIALOG_SYNTHESIZER      133
#define DIALOG_SEQUENCER        134
#define DIALOG_SHAPEEXTRUDER    135
#define DIALOG_JUMPER           136
#define DIALOG_REGISTER         137
#define DIALOG_PUBLISHED        138

#define DIALOG_CURSORFIELD      140
#define DIALOG_ESCRIPT          141
#define DIALOG_ITEM             142

#define DIALOG_SANDBOX_MODE     143

#define DIALOG_RUBBER           144

// 145 used to be the main menu package dialog

#define DIALOG_SOUNDMAN         148
#define DIALOG_FACTORY          149

#define DIALOG_SET_FACTION      150
#define DIALOG_RESOURCE         151
#define DIALOG_VENDOR           152
#define DIALOG_ANIMAL           153
#define DIALOG_POLYGON          154
#define DIALOG_KEY_LISTENER     155
#define DIALOG_OPEN_STATE       156
#define DIALOG_POLYGON_COLOR    157
#define DIALOG_MULTI_CONFIG     158
#define DIALOG_EMITTER          159
#define DIALOG_TREASURE_CHEST   160
#define DIALOG_DECORATION       161
#define DIALOG_SFXEMITTER_2     162

#define CLOSE_ALL_DIALOGS                  200
#define CLOSE_ABSOLUTELY_ALL_DIALOGS       201
#define CLOSE_REGISTER_DIALOG              202
#define DISABLE_REGISTER_LOADER            203

#define DIALOG_PUBLISH          300
#define DIALOG_LOGIN            301

#define SIGNAL_LOGIN_SUCCESS        100
#define SIGNAL_LOGIN_FAILED         101
#define SIGNAL_QUICKADD_REFRESH     200
#define SIGNAL_REFRESH_BORDERS      300

#define SIGNAL_REGISTER_SUCCESS     110
#define SIGNAL_REGISTER_FAILED      111

#define SIGNAL_ENTITY_CONSTRUCTED   406

enum {
    ALERT_INFORMATION,
};

enum {
    CONFIRM_TYPE_DEFAULT,
    CONFIRM_TYPE_BACK_SANDBOX,
};

struct confirm_data
{
    int confirm_type;

    confirm_data(int _confirm_type)
        : confirm_type(_confirm_type)
    { }
};

#ifdef __cplusplus
class ui
{
  public:
    static int next_action;

    static void init();
    static void message(const char *str, bool long_duration=false);
    static void messagef(const char *str, ...);
    static void open_dialog(int num, void *data=0);
    static void open_error_dialog(const char *error_string);
    static void open_sandbox_tips();
    static void open_url(const char *url);
    static void emit_signal(int num, void *data=0);
    static void set_next_action(int action_id);
    static void set_fail_action(int action_id);
    static void set_cancel_action(int action_id);
    static void quit();
    static void confirm(const char *text,
            const char *button1, struct principia_action action1,
            const char *button2, struct principia_action action2,
            const char *button3=0, struct principia_action action3=ACTION_IGNORE,
            confirm_data cd=confirm_data(CONFIRM_TYPE_DEFAULT)
            );
    static void alert(const char *text, uint8_t alert_type=ALERT_INFORMATION);
    static void render();
};
#endif

#if defined(TMS_BACKEND_PC) && !defined(NO_UI)
extern int prompt_is_open;
#endif

extern const char* tips[];
extern const int num_tips;
extern int ctip;
