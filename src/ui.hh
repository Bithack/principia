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
#define DIALOG_HELP             124
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
extern "C" {
#endif
const char* ui_get_property_string(int index);
void ui_set_property_string(int index, const char* val);
uint8_t ui_get_property_uint8(int index);
void ui_set_property_uint8(int index, uint8_t val);
uint32_t ui_get_property_uint32(int index);
void ui_set_property_uint32(int index, uint32_t val);
float ui_get_property_float(int index);
void ui_set_property_float(int index, float val);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class ui
{
  public:
    static int next_action;

    static void init();
    static void message(const char *str, bool long_duration=false);
    static void messagef(const char *str, ...);
    static void open_dialog(int num, void *data=0);
    static void open_help_dialog(const char *title, const char *description);
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
extern "C" {
#endif

#ifdef TMS_BACKEND_IOS
/* ios callbacks */
#include <stdint.h>
    struct ios_menu_object {
        const char *name;
        uint32_t g_id;
    };
    extern struct ios_menu_object *g_all_menu_objects;
    extern int g_num_menu_objects;

    extern const char *_prompt_buttons[3];
    extern const char *_prompt_text;

    void ui_cb_prompt_response(int r);
    int ui_settings_get_shadow_resx();
    int ui_settings_get_shadow_resy();
    int ui_settings_get_enable_shadows();
    int ui_settings_get_shadow_quality();
    int ui_settings_get_ao_quality();
    int ui_settings_get_enable_ao();
    float ui_settings_get_ui_scale();
    float ui_settings_get_cam_speed();
    float ui_settings_get_zoom_speed();
    int ui_settings_get_enable_smooth_cam();
    int ui_settings_get_enable_smooth_zoom();

    void ui_settings_set_shadow_resx(int r);
    void ui_settings_set_shadow_resy(int y);
    void ui_settings_set_enable_shadows(int e);
    void ui_settings_set_shadow_quality(int);
    void ui_settings_set_ao_quality(int v);
    void ui_settings_set_enable_ao(int v);
    void ui_settings_set_ui_scale(float v);
    void ui_settings_set_cam_speed(float v);
    void ui_settings_set_zoom_speed(float v);
    void ui_settings_set_enable_smooth_cam(int v);
    void ui_settings_set_enable_smooth_zoom(int v);
    void ui_settings_save();

    void ui_cb_update_jumper();

    int ui_settings_get_enable_border_scrolling();
    float ui_settings_get_border_scrolling_speed();
    int ui_settings_get_enable_object_ids();
    void ui_settings_set_enable_border_scrolling(int s);
    void ui_settings_set_border_scrolling_speed(float s);
    void ui_settings_set_enable_object_ids(int s);

    void ui_message(const char *msg, bool long_duration);
    void ui_open_dialog(int num);
    void ui_open_sandbox_tips();
    void ui_open_error_dialog(const char *error_str);

    void P_set_can_reload_graphics(int v);
    void P_set_can_set_settings(int v);
    int P_get_can_reload_graphics(void);
    int P_get_can_set_settings(void);

    /* XXX GID XXX */
    uint8_t ui_get_entity_gid();
    void ui_cb_refresh_sequencer();
    void ui_cb_set_color(float r, float g, float b, float a);
    void ui_cb_reset_variable(const char *var);
    void ui_cb_reset_all_variables();
    void ui_cb_set_robot_dir(int dir);
    void ui_cb_set_level_type(int type);
    void ui_cb_set_locked(int v);
    int ui_cb_get_locked();
    int ui_cb_get_level_type();
    char *ui_cb_get_level_title();
    const char *ui_cb_get_level_description();
    void ui_cb_set_level_title(const char *s);
    void ui_cb_set_level_description(const char*s);
    int ui_cb_get_level_flag(uint64_t f);
    void ui_cb_set_level_flag(uint64_t f, int v);
    void ui_cb_set_pause_on_win(int v);
    void ui_cb_set_display_score(int v);
    int ui_cb_get_pause_on_win();
    int ui_cb_get_display_score();
    void ui_cb_menu_item_selected(int n);
    void ui_cb_set_command(int n);
    void ui_cb_set_consumable(int n);
    int ui_cb_get_command();
    int ui_cb_get_followmode();
    void ui_cb_set_followmode(int n);
    int ui_cb_get_event();
    void ui_cb_set_event(int n);
    int ui_cb_get_fx(int n);
    float ui_cb_get_level_prism_tolerance(void);
    void ui_cb_set_level_prism_tolerance(float s);
    float ui_cb_get_level_pivot_tolerance(void);
    void ui_cb_set_level_pivot_tolerance(float s);
    void ui_cb_set_fx(int n, int fx);
    void ui_cb_update_rubber();
    void ui_cb_set_level_border(int d, uint16_t);
    uint16_t ui_cb_get_level_border(int d);
    void ui_cb_set_level_bg(uint8_t bg);
    uint8_t ui_cb_get_level_bg();
    void ui_cb_set_level_gravity_x(float v);
    float ui_cb_get_level_gravity_x();
    void ui_cb_set_level_gravity_y(float v);
    float ui_cb_get_level_gravity_y();
    uint8_t ui_cb_get_level_pos_iter(void);
    void ui_cb_set_level_pos_iter(uint8_t s);
    uint8_t ui_cb_get_level_vel_iter(void);
    void ui_cb_set_level_vel_iter(uint8_t s);
    uint32_t ui_cb_get_level_final_score(void);
    void ui_cb_set_level_final_score(uint32_t s);
    const char *ui_cb_get_tip();

    int ui_cb_get_hide_tips();
    void ui_cb_set_hide_tips(int h);
#endif

#ifdef __cplusplus
}
#endif

#if defined(TMS_BACKEND_PC) && !defined(NO_UI)
extern int prompt_is_open;
#endif

extern const char* tips[];
extern const int num_tips;
extern int ctip;
