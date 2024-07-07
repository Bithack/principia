#pragma once

#ifdef __cplusplus

#include "misc.hh"

#include <tms/bindings/cpp/cpp.hh>

#include <vector>

class pscreen;
class menu_pkg;
class menu_main;
class menu_create;
class menu_play;
class loading_screen;
class settings;
class p_text;
#endif

class pkginfo;

#define COMMUNITY_URL(uri, ...) \
    char url[256]; \
    snprintf(url, 255, "https://%s/" uri, P.community_host, ##__VA_ARGS__);

#define STR1(x) #x
#define STR(x) STR1(x)

#define DRAG_TIME_EPS 100000
#define DRAG_DIST_EPS 30.f
#define DRAG_DIST_MIN_EPS 0.f

enum {
    ACTION_OPEN = 1,
    ACTION_RELOAD_GRAPHICS,
    ACTION_RELOAD_LEVEL,
    ACTION_SAVE,
    ACTION_NEW_LEVEL,
    ACTION_STICKY,
    ACTION_LOGIN,
    ACTION_SAVE_COPY,
    ACTION_CONSTRUCT_ENTITY,
    ACTION_OPEN_PLAY, /* 10 */
    ACTION_PUBLISH,
    ACTION_PLAY_PKG,
    ACTION_WARP,
    ACTION_PUBLISH_PKG,
    ACTION_PING,
    ACTION_UPGRADE_LEVEL,
    ACTION_DERIVE,
    ACTION_SET_STICKY_TEXT,
    ACTION_IMPORT_OBJECT,
    ACTION_EXPORT_OBJECT, /* 20 */
    ACTION_MULTIEMITTER_SET,
    ACTION_PUZZLEPLAY,
    ACTION_EDIT,
    ACTION_AUTOFIT_LEVEL_BORDERS,
    ACTION_RESTART_LEVEL,
    ACTION_BACK,
    ACTION_RESELECT,
    ACTION_HIGHLIGHT_SELECTED,
    ACTION_REGISTER,
    ACTION_LINK_ACCOUNT, /* 30 */
    ACTION_OPEN_AUTOSAVE,
    ACTION_REMOVE_AUTOSAVE,
    ACTION_GOTO_MAINMENU,
    ACTION_DELETE_LEVEL,
    ACTION_DELETE_PARTIAL,
    ACTION_SET_LEVEL_TYPE,
    ACTION_RELOAD_DISPLAY,
    ACTION_ENTITY_MODIFIED,
    ACTION_SET_MODE,
    ACTION_MAIN_MENU_PKG, /* 40 */
    ACTION_WORLD_PAUSE,
    ACTION_VERSION_CHECK,
    ACTION_LICENSE_CHECK,
    ACTION_LICENSE_CHECK_OFFLINE,
    ACTION_CONSTRUCT_ITEM,
    ACTION_SUBMIT_SCORE,
    ACTION_MULTI_DELETE,
    ACTION_OPEN_STATE,
    ACTION_AUTOSAVE,
    ACTION_MULTI_JOINT_STRENGTH, /* 50 */
    ACTION_MULTI_PLASTIC_COLOR,
    ACTION_MULTI_PLASTIC_DENSITY,
    ACTION_MULTI_CHANGE_CONNECTION_RENDER_TYPE,
    ACTION_MULTI_UNLOCK_ALL,
    ACTION_MULTI_DISCONNECT_ALL,
    ACTION_SELECT_IMPORT_OBJECT,
    ACTION_REFRESH_WIDGETS,
    ACTION_CREATE_ADVENTURE_ROBOT,
    ACTION_DELETE_SELECTION,
    ACTION_GET_FEATURED_LEVELS, /* 60 */
    ACTION_GOTO_CREATE,
    ACTION_REFRESH_HEADER_DATA,
    ACTION_CONSTRUCT_DECORATION,
    ACTION_GOTO_PLAY,
    ACTION_SAVE_STATE, /* 65 */
    ACTION_OPEN_MAIN_PUZZLE_SOLUTION,
    ACTION_CREATE_MAIN_PUZZLE_SOLUTION,
    ACTION_OPEN_LATEST_STATE,
    ACTION_OPEN_URL,
    ACTION_NEW_GENERATED_LEVEL, /* 70 */
    ACTION_SELF_DESTRUCT,

    ACTION_IGNORE
};

#define MAX_ACTIONS 10
#define MAX_FEATURED_LEVELS_FETCHED 16

#define ERROR_ACTION_LOG_IN 1

struct action {
    int id;
    void *data;
};

struct login_data {
    char username[256];
    char password[256];
};

struct register_data {
    char username[256];
    char email[256];
    char password[256];
};

/*
struct user {
    char username[255];
    bool logged_in;
};
*/

#ifdef __cplusplus

class intermediary : public tms::screen
{
  public:
    int (*loader)(int);
    tms::screen *next;

    intermediary(){};
    void prepare(int (*loader)(int), tms::screen *s);
    int resume(void);
    int render();
    void window_size_changed();
};

extern class principia
{
  public:
    ~principia();
    intermediary   *s_intermediary;
    loading_screen *s_loading_screen;
    menu_pkg       *s_menu_pkg;
    menu_main      *s_menu_main;
    menu_create    *s_menu_create;
    menu_play      *s_menu_play;

    std::vector<pscreen*> screens;

    SDL_mutex      *curl_mutex;
    void           *curl;

    // LOL
    int best_variable_in_the_world;
    int best_variable_in_the_world2;
    int best_variable_in_the_world3;

    struct action actions[MAX_ACTIONS];
    int num_actions;

    int  focused;
    bool loaded;

    const char *community_host;
    char *username;
    int user_id;
    int num_unread_messages;
    char *message;

    bool new_version_available;

    SDL_mutex *action_mutex;
    bool       can_reload_graphics;
    bool       can_set_settings;

    float default_ambient;
    float default_diffuse;

    void add_action(int id, void *data);
    void add_action(int id, uint32_t data=0)
    {
        this->add_action(id, UINT_TO_VOID(data));
    };
    void add_action(int id, int data=0)
    {
        this->add_action(id, INT_TO_VOID(data));
    };
    void add_action(int id, const char *data)
    {
        this->add_action(id, (void*)data);
    };

    tvec3 get_light_normal();
} P;

struct principia_action
{
    int action_id;
    void *action_data;

    principia_action(int id, void *data=0)
        : action_id(id)
        , action_data(data)
    { }
};

class open_play_data
{
  public:
    int id_type;
    uint32_t id;
    pkginfo *pkg;
    bool test_playing;
    int is_main_puzzle;

    open_play_data(int _id_type, uint32_t _id, pkginfo *_pkg, bool _test_playing=false, int _is_main_puzzle=0)
        : id_type(_id_type)
        , id(_id)
        , pkg(_pkg)
        , test_playing(_test_playing)
        , is_main_puzzle(_is_main_puzzle)
    { }
};

#else
void P_add_action(int id, void *data);
void P_focus(int focus);
#endif

