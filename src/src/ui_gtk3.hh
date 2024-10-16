
#ifdef TMS_BACKEND_PC

// fuckgtk3
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef USE_GTK_SOURCE_VIEW
#include <gtksourceview/gtksource.h>
#endif

#ifdef TMS_BACKEND_WINDOWS
#include <windows.h>
#include <shellapi.h>
#endif

static gboolean _close_all_dialogs(gpointer unused);

SDL_bool   ui_ready = SDL_FALSE;
SDL_cond  *ui_cond;
SDL_mutex *ui_lock;
static gboolean _sig_ui_ready(gpointer unused);

typedef std::map<int, std::pair<int, int> > freq_container;

enum {
    LF_MENU,
    LF_ITEM,
    LF_DECORATION,

    NUM_LF
};

/* open window columns */
enum {
    OC_ID,
    OC_NAME,
    OC_VERSION,
    OC_DATE,

    OC_NUM_COLUMNS
};

enum {
    OSC_ID,
    OSC_NAME,
    OSC_DATE,
    OSC_SAVE_ID,
    OSC_ID_TYPE,

    OSC_NUM_COLUMNS
};

enum {
    FC_FREQUENCY,
    FC_RECEIVERS,
    FC_TRANSMITTERS,

    FC_NUM_COLUMNS
};

enum {
    RESPONSE_PUZZLE,
    RESPONSE_EMPTY_ADVENTURE,
    RESPONSE_ADVENTURE,
    RESPONSE_CUSTOM,
};

enum {
    RESPONSE_CONN_EDIT,
    RESPONSE_MULTISEL,
    RESPONSE_DRAW,
};

typedef struct {
    uint32_t id;
    gchar   *name;
    long   time;
} oc_column;

int prompt_is_open = 0;
GtkDialog *cur_prompt = 0;

enum mark_type {
    MARK_ENTITY,
    MARK_POSITION,
    MARK_PLAYER,
};

struct goto_mark {
    mark_type type;
    const char *label;
    uint32_t id;
    tvec2 pos;
    GtkMenuItem *menuitem;
    guint key;

    goto_mark(mark_type _type, const char *_label, uint32_t _id, tvec2 _pos)
        : type(_type)
        , label(_label)
        , id(_id)
        , pos(_pos)
        , menuitem(0)
        , key(0)
    { }
};

/** --Menu **/
GtkMenu         *editor_menu;
static uint8_t   editor_menu_on_entity = 0;
GtkMenuItem     *editor_menu_header;
/* ---------------------- */
GtkMenuItem     *editor_menu_move_here_player;
GtkMenuItem     *editor_menu_move_here_object;
GtkMenuItem     *editor_menu_go_to; /* submenu */
GtkMenu         *editor_menu_go_to_menu;
/* -------------------------- */
GtkMenuItem     *editor_menu_set_as_player;
GtkMenuItem     *editor_menu_toggle_mark_entity;
/* -------------------------- */
GtkMenuItem     *editor_menu_lvl_prop;
GtkMenuItem     *editor_menu_save;
GtkMenuItem     *editor_menu_save_copy;
#ifdef BUILD_PKGMGR
GtkMenuItem     *editor_menu_package_manager;
#endif
GtkMenuItem     *editor_menu_publish;
GtkMenuItem     *editor_menu_settings;
GtkMenuItem     *editor_menu_login;
struct goto_mark *editor_menu_last_created = new goto_mark(MARK_ENTITY, "Last created entity", 0, tvec2f(0.f, 0.f));
struct goto_mark *editor_menu_last_cam_pos = new goto_mark(MARK_POSITION, "Last camera position", 0, tvec2f(0.f, 0.f));
static std::deque<struct goto_mark*> editor_menu_marks;

static guint valid_keys[9] = {
    GDK_KEY_1,
    GDK_KEY_2,
    GDK_KEY_3,
    GDK_KEY_4,
    GDK_KEY_5,
    GDK_KEY_6,
    GDK_KEY_7,
    GDK_KEY_8,
    GDK_KEY_9
};

static void
refresh_mark_menuitems()
{
    GtkAccelGroup *accel_group = gtk_menu_get_accel_group(editor_menu);
    int x=0;

    for (std::deque<struct goto_mark*>::iterator it = editor_menu_marks.begin();
            it != editor_menu_marks.end(); ++it) {
        struct goto_mark* mark = *it;
        GtkMenuItem *item = mark->menuitem;
        if (x < 9) {
            mark->key = valid_keys[x];
            gtk_widget_add_accelerator (GTK_WIDGET(item), "activate", accel_group,
                    valid_keys[x], (GdkModifierType)0, GTK_ACCEL_VISIBLE);

            ++ x;
        } else {
            mark->key = 0;
        }
    }
}

/** --Play menu **/
GtkMenu         *play_menu;

/** --Open state **/
GtkWindow    *open_state_window;
GtkTreeModel *open_state_treemodel;
GtkTreeView  *open_state_treeview;
GtkButton    *open_state_btn_open;
GtkButton    *open_state_btn_cancel;
static bool   open_state_no_testplaying = false;

/** --Multi config **/
GtkWindow    *multi_config_window;
GtkNotebook  *multi_config_nb;
GtkButton    *multi_config_apply;
GtkButton    *multi_config_cancel;
int           multi_config_cur_tab = 0;
enum {
    TAB_JOINT_STRENGTH,
    TAB_PLASTIC_COLOR,
    TAB_PLASTIC_DENSITY,
    TAB_CONNECTION_RENDER_TYPE,
    TAB_MISCELLANEOUS,

    NUM_MULTI_CONFIG_TABS
};
/* Joint strength */
GtkScale    *multi_config_joint_strength;
/* Plastic color */
GtkColorChooserWidget *multi_config_plastic_color;
/* Plastic density */
GtkScale    *multi_config_plastic_density;
/* Connection render type */
GtkRadioButton  *multi_config_render_type_normal;
GtkRadioButton  *multi_config_render_type_small;
GtkRadioButton  *multi_config_render_type_hide;
/* Miscellaneous */
GtkButton       *multi_config_unlock_all;
GtkButton       *multi_config_disconnect_all;

/** --Open level **/
GtkWindow    *open_window;
GtkTreeModel *open_treemodel;
GtkTreeView  *open_treeview;
GtkButton    *open_btn_open;
GtkButton    *open_btn_cancel;
GtkMenu      *open_menu;
GtkMenuItem  *open_menu_information;
GtkMenuItem  *open_menu_delete;

/** --Open object **/
bool         object_window_multiemitter;
GtkWindow    *object_window;
GtkTreeModel *object_treemodel;
GtkTreeView  *object_treeview;
GtkButton    *object_btn_open;
GtkButton    *object_btn_cancel;

/* --Save and Save as copy */
GtkWindow *save_window;
GtkEntry  *save_entry;
GtkLabel  *save_status;
GtkButton *save_ok;
GtkButton *save_cancel;
uint8_t    save_type = SAVE_REGULAR;

/* --Export */
GtkWindow *export_window;
GtkEntry  *export_entry;
GtkLabel  *export_status;
GtkButton *export_ok;
GtkButton *export_cancel;

/** --Package manager **/
GtkWindow       *package_window;
GtkTreeModel    *pk_pkg_treemodel;
GtkTreeView     *pk_pkg_treeview;
GtkCheckButton  *pk_pkg_first_is_menu;
GtkCheckButton  *pk_pkg_return_on_finish;
GtkSpinButton   *pk_pkg_unlock_count;
//GtkWidget       *pk_pkg_delete;
GtkWidget       *pk_pkg_create;
GtkWidget       *pk_pkg_play;
GtkWidget       *pk_pkg_publish;
GtkTreeModel    *pk_lvl_treemodel;
GtkTreeView     *pk_lvl_treeview;
GtkWidget       *pk_lvl_add;
GtkWidget       *pk_lvl_del;
GtkWidget       *pk_lvl_play;
bool pk_ignore_lvl_changes = true;

/* --Package name dialog */
GtkDialog *pkg_name_dialog;
GtkEntry  *pkg_name_entry;
GtkButton *pkg_name_ok;

/** --Level properties **/
GtkDialog       *properties_dialog;
GtkButton       *lvl_ok;
GtkButton       *lvl_cancel;
GtkRadioButton  *lvl_radio_adventure;
GtkRadioButton  *lvl_radio_custom;
GtkEntry        *lvl_title;
GtkTextView     *lvl_descr;
GtkComboBoxText *lvl_bg;
GtkColorButton  *lvl_bg_color;
uint32_t         new_bg_color;
GtkEntry        *lvl_width_left;
GtkEntry        *lvl_width_right;
GtkEntry        *lvl_height_down;
GtkEntry        *lvl_height_up;
GtkButton       *lvl_autofit;
GtkSpinButton   *lvl_gx;
GtkSpinButton   *lvl_gy;
GtkScale       *lvl_pos_iter;
GtkScale       *lvl_vel_iter;
GtkScale       *lvl_prismatic_tol;
GtkScale       *lvl_pivot_tol;
GtkScale       *lvl_linear_damping;
GtkScale       *lvl_angular_damping;
GtkScale       *lvl_joint_friction;
GtkScale       *lvl_enemy_absorb_time;
GtkScale       *lvl_player_respawn_time;

GtkEntry        *lvl_score;
GtkCheckButton  *lvl_pause_on_win;
GtkCheckButton  *lvl_show_score;
GtkButton       *lvl_upgrade;

enum ROW_TYPES {
    ROW_CHECKBOX,
    ROW_HSCALE,
};

struct setting_row_type
{
    int type;

    /* hscale */
    double min;
    double max;
    double step;

    static const struct setting_row_type
    create_checkbox()
    {
        struct setting_row_type srt;
        srt.type = ROW_CHECKBOX;

        return srt;
    }

    static const struct setting_row_type
    create_hscale(double min, double max, double step)
    {
        struct setting_row_type srt;
        srt.type = ROW_HSCALE;

        srt.min = min;
        srt.max = max;
        srt.step = step;

        return srt;
    }
};

struct table_setting_row {
    const char *label;
    const char *help;
    const char *setting_name;
    const struct setting_row_type row;
    GtkWidget *wdg;
};

struct table_setting_row settings_graphic_rows[] = {
    {
        "Enable bloom",
        0,
        "enable_bloom",
        setting_row_type::create_checkbox()
    }, {
        "Vertical sync",
        0,
        "vsync",
        setting_row_type::create_checkbox()
    }, {
        "Gamma correction",
        0,
        "gamma_correct",
        setting_row_type::create_checkbox()
    },
};

struct table_setting_row settings_audio_rows[] = {
    {
        "Volume",
        "Master volume",
        "volume",
        setting_row_type::create_hscale(0.0, 1.0, 0.05),
    }, {
        "Mute all sounds",
        0,
        "muted",
        setting_row_type::create_checkbox()
    },
};

struct table_setting_row settings_control_rows[] = {
    {
        "Enable touch controls",
        "Enable this if you want widgets on the screen that you can control the adventure robot with.",
        "touch_controls",
        setting_row_type::create_checkbox()
    }, {
        "Enable cursor jail",
        "Enable this if you want the cursor to be locked to the game while playing a level.",
        "jail_cursor",
        setting_row_type::create_checkbox()
    }, {
        "Smooth camera",
        "Whether the camera movement should be smooth or direct.",
        "smooth_cam",
        setting_row_type::create_checkbox()
    }, {
        "Camera speed",
        "How fast you can move the camera.",
        "cam_speed_modifier",
        setting_row_type::create_hscale(0.1, 15.0, 0.5),
    }, {
        "Smooth zoom",
        "Whether the zooming should be smooth or direct.",
        "smooth_zoom",
        setting_row_type::create_checkbox()
    }, {
        "Zoom speed",
        "How fast you can zoom in your level.",
        "zoom_speed",
        setting_row_type::create_hscale(0.1, 3.0, 0.5),
    }, {
        "Smooth menu",
        "Whether the menu scrolling should be smooth or direct.",
        "smooth_menu",
        setting_row_type::create_checkbox()
    }, {
        "Menu scroll speed",
        "How fast you can scroll through the menu.",
        "menu_speed",
        setting_row_type::create_hscale(1.0, 15.0, 0.5),
    }, {
        "Widget sensitivity",
        "Controls the mouse-movement-sensitivity used to control sliders, radials and fields using the hotkey mode.",
        "widget_control_sensitivity",
        setting_row_type::create_hscale(0.1, 8.0, 0.25),
    }, {
        "Enable RC cursor lock",
        "Lock the cursor if you active an RC widgets mouse control.",
        "rc_lock_cursor",
        setting_row_type::create_checkbox()
    }, {
        "Emulate touch device",
        "Enable this if you use an external device other than a mouse to control Principia, such as a Wacom pad.",
        "emulate_touch",
        setting_row_type::create_checkbox()
    },
};

struct table_setting_row settings_interface_rows[] = {
    {
        "UI scale",
        "A restart is required for this change to take effect",
        "uiscale",
        setting_row_type::create_hscale(0.25, 2.0, 0.05),
    },{
        "Fullscreen mode",
        "Toggle fullscreen mode",
        "window_fullscreen",
        setting_row_type::create_checkbox()
    }, {
        "Display object ID",
        "Display ID of object on selection (bottom-left corner).",
        "display_object_id",
        setting_row_type::create_checkbox()
    }, {
        "Display grapher value",
        "Display the value that passes through the grapher in play-mode (sandbox only).",
        "display_grapher_value",
        setting_row_type::create_checkbox()
    }, {
        "Display wireless frequency",
        "Display the frequency of the Receiver or the Mini transmitter when paused and zoomed in (sandbox only).",
        "display_wireless_frequency",
        setting_row_type::create_checkbox()
    }, {
        "Hide tips",
        "Do not show when I create a new level.",
        "hide_tips",
        setting_row_type::create_checkbox()
    }, {
        "Do not confirm quitting sandbox adventure",
        "Do not show the \"Are you sure you want to quit?\" dialog when exiting a sandbox adventure level.",
        "dna_sandbox_back",
        setting_row_type::create_checkbox()
    }, {
        "Automatically submit highscores",
        0,
        "score_automatically_submit",
        setting_row_type::create_checkbox()
    }, {
        "Ask before submitting highscore",
        "Always ask before submitting highscore.",
        "score_ask_before_submitting",
        setting_row_type::create_checkbox()
    }, {
        "Resizable window",
        "Allow the window to be resized. NOTE: Principia does not support resizing while in-game. Things will break.",
        "window_resizable",
        setting_row_type::create_checkbox()
    }, {
        "Autosave screen size",
        "Save the screen size when resizing the window.",
        "autosave_screensize",
        setting_row_type::create_checkbox()
    },
};

static const int settings_num_graphic_rows = sizeof(settings_graphic_rows) / sizeof(settings_graphic_rows[0]);
static const int settings_num_audio_rows = sizeof(settings_audio_rows) / sizeof(settings_audio_rows[0]);
static const int settings_num_control_rows = sizeof(settings_control_rows) / sizeof(settings_control_rows[0]);
static const int settings_num_interface_rows = sizeof(settings_interface_rows) / sizeof(settings_interface_rows[0]);

struct gtk_level_property {
    uint64_t flag;
    const char *label;
    const char *help;
    GtkCheckButton *checkbutton;
};

struct gtk_level_property gtk_level_properties[] = {
    { LVL_DISABLE_LAYER_SWITCH,
      "Disable layer switch",
      "If adventure mode, disable manual layer switching of the robots.\nIf puzzle mode, disable layer switching of objects." },
    { LVL_DISABLE_INTERACTIVE,
      "Disable interactive",
      "Disable the ability to handle interactive objects." },
    { LVL_DISABLE_FALL_DAMAGE,
      "Disable fall damage",
      "Disable the damage robots take when they fall." },
    { LVL_DISABLE_CONNECTIONS,
      "Disable connections",
      "Puzzle mode only, disable the ability to create connections." },
    { LVL_DISABLE_STATIC_CONNS,
      "Disable static connections",
      "Puzzle mode only, disable connections to static objects such as platforms." },
    { LVL_DISABLE_JUMP,
      "Disable jumping",
      "Adventure mode only, disable the robots ability to manually jump." },
    { LVL_DISABLE_ROBOT_HIT_SCORE,
      "Disable robot hit score",
      "Disable score increase by shooting other robots." },
    { LVL_DISABLE_ZOOM,
      "Disable zoom",
      "Disable the players ability to zoom." },
    { LVL_DISABLE_CAM_MOVEMENT,
      "Disable cam movement",
      "Disable the players ability to manually move the camera." },
    { LVL_DISABLE_INITIAL_WAIT,
      "Disable initial wait",
      "Disable the waiting state when a level is started." },
    { LVL_UNLIMITED_ENEMY_VISION,
      "Unlimited enemy vision",
      "If enabled, enemy robots will see the player from any distance and through any obstacles, and always try to find a path to the player." },
    { LVL_ENABLE_INTERACTIVE_DESTRUCTION,
      "Interactive destruction",
      "If enabled, interactive objects can be destroyed by shooting them a few times or blowing them up." },
    { LVL_ABSORB_DEAD_ENEMIES,
      "Absorb dead enemies",
      "If enabled, dead enemies will disappear from the game after a short interval after they die." },
    { LVL_SNAP,
      "Snap by default",
      "For puzzle levels, when the player drags or rotates an object it will snap to a grid by default (good for easy beginner levels)." },
    { LVL_NAIL_CONNS,
      "Hide beam connections",
      "Use less visible nail-shaped connections for planks and beams. Existing connections will not be changed if this flag is changed." },
    { LVL_DISABLE_CONTINUE_BUTTON,
      "Disable continue button",
      "If initial wait is disabled, this option disables the Continue button in the lower right corner. Use pkgwarp to go to the next level instead." },
    { LVL_SINGLE_LAYER_EXPLOSIONS,
      "Single-layer explosions",
      "Enable this flag to prevent explosions from reaching objects in other layers." },
    { LVL_DISABLE_DAMAGE,
      "Disable damage",
      "Disable damage to any robot." },
    { LVL_DISABLE_3RD_LAYER,
      "Disable third layer",
      "If enabled and puzzle mode, disable moving objects to the third layer." },
    { LVL_PORTRAIT_MODE,
      "Portrait mode",
      "If enabled, the view will be set to portrait mode (vertical) during play." },
    { LVL_DISABLE_RC_CAMERA_SNAP,
      "Disable RC camera snap",
      "If enabled, the camera won't move to any selected RC." },
    { LVL_DISABLE_PHYSICS,
      "Disable physics",
      "If enabled, physics simulation in the level will be disabled." },
    { LVL_DO_NOT_REQUIRE_DRAGFIELD,
      "Do not require dragfield",
      "If enabled, dragfields are not required to move interactive objects." },
    { LVL_DISABLE_ROBOT_SPECIAL_ACTION,
      "Disable robot special action",
      "If enabled, the adventure robot cannot perform its special action." },
    { LVL_DISABLE_ADVENTURE_MAX_ZOOM,
      "Disable adventure max zoom",
      "If enabled, the zoom is no longer limited when following the adventure robot." },
    { LVL_DISABLE_ROAM_LAYER_SWITCH,
      "Disable roam layer switch",
      "Disable the roaming robots ability to change layer." },
    { LVL_CHUNKED_LEVEL_LOADING,
      "Chunked level loading",
      "Splits up the level into chunks, leading to better performance for large levels." },
    { LVL_DISABLE_CAVEVIEW,
      "Disable adventure caveview",
      "Disable the caveview which appears when the adventure robot is in layer two, with terrain in front of him in layer three." },
    { LVL_DISABLE_ROCKET_TRIGGER_EXPLOSIVES,
      "Disable rocket triggering explosives",
      "Disable the rocket from triggering any explosives when contact with its flames occurs." },
    { LVL_STORE_SCORE_ON_GAME_OVER,
      "Store high score on game over",
      "Allow players to submit a high score even if they did not win the level." },
    { LVL_ALLOW_HIGH_SCORE_SUBMISSIONS,
      "Allow high score submissions",
      "Allow players to submit their high scores to be displayed on your levels community page." },
    { LVL_LOWER_SCORE_IS_BETTER,
      "Lower score is better",
      "A lower score is considered better than a higher score." },
    { LVL_AUTOMATICALLY_SUBMIT_SCORE,
      "Automatically submit score on finish",
      "Automatically submit score for the user when the level finishes." },
    { LVL_DISABLE_ENDSCREENS,
      "Disable end-screens",
      "Disable any end-game sound or messages. Works well when Pause on WIN is disabled. Note that this also disabled the score submission button.\nTo submit highscore without the button you can use the luascript function game:submit_score()." },
    { LVL_ALLOW_QUICKSAVING,
      "Allow quicksaving",
      "If enabled, the player can save his progress at any time." },
    { LVL_ALLOW_RESPAWN_WITHOUT_CHECKPOINT,
      "Allow respawn without checkpoint",
      "If disabled, robots cannot respawn if they are not connected to any checkpoint." },
    { LVL_DEAD_CREATURE_DESTRUCTION,
      "Allow dead creature destruction",
      "If enabled, creature corpses can be destroyed by shooting them." },
};

static int num_gtk_level_properties = sizeof(gtk_level_properties) / sizeof(gtk_level_properties[0]);

/** --Publish **/
GtkDialog      *publish_dialog;
GtkEntry       *publish_name;
GtkTextView    *publish_descr;
GtkCheckButton *publish_locked;

/** --New level **/
GtkDialog      *new_level_dialog;

/** --Mode **/
GtkDialog      *mode_dialog;

/** --Command pad **/
GtkDialog       *command_pad_dialog;
GtkComboBoxText *command_pad_cb;

/** --Key Listener **/
GtkDialog       *key_listener_dialog;
GtkListStore    *key_listener_ls;
GtkComboBox     *key_listener_cb;

/** --Item **/
GtkDialog       *item_dialog;
GtkComboBoxText *item_cb;

/** --Decoration **/
GtkDialog       *decoration_dialog;
GtkComboBoxText *decoration_cb;

/** --Resource **/
GtkDialog       *resource_dialog;
GtkComboBoxText *resource_cb;

/** --Vendor **/
GtkDialog       *vendor_dialog;
GtkSpinButton   *vendor_amount;

/** --Animal **/
GtkDialog       *animal_dialog;
GtkComboBoxText *animal_cb;

/** --Soundman **/
GtkDialog       *soundman_dialog;
GtkComboBoxText *soundman_cb;
GtkCheckButton  *soundman_catch_all;

/** --Factory **/
GtkDialog       *factory_dialog;
GtkSpinButton   *factory_faction;
GtkSpinButton   *factory_oil;
GtkSpinButton   *factory_resources[NUM_RESOURCES];
GtkListStore    *factory_liststore;
GtkTreeView     *factory_treeview;
GtkButton       *factory_cancel;
enum
{
  FACTORY_COLUMN_ENABLED,
  FACTORY_COLUMN_INDEX,
  FACTORY_COLUMN_RECIPE,
  FACTORY_COLUMN_RECIPE_ID,
};

/** --Treasure chest **/
GtkDialog       *tchest_dialog;
GtkScale       *tchest_auto_absorb;
GtkListStore    *tchest_liststore;
GtkTreeView     *tchest_treeview;
GtkComboBoxText *tchest_entity;
GtkComboBoxText *tchest_sub_entity;
GtkSpinButton   *tchest_count;
GtkButton       *tchest_add_entity;
GtkButton       *tchest_remove_selected;
GtkButton       *tchest_cancel;

uint32_t tchest_translations[MAX_OF_ID] = {0, };

enum
{
  TCHEST_COLUMN_G_ID,
  TCHEST_COLUMN_SUB_ID,
  TCHEST_COLUMN_NAME,
  TCHEST_COLUMN_COUNT,
};

/** --Faction **/
GtkDialog       *faction_dialog;
GtkComboBoxText *faction_cb;

/** --Sticky **/
GtkDialog       *sticky_dialog;
GtkTextView     *sticky_text;
GtkSpinButton   *sticky_font_size;
GtkCheckButton  *sticky_center_x;
GtkCheckButton  *sticky_center_y;

/** --Digital Display **/
GtkDialog       *digi_dialog;
GtkCheckButton  *digi_wrap;
GtkToggleButton *digi_check[7][5];
GtkSpinButton   *digi_initial;

GtkLabel   *digi_label;

GtkButton   *digi_prev;
GtkButton   *digi_next;
GtkButton   *digi_insert;
GtkButton   *digi_append;
GtkButton   *digi_delete;

/** --FX Emitter **/
GtkDialog       *fxemitter_dialog;
GtkComboBoxText *fxemitter_cb[4];
GtkScale       *fxemitter_radius;
GtkScale       *fxemitter_count;
GtkScale       *fxemitter_interval;

/** --SFX Emitter **/
GtkDialog       *sfx_dialog;
GtkComboBoxText *sfx_cb;
GtkCheckButton  *sfx_global;

/** --SFX Emitter 2 **/
GtkDialog       *sfx2_dialog;
GtkComboBoxText *sfx2_cb;
GtkComboBoxText *sfx2_sub_cb;
GtkCheckButton  *sfx2_global;
GtkCheckButton  *sfx2_loop;

/** --Event listener  **/
GtkDialog       *elistener_dialog;
GtkComboBoxText *elistener_cb;

/** --Cam targeter **/
GtkDialog       *camtargeter_dialog;
GtkComboBoxText *camtargeter_mode;
GtkComboBoxText *camtargeter_offset_mode;
GtkRange        *camtargeter_x_offset;
GtkEntry        *camtargeter_x_offset_entry;
GtkRange        *camtargeter_y_offset;
GtkEntry        *camtargeter_y_offset_entry;
GtkButton       *camtargeter_save;
GtkButton       *camtargeter_cancel;

/** --Quickadd **/
GtkWindow *quickadd_window;
GtkEntry  *quickadd_entry;

/** --Color Chooser (for Plastic beam & Pixel) **/
GtkColorSelectionDialog *beam_color_dialog;

/** --Info Dialog **/
GtkWindow       *info_dialog;
GtkLabel        *info_name;
GtkLabel        *info_text;
char            *_pass_info_descr;
char            *_pass_info_name;

/** --Error Dialog **/
GtkDialog       *error_dialog;
GtkLabel        *error_text;
char            *_pass_error_text;

/** --Emitter **/
GtkDialog       *emitter_dialog;
GtkScale       *emitter_auto_absorb;

/** --Confirm Dialog **/
GtkDialog       *confirm_dialog;
GtkLabel        *confirm_text;
GtkButton       *confirm_button1;
GtkButton       *confirm_button2;
GtkButton       *confirm_button3;
GtkCheckButton  *confirm_dna_sandbox_back;
struct confirm_data confirm_data(CONFIRM_TYPE_DEFAULT);
char            *_pass_confirm_text;
char            *_pass_confirm_button1;
char            *_pass_confirm_button2;
char            *_pass_confirm_button3;
int              confirm_action1;
int              confirm_action2;
int              confirm_action3;
void            *confirm_action1_data = 0;
void            *confirm_action2_data = 0;
void            *confirm_action3_data = 0;

/** --Alert Dialog **/
GtkMessageDialog    *alert_dialog;
char                *_alert_text = 0;
uint8_t              _alert_type;

/** --Tips Dialog **/
GtkDialog       *tips_dialog;
GtkLabel        *tips_text;
GtkCheckButton  *tips_hide;

/** --Autosave Dialog **/
GtkDialog       *autosave_dialog;

/** --Frequency Dialog **/
GtkWindow       *frequency_window;
GtkSpinButton   *frequency_value;
GtkTreeModel    *frequency_treemodel;
GtkButton       *frequency_ok;
GtkButton       *frequency_cancel;

/** --Frequency range dialog **/
GtkWindow       *freq_range_window;
GtkSpinButton   *freq_range_value;
GtkSpinButton   *freq_range_offset;
GtkTreeModel    *freq_range_treemodel;
GtkButton       *freq_range_ok;
GtkButton       *freq_range_cancel;
GtkLabel        *freq_range_info;

/** --Login **/
GtkWindow       *login_window;
GtkEntry        *login_username;
GtkEntry        *login_password;
GtkLabel        *login_status;
GtkButton       *login_btn_log_in;
GtkButton       *login_btn_cancel;
GtkButton       *login_btn_register;

/** --Settings **/
GtkDialog       *settings_dialog;

/* Graphics */
GtkCheckButton  *settings_enable_shadows;
GtkSpinButton   *settings_shadow_quality;
GtkComboBoxText *settings_shadow_res;
//GtkSpinButton   *settings_ao_quality;
GtkCheckButton  *settings_enable_ao;
GtkComboBoxText *settings_ao_res;
GtkCheckButton  *settings_enable_bloom;

/* Controls */
GtkComboBoxText *settings_control_type;

/** --Confirm Quit Dialog **/
GtkDialog       *confirm_quit_dialog;
GtkButton       *confirm_btn_quit;

/** --Level upgrade Dialog **/
GtkDialog       *confirm_upgrade_dialog;

/** --Package level chooser **/
GtkDialog       *pkg_lvl_chooser;
GtkSpinButton   *pkg_lvl_chooser_lvl_id;

/** --Variable chooser **/
GtkWindow       *variable_dialog;
GtkEntry        *variable_name;
GtkButton       *variable_reset_this;
GtkButton       *variable_reset_all;
GtkButton       *variable_ok;
GtkButton       *variable_cancel;

/** --Robot **/
GtkWindow       *robot_window;
GtkButton       *robot_btn_ok;
GtkButton       *robot_btn_cancel;
GtkRadioButton  *robot_state_idle;
GtkRadioButton  *robot_state_walk;
GtkRadioButton  *robot_state_dead;
GtkCheckButton  *robot_roam;
GtkRadioButton  *robot_dir_left;
GtkRadioButton  *robot_dir_random;
GtkRadioButton  *robot_dir_right;
GtkRadioButton  *robot_faction[NUM_FACTIONS];
GtkListStore    *robot_ls_equipment;
GtkTreeView     *robot_tv_equipment;
enum
{
  ROBOT_COLUMN_EQUIPPED,
  ROBOT_COLUMN_ITEM,
  ROBOT_COLUMN_ITEM_ID,
};
GtkComboBoxText *robot_head;
GtkComboBoxText *robot_feet;
GtkComboBoxText *robot_bolts;
GtkComboBoxText *robot_back_equipment;
GtkComboBoxText *robot_front_equipment;
GtkComboBoxText *robot_head_equipment;

/** --Puzzle play **/
GtkDialog       *puzzle_play_dialog;

/** --Timer **/
GtkDialog       *timer_dialog;
GtkLabel        *timer_time;
GtkSpinButton   *timer_seconds;
GtkSpinButton   *timer_milliseconds;
GtkSpinButton   *timer_num_ticks;
GtkCheckButton  *timer_use_system_time;

/** --Rubber **/
GtkDialog       *rubber_dialog;
GtkScale       *rubber_restitution;
GtkScale       *rubber_friction;

/** --Published **/
GtkDialog       *published_dialog;

/** --Community **/
GtkDialog       *community_dialog;

/** --Sequencer **/
GtkWindow       *sequencer_window;
GtkLabel        *sequencer_state;
GtkEntry        *sequencer_sequence;
GtkSpinButton   *sequencer_seconds;
GtkSpinButton   *sequencer_milliseconds;
GtkCheckButton  *sequencer_wrap_around;
GtkButton       *sequencer_save;
GtkButton       *sequencer_cancel;
int              sequencer_num_steps;

/** --Jumper **/
GtkDialog       *jumper_dialog;
GtkRange        *jumper_value;
GtkEntry        *jumper_value_entry;
GtkButton       *jumper_save;
GtkButton       *jumper_cancel;

/** --Shape extruder **/
GtkDialog       *shapeextruder_dialog;
GtkRange        *shapeextruder_right;
GtkRange        *shapeextruder_up;
GtkRange        *shapeextruder_left;
GtkRange        *shapeextruder_down;

/** --Polygon **/
GtkDialog       *polygon_dialog;
GtkRange        *polygon_sublayer_depth;
GtkCheckButton  *polygon_front_align;

/** --cursorfield **/
GtkDialog       *cursorfield_dialog;
GtkRange        *cursorfield_right;
GtkRange        *cursorfield_up;
GtkRange        *cursorfield_left;
GtkRange        *cursorfield_down;

/** --escript **/
GtkWindow       *escript_window;
GtkWidget       *escript_code;
GtkTextBuffer   *escript_buffer;
GtkCheckButton  *escript_use_external_editor;
GtkBox          *escript_external_box;
GtkLabel        *escript_external_file_path;
GtkStatusbar    *escript_statusbar;
GtkButton       *escript_save;
GtkButton       *escript_cancel;
GtkTextTag      *escript_tt_function;

/** --Synthesizer **/
GtkDialog       *synth_dialog;
GtkSpinButton   *synth_hz_low;
GtkSpinButton   *synth_hz_high;

GtkRange       *synth_bitcrushing;

GtkRange       *synth_freq_vibrato_hz;
GtkRange       *synth_freq_vibrato_extent;

GtkRange       *synth_vol_vibrato_hz;
GtkRange       *synth_vol_vibrato_extent;

GtkRange       *synth_pulse_width;

GtkComboBoxText *synth_waveform;

/** --Prompt Settings Dialog **/
GtkWindow       *prompt_settings_dialog;
GtkTextView     *prompt_message;
GtkEntry        *prompt_b1;
GtkEntry        *prompt_b2;
GtkEntry        *prompt_b3;
GtkButton       *prompt_save;
GtkButton       *prompt_cancel;

gboolean on_digi_next_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data);
gboolean on_digi_prev_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data);
gboolean on_digi_insert_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data);
gboolean on_digi_append_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data);
gboolean on_digi_delete_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data);

static gboolean
on_window_close(GtkWidget *w, void *unused)
{
    P.focused = true;
    gtk_widget_hide(w);
    return true;
}

/* Generate help widget with a tooltip */
static GtkWidget* help_widget(const char *text) {
    //help-about
    //help-browser-symbolic
    //dialog-information-symbolic
    GtkWidget *r = gtk_image_new_from_icon_name("help-about", GTK_ICON_SIZE_MENU); //16px
    gtk_widget_set_tooltip_text(r, text);

    return r;
}

static GtkCellRenderer*
add_text_column(GtkTreeView *tv, const char *title, int id)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
    column = GTK_TREE_VIEW_COLUMN(gtk_tree_view_column_new_with_attributes(title, renderer, "text", id, NULL));

    gtk_tree_view_column_set_sort_column_id(column, id);
    gtk_tree_view_append_column(tv, column);

    return renderer;
}

static GtkWidget*
new_lbl(const char *text)
{
    GtkWidget *r = gtk_label_new(0);
    gtk_label_set_markup(GTK_LABEL(r), text);

    return r;
}

static GtkComboBoxText*
new_item_cb()
{
    GtkListStore *store;
    GtkComboBoxText *cb;


    store = gtk_list_store_new(1, G_TYPE_STRING);

    cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
    g_object_unref(store);

    return cb;
}

static void
item_cb_append(GtkComboBoxText *cb, uint32_t item_id, bool first_is_none)
{
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(cb));
    int num = gtk_tree_model_iter_n_children(model, 0);

    if (first_is_none && num == 0) {
        gtk_combo_box_text_append_text(cb, "None");
    } else {
        gtk_combo_box_text_append_text(cb, item::get_ui_name(item_id));
    }
}

static void
clear_cb(GtkComboBoxText *cb)
{
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(cb));
    gtk_list_store_clear(GTK_LIST_STORE(model));
}

static GtkCheckButton*
new_check_button(const char *lbl)
{
    GtkCheckButton *ret = GTK_CHECK_BUTTON(gtk_check_button_new_with_label(lbl));

    return ret;
}

static GtkButton*
new_lbtn(const char *text, gboolean (*on_click)(GtkWidget*, GdkEventButton*, gpointer))
{
    GtkButton *btn = GTK_BUTTON(gtk_button_new_with_label(text));
    g_signal_connect(btn, "clicked",
            G_CALLBACK(on_click), 0);

    return btn;
}

static GtkWidget*
new_clbl(const char *text)
{
    GtkWidget *r = gtk_label_new(0);
    gtk_label_set_markup(GTK_LABEL(r), text);
    gtk_label_set_xalign(GTK_LABEL(r), 0.0f);
    gtk_label_set_yalign(GTK_LABEL(r), 0.5f);
    return r;
}

static GtkWidget*
new_rlbl(const char *text)
{
    GtkWidget *r = gtk_label_new(0);
    gtk_label_set_markup(GTK_LABEL(r), text);
    gtk_label_set_xalign(GTK_LABEL(r), 1.0f);
    gtk_label_set_yalign(GTK_LABEL(r), 0.5f);
    return r;
}

static void
notebook_append(GtkNotebook *nb, const char *title, GtkBox *base)
{
    gtk_notebook_append_page(nb, GTK_WIDGET(base), new_lbl(title));
}

static void apply_dialog_defaults(
    void *w,
    GtkCallback on_show=0,
    gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)=0
) {
    gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);
    //gtk_window_set_keep_above(GTK_WINDOW(w), TRUE);
    g_signal_connect(w, "delete-event", G_CALLBACK(on_window_close), 0);

    if (on_show) {
        g_signal_connect(w, "show", G_CALLBACK(on_show), 0);
    }
    if (on_keypress) {
        g_signal_connect(w, "key-press-event", G_CALLBACK(on_keypress), 0);
    }
}

static GtkGrid* create_settings_table() {
    GtkGrid *tbl = GTK_GRID(gtk_grid_new());

    gtk_grid_set_column_spacing(tbl, 15);
    gtk_grid_set_row_spacing(tbl, 6);

    gtk_grid_set_column_homogeneous(tbl, false);
    gtk_grid_set_row_homogeneous(tbl, false);

    g_object_set (
        G_OBJECT(tbl),
        "margin", 10,
        NULL
    );

    return tbl;
}

static void add_setting_row(GtkGrid *tbl, int y, const char *label, GtkWidget *widget, const char *help_text = NULL) {
    //label
    gtk_grid_attach(
        tbl, new_rlbl(label),
        0, y,
        1, 1
    );


    //widget
    gtk_widget_set_hexpand(widget, true);
    gtk_grid_attach(
        tbl, widget,
        1, y,
        1, 1
    );

    //help
    if (help_text) {
        gtk_grid_attach(
            tbl, help_widget(help_text),
            2, y,
            1, 1
        );
    }
}

static GtkMenuItem*
add_menuitem_m(GtkMenu *menu, const char *label, void (*on_activate)(GtkMenuItem*, gpointer userdata)=0, gpointer userdata=0)
{
    GtkMenuItem *i = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(label));

    if (on_activate) {
        g_signal_connect(i, "activate", G_CALLBACK(on_activate), userdata);
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(i));

    return i;
}

static GtkMenuItem*
add_menuitem(GtkMenu *menu, const char *label, void (*on_activate)(GtkMenuItem*, gpointer userdata)=0, gpointer userdata=0)
{
    GtkMenuItem *i = GTK_MENU_ITEM(gtk_menu_item_new_with_label(label));

    if (on_activate) {
        g_signal_connect(i, "activate", G_CALLBACK(on_activate), userdata);
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(i));

    return i;
}

static GtkMenuItem*
add_separator(GtkMenu *menu)
{
    GtkMenuItem *i = GTK_MENU_ITEM(gtk_separator_menu_item_new());

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(i));

    return i;
}

static GtkDialog* new_dialog_defaults(const char *title, GtkCallback on_show=0, gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)=0);

static GtkDialog*
new_dialog_defaults(const char *title, GtkCallback on_show/*=0*/, gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)/*=0*/)
{
    GtkWidget *r = gtk_dialog_new_with_buttons(
            title,
            0, (GtkDialogFlags)(0),
            "_OK", GTK_RESPONSE_ACCEPT,
            "_Cancel", GTK_RESPONSE_REJECT,
            NULL);

    apply_dialog_defaults(r, on_show, on_keypress);

    return GTK_DIALOG(r);
}

static GtkWindow* new_window_defaults(const char *title, GtkCallback on_show=0, gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)=0);

static GtkWindow*
new_window_defaults(const char *title, GtkCallback on_show/*=0*/, gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)/*=0*/)
{
    GtkWidget *r = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(r), 10);
    gtk_window_set_title(GTK_WINDOW(r), title);
    gtk_window_set_resizable(GTK_WINDOW(r), false);
    // gtk_window_set_policy(GTK_WINDOW(r),
    //         FALSE,
    //         FALSE, FALSE);

    apply_dialog_defaults(r, on_show, on_keypress);

    return GTK_WINDOW(r);
}

static inline void
update_all_spin_buttons(GtkWidget *wdg, gpointer unused)
{
    if (GTK_IS_SPIN_BUTTON(wdg)) {
        gtk_spin_button_update(GTK_SPIN_BUTTON(wdg));
    } else if (GTK_IS_CONTAINER(wdg)) {
        gtk_container_forall(GTK_CONTAINER(wdg), update_all_spin_buttons, NULL);
    }
}

struct cb_find_data {
    int index;
    const char *str;
};

static gchar*
format_joint_strength(GtkScale *scale, gdouble value)
{
    if (value >= 1.0) {
        return g_strdup("Indestructible");
    } else {
        return g_strdup_printf("%0.*f", gtk_scale_get_digits(scale), value);
    }
}

static gchar*
format_auto_absorb(GtkScale *scale, gdouble value)
{
    if (value <= 1.0) {
        return g_strdup("Don't absorb");
    } else {
        return g_strdup_printf("%0.*f seconds", gtk_scale_get_digits(scale), value);
    }
}

gboolean
foreach_model_find_str(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, struct cb_find_data** user_data)
{
    GValue val = {0, };
    gtk_tree_model_get_value(model, iter, 0, &val);

    if (strcmp(g_value_get_string(&val), (*user_data)->str) == 0) {
        gint *index = gtk_tree_path_get_indices(path);

        (*user_data)->index = index[0];
        g_value_unset(&val);
        return true;
    }

    g_value_unset(&val);
    return false;
}

const char*
get_cb_val(GtkComboBoxText *cb)
{
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(cb));
    GtkTreeIter iter;
    gboolean r = false;
    r = gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cb), &iter);
    if (r == false) {
        tms_errorf("unable to get cb value");
        return "";
    }

    const char *ret;
    GValue val = {0, };

    gtk_tree_model_get_value(model, &iter, 0, &val);
    ret = g_value_dup_string(&val);

    g_value_unset(&val);
    return ret;
}

gint
find_cb_val(GtkComboBoxText *cb, const char *str)
{
    gint ret = -1;
    struct cb_find_data *d = (struct cb_find_data*)malloc(sizeof(struct cb_find_data));
    d->index = -1;
    d->str = str;

    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(cb));
    gtk_tree_model_foreach(model, (GtkTreeModelForeachFunc)(foreach_model_find_str), &d);
    ret = d->index;
    free(d);

    return ret;
}

bool
btn_pressed(GtkWidget *ref, GtkButton *btn, gpointer user_data)
{
    return (
        ref == GTK_WIDGET(btn) &&
        (
            ((gtk_widget_get_state_flags(ref) & GTK_STATE_ACTIVE) != 0) ||
            GPOINTER_TO_INT(user_data) == 1
        )
    );
}

void
pk_reload_pkg_list()
{
    GtkTreeIter iter;
    gtk_list_store_clear(GTK_LIST_STORE(pk_pkg_treemodel));

    pkginfo *p = pkgman::get_pkgs(LEVEL_LOCAL);

    while (p) {
        gtk_list_store_append(GTK_LIST_STORE(pk_pkg_treemodel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(pk_pkg_treemodel), &iter,
                0, p->id,
                1, p->name,
                -1
                );
        p = p->next;
    }
}

bool
pk_get_current(pkginfo *out)
{
    GtkTreeSelection *sel;
    GtkTreeIter       iter;
    GValue            val = {0, };
    sel = gtk_tree_view_get_selection(pk_pkg_treeview);
    if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
        gtk_tree_model_get_value(pk_pkg_treemodel,
                                 &iter,
                                 0,
                                 &val);

        uint32_t pkg_id = g_value_get_uint(&val);

        out->open(LEVEL_LOCAL, pkg_id);

        return true;
    }

    return false;
}

void pk_update_level_list()
{
    /* update the list of levels in the package according to the
     * level treemodel */
    GtkTreeIter iter;

    tms_infof("update level list");

    pkginfo p;
    if (pk_get_current(&p)) {
        p.clear_levels();

        if (gtk_tree_model_get_iter_first(
                GTK_TREE_MODEL(pk_lvl_treemodel),
                &iter)) {
            do {
                GValue val = {0, };
                gtk_tree_model_get_value(pk_lvl_treemodel,
                                         &iter,
                                         0,
                                         &val);
                p.add_level((uint32_t)g_value_get_uint(&val));
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(pk_lvl_treemodel), &iter));
        } else {
        }

        p.save();
    }
}

void pk_lvl_row_inserted(GtkTreeModel *treemodel,
                         GtkTreePath *arg1,
                         GtkTreeIter *arg2,
                         gpointer user_data)
{
    if (!pk_ignore_lvl_changes)
        pk_update_level_list();
}

void pk_lvl_row_deleted(GtkTreeModel *treemodel,
                           GtkTreePath *arg1,
                           gpointer user_data)
{
    if (!pk_ignore_lvl_changes)
        pk_update_level_list();
}

void
pk_lvl_row_activated(GtkTreeView *view,
                     GtkTreePath *path,
                     GtkTreeViewColumn *col,
                     gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter_from_string(model, &iter, gtk_tree_path_to_string(path));

    guint lvl_id;
    gtk_tree_model_get(model, &iter,
                       0, &lvl_id,
                       -1);

    P.add_action(ACTION_OPEN, (uint32_t)lvl_id);
}

void
pk_reload_level_list()
{
    GtkTreeIter iter;
    char tmp[257];

    pk_ignore_lvl_changes = true;

    pkginfo p;
    gtk_list_store_clear(GTK_LIST_STORE(pk_lvl_treemodel));

    if (pk_get_current(&p)) {

        for (int x=0; x<p.num_levels; x++) {
            pkgman::get_level_name(p.type, p.levels[x], 0, tmp);
            gtk_list_store_append(GTK_LIST_STORE(pk_lvl_treemodel), &iter);
            gtk_list_store_set(GTK_LIST_STORE(pk_lvl_treemodel), &iter,
                    0, p.levels[x],
                    1, tmp,
                    -1
                    );
        }

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pk_pkg_first_is_menu), (bool)p.first_is_menu);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pk_pkg_return_on_finish), (bool)p.return_on_finish);
        gtk_spin_button_set_value(pk_pkg_unlock_count, p.unlock_count);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_unlock_count), true);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_return_on_finish), true);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_first_is_menu), true);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_treeview), true);

        if (G->state.sandbox) {
            gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_add), true);
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_add), false);
        }

        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_del), true);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_play), true);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_play), true);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_publish), true);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_unlock_count), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_return_on_finish), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_first_is_menu), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_treeview), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_add), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_play), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_publish), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_del), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_play), false);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pk_pkg_first_is_menu), false);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pk_pkg_return_on_finish), false);
    }

    pk_ignore_lvl_changes = false;
}

void
value_changed_unlock_count(GtkSpinButton *btn, gpointer unused)
{
    pkginfo p;

    if (pk_get_current(&p)) {
        p.unlock_count = (uint8_t)gtk_spin_button_get_value(pk_pkg_unlock_count);
        p.save();
    }
}

void
toggle_first_is_menu(GtkToggleButton *btn, gpointer unused)
{
    pkginfo p;

    if (pk_get_current(&p)) {
        p.first_is_menu = (uint8_t)gtk_toggle_button_get_active(btn);
        p.save();
    }
}

void
toggle_return_on_finish(GtkToggleButton *btn, gpointer unused)
{
    pkginfo p;

    if (pk_get_current(&p)) {
        p.return_on_finish = (uint8_t)gtk_toggle_button_get_active(btn);
        p.save();
    }
}

void
pk_name_edited(GtkCellRendererText *cell, gchar *path, gchar *new_text, gpointer unused)
{
    GtkTreeIter iter;
    GValue val = {0, };

    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(pk_pkg_treemodel),
            &iter, path);

    gtk_tree_model_get_value(pk_pkg_treemodel,
                             &iter,
                             0,
                             &val);

    uint32_t pkg_id = g_value_get_uint(&val);

    pkginfo p;
    if (p.open(LEVEL_LOCAL, pkg_id)) {

        if (strcmp(p.name, new_text) == 0)
            return;

        strncpy(p.name, new_text, 255);
        p.save();
        pk_reload_pkg_list();
        pk_reload_level_list();
        //
        //gtk_cell_renderer_text_set_text(cell, new_text);
        tms_infof("name edited of %u", pkg_id);
    }
}

void
press_add_current_level(GtkButton *w, gpointer unused)
{
    GtkTreeSelection *sel;
    GtkTreeIter       iter;
    GValue            val = {0, };

    if (W->level.local_id == 0) {
        ui::message("Please save the current level before adding it.");
    } else if (!G->state.sandbox) {
        ui::message("You must be in edit mode of the level when adding it.");
    } else {
        sel = gtk_tree_view_get_selection(pk_pkg_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            gtk_tree_model_get_value(pk_pkg_treemodel,
                                     &iter,
                                     0,
                                     &val);

            uint32_t pkg_id = g_value_get_uint(&val);

            pkginfo p;

            p.open(LEVEL_LOCAL, pkg_id);

            if (!p.add_level(W->level.local_id))
                ui::message("Level already added to package.");
            else
                p.save();

            pk_reload_level_list();
        } else
            ui::message("No package selected!");
    }
}

void
press_del_selected(GtkButton *w, gpointer unused)
{
    GtkTreeSelection *sel;
    GtkTreeIter       iter;

    sel = gtk_tree_view_get_selection(pk_lvl_treeview);
    if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
        gtk_list_store_remove(GTK_LIST_STORE(pk_lvl_treemodel), &iter);
    }
}

void
press_play_selected(GtkButton *w, gpointer unused)
{
    GtkTreeSelection *sel;
    GtkTreeIter       iter;
    GValue            val = {0, };

    sel = gtk_tree_view_get_selection(pk_lvl_treeview);
    if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
        gtk_tree_model_get_value(pk_lvl_treemodel, &iter, 0, &val);
        uint32_t lvl_id = g_value_get_uint(&val);
        P.add_action(ACTION_OPEN, lvl_id);
    }
}

#ifdef BUILD_PKGMGR

void
cursor_changed_pk_pkg(GtkTreeView *w, gpointer unused)
{
    pk_reload_level_list();

}

void
press_publish_pkg(GtkButton *w, gpointer unused)
{
    GtkTreeSelection *sel;
    GtkTreePath      *path;
    GtkTreeIter       iter;
    GValue            val = {0, };

    sel = gtk_tree_view_get_selection(pk_pkg_treeview);
    if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
        gtk_tree_model_get_value(pk_pkg_treemodel,
                                 &iter,
                                 0,
                                 &val);

        uint32_t pkg_id = g_value_get_uint(&val);
        P.add_action(ACTION_PUBLISH_PKG, pkg_id);
    } else
        ui::message("Please select a package");
}

void
press_play_pkg(GtkButton *w, gpointer unused)
{
    GtkTreeSelection *sel;
    GtkTreePath      *path;
    GtkTreeIter       iter;
    GValue            val = {0, };

    sel = gtk_tree_view_get_selection(pk_pkg_treeview);
    if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
        gtk_tree_model_get_value(pk_pkg_treemodel,
                                 &iter,
                                 0,
                                 &val);

        uint32_t pkg_id = g_value_get_uint(&val);
        P.add_action(ACTION_PLAY_PKG, pkg_id);
    } else
        ui::message("Please select a package");
}

void
press_create_pkg(GtkButton *w, gpointer unused)
{
    if (gtk_dialog_run(pkg_name_dialog) == GTK_RESPONSE_ACCEPT) {
        pkginfo p;
        p.type = LEVEL_LOCAL;
        p.id = pkgman::get_next_pkg_id();
        const char *nm = gtk_entry_get_text(pkg_name_entry);
        strncpy(p.name, nm, 255);

        if (!p.save())
            ui::message("Could not create package!");
        else
            ui::message("Package created successfully!");

        pk_reload_pkg_list();
    }
    gtk_widget_hide(GTK_WIDGET(pkg_name_dialog));
}

void
activate_pkg_lvl_chooser(GtkEntry *e, gpointer unused)
{
    gtk_widget_show(GTK_WIDGET(pkg_lvl_chooser));
}

#endif

void
editor_mark_activate(GtkMenuItem *i, gpointer mark_pointer)
{
    struct goto_mark *mark = static_cast<struct goto_mark*>(mark_pointer);
    tvec2 prev_pos = tvec2f(G->cam->_position.x, G->cam->_position.y);

    switch (mark->type) {
        case MARK_ENTITY:
            {
                entity *e = W->get_entity_by_id(mark->id);

                if (!e) {
                    return;
                }

                G->cam->_position.x = e->get_position().x;
                G->cam->_position.y = e->get_position().y;
            }
            break;

        case MARK_POSITION:
            G->cam->_position.x = mark->pos.x;
            G->cam->_position.y = mark->pos.y;
            break;

        case MARK_PLAYER:
            if (W->is_adventure() && adventure::player) {
                G->cam->_position.x = adventure::player->get_position().x;
                G->cam->_position.y = adventure::player->get_position().y;
            }
            break;
    }

    editor_menu_last_cam_pos->pos = prev_pos;
}

void
editor_menu_activate(GtkMenuItem *i, gpointer unused)
{
    if (i == editor_menu_lvl_prop) {
        if (gtk_dialog_run(properties_dialog) == GTK_RESPONSE_ACCEPT) {
            const char *name = gtk_entry_get_text(lvl_title);
            int name_len = strlen(name);
            W->level.name_len = name_len;
            memcpy(W->level.name, name, name_len);

            GtkTextIter start, end;
            GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(lvl_descr);

            gtk_text_buffer_get_bounds(text_buffer, &start, &end);

            const char *descr = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
            int descr_len = strlen(descr);

            if (descr_len > 0) {
                W->level.descr_len = descr_len;
                W->level.descr = (char*)realloc(W->level.descr, descr_len);

                memcpy(W->level.descr, descr, descr_len);
            } else {
                W->level.descr_len = 0;
            }

            uint16_t left  = (uint16_t)atoi(gtk_entry_get_text(lvl_width_left));
            uint16_t right = (uint16_t)atoi(gtk_entry_get_text(lvl_width_right));
            uint16_t down  = (uint16_t)atoi(gtk_entry_get_text(lvl_height_down));
            uint16_t up    = (uint16_t)atoi(gtk_entry_get_text(lvl_height_up));

            float w = (float)left + (float)right;
            float h = (float)down + (float)up;

            bool resized = false;

            if (w < 5.f) {
                resized = true;
                left += 6-(uint16_t)w;
            }
            if (h < 5.f) {
                resized = true;
                down += 6-(uint16_t)w;
            }

            if (resized)
                ui::message("Your level size was increased to the minimum allowed.");

            W->level.size_x[0] = left;
            W->level.size_x[1] = right;
            W->level.size_y[0] = down;
            W->level.size_y[1] = up;
            W->level.gravity_x = (float)gtk_spin_button_get_value(lvl_gx);
            W->level.gravity_y = (float)gtk_spin_button_get_value(lvl_gy);

            W->level.dead_enemy_absorb_time = gtk_range_get_value(GTK_RANGE(lvl_enemy_absorb_time));
            W->level.time_before_player_can_respawn = gtk_range_get_value(GTK_RANGE(lvl_player_respawn_time));

            uint8_t vel_iter = (uint8_t)gtk_range_get_value(GTK_RANGE(lvl_vel_iter));
            uint8_t pos_iter = (uint8_t)gtk_range_get_value(GTK_RANGE(lvl_pos_iter));

            float prismatic_tolerance = gtk_range_get_value(GTK_RANGE(lvl_prismatic_tol));
            float pivot_tolerance = gtk_range_get_value(GTK_RANGE(lvl_pivot_tol));

            float angular_damping = gtk_range_get_value(GTK_RANGE(lvl_angular_damping));
            float joint_friction = gtk_range_get_value(GTK_RANGE(lvl_joint_friction));
            float linear_damping = gtk_range_get_value(GTK_RANGE(lvl_linear_damping));

            W->level.angular_damping = angular_damping;
            W->level.joint_friction = joint_friction;
            W->level.linear_damping = linear_damping;

            W->level.prismatic_tolerance = prismatic_tolerance;
            W->level.pivot_tolerance = pivot_tolerance;

            tms_infof("vel_iter: %d,  pos_iter: %d", vel_iter, pos_iter);
            W->level.velocity_iterations = vel_iter;
            W->level.position_iterations = pos_iter;
            W->level.final_score = (uint32_t)atoi(gtk_entry_get_text(lvl_score));

            if (W->level.version >= 7) {
                W->level.show_score = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lvl_show_score));
                W->level.pause_on_finish = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lvl_pause_on_win));
            }

            if (W->level.version >= 9) {
                W->level.show_score = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lvl_show_score));
                W->level.flags = 0;
                for (int x=0; x<num_gtk_level_properties; ++x) {
                    W->level.flags |= ((int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_level_properties[x].checkbutton)) * gtk_level_properties[x].flag);
                }
            }

            W->level.bg = gtk_combo_box_get_active(GTK_COMBO_BOX(lvl_bg));
            W->level.bg_color = new_bg_color;

            if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lvl_radio_adventure))) {
                P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_ADVENTURE);
            } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lvl_radio_custom))) {
                P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_CUSTOM);
            }

            P.add_action(ACTION_RELOAD_LEVEL, 0);
        }

        gtk_widget_hide(GTK_WIDGET(properties_dialog));

#ifdef BUILD_PKGMGR
    } else if (i == editor_menu_package_manager) {
        gtk_widget_show(GTK_WIDGET(package_window));
#endif

    } else if (i == editor_menu_move_here_player) {

        if (adventure::player) {
            const b2Vec2 _pos = G->get_last_cursor_pos(adventure::player->get_layer());

            b2Vec2 *pos = new b2Vec2();
            pos->Set(_pos.x, _pos.y);

            W->add_action(adventure::player->id, ACTION_MOVE_ENTITY, (void*)pos);
        } else {
            const b2Vec2 _pos = G->get_last_cursor_pos(G->state.edit_layer);

            b2Vec2 *pos = new b2Vec2();
            pos->Set(_pos.x, _pos.y);

            P.add_action(ACTION_CREATE_ADVENTURE_ROBOT, (void*)pos);
        }
    } else if (i == editor_menu_move_here_object) {
        if (G->selection.e) {
            const b2Vec2 _pos = G->get_last_cursor_pos(G->selection.e->get_layer());

            b2Vec2 *pos = new b2Vec2();
            pos->Set(_pos.x, _pos.y);

            W->add_action(G->selection.e->id, ACTION_MOVE_ENTITY, (void*)pos);
        }
    } else if (i == editor_menu_set_as_player) {
        if (W->is_adventure() && G->selection.e && G->selection.e->is_creature()) {
            creature *c = static_cast<creature*>(G->selection.e);

            if (c->is_robot()) {
                robot_base *r = static_cast<robot_base*>(c);
                r->set_faction(FACTION_FRIENDLY);
            }

            W->level.set_adventure_id(c->id);
            G->state.adventure_id = c->id;
            adventure::player = c;
        }
    } else if (i == editor_menu_toggle_mark_entity) {
        if (G->selection.e) {
            for (std::deque<struct goto_mark*>::iterator it = editor_menu_marks.begin();
                    it != editor_menu_marks.end(); ++it) {
                struct goto_mark *mark = *it;

                if (mark != editor_menu_last_created && mark->type == MARK_ENTITY && mark->id == G->selection.e->id) {
                    gtk_container_remove(GTK_CONTAINER(editor_menu_go_to_menu), GTK_WIDGET(mark->menuitem));

                    editor_menu_marks.erase(it);

                    delete mark;

                    return;
                }
            }

            char tmp[128];
            snprintf(tmp, 127, "%s - %d", G->selection.e->get_name(), G->selection.e->id);
            struct goto_mark *mark = new goto_mark(MARK_ENTITY, tmp, G->selection.e->id, tvec2f(0.f, 0.f));
            mark->menuitem = add_menuitem(editor_menu_go_to_menu, mark->label, editor_mark_activate, (gpointer)mark);

            editor_menu_marks.push_back(mark);

            refresh_mark_menuitems();
        }
    }
}

void
activate_mode_dialog(GtkMenuItem *i, gpointer unused)
{
    gint result = gtk_dialog_run(mode_dialog);

    switch (result) {
        case RESPONSE_DRAW:
            G->set_mode(GAME_MODE_DRAW);
            break;
        case RESPONSE_MULTISEL:
            G->set_mode(GAME_MODE_MULTISEL);
            break;
        case RESPONSE_CONN_EDIT:
            G->set_mode(GAME_MODE_CONN_EDIT);
            break;

        default: break;
    }

    gtk_widget_hide(GTK_WIDGET(mode_dialog));
}

void
activate_new_level(GtkMenuItem *i, gpointer unused)
{
    gint result = gtk_dialog_run(new_level_dialog);

    switch (result) {
        case RESPONSE_PUZZLE:
            P.add_action(ACTION_NEW_LEVEL, LCAT_PUZZLE);
            break;
        case RESPONSE_EMPTY_ADVENTURE:
            P.add_action(ACTION_NEW_LEVEL, LCAT_ADVENTURE);
            break;
        case RESPONSE_ADVENTURE:
            P.add_action(ACTION_NEW_GENERATED_LEVEL, LCAT_ADVENTURE);
            break;
        case RESPONSE_CUSTOM:
            P.add_action(ACTION_NEW_LEVEL, LCAT_CUSTOM);
            break;

        default: break;
    }

    gtk_widget_hide(GTK_WIDGET(new_level_dialog));
}

void
activate_frequency(GtkMenuItem *i, gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(frequency_window));
}

/** --Confirm Quit Dialog **/
void
on_confirm_quit_show(GtkWidget *wdg, gpointer unused)
{
    gtk_widget_grab_focus(GTK_WIDGET(confirm_btn_quit));
}

/** --Command pad **/
void
on_command_pad_show(GtkWidget *wdg, void *ununused)
{
    char tmp[64];
    entity *e = G->selection.e;

    if (e && e->g_id == O_COMMAND_PAD) {
        command *pad = static_cast<command*>(e);
        int cmd = pad->get_command();
        switch (cmd) {
            case COMMAND_STOP:      strcpy(tmp, "Stop"); break;
            case COMMAND_STARTSTOP: strcpy(tmp, "Start/Stop toggle"); break;
            case COMMAND_LEFT:      strcpy(tmp, "Left"); break;
            case COMMAND_RIGHT:     strcpy(tmp, "Right"); break;
            case COMMAND_LEFTRIGHT: strcpy(tmp, "Left/Right toggle"); break;
            case COMMAND_JUMP:      strcpy(tmp, "Jump"); break;
            case COMMAND_AIM:       strcpy(tmp, "Aim"); break;
            case COMMAND_ATTACK:    strcpy(tmp, "Attack"); break;
            case COMMAND_LAYERUP:   strcpy(tmp, "Layer up"); break;
            case COMMAND_LAYERDOWN: strcpy(tmp, "Layer down"); break;
            case COMMAND_INCRSPEED: strcpy(tmp, "Increase speed"); break;
            case COMMAND_DECRSPEED: strcpy(tmp, "Decrease speed"); break;
            case COMMAND_SETSPEED:  strcpy(tmp, "Set speed"); break;
            case COMMAND_HEALTH:    strcpy(tmp, "Full health"); break;
            default:                strcpy(tmp, "Stop"); break;
        }

        gint index = find_cb_val(command_pad_cb, tmp);
        if (index != -1) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(command_pad_cb), index);
        } else {
            tms_infof("unknown index");
        }
    }
}

/** --Frequency range Dialog **/
void
on_freq_range_show(GtkWidget *wdg, void *unused)
{
    GtkTreeIter iter;
    std::map<uint32_t, entity*> all_entities;
    // <Frequency, <Num Receivers, Num Transmitters> >
    std::map<int, std::pair<int, int> > frequencies;

    /* Reset widgets */
    gtk_spin_button_set_value(freq_range_value, 1);
    gtk_spin_button_set_value(freq_range_offset, 10);
    gtk_list_store_clear(GTK_LIST_STORE(freq_range_treemodel));

    /* Fetch current freq_range from selection */
    if (G->selection.e && G->selection.e->g_id == O_BROADCASTER) {
        gtk_spin_button_set_value(freq_range_value, (gdouble)G->selection.e->properties[0].v.i);
        gtk_spin_button_set_value(freq_range_offset, (gdouble)G->selection.e->properties[1].v.i);
    }

    all_entities = W->get_all_entities();
    for (std::map<uint32_t, entity*>::iterator i = all_entities.begin();
            i != all_entities.end(); i++) {
        entity *e = i->second;

        if (e->g_id == O_RECEIVER) {
            std::pair<freq_container::iterator, bool> ret;
            std::pair<int, int> data = std::make_pair(1, 0);
            ret = frequencies.insert(std::pair<int, std::pair<int, int> >((int)e->properties[0].v.i, data));

            if (!ret.second) {
                ((ret.first)->second).first += 1;
            }
        } else if (e->g_id == O_TRANSMITTER || e->g_id == O_MINI_TRANSMITTER) {
            std::pair<freq_container::iterator, bool> ret;
            std::pair<int, int> data = std::make_pair(0, 1);
            ret = frequencies.insert(std::pair<int, std::pair<int, int> >((int)e->properties[0].v.i, data));

            if (!ret.second) {
                ((ret.first)->second).second += 1;
            }
        } else if (e->g_id == O_PIXEL && e->properties[4].v.i != 0) {
            /* Pixel */
            std::pair<freq_container::iterator, bool> ret;
            std::pair<int, int> data = std::make_pair(1, 0);
            ret = frequencies.insert(std::pair<int, std::pair<int, int> >((int)e->properties[4].v.i, data));

            if (!ret.second) {
                ((ret.first)->second).first += 1;
            }
        }
    }

    for (freq_container::iterator i = frequencies.begin();
            i != frequencies.end(); ++i) {
        gtk_list_store_append(GTK_LIST_STORE(freq_range_treemodel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(freq_range_treemodel), &iter,
                FC_FREQUENCY,       i->first,
                FC_RECEIVERS,       (i->second).first,
                FC_TRANSMITTERS,    (i->second).second,
                -1
                );
    }

}

gboolean
on_freq_range_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, freq_range_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(freq_range_window));
    } else if (btn_pressed(w, freq_range_ok, user_data)) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_BROADCASTER) {
            e->set_property(0, (uint32_t)gtk_spin_button_get_value(freq_range_value));
            e->set_property(1, (uint32_t)gtk_spin_button_get_value(freq_range_offset));

            ui::messagef("Frequency set to %u (+%u)", e->properties[0].v.i, e->properties[1].v.i);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            gtk_widget_hide(GTK_WIDGET(freq_range_window));
        }
    }

    return false;
}

gboolean
on_freq_range_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    switch (key->keyval) {
        case GDK_KEY_Escape:
            gtk_widget_hide(w);
            return false;

        case GDK_KEY_Return:
            if (gtk_widget_has_focus(GTK_WIDGET(freq_range_cancel)))
                on_freq_range_click(GTK_WIDGET(freq_range_cancel), NULL, GINT_TO_POINTER(1));
            else
                on_freq_range_click(GTK_WIDGET(freq_range_ok), NULL, GINT_TO_POINTER(1));

            return true;
            break;
    }

    return false;
}

void
activate_freq_range_row(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter_from_string(model, &iter, gtk_tree_path_to_string(path));

    guint _freq_range;
    gtk_tree_model_get(model, &iter,
                       FC_FREQUENCY, &_freq_range,
                       -1);

    gtk_spin_button_set_value(freq_range_value, _freq_range);
}

void
activate_freq_range(GtkMenuItem *i, gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(freq_range_window));
}

static void
freq_range_value_changed(GtkSpinButton *btn, gpointer unused)
{
    gtk_spin_button_update(freq_range_value);
    gtk_spin_button_update(freq_range_offset);

    uint32_t begin = gtk_spin_button_get_value(freq_range_value);
    uint32_t end = begin + gtk_spin_button_get_value(freq_range_offset);

    char tmp[256];

    snprintf(tmp, 255, "Frequencies: %u - %u", begin, end);
    gtk_label_set_text(freq_range_info, tmp);
}

static void
freq_range_value_text_changed(GtkEditable *editable, gpointer unused)
{
    gtk_spin_button_update(freq_range_value);
    gtk_spin_button_update(freq_range_offset);

    uint32_t begin = gtk_spin_button_get_value(freq_range_value);
    uint32_t end = begin + gtk_spin_button_get_value(freq_range_offset);

    char tmp[256];

    snprintf(tmp, 255, "Frequencies: %u - %u", begin, end);
    gtk_label_set_text(freq_range_info, tmp);
}

/** --digital display **/

uint64_t symbols[DISPLAY_MAX_SYMBOLS];
int num_digi_symbols = 0;
int curr_digi_symbol = 0;

void digi_load_symbols()
{
    display *e = (display*)G->selection.e;

    num_digi_symbols = e->num_symbols;
    curr_digi_symbol = e->properties[1].v.i8;

    memcpy(symbols, e->symbols, DISPLAY_MAX_SYMBOLS*sizeof(uint64_t));
}

void digi_refresh_symbol()
{
    char txt[256];

    if (curr_digi_symbol < 0) curr_digi_symbol = num_digi_symbols-1;
    else if (curr_digi_symbol >= num_digi_symbols) curr_digi_symbol = 0;

    sprintf(txt, "<b>Symbol %d/%d</b>", curr_digi_symbol+1, num_digi_symbols);
    gtk_label_set_markup(GTK_LABEL(digi_label), txt);

    for (int y=0; y<7; y++) {
        for (int x=0; x<5; x++) {
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(digi_check[y][x]),
                (symbols[curr_digi_symbol] & (1ull << ((uint64_t)y*5ull + (uint64_t)x))) ? true : false
            );
        }
    }
}

void on_digi_toggle(GtkToggleButton *togglebutton,
                    gpointer user_data)
{
    uint64_t which = (uint64_t)user_data;

    if (gtk_toggle_button_get_active(togglebutton)) {
        symbols[curr_digi_symbol] |= (1ull << which);
    } else {
        symbols[curr_digi_symbol] &= ~(1ull << which);
    }
}

gboolean
on_digi_next_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    curr_digi_symbol ++;
    digi_refresh_symbol();
    return false;
}

gboolean
on_digi_prev_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    curr_digi_symbol --;
    digi_refresh_symbol();
    return false;
}

gboolean
on_digi_insert_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (num_digi_symbols < DISPLAY_MAX_SYMBOLS) {
        num_digi_symbols ++;

        size_t sz = (num_digi_symbols - curr_digi_symbol - 1)*sizeof(uint64_t);
        if (sz > 0)
            memcpy(&symbols[curr_digi_symbol+1], &symbols[curr_digi_symbol], sz);
        memset(&symbols[curr_digi_symbol], 0, sizeof(uint64_t));
        digi_refresh_symbol();
    }
    return false;
}

gboolean
on_digi_append_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (num_digi_symbols < DISPLAY_MAX_SYMBOLS) {
        num_digi_symbols ++;
        digi_refresh_symbol();
    }
    return false;
}

gboolean
on_digi_delete_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (num_digi_symbols > 1) {
        if (curr_digi_symbol == num_digi_symbols-1) {
            num_digi_symbols --;
        } else {
            size_t sz = (num_digi_symbols - (curr_digi_symbol+1))*sizeof(uint64_t);
            if (sz > 0) {
                memcpy(&symbols[curr_digi_symbol], &symbols[curr_digi_symbol+1], sz);
            }
            num_digi_symbols --;
        }
        digi_refresh_symbol();
    }
    return false;
}

void
on_digi_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && (e->g_id == O_PASSIVE_DISPLAY || e->g_id == O_ACTIVE_DISPLAY)) {

        digi_load_symbols();
        digi_refresh_symbol();

        gtk_spin_button_set_value(digi_initial, e->properties[1].v.i8+1);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(digi_wrap), e->properties[0].v.i8);

        if (e->g_id == O_ACTIVE_DISPLAY) {
            gtk_widget_set_sensitive(GTK_WIDGET(digi_wrap), false);
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(digi_wrap), true);
        }
    }
}

/** --Sticky **/
void
on_sticky_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_STICKY_NOTE) {
        GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(sticky_text);
        gtk_text_buffer_set_text(text_buffer, e->properties[0].v.s.buf, -1);
        gtk_spin_button_set_value(sticky_font_size, e->properties[3].v.i8);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sticky_center_x), (bool) e->properties[1].v.i8);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sticky_center_y), (bool) e->properties[2].v.i8);
    }
}

#define MAX_BUFFER_LENGTH 255

void
sticky_text_changed(GtkTextBuffer *buffer, void *unused)
{
    GtkTextIter start, end;
    char *text;

    gtk_text_buffer_get_bounds(buffer, &start, &end);
    text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    int len = strlen(text);

    if (len > MAX_BUFFER_LENGTH-1) {
        gtk_text_buffer_get_iter_at_offset(buffer,
                &start,
                MAX_BUFFER_LENGTH-1);
        gtk_text_buffer_get_iter_at_offset(buffer,
                &end,
                MAX_BUFFER_LENGTH);

        gtk_text_buffer_delete(buffer, &start, &end);
    }
}

/** --Shape extruder **/
void
on_shapeextruder_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_SHAPE_EXTRUDER) {
        gtk_range_set_value(shapeextruder_right, e->properties[0].v.f);
        gtk_range_set_value(shapeextruder_up, e->properties[1].v.f);
        gtk_range_set_value(shapeextruder_left, e->properties[2].v.f);
        gtk_range_set_value(shapeextruder_down, e->properties[3].v.f);
    }
}

/** --Polygon **/
static void
on_polygon_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_PLASTIC_POLYGON) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(polygon_front_align), e->properties[1].v.i8);
        gtk_range_set_value(GTK_RANGE(polygon_sublayer_depth), e->properties[0].v.i8+1);
    }
}

/** --cursorfield **/
void
on_cursorfield_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_CURSOR_FIELD) {
        gtk_range_set_value(cursorfield_right, e->properties[0].v.f);
        gtk_range_set_value(cursorfield_up, e->properties[1].v.f);
        gtk_range_set_value(cursorfield_left, e->properties[2].v.f);
        gtk_range_set_value(cursorfield_down, e->properties[3].v.f);
    }
}

/** --escript **/
static void
on_escript_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, escript_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(escript_window));
    } else if (btn_pressed(w, escript_save, user_data)) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_ESCRIPT) {
            bool use_external_editor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(escript_use_external_editor));

            e->properties[1].v.i = 0;
            // Always enable string, table and on_input (backwards compat)
            e->properties[1].v.i |= ESCRIPT_INCLUDE_STRING;
            e->properties[1].v.i |= ESCRIPT_INCLUDE_TABLE;
            e->properties[1].v.i |= ESCRIPT_LISTEN_ON_INPUT;
            e->properties[1].v.i |= ((int)use_external_editor * ESCRIPT_USE_EXTERNAL_EDITOR);

            GtkTextIter start, end;
            GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(escript_buffer);
            char *text;
            gtk_text_buffer_get_bounds(text_buffer, &start, &end);
            text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);

            if (use_external_editor) {
                char file_path[1024];
                snprintf(file_path, 1023, "%s/%d-%d.lua",
                        pkgman::get_cache_path(W->level_id_type),
                        W->level.local_id, e->id);

                FILE *fh = fopen(file_path, "w");

                if (fh) {
                    fwrite(text, sizeof(char), strlen(text), fh);
                    tms_infof("Write to %s", file_path);
                    fclose(fh);
                }
            }

            e->set_property(0, text);

            free(text);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_ENTITY_MODIFIED, 0);
            P.add_action(ACTION_AUTOSAVE, 0);
        }

        gtk_widget_hide(GTK_WIDGET(escript_window));
    }
}

static void
on_escript_external_editor_toggled(GtkToggleButton *tb, gpointer userdata)
{
    bool external_editor_active = gtk_toggle_button_get_active(tb);

    gtk_widget_set_sensitive(GTK_WIDGET(escript_code), !external_editor_active);

    if (!external_editor_active) {
        gtk_widget_hide(GTK_WIDGET(escript_external_box));
    } else {
        gtk_widget_show(GTK_WIDGET(escript_external_box));
    }
}

void
on_escript_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_ESCRIPT) {
        GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(escript_buffer);
        GtkTextIter start, end;

        char *code = (char*)malloc(e->properties[0].v.s.len+1);
        memcpy(code, e->properties[0].v.s.buf, e->properties[0].v.s.len);
        code[e->properties[0].v.s.len] = '\0';

        gtk_text_buffer_get_bounds(text_buffer, &start, &end);
        char *old_code = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);

        /* Only replace text buffer if it differs from the previous text */
        if (strcmp(old_code, code) != 0) {
            gtk_text_buffer_set_text(text_buffer, code, -1);
        }

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(escript_use_external_editor), e->properties[1].v.i & ESCRIPT_USE_EXTERNAL_EDITOR);

        bool external_editor_active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(escript_use_external_editor));

        gtk_widget_set_sensitive(GTK_WIDGET(escript_code), !external_editor_active);

        char file_path[ESCRIPT_EXTERNAL_PATH_LEN];
        ((escript*)e)->generate_external_path(file_path);

        char external_path[2048];
        snprintf(external_path, 2047, "External path: <b>%s</b>",
                file_path);

        gtk_label_set_markup(escript_external_file_path, external_path);

        if (!external_editor_active) {
            gtk_widget_hide(GTK_WIDGET(escript_external_box));
        } else {
            gtk_widget_show(GTK_WIDGET(escript_external_box));

        }

        free(old_code);
    }
}

gboolean
on_escript_keypress(GtkWidget *w, GdkEventKey *event, gpointer unused)
{
    if (GDK_KEY_s && event->state & GDK_CONTROL_MASK) {
        on_escript_btn_click(GTK_WIDGET(escript_save), NULL, GINT_TO_POINTER(1));
        return true;
    }

    return false;
}

static void
on_escript_mark_set(GtkTextBuffer *buffer, const GtkTextIter *new_location, GtkTextMark *mark, gpointer data)
{
    gchar *msg;
    gint row, col;
    GtkTextIter iter;

    gtk_text_buffer_get_iter_at_mark(buffer,
            &iter, gtk_text_buffer_get_insert(buffer));

    row = gtk_text_iter_get_line(&iter);
    col = gtk_text_iter_get_line_offset(&iter);

    msg = g_strdup_printf("Col %d Ln %d", col+1, row+1);

    gtk_statusbar_push(escript_statusbar, 0, msg);

    g_free(msg);
}

/** --Jumper **/
gboolean
on_jumper_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    switch (key->keyval) {
        case GDK_KEY_Escape:
            gtk_dialog_response(jumper_dialog, GTK_RESPONSE_CANCEL);
            break;

        case GDK_KEY_Return:
            if (!gtk_widget_has_focus(GTK_WIDGET(jumper_cancel))) {
                gtk_dialog_response(jumper_dialog, GTK_RESPONSE_ACCEPT);
            }
            break;
    }

    return false;
}

void
on_jumper_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_JUMPER) {
        gtk_range_set_value(jumper_value, e->properties[0].v.f);
        char tmp[8];
        sprintf(tmp, "%.5f", e->properties[0].v.f);
        gtk_entry_set_text(jumper_value_entry, tmp);

        gtk_widget_grab_focus(GTK_WIDGET(jumper_value_entry));
    }
}

void
jumper_value_changed(GtkRange *range, void *unused)
{
    if (gtk_widget_has_focus(GTK_WIDGET(range))) {
        char tmp[8];
        sprintf(tmp, "%.5f", gtk_range_get_value(range));
        gtk_entry_set_text(jumper_value_entry, tmp);
    }
}

void
jumper_value_entry_changed(GtkEditable *editable, void *unused)
{
    if (gtk_widget_has_focus(GTK_WIDGET(editable))) {
        float v = atof(gtk_editable_get_chars(editable, 0, -1));
        if (v < 0.f) {
            v = 0.f;
            char tmp[8];
            sprintf(tmp, "%.5f", v);
            gtk_entry_set_text(jumper_value_entry, tmp);
            gtk_editable_set_position(editable, 0);
        } else if (v > 1.f) {
            v = 1.f;
            char tmp[8];
            sprintf(tmp, "%.5f", v);
            gtk_entry_set_text(jumper_value_entry, tmp);
            gtk_editable_set_position(editable, 0);
        }
        gtk_range_set_value(GTK_RANGE(jumper_value), v);
    }
}

void
jumper_value_entry_insert_text(GtkEditable *editable, gchar *new_text,
        gint new_text_length, gpointer position, gpointer *user_data)
{
    for (int n=0; n<new_text_length; ++n) {
        if (!isdigit(new_text[n]) && new_text[n] != '.' && new_text[n] != ',') {
            g_signal_stop_emission_by_name(editable, "insert-text");
            break;
        }
    }
}

/** --Key Listener **/
void
on_key_listener_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_KEY_LISTENER) {
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter_first(
                    GTK_TREE_MODEL(key_listener_ls),
                    &iter)) {
            int x = 0;

            do {
                GValue val = {0, };
                gtk_tree_model_get_value(GTK_TREE_MODEL(key_listener_ls),
                                         &iter,
                                         1,
                                         &val);

                uint32_t key = g_value_get_uint(&val);

                if (key == e->properties[0].v.i) {
                    gtk_combo_box_set_active(GTK_COMBO_BOX(key_listener_cb), x);
                    break;
                }

                ++x;
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(key_listener_ls), &iter));
        }
    }
}

/** --Synthesizer **/
void
on_synth_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_SYNTHESIZER) {
        float low = e->properties[0].v.f;
        float high = e->properties[1].v.f;
        gtk_spin_button_set_value(synth_hz_low, low);
        gtk_spin_button_set_value(synth_hz_high, high);

        gtk_range_set_value(synth_bitcrushing, e->properties[3].v.f);

        gtk_range_set_value(synth_pulse_width, e->properties[8].v.f);
        gtk_range_set_value(synth_vol_vibrato_hz, e->properties[4].v.f);
        gtk_range_set_value(synth_freq_vibrato_hz, e->properties[5].v.f);
        gtk_range_set_value(synth_vol_vibrato_extent, e->properties[6].v.f);
        gtk_range_set_value(synth_freq_vibrato_extent, e->properties[7].v.f);
        gtk_combo_box_set_active(GTK_COMBO_BOX(synth_waveform), e->properties[2].v.i);

        gtk_widget_grab_focus(GTK_WIDGET(synth_hz_low));
    }
}

/** --Rubber **/
void
on_rubber_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && (e->g_id == O_WHEEL || e->g_id == O_RUBBER_BEAM)) {
        gtk_range_set_value(GTK_RANGE(rubber_restitution), e->properties[1].v.f);
        gtk_range_set_value(GTK_RANGE(rubber_friction), e->properties[2].v.f);
    }
}

gboolean
on_rubber_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    switch (key->keyval) {
        case GDK_KEY_Escape:
            gtk_widget_hide(w);
            return false;

        case GDK_KEY_Return:
            {
                entity *e = G->selection.e;

                if (e && (e->g_id == O_WHEEL || e->g_id == O_RUBBER_BEAM)) {
                    float restitution = gtk_range_get_value(GTK_RANGE(rubber_restitution));
                    float friction = gtk_range_get_value(GTK_RANGE(rubber_friction));

                    e->properties[1].v.f = restitution;
                    e->properties[2].v.f = friction;

                    if (e->g_id == O_RUBBER_BEAM) {
                        ((beam*)e)->do_update_fixture = true;
                    } else {
                        ((wheel*)e)->do_update_fixture = true;
                    }

                    P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
                    P.add_action(ACTION_RESELECT, 0);
                }
            }
            break;
    }

    return false;
}

/** --Timer **/
void
on_timer_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_TIMER) {
        float s = floor((float)(e->properties[0].v.i) / 1000.f);
        float ms = (float)(e->properties[0].v.i % 1000);
        gtk_spin_button_set_value(timer_seconds, s);
        gtk_spin_button_set_value(timer_milliseconds, ms);

        gtk_spin_button_set_value(timer_num_ticks, (gdouble)e->properties[1].v.i8);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(timer_use_system_time), e->properties[2].v.i);
    }
}

void
timer_time_changed(GtkSpinButton *btn, gpointer unused)
{
    char tmp[64];
    int seconds = gtk_spin_button_get_value(timer_seconds);
    int milliseconds = gtk_spin_button_get_value(timer_milliseconds);
    uint32_t full_time = (seconds*1000) + milliseconds;

    if (full_time < TIMER_MIN_TIME) {
        milliseconds = TIMER_MIN_TIME;
        gtk_spin_button_set_value(timer_milliseconds, TIMER_MIN_TIME);
        full_time = TIMER_MIN_TIME;
    }

    snprintf(tmp, 63, "%d.%ds", seconds, milliseconds);
    gtk_label_set_text(timer_time, tmp);
}

gboolean
on_timer_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    switch (key->keyval) {
        case GDK_KEY_Escape:
            gtk_dialog_response(timer_dialog, GTK_RESPONSE_CANCEL);
            break;

        case GDK_KEY_Return:
            gtk_dialog_response(timer_dialog, GTK_RESPONSE_ACCEPT);
            break;
    }

    return false;
}

/** --Sequencer **/
void
sequencer_time_changed(GtkSpinButton *btn, gpointer unused)
{
    char tmp[128];
    int seconds = gtk_spin_button_get_value(sequencer_seconds);
    int milliseconds = gtk_spin_button_get_value(sequencer_milliseconds);
    uint32_t full_time = (seconds*1000) + milliseconds;

    if (full_time < SEQUENCER_MIN_TIME) {
        milliseconds = SEQUENCER_MIN_TIME;
        gtk_spin_button_set_value(sequencer_milliseconds, SEQUENCER_MIN_TIME);
        full_time = SEQUENCER_MIN_TIME;
    }

    snprintf(tmp, 127, "%d.%ds. %d steps", seconds, milliseconds, sequencer_num_steps);
    gtk_label_set_text(sequencer_state, tmp);
}

gboolean
sequencer_sequence_focus_out(GtkWidget *wdg, GdkEventFocus *event, gpointer unused)
{
    const char *tmp = gtk_entry_get_text(sequencer_sequence);
    sequencer_num_steps = 0;

    if (!tmp) {
        sequencer_num_steps = 0;
    } else {
        while (*tmp && sequencer_num_steps < SEQUENCER_MAX_LENGTH) {
            if (*tmp == '1' || *tmp == '0') ++sequencer_num_steps;
            ++tmp;
        }
    }

    sequencer_time_changed(0, 0); /* refresh the state label */
    return false;
}

void
on_sequencer_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_SEQUENCER) {
        float s = floor((float)(e->properties[1].v.i) / 1000.f);
        float ms = (float)(e->properties[1].v.i % 1000);
        gtk_spin_button_set_value(sequencer_seconds, s);
        gtk_spin_button_set_value(sequencer_milliseconds, ms);

        gtk_entry_set_text(sequencer_sequence, e->properties[0].v.s.buf);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sequencer_wrap_around), e->properties[2].v.i8 == 1);

        sequencer_num_steps = ((sequencer*)e)->get_num_steps();

        sequencer_sequence_focus_out(0,0,0);

        gtk_widget_grab_focus(GTK_WIDGET(sequencer_sequence));
    }
}

gboolean
on_sequencer_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, sequencer_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(sequencer_window));
    } else if (btn_pressed(w, sequencer_save, user_data)) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_SEQUENCER) {
            const char *tmp = gtk_entry_get_text(sequencer_sequence);
            gtk_spin_button_update(sequencer_seconds);
            gtk_spin_button_update(sequencer_milliseconds);
            bool wrap_around = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sequencer_wrap_around));
            int seconds = gtk_spin_button_get_value(sequencer_seconds);
            int milliseconds = gtk_spin_button_get_value(sequencer_milliseconds);
            uint32_t full_time = (seconds*1000) + milliseconds;
            if (full_time < SEQUENCER_MIN_TIME)
                full_time = SEQUENCER_MIN_TIME;

            if (!strlen(tmp)) {
                e->set_property(0, "010101010");
            } else {
                e->set_property(0, tmp);
            }

            e->properties[1].v.i = full_time;
            e->properties[2].v.i8 = wrap_around ? 1 : 0;

            ((sequencer*)e)->refresh_sequence();

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            gtk_widget_hide(GTK_WIDGET(sequencer_window));
        }
    }

    return false;
}

gboolean
on_sequencer_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    switch (key->keyval) {
        case GDK_KEY_Escape:
            gtk_widget_hide(w);
            return false;

        case GDK_KEY_Return:
            {
                if (gtk_widget_has_focus(GTK_WIDGET(sequencer_cancel))) {
                    on_sequencer_click(GTK_WIDGET(sequencer_cancel), NULL, GINT_TO_POINTER(1));
                } else {
                    on_sequencer_click(GTK_WIDGET(sequencer_save), NULL, GINT_TO_POINTER(1));
                }
                gtk_widget_hide(w);
                return true;

            }
            break;
    }

    return false;
}

/** --Prompt Settings Dialog **/
void
on_prompt_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_PROMPT) {
        gtk_entry_set_text(prompt_b1, e->properties[0].v.s.buf);
        gtk_entry_set_text(prompt_b2, e->properties[1].v.s.buf);
        gtk_entry_set_text(prompt_b3, e->properties[2].v.s.buf);

        GtkTextBuffer *tb = gtk_text_view_get_buffer(prompt_message);
        gtk_text_buffer_set_text(tb, e->properties[3].v.s.buf, -1);
    }
}

gboolean
on_prompt_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
    }

    return false;
}

gboolean
on_prompt_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, prompt_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(prompt_settings_dialog));
    } else if (btn_pressed(w, prompt_save, user_data)) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_PROMPT) {
            const char *b1 = gtk_entry_get_text(prompt_b1);
            const char *b2 = gtk_entry_get_text(prompt_b2);
            const char *b3 = gtk_entry_get_text(prompt_b3);

            if (!strlen(b1) && !strlen(b2) && !strlen(b3)) {
                ui::message("You must use at least one button.");
                return false;
            }

            GtkTextIter start, end;
            GtkTextBuffer *tb = gtk_text_view_get_buffer(prompt_message);
            gtk_text_buffer_get_bounds(tb, &start, &end);

            const char *message = gtk_text_buffer_get_text(tb, &start, &end, FALSE);

            if (!strlen(message)) {
                ui::message("You must enter a message.");
                return false;
            }

            e->set_property(0, b1);
            e->set_property(1, b2);
            e->set_property(2, b3);
            e->set_property(3, message);

            gtk_widget_hide(GTK_WIDGET(prompt_settings_dialog));

            ui::message("Prompt properties saved!");

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    return false;
}

/** --Variable Chooser **/
gboolean
on_variable_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, variable_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(variable_dialog));
    } else if (btn_pressed(w, variable_ok, user_data)) {
        entity *e = G->selection.e;

        if (e && (e->g_id == O_VAR_SETTER || e->g_id == O_VAR_GETTER)) {
            const char *vn = gtk_entry_get_text(variable_name);

            if (strlen(vn) && strlen(vn) <= 50) {
                char var_name[51];

                int i = 0;
                for (int x=0; x<strlen(vn); ++x) {
                    if (isalnum(vn[x]) || vn[x] == '_' || vn[x] == '-') {
                        var_name[i++] = vn[x];
                    }
                }
                var_name[i] = '\0';

                if (strlen(var_name)) {
                    e->set_property(0, var_name);
                    ui::messagef("Variable name '%s' saved.", var_name);
                    P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
                    P.add_action(ACTION_RESELECT, 0);
                    gtk_widget_hide(GTK_WIDGET(variable_dialog));
                } else
                    ui::message("The variable name must contain at least one 'a-z0-9-_'-character.");
            } else
                ui::message("The variable name must contain at least one 'a-z0-9-_'-character.");
        }
    } else if (btn_pressed(w, variable_reset_this, user_data)) {
        const char *vn = gtk_entry_get_text(variable_name);
        std::map<std::string, float>::size_type num_deleted = W->level_variables.erase(vn);
        if (num_deleted != 0) {
            W->save_cache(W->level_id_type, W->level.local_id);
            ui::messagef("Successfully deleted data for variable '%s'", vn);
        } else
            ui::messagef("No data found for variable '%s'", vn);

    } else if (btn_pressed(w, variable_reset_all, user_data)) {
        W->level_variables.clear();
        if (W->save_cache(W->level_id_type, W->level.local_id))
            ui::message("All level-specific variables cleared.");
        else
            ui::message("Unable to delete level-specific variables.");
    }

    return false;
}

gboolean
on_variable_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(variable_cancel)))
            on_variable_btn_click(GTK_WIDGET(variable_cancel), NULL, GINT_TO_POINTER(1));
        else
            on_variable_btn_click(GTK_WIDGET(variable_ok), NULL, GINT_TO_POINTER(1));
    }

    return false;
}

/** --SFX Emitter **/
void
on_sfx_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_SFX_EMITTER) {
        if (e->properties[0].v.i >= NUM_SFXEMITTER_OPTIONS) e->properties[0].v.i = NUM_SFXEMITTER_OPTIONS-1;

        gtk_combo_box_set_active(GTK_COMBO_BOX(sfx_cb), e->properties[0].v.i);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sfx_global), (e->properties[1].v.i8 == 1));
    }
}

/** --SFX Emitter 2 **/
static void
on_sfx2_cb_changed(GtkComboBoxText *cb, gpointer user_data)
{
    int index = gtk_combo_box_get_active(GTK_COMBO_BOX(cb));
    if (index < 0) {
        return;
    }

    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(sfx2_sub_cb));
    int num = gtk_tree_model_iter_n_children(model, 0);
    for (int x=0; x<num; ++x) {
        gtk_combo_box_text_remove(sfx2_sub_cb, 0);
    }

    const sm_sound *snd = sm::get_sound_by_id(index);

    if (!snd) {
        return;
    }

    gtk_combo_box_text_append_text(sfx2_sub_cb, "Random");
    for (int x=0; x<snd->num_chunks; ++x) {
        const sm_chunk &chunk = snd->chunks[x];

        if (chunk.name) {
            gtk_combo_box_text_append_text(sfx2_sub_cb, chunk.name);
        }
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(sfx2_sub_cb), 0);
}
void
on_sfx2_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_SFX_EMITTER) {
        if (e->properties[0].v.i >= SND__NUM) e->properties[0].v.i = SND__NUM-1;

        gtk_combo_box_set_active(GTK_COMBO_BOX(sfx2_cb), e->properties[0].v.i);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sfx2_global), (e->properties[1].v.i8 == 1));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sfx2_loop), (e->properties[3].v.i8 == 1));

        gtk_combo_box_set_active(GTK_COMBO_BOX(sfx2_sub_cb), e->properties[2].v.i == SFX_CHUNK_RANDOM ? 0 : e->properties[2].v.i+1);
    }
}

/** --Item **/
void
on_item_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_ITEM) {
        if (e->properties[0].v.i >= NUM_ITEMS) e->properties[0].v.i = NUM_ITEMS-1;

        clear_cb(item_cb);

        for (int x=0; x<NUM_ITEMS; x++) {
            item_cb_append(item_cb, x, false);
        }

        gtk_combo_box_set_active(GTK_COMBO_BOX(item_cb), e->properties[0].v.i);
    }
}

/** --Decoration **/
void
on_decoration_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_DECORATION) {
        if (e->properties[0].v.i >= NUM_DECORATIONS) e->properties[0].v.i = NUM_DECORATIONS-1;

        gtk_combo_box_set_active(GTK_COMBO_BOX(decoration_cb), e->properties[0].v.i);
    }
}

/** --Resource **/
void
on_resource_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_RESOURCE) {
        if (e->properties[0].v.i8 >= NUM_RESOURCES) e->properties[0].v.i8 = NUM_RESOURCES-1;

        gtk_combo_box_set_active(GTK_COMBO_BOX(resource_cb), e->properties[0].v.i8);
    }
}

/** --Vendor **/
void
on_vendor_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_VENDOR) {
        gtk_spin_button_set_value(vendor_amount, e->properties[2].v.i);
    }
}

/** --Animal **/
void
on_animal_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_ANIMAL) {
        if (e->properties[0].v.i >= NUM_ANIMAL_TYPES) e->properties[0].v.i = NUM_ANIMAL_TYPES-1;

        gtk_combo_box_set_active(GTK_COMBO_BOX(animal_cb), e->properties[0].v.i);
    }
}

/** --Soundman **/
void
on_soundman_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_SOUNDMAN) {
        if (e->properties[0].v.i >= SND__NUM) e->properties[0].v.i = SND__NUM-1;

        gtk_combo_box_set_active(GTK_COMBO_BOX(soundman_cb), e->properties[0].v.i);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(soundman_catch_all), e->properties[1].v.i8 != 0);
    }
}

/** --Faction **/
void
on_faction_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && (e->g_id == O_GUARDPOINT)) {
        if (e->properties[0].v.i8 < 0) e->properties[0].v.i8 = 0;
        if (e->properties[0].v.i8 >= NUM_FACTIONS) e->properties[0].v.i8 = NUM_FACTIONS-1;

        gtk_combo_box_set_active(GTK_COMBO_BOX(faction_cb), e->properties[0].v.i8);
    }
}

/** --Factory **/
static void
factory_calculate_indices()
{
    tms_debugf("Calculating indices...");
    GtkTreeModel *model = GTK_TREE_MODEL(factory_liststore);
    GtkTreeIter iter;
    int index = 0;

    if (gtk_tree_model_get_iter_first(
            model,
            &iter)) {
        do {
            GValue val = {0, };
            gtk_tree_model_get_value(model,
                                     &iter,
                                     FACTORY_COLUMN_ENABLED,
                                     &val);
            gboolean enabled = g_value_get_boolean(&val);
            if (enabled == TRUE) {
                gtk_list_store_set(factory_liststore, &iter, FACTORY_COLUMN_INDEX, ++index, -1);
            } else {
                gtk_list_store_set(factory_liststore, &iter, FACTORY_COLUMN_INDEX, -1, -1);
            }
        } while (gtk_tree_model_iter_next(model, &iter));
    }
}

static void
factory_enable_toggled(GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
    GtkTreeModel *model = (GtkTreeModel *)data;
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean fixed;

    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, 0, &fixed, -1);

    fixed ^= 1;

    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, fixed, -1);

    gtk_tree_path_free(path);

    factory_calculate_indices();
}

gboolean
on_factory_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(factory_cancel))) {
            gtk_dialog_response(factory_dialog, GTK_RESPONSE_CANCEL);
        } else {
            gtk_dialog_response(factory_dialog, GTK_RESPONSE_ACCEPT);
        }
    }

    return false;
}

void
on_factory_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && IS_FACTORY(e->g_id)) {
        gtk_spin_button_set_value(factory_oil, e->properties[1].v.i);
        gtk_spin_button_set_value(factory_faction, e->properties[2].v.i);
        for (int x=0; x<NUM_RESOURCES; ++x) {
            gtk_spin_button_set_value(factory_resources[x], e->properties[FACTORY_NUM_EXTRA_PROPERTIES+x].v.i);
        }

        factory *fa = static_cast<factory*>(e);

        std::vector<struct factory_object> &objs = fa->objects();

        gtk_list_store_clear(factory_liststore);

        std::vector<uint32_t> recipes;
        factory::generate_recipes(&recipes, fa->properties[0].v.s.buf);

        GtkTreeIter iter;
        for (std::vector<struct factory_object>::const_iterator it = objs.begin();
                it != objs.end(); ++it) {
            const struct factory_object &fo = *it;
            int x = it - objs.begin();

            gtk_list_store_append(factory_liststore, &iter);
            gboolean enabled = FALSE;
            for (std::vector<uint32_t>::iterator it = recipes.begin(); it != recipes.end(); ++it) {
                if (*it == x) {
                    enabled = TRUE;
                    break;
                }
            }
            gtk_list_store_set(factory_liststore, &iter,
                    FACTORY_COLUMN_ENABLED, enabled,
                    FACTORY_COLUMN_INDEX, -1,
                    FACTORY_COLUMN_RECIPE, ((fa->factory_type == FACTORY_ARMORY || fa->factory_type == FACTORY_OIL_MIXER) ? item_options[fo.gid].name : of::get_object_name_by_gid(fo.gid)),
                    FACTORY_COLUMN_RECIPE_ID, x,
                    -1
                    );
        }

        factory_calculate_indices();
    }
}

/** --Treasure chest **/
static void
on_tchest_entity_changed(GtkComboBoxText *cb, gpointer user_data)
{
    int index = gtk_combo_box_get_active(GTK_COMBO_BOX(cb));
    if (index < 0) {
        return;
    }
    int g_id = -1;
    for (int x=0; x<MAX_OF_ID; ++x) {
        if (tchest_translations[x] == index) {
            g_id = x;
            break;
        }
    }

    if (g_id < 0) {
        tms_errorf("invalid g_id: %d", g_id);
        return;
    }

    //gtk_cell_layout_clear(GTK_CELL_LAYOUT(tchest_sub_entity));

    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(tchest_sub_entity));
    int num = gtk_tree_model_iter_n_children(model, 0);
    for (int x=0; x<num; ++x) {
        //gtk_combo_box_text_remove(tchest_sub_entity, x);
        gtk_combo_box_text_remove(tchest_sub_entity, 0);
    }

    if (g_id == O_ITEM) {
        for (int x=0; x<NUM_ITEMS; x++) {
            item_cb_append(tchest_sub_entity, x, false);
        }

        int item_id = rand()%NUM_ITEMS;

        gtk_combo_box_set_active(GTK_COMBO_BOX(tchest_sub_entity), item_id);

    } else if (g_id == O_RESOURCE) {
        for (int x=0; x<NUM_RESOURCES; x++) {
            gtk_combo_box_text_append_text(tchest_sub_entity, resource_data[x].name);
        }

        gtk_combo_box_set_active(GTK_COMBO_BOX(tchest_sub_entity), rand()%NUM_RESOURCES);
    }
}

static void
on_tchest_selection_changed(GtkTreeView *tv, gpointer user_data)
{
    GtkTreeSelection *sel;
    GtkTreeIter       iter;

    sel = gtk_tree_view_get_selection(tv);
    if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
        gtk_widget_set_sensitive(GTK_WIDGET(tchest_remove_selected), true);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(tchest_remove_selected), false);
    }
}

static gboolean
on_tchest_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, tchest_add_entity, user_data)) {
        char search[128];
        strcpy(search, get_cb_val(tchest_entity));
        int len = strlen(search);
        int found_arg = -1;
        int found_score = -10000000;

        for (int x=0; x<menu_objects.size(); ++x) {
            int diff = strncasecmp(search, menu_objects[x].e->get_name(), len);
            /* Only look for 'exact' matches, meaning they must contain that exact string in the beginning
             * i.e. 'sub' fits 'sub' and 'sublayer plank' */

            if (diff == 0) {
                /* Now we find out what the real difference between the match is */
                int score = strcasecmp(search, menu_objects[x].e->get_name());

                if (score == 0) {
                    /* A return value of 0 means it's an exacth match, i.e. 'sub' == 'sub' */
                    found_arg = menu_objects[x].e->g_id;
                    found_score = 0;
                    break;
                } else if (score < 0 && score > found_score) {
                    /* Otherwise, we could settle for this half-match, i.e. 'sub' == 'sublayer plank' */
                    found_arg = menu_objects[x].e->g_id;
                    found_score = score;
                }
            }
        }

        if (found_arg >= 0) {
            gtk_spin_button_update(tchest_count);

            int g_id = found_arg;
            int sub_id = gtk_combo_box_get_active(GTK_COMBO_BOX(tchest_sub_entity));
            int count = gtk_spin_button_get_value(tchest_count);

            char name[128];

            if (g_id == O_ITEM)
                snprintf(name, 127, "Item (%s)", item_options[sub_id].name);
            else if (g_id == O_RESOURCE)
                snprintf(name, 127, "Resource (%s)", resource_data[sub_id].name);
            else
                strcpy(name, search);

            GtkTreeIter iter;
            gtk_list_store_append(tchest_liststore, &iter);
            gtk_list_store_set(tchest_liststore, &iter,
                    TCHEST_COLUMN_G_ID, g_id,
                    TCHEST_COLUMN_SUB_ID, sub_id,
                    TCHEST_COLUMN_NAME, name,
                    TCHEST_COLUMN_COUNT, count,
                    -1
                    );
        }
    } else if (btn_pressed(w, tchest_remove_selected, user_data)) {
        GtkTreeSelection *sel;
        GtkTreeIter       iter;

        sel = gtk_tree_view_get_selection(tchest_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            gtk_list_store_remove(tchest_liststore, &iter);
            gtk_widget_set_sensitive(GTK_WIDGET(tchest_remove_selected), false);
        }
    }

    return false;
}

gboolean
on_tchest_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(tchest_cancel))) {
            gtk_dialog_response(tchest_dialog, GTK_RESPONSE_CANCEL);
        } else {
            gtk_dialog_response(tchest_dialog, GTK_RESPONSE_ACCEPT);
        }
    }

    return false;
}

void
on_tchest_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (!e || e->g_id != O_TREASURE_CHEST)
        return;

    gtk_spin_button_set_value(tchest_count, 1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tchest_entity), tchest_translations[O_ITEM]);

    gtk_list_store_clear(tchest_liststore);

    char *str = strdup(e->properties[0].v.s.buf);
    std::vector<struct treasure_chest_item> items = treasure_chest::parse_items(str);
    free(str);

    for (std::vector<struct treasure_chest_item>::iterator it = items.begin();
            it != items.end(); ++it) {
        struct treasure_chest_item &tci = *it;

        char name[128];

        if (tci.g_id == O_ITEM)
            snprintf(name, 127, "Item (%s)", item_options[tci.sub_id].name);
        else if (tci.g_id == O_RESOURCE)
            snprintf(name, 127, "Resource (%s)", resource_data[tci.sub_id].name);
        else
            snprintf(name, 127, "%s", menu_objects.at((gid_to_menu_pos[tci.g_id])).e->get_name());

        GtkTreeIter iter;
        gtk_list_store_append(tchest_liststore, &iter);
        gtk_list_store_set(tchest_liststore, &iter,
                TCHEST_COLUMN_G_ID, tci.g_id,
                TCHEST_COLUMN_SUB_ID, tci.sub_id,
                TCHEST_COLUMN_NAME, name,
                TCHEST_COLUMN_COUNT, tci.count,
                -1
                );
    }
}

/** --Event listener **/
void
on_elistener_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_EVENT_LISTENER) {
        if (e->properties[0].v.i >= WORLD_EVENT__NUM) e->properties[0].v.i = WORLD_EVENT__NUM-1;

        gtk_combo_box_set_active(GTK_COMBO_BOX(elistener_cb), e->properties[0].v.i);
    }
}

/** --Emitter **/
void
on_emitter_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (e && (e->g_id == O_EMITTER || e->g_id == O_MINI_EMITTER)) {
        gtk_range_set_value(GTK_RANGE(emitter_auto_absorb), (double)e->properties[6].v.f);
    }
}

/** --FX Emitter **/
void
on_fxemitter_show(GtkWidget *wdg, void *ununused)
{
    entity *e = G->selection.e;

    if (!e || e->g_id != O_FX_EMITTER)
        return;

    for (int x=0; x<4; x++) {
        gint index = 0;
        if (e->properties[3+x].v.i != FX_INVALID)
            index = e->properties[3+x].v.i+1;

        gtk_combo_box_set_active(GTK_COMBO_BOX(fxemitter_cb[x]), index);
    }

    gtk_range_set_value(GTK_RANGE(fxemitter_radius), (double)e->properties[0].v.f);
    gtk_range_set_value(GTK_RANGE(fxemitter_count), (double)e->properties[1].v.i);
    gtk_range_set_value(GTK_RANGE(fxemitter_interval), (double)e->properties[2].v.f);
}

/** --Cam targeter **/
void
camtargeter_insert_text(GtkEditable *editable, gchar *new_text,
        gint new_text_length, gpointer position, gpointer *user_data)
{
    for (int n=0; n<new_text_length; ++n) {
        if (!isdigit(new_text[n]) && new_text[n] != '.' && new_text[n] != ',' && new_text[n] != '-') {
            g_signal_stop_emission_by_name(editable, "insert-text");
            break;
        }
    }
}
void
camtargeter_entry_changed(GtkEditable *unused_editable, void *unused)
{
    GtkEntry *entry = 0;
    GtkRange *range = 0;
    GtkEditable *editable = 0;
    if (gtk_widget_has_focus(GTK_WIDGET(camtargeter_y_offset_entry))) {
        range = camtargeter_y_offset;
        entry = camtargeter_y_offset_entry;
        editable = GTK_EDITABLE(entry);
    } else if (gtk_widget_has_focus(GTK_WIDGET(camtargeter_x_offset_entry))) {
        range = camtargeter_x_offset;
        entry = camtargeter_x_offset_entry;
        editable = GTK_EDITABLE(entry);
    }

    if (entry) {
        float v = atof(gtk_editable_get_chars(editable, 0, -1));
        /* clamp! */
        if (v < -150.f) {
            v = -150.f;
            char tmp[8];
            snprintf(tmp, 7, "%.2f", v);
            gtk_entry_set_text(entry, tmp);
            gtk_editable_set_position(editable, 0);
        } else if (v > 150.f) {
            v = 150.f;
            char tmp[8];
            snprintf(tmp, 7, "%.2f", v);
            gtk_entry_set_text(entry, tmp);
            gtk_editable_set_position(editable, 0);
        }
        gtk_range_set_value(range, v);
    }
}

void
camtargeter_value_changed(GtkRange *unused_range, void *unused)
{
    GtkRange *range = 0;
    GtkEntry *entry = 0;
    if (gtk_widget_has_focus(GTK_WIDGET(camtargeter_x_offset))) {
        range = camtargeter_x_offset;
        entry = camtargeter_x_offset_entry;
    } else if (gtk_widget_has_focus(GTK_WIDGET(camtargeter_y_offset))) {
        range = camtargeter_y_offset;
        entry = camtargeter_y_offset_entry;
    }

    if (range) {
        char tmp[8];
        snprintf(tmp, 7, "%.2f", gtk_range_get_value(range));
        gtk_entry_set_text(entry, tmp);
    }
}

gboolean
on_camtargeter_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    switch (key->keyval) {
        case GDK_KEY_Escape:
            gtk_dialog_response(camtargeter_dialog, GTK_RESPONSE_CANCEL);
            break;

        case GDK_KEY_Return:
            if (gtk_widget_has_focus(GTK_WIDGET(camtargeter_cancel))) {
                gtk_dialog_response(camtargeter_dialog, GTK_RESPONSE_CANCEL);
            } else {
                gtk_dialog_response(camtargeter_dialog, GTK_RESPONSE_ACCEPT);
            }
            break;
    }

    return false;
}

void
on_camtargeter_show(GtkWidget *wdg, void *ununused)
{
    char tmp[8];
    entity *e = G->selection.e;

    if (e && e->g_id == O_CAM_TARGETER) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(camtargeter_mode), e->properties[1].v.i8);
        gtk_combo_box_set_active(GTK_COMBO_BOX(camtargeter_offset_mode), e->properties[2].v.i8);

        gtk_range_set_value(camtargeter_x_offset, e->properties[3].v.f);
        snprintf(tmp, 7, "%.2f", e->properties[3].v.f);
        gtk_entry_set_text(camtargeter_x_offset_entry, tmp);

        gtk_range_set_value(camtargeter_y_offset, e->properties[4].v.f);
        snprintf(tmp, 7, "%.2f", e->properties[4].v.f);
        gtk_entry_set_text(camtargeter_y_offset_entry, tmp);
    }
}

void
activate_frequency_row(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter_from_string(model, &iter, gtk_tree_path_to_string(path));

    guint _frequency;
    gtk_tree_model_get(model, &iter,
                       FC_FREQUENCY, &_frequency,
                       -1);

    gtk_spin_button_set_value(frequency_value, _frequency);
}

void
on_pkg_name_show(GtkWidget *wdg, void *unused)
{
    gtk_entry_set_text(pkg_name_entry, "");
}

void
on_tips_show(GtkWidget *wdg, void *unused)
{
    if (ctip == -1) ctip = rand()%num_tips;

    gtk_label_set_markup(tips_text, tips[ctip]);

    ctip = (ctip+1)%num_tips;
}

void
on_publish_show(GtkWidget *wdg, void *unused)
{
    char *current_descr = (char*)malloc(W->level.descr_len+1);
    memcpy(current_descr, W->level.descr, W->level.descr_len);
    current_descr[W->level.descr_len] = '\0';
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(publish_descr);
    gtk_text_buffer_set_text(text_buffer, current_descr, -1);

    char current_name[257];
    memcpy(current_name, W->level.name, W->level.name_len);
    current_name[W->level.name_len] = '\0';
    gtk_entry_set_text(publish_name, current_name);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(publish_locked), W->level.visibility == LEVEL_LOCKED);

    free(current_descr);
}

void
on_package_manager_show(GtkWidget *wdg, void *unused)
{
    pk_reload_pkg_list();
    pk_reload_level_list();
}

void
on_pkg_lvl_chooser_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && (e->g_id == O_VAR_GETTER || e->g_id == O_VAR_SETTER)) {
        gtk_spin_button_set_value(pkg_lvl_chooser_lvl_id, e->properties[0].v.i8);
    }
}

void
on_variable_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;

    if (e && (e->g_id == O_VAR_GETTER || e->g_id == O_VAR_SETTER)) {
        gtk_entry_set_text(variable_name, e->properties[0].v.s.buf);
        gtk_widget_grab_focus(GTK_WIDGET(variable_name));
    }
}

void
on_object_show(GtkWidget *wdg, void *unused)
{
    GtkTreeIter iter;

    gtk_list_store_clear(GTK_LIST_STORE(object_treemodel));

    lvlfile *level = pkgman::get_levels(LEVEL_PARTIAL);

    while (level) {
        gtk_list_store_append(GTK_LIST_STORE(object_treemodel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(object_treemodel), &iter,
                OC_ID, level->id,
                OC_NAME, level->name,
                OC_DATE, level->modified_date,
                -1
                );
        lvlfile *next = level->next;
        delete level;
        level = next;
    }

    GtkTreePath      *path;
    GtkTreeSelection *sel;

    path = gtk_tree_path_new_from_indices(0, -1);
    sel  = gtk_tree_view_get_selection(object_treeview);

    gtk_tree_model_get_iter(object_treemodel,
                            &iter,
                            path);

    GValue val = {0, };

    gtk_tree_model_get_value(object_treemodel,
                             &iter,
                             0,
                             &val);

    gtk_tree_selection_select_path(sel, path);

    gtk_tree_path_free(path);
}

/** --Open state **/
void
on_open_state_show(GtkWidget *wdg, void *unused)
{
    GtkTreeIter iter;

    gtk_list_store_clear(GTK_LIST_STORE(open_state_treemodel));

    lvlfile *level = pkgman::get_levels(LEVEL_LOCAL_STATE);

    while (level) {
        gtk_list_store_append(GTK_LIST_STORE(open_state_treemodel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(open_state_treemodel), &iter,
                OSC_ID, level->id,
                OSC_NAME, level->name,
                OSC_DATE, level->modified_date,
                OSC_SAVE_ID, level->save_id,
                OSC_ID_TYPE, level->id_type,
                -1
                );
        lvlfile *next = level->next;
        delete level;
        level = next;
    }

    GtkTreePath      *path;
    GtkTreeSelection *sel;

    path = gtk_tree_path_new_from_indices(0, -1);
    sel  = gtk_tree_view_get_selection(open_state_treeview);

    gtk_tree_model_get_iter(open_state_treemodel,
                            &iter,
                            path);

    GValue val = {0, };

    gtk_tree_model_get_value(open_state_treemodel,
                             &iter,
                             0,
                             &val);

    gtk_tree_selection_select_path(sel, path);

    tms_infof("got id: %d", g_value_get_uint(&val));
    gtk_tree_path_free(path);
}

static void
open_state_row(GtkTreeIter *iter)
{
    if (!iter) {
        return;
    }

    guint _level_id;
    gtk_tree_model_get(open_state_treemodel, iter,
            OSC_ID, &_level_id,
            -1);
    guint _save_id;
    gtk_tree_model_get(open_state_treemodel, iter,
            OSC_SAVE_ID, &_save_id,
            -1);

    guint _level_id_type;
    gtk_tree_model_get(open_state_treemodel, iter,
            OSC_ID_TYPE, &_level_id_type,
            -1);

    uint32_t level_id = (uint32_t)_level_id;
    uint32_t save_id = (uint32_t)_save_id;
    uint32_t id_type = (uint32_t)_level_id_type;

    tms_infof("clicked level id %u save %u ", level_id, save_id);

    uint32_t *info = (uint32_t*)malloc(sizeof(uint32_t)*3);
    info[0] = id_type;
    info[1] = level_id;
    info[2] = save_id;

    if (open_state_no_testplaying) {
        G->state.test_playing = false;
        G->screen_back = P.s_menu_play;
    }

    P.add_action(ACTION_OPEN_STATE, info);

    gtk_widget_hide(GTK_WIDGET(open_state_window));
}

static void
activate_open_state_row(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter_from_string(model, &iter, gtk_tree_path_to_string(path));

    open_state_row(&iter);
}

/** --Open level **/
void
on_open_show(GtkWidget *wdg, void *unused)
{
    GtkTreeIter iter;

    gtk_list_store_clear(GTK_LIST_STORE(open_treemodel));

    lvlfile *level = pkgman::get_levels(LEVEL_LOCAL);

    while (level) {
        gtk_list_store_append(GTK_LIST_STORE(open_treemodel), &iter);
        const char *version_string = level_version_string(level->version);
        gtk_list_store_set(GTK_LIST_STORE(open_treemodel), &iter,
                OC_ID, level->id,
                OC_NAME, level->name,
                OC_VERSION, version_string,
                OC_DATE, level->modified_date,
                -1
                );
        lvlfile *next = level->next;
        delete level;
        level = next;
    }

    GtkTreePath      *path;
    GtkTreeSelection *sel;

    path = gtk_tree_path_new_from_indices(0, -1);
    sel  = gtk_tree_view_get_selection(open_treeview);

    gtk_tree_model_get_iter(open_treemodel,
                            &iter,
                            path);

    GValue val = {0, };

    gtk_tree_model_get_value(open_treemodel,
                             &iter,
                             0,
                             &val);

    gtk_tree_selection_select_path(sel, path);

    tms_infof("got id: %d", g_value_get_uint(&val));
    gtk_tree_path_free(path);
}

static void
activate_open_row(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter_from_string(model, &iter, gtk_tree_path_to_string(path));

    guint _level_id;
    gtk_tree_model_get(model, &iter,
                       OC_ID, &_level_id,
                       -1);

    uint32_t level_id = (uint32_t)_level_id;

    tms_infof("clicked level id %u", level_id);

    P.add_action(ACTION_OPEN, level_id);

    gtk_widget_hide(GTK_WIDGET(open_window));
}

static void
open_menu_item_activated(GtkMenuItem *i, gpointer userdata)
{
    if (i == open_menu_information) {
        static GtkMessageDialog *msg_dialog = 0;

        if (msg_dialog == 0) {
            msg_dialog = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(
                    0, (GtkDialogFlags)(0),
                    GTK_MESSAGE_INFO,
                    GTK_BUTTONS_CLOSE,
                    "Level information"));
        }

        GtkTreeSelection *sel;
        GtkTreeIter       iter;

        sel = gtk_tree_view_get_selection(open_treeview);

        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            GValue val_name = {0, };
            GValue val_date = {0, };
            GValue val_version = {0, };
            char msg[2048];
            gtk_tree_model_get_value(open_treemodel,
                    &iter,
                    OC_NAME,
                    &val_name);
            const char *name = g_value_get_string(&val_name);

            gtk_tree_model_get_value(open_treemodel,
                    &iter,
                    OC_DATE,
                    &val_date);
            const char *lastmodified = g_value_get_string(&val_date);

            gtk_tree_model_get_value(open_treemodel,
                    &iter,
                    OC_VERSION,
                    &val_version);
            const char *version = g_value_get_string(&val_version);

            snprintf(
                msg, 2048,
                "Name: %s\nVersion: %s\nLast modified: %s",
                name, version, lastmodified
            );

            g_object_set(msg_dialog, "text", msg, NULL);

            int r = gtk_dialog_run(GTK_DIALOG(msg_dialog));
            switch (r) {
                default:
                    gtk_widget_hide(GTK_WIDGET(msg_dialog));
                    break;
            }
        }
    } else if (i == open_menu_delete) {
        GtkDialog* confirm = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Delete level",
            GTK_WINDOW(open_window),
            (GtkDialogFlags)(0),
            "_Confirm",
            GTK_RESPONSE_ACCEPT,
            "_Cancel",
            GTK_RESPONSE_REJECT,
            NULL
        ));
        gtk_container_add(
            GTK_CONTAINER(gtk_dialog_get_content_area(confirm)),
            gtk_label_new("Are you sure you want to delete this level")
        );
        int r = gtk_dialog_run(confirm);
        switch (r) {
            case GTK_RESPONSE_ACCEPT: {
                tms_infof("deleting uwu");

                //get level id
                GtkTreeIter iter;
                GtkTreeSelection *sel = gtk_tree_view_get_selection(open_treeview);
                if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
                    GValue val_id = {0, };
                    gtk_tree_model_get_value(
                        open_treemodel,
                        &iter,
                        OC_ID,
                        &val_id
                    );
                    uint32_t level_id = g_value_get_uint(&val_id);
                    tms_infof("will DELETE local level with id of %d RIGHT NOW!", level_id);

                    //XXX: Levels in the "open" should always be local
                    //XXX: Save id is only used for state saves (LEVEL_*_STATE), not levels
                    if (G->delete_level(LEVEL_LOCAL, level_id, -1)) {
                        //success
                        tms_infof("deleted successfully :3");

                        //remove from the list
                        gtk_list_store_remove(GTK_LIST_STORE(open_treemodel), &iter);
                    } else {
                        //error
                        tms_errorf("unlink failed");

                        //show error dialog
                        GtkDialog* error = GTK_DIALOG(gtk_message_dialog_new(
                            GTK_WINDOW(open_window),
                            GTK_DIALOG_DESTROY_WITH_PARENT,
                            GTK_MESSAGE_ERROR,
                            GTK_BUTTONS_CLOSE,
                            "Failed to delete the level"
                        ));
                        g_signal_connect_swapped(
                            G_OBJECT(error), "response",
                            G_CALLBACK(gtk_widget_destroy),
                            error
                        );
                        gtk_dialog_run(error);
                    }
                }
                break;
            }
            default: {
                gtk_widget_hide(GTK_WIDGET(confirm));
            }
        }
        gtk_widget_destroy(GTK_WIDGET(confirm));
    }
}

static gboolean
open_row_button_press(GtkWidget *wdg, GdkEvent *event, gpointer userdata)
{
    if (event->type == GDK_BUTTON_PRESS) {
        GdkEventButton *bevent = (GdkEventButton*)event;
        if (bevent->button == 3) {
            gtk_widget_show_all(GTK_WIDGET(open_menu));
            gtk_menu_popup_at_pointer(open_menu, event);
        }
    }
    return FALSE;
}

static void
confirm_import(uint32_t level_id)
{
    if (object_window_multiemitter) {
        P.add_action(ACTION_MULTIEMITTER_SET, level_id);
    } else {
        P.add_action(ACTION_SELECT_IMPORT_OBJECT, level_id);
    }
}

void
activate_object_row(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter_from_string(model, &iter, gtk_tree_path_to_string(path));

    guint _level_id;
    gtk_tree_model_get(model, &iter,
                       OC_ID, &_level_id,
                       -1);

    confirm_import((uint32_t)_level_id);

    gtk_widget_hide(GTK_WIDGET(object_window));
}

gboolean
on_autofit_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, (GtkButton*)w, user_data)) {
        P.add_action(ACTION_AUTOFIT_LEVEL_BORDERS, 0);
    }

    return false;
}

gboolean
on_lvl_bg_changed(GtkWidget *w, GdkEventButton *ev, gpointer user_data) {
    for (const int *ptr = colored_bgs; ; ++ptr) {
        if ((*ptr == -1) || (*ptr == gtk_combo_box_get_active(GTK_COMBO_BOX(lvl_bg)))) {
            gtk_widget_set_visible(GTK_WIDGET(lvl_bg_color), *ptr != -1);
            break;
        }
    }

    return false;
}

gboolean
on_lvl_bg_color_set(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    tms_debugf("bg color button COLOR SET");

    GtkColorChooser *sel = GTK_COLOR_CHOOSER(lvl_bg_color);

    GdkRGBA new_color;
    gtk_color_chooser_get_rgba(sel, &new_color);

    tms_debugf("new_r: %.2f", new_color.red);
    tms_debugf("new_g: %.2f", new_color.green);
    tms_debugf("new_b: %.2f", new_color.blue);

    new_bg_color = pack_rgba(new_color.red, new_color.green, new_color.blue, 1.f);

    return false;
}

gboolean
on_upgrade_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, (GtkButton*)w, user_data)) {
        gint result = gtk_dialog_run(confirm_upgrade_dialog);

        if (result == GTK_RESPONSE_ACCEPT) {
            P.add_action(ACTION_UPGRADE_LEVEL, 0);
            _close_all_dialogs(0);
        }

        gtk_widget_hide(GTK_WIDGET(confirm_upgrade_dialog));
    }

    return false;
}

gboolean
on_open_state_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, open_state_btn_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(open_state_window));
    } else if (btn_pressed(w, open_state_btn_open, user_data)) {
        /* open ! */
        GtkTreeSelection *sel;
        GtkTreeIter       iter;

        sel = gtk_tree_view_get_selection(open_state_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */
            open_state_row(&iter);

        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean
on_open_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, open_btn_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(open_window));
    } else if (btn_pressed(w, open_btn_open, user_data)) {
        /* open ! */
        GtkTreeSelection *sel;
        GtkTreeIter       iter;
        GValue            val = {0, };

        sel = gtk_tree_view_get_selection(open_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */

            /* Fetch the value of the first column into `val' */
            gtk_tree_model_get_value(open_treemodel,
                                     &iter,
                                     0,
                                     &val);

            uint32_t level_id = g_value_get_uint(&val);

            tms_infof("Opening level %d from Open window", level_id);

            P.add_action(ACTION_OPEN, level_id);

            gtk_widget_hide(GTK_WIDGET(open_window));
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean
on_object_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, object_btn_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(object_window));
    } else if (btn_pressed(w, object_btn_open, user_data)) {
        /* open ! */
        GtkTreeSelection *sel;
        GtkTreeIter       iter;
        GValue            val = {0, };

        sel = gtk_tree_view_get_selection(object_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */

            /* Fetch the value of the first column into `val' */
            gtk_tree_model_get_value(object_treemodel,
                                     &iter,
                                     0,
                                     &val);

            uint32_t level_id = g_value_get_uint(&val);

            confirm_import(level_id);

            gtk_widget_hide(GTK_WIDGET(object_window));
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

/** --Robot **/
static void
robot_item_toggled(GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
    GtkTreeModel *model = (GtkTreeModel *)data;
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean fixed;

    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, 0, &fixed, -1);

    fixed ^= 1;

    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, fixed, -1);

    gtk_tree_path_free(path);
}

static void
on_robot_show(GtkWidget *wdg, void *unused)
{
    entity *e = G->selection.e;
    if (e && e->flag_active(ENTITY_IS_ROBOT)) {
        clear_cb(robot_head_equipment);
        clear_cb(robot_head);
        clear_cb(robot_back_equipment);
        clear_cb(robot_front_equipment);
        clear_cb(robot_feet);
        clear_cb(robot_bolts);

        for (int x=0; x<NUM_HEAD_EQUIPMENT_TYPES; ++x) {
            GtkComboBoxText *cb = robot_head_equipment;
            uint32_t item_id = _head_equipment_to_item[x];

            item_cb_append(cb, item_id, true);
        }

        for (int x=0; x<NUM_HEAD_TYPES; ++x) {
            GtkComboBoxText *cb = robot_head;
            uint32_t item_id = _head_to_item[x];

            item_cb_append(cb, item_id, true);
        }

        for (int x=0; x<NUM_BACK_EQUIPMENT_TYPES; ++x) {
            GtkComboBoxText *cb = robot_back_equipment;
            uint32_t item_id = _back_to_item[x];

            item_cb_append(cb, item_id, true);
        }

        for (int x=0; x<NUM_FRONT_EQUIPMENT_TYPES; ++x) {
            GtkComboBoxText *cb = robot_front_equipment;
            uint32_t item_id = _front_to_item[x];

            item_cb_append(cb, item_id, true);
        }

        for (int x=0; x<NUM_FEET_TYPES; ++x) {
            GtkComboBoxText *cb = robot_feet;
            uint32_t item_id = _feet_to_item[x];

            item_cb_append(cb, item_id, true);
        }

        for (int x=0; x<NUM_BOLT_SETS; ++x) {
            GtkComboBoxText *cb = robot_bolts;
            uint32_t item_id = _bolt_to_item[x];

            item_cb_append(cb, item_id, false);
        }

        /* Clear all equipment boxes and refill them. */
        if (e->id == G->state.adventure_id && W->is_adventure()) {
            gtk_widget_set_sensitive(GTK_WIDGET(robot_state_idle), false);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_state_walk), false);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_state_dead), false);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_roam),       false);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_dir_left),   false);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_dir_random), false);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_dir_right),  false);
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(robot_state_idle), true);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_state_walk), true);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_state_dead), true);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_roam),       true);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_dir_left),   true);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_dir_random), true);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_dir_right),  true);
        }

        if (static_cast<creature*>(e)->has_feature(CREATURE_FEATURE_HEAD)) {
            gtk_widget_set_sensitive(GTK_WIDGET(robot_head), true);
            gtk_combo_box_set_active(GTK_COMBO_BOX(robot_head), e->properties[ROBOT_PROPERTY_HEAD].v.i8);

            gtk_widget_set_sensitive(GTK_WIDGET(robot_head_equipment), true);
            gtk_combo_box_set_active(GTK_COMBO_BOX(robot_head_equipment), e->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8);
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(robot_head_equipment), false);
            gtk_widget_set_sensitive(GTK_WIDGET(robot_head), false);
        }

        if (static_cast<creature*>(e)->has_feature(CREATURE_FEATURE_BACK_EQUIPMENT)) {
            gtk_widget_set_sensitive(GTK_WIDGET(robot_back_equipment), true);
            gtk_combo_box_set_active(GTK_COMBO_BOX(robot_back_equipment), e->properties[ROBOT_PROPERTY_BACK].v.i8);
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(robot_back_equipment), false);
        }

        if (static_cast<creature*>(e)->has_feature(CREATURE_FEATURE_FRONT_EQUIPMENT)) {
            gtk_widget_set_sensitive(GTK_WIDGET(robot_front_equipment), true);
            gtk_combo_box_set_active(GTK_COMBO_BOX(robot_front_equipment), e->properties[ROBOT_PROPERTY_FRONT].v.i8);
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(robot_front_equipment), false);
        }

        gtk_combo_box_set_active(GTK_COMBO_BOX(robot_feet), e->properties[8].v.i8);
        gtk_combo_box_set_active(GTK_COMBO_BOX(robot_bolts), e->properties[ROBOT_PROPERTY_BOLT_SET].v.i8);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(robot_state_idle), (e->properties[1].v.i8 == CREATURE_IDLE));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(robot_state_walk), (e->properties[1].v.i8 == CREATURE_WALK));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(robot_state_dead), (e->properties[1].v.i8 == CREATURE_DEAD));

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(robot_roam), (bool)(e->properties[2].v.i8));

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(robot_dir_left), (e->properties[4].v.i8 == 1));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(robot_dir_right), (e->properties[4].v.i8 == 2));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(robot_state_walk), (e->properties[4].v.i8 != 1 && e->properties[4].v.i8 != 2));

        for (int x=0; x<NUM_FACTIONS; ++x) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(robot_faction[x]), (e->properties[6].v.i8 == x));
        }

        gtk_list_store_clear(robot_ls_equipment);

        std::vector<uint32_t> equipment;

        std::vector<char*> eq_parts = p_split(e->properties[ROBOT_PROPERTY_EQUIPMENT].v.s.buf, e->properties[ROBOT_PROPERTY_EQUIPMENT].v.s.len, ";");

        for (std::vector<char*>::iterator it = eq_parts.begin();
                it != eq_parts.end(); ++it) {
            equipment.push_back(atoi(*it));
        }

        GtkTreeIter iter;
        for (int x=0; x<NUM_ITEMS; ++x) {
            struct item_option *io = &item_options[x];

            if (io->category != ITEM_CATEGORY_WEAPON &&
                io->category != ITEM_CATEGORY_TOOL &&
                io->category != ITEM_CATEGORY_CIRCUIT
                ) {
                continue;
            }

            gtk_list_store_append(robot_ls_equipment, &iter);

            gboolean equipped = FALSE;
            for (std::vector<uint32_t>::iterator it = equipment.begin(); it != equipment.end(); ++it) {
                if (*it == x) {
                    equipped = TRUE;
                    break;
                }
            }
            gtk_list_store_set(robot_ls_equipment, &iter,
                    ROBOT_COLUMN_EQUIPPED, equipped,
                    ROBOT_COLUMN_ITEM, item::get_ui_name(x),
                    ROBOT_COLUMN_ITEM_ID, x,
                    -1
                    );
        }
    }
}

gboolean
on_robot_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, robot_btn_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(robot_window));
    } else if (btn_pressed(w, robot_btn_ok, user_data)) {
        entity *e = G->selection.e;

        if (e && e->flag_active(ENTITY_IS_ROBOT)) {
            if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(robot_state_idle))) {
                e->properties[1].v.i8 = CREATURE_IDLE;
            } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(robot_state_walk))) {
                e->properties[1].v.i8 = CREATURE_WALK;
            } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(robot_state_dead))) {
                e->properties[1].v.i8 = CREATURE_DEAD;
            } else {
                tms_warnf("Unknown robot state");
            }

            if (static_cast<creature*>(e)->has_feature(CREATURE_FEATURE_HEAD)) {
                e->properties[ROBOT_PROPERTY_HEAD].v.i8 = gtk_combo_box_get_active(GTK_COMBO_BOX(robot_head));
                e->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8 = gtk_combo_box_get_active(GTK_COMBO_BOX(robot_head_equipment));
            }

            if (static_cast<creature*>(e)->has_feature(CREATURE_FEATURE_BACK_EQUIPMENT)) {
                e->properties[ROBOT_PROPERTY_BACK].v.i8 = gtk_combo_box_get_active(GTK_COMBO_BOX(robot_back_equipment));
            }

            if (static_cast<creature*>(e)->has_feature(CREATURE_FEATURE_FRONT_EQUIPMENT)) {
                e->properties[ROBOT_PROPERTY_FRONT].v.i8 = gtk_combo_box_get_active(GTK_COMBO_BOX(robot_front_equipment));
            }

            e->properties[ROBOT_PROPERTY_FEET].v.i8 = gtk_combo_box_get_active(GTK_COMBO_BOX(robot_feet));
            e->properties[ROBOT_PROPERTY_BOLT_SET].v.i8 = gtk_combo_box_get_active(GTK_COMBO_BOX(robot_bolts));

            e->properties[ROBOT_PROPERTY_ROAMING].v.i8 = (uint8_t)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(robot_roam));

            if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(robot_dir_left))) {
                e->properties[4].v.i8 = 1;
                ((robot_base*)e)->set_i_dir(DIR_LEFT);
            } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(robot_dir_random))) {
                e->properties[4].v.i8 = 0;
                ((robot_base*)e)->set_i_dir(0.f);
            } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(robot_dir_right))) {
                e->properties[4].v.i8 = 2;
                ((robot_base*)e)->set_i_dir(DIR_RIGHT);
            } else {
                tms_warnf("Unknown default direction");
            }

            for (int x=0; x<=NUM_FACTIONS; ++x) {
                if (x == NUM_FACTIONS) {
                    e->properties[6].v.i8 = FACTION_ENEMY;
                    ((robot_base*)e)->set_faction(e->properties[6].v.i8);
                } else {
                    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(robot_faction[x]))) {
                        e->properties[6].v.i8 = x;
                        ((robot_base*)e)->set_faction(e->properties[6].v.i8);
                        break;
                    }
                }
            }

            GtkTreeModel *model = GTK_TREE_MODEL(robot_ls_equipment);
            GtkTreeIter iter;
            int x = 0;
            std::stringstream ss;

            if (gtk_tree_model_get_iter_first(
                        model,
                        &iter)) {
                do {
                    GValue val = {0, };
                    GValue val_id = {0, };
                    gtk_tree_model_get_value(model,
                            &iter,
                            ROBOT_COLUMN_EQUIPPED,
                            &val);
                    gtk_tree_model_get_value(model,
                            &iter,
                            ROBOT_COLUMN_ITEM_ID,
                            &val_id);
                    gboolean enabled = g_value_get_boolean(&val);
                    gint id = g_value_get_int(&val_id);
                    if (enabled == TRUE) {
                        if (x > 0) {
                            ss << ";";
                        }

                        ss << id;

                        ++ x;
                    }
                } while (gtk_tree_model_iter_next(model, &iter));
            }

            e->set_property(ROBOT_PROPERTY_EQUIPMENT, ss.str().c_str());

            ui::message("Robot properties saved!");
            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            W->add_action(e->id, ACTION_CALL_ON_LOAD);
        }

        gtk_widget_hide(GTK_WIDGET(robot_window));
    }

    return false;
}

gboolean
on_robot_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape) {
        gtk_widget_hide(w);
    } else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(robot_btn_cancel))) {
            on_robot_btn_click(GTK_WIDGET(robot_btn_cancel), NULL, GINT_TO_POINTER(1));
        } else {
            on_robot_btn_click(GTK_WIDGET(robot_btn_ok), NULL, GINT_TO_POINTER(1));
        }
    }

    return false;
}

gboolean
on_object_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        GtkTreeSelection *sel;
        GtkTreeIter       iter;
        GValue            val = {0, };

        sel = gtk_tree_view_get_selection(object_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */

            /* Fetch the value of the first column into `val' */
            gtk_tree_model_get_value(object_treemodel,
                                     &iter,
                                     0,
                                     &val);

            uint32_t level_id = g_value_get_uint(&val);

            confirm_import(level_id);

            gtk_widget_hide(w);
            return true;
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean
on_synth_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        /* duplicate code from _open_synth */
        entity *e = G->selection.e;

        if (e && e->g_id == O_SYNTHESIZER) {
            gtk_spin_button_update(synth_hz_low);
            gtk_spin_button_update(synth_hz_high);
            //gtk_spin_button_update(synth_vol_vibrato);
            //gtk_spin_button_update(synth_freq_vibrato);
            //gtk_spin_button_update(synth_bitcrushing);
            float low = gtk_spin_button_get_value(synth_hz_low);
            float high = gtk_spin_button_get_value(synth_hz_high);
            float pw = gtk_range_get_value(synth_pulse_width);
            float vb = gtk_range_get_value(synth_vol_vibrato_hz);
            float fb = gtk_range_get_value(synth_freq_vibrato_hz);
            float vba = gtk_range_get_value(synth_vol_vibrato_extent);
            float fba = gtk_range_get_value(synth_freq_vibrato_extent);
            float bitcrushing = gtk_range_get_value(synth_bitcrushing);

            if (high < low) high = low;

            e->properties[0].v.f = low;
            e->properties[1].v.f = high;

            int index = gtk_combo_box_get_active(GTK_COMBO_BOX(synth_waveform));

            e->properties[2].v.i = index;

            e->properties[3].v.f = bitcrushing;
            e->properties[4].v.f = vb;
            e->properties[5].v.f = fb;

            e->properties[6].v.f = vba;
            e->properties[7].v.f = fba;

            e->properties[8].v.f = pw;
        }
        gtk_widget_hide(w);
    }

    return false;
}

gboolean
on_open_state_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        GtkTreeSelection *sel;
        GtkTreeIter       iter;

        sel = gtk_tree_view_get_selection(open_state_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */
            guint _level_id;
            gtk_tree_model_get(open_state_treemodel, &iter,
                               OSC_ID, &_level_id,
                               -1);
            guint _save_id;
            gtk_tree_model_get(open_state_treemodel, &iter,
                               OSC_SAVE_ID, &_save_id,
                               -1);

            guint _level_id_type;
            gtk_tree_model_get(open_state_treemodel, &iter,
                               OSC_ID_TYPE, &_level_id_type,
                               -1);

            uint32_t level_id = (uint32_t)_level_id;
            uint32_t save_id = (uint32_t)_save_id;
            uint32_t id_type = (uint32_t)_level_id_type;

            tms_infof("clicked level id %u save %u ", level_id, save_id);

            uint32_t *info = (uint32_t*)malloc(sizeof(uint32_t)*3);
            info[0] = id_type;
            info[1] = level_id;
            info[2] = save_id;

            P.add_action(ACTION_OPEN_STATE, info);

            gtk_widget_hide(w);
            return true;
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean
on_open_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        GtkTreeSelection *sel;
        GtkTreeIter       iter;
        GValue            val = {0, };

        sel = gtk_tree_view_get_selection(open_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */

            /* Fetch the value of the first column into `val' */
            gtk_tree_model_get_value(open_treemodel,
                                     &iter,
                                     0,
                                     &val);

            uint32_t level_id = g_value_get_uint(&val);

            tms_infof("Opening level %d from Open window", level_id);

            P.add_action(ACTION_OPEN, level_id);

            gtk_widget_hide(w);
            return true;
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean
on_color_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    GtkWidget *ok_button = gtk_dialog_get_widget_for_response(GTK_DIALOG(beam_color_dialog), GTK_RESPONSE_OK);

    if (key->keyval == GDK_KEY_Escape) {
        gtk_widget_hide(w);
    } else if (key->keyval == GDK_KEY_Return
            && (w == gtk_dialog_get_content_area(GTK_DIALOG(beam_color_dialog)) ||
                w == ok_button)) {
        gtk_button_clicked(GTK_BUTTON(ok_button));
        return true;
    }

    return false;
}

gboolean
on_lvl_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape) {
        gtk_widget_hide(GTK_WIDGET(save_window));
        return true;
    }

    if (key->keyval == GDK_KEY_Return) {
        gtk_button_clicked(save_ok);
        return true;
    }

    return false;
}

static void
save_setting_row(struct table_setting_row *r)
{
    const struct setting_row_type &row = r->row;

    switch (row.type) {
        case ROW_CHECKBOX:
            settings[r->setting_name]->v.b = (bool)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(r->wdg));
            break;

        case ROW_HSCALE:
            settings[r->setting_name]->v.f = (float)gtk_range_get_value(GTK_RANGE(r->wdg));
            break;

        default:
            tms_errorf("Unknown row type: %d", row.type);
            break;
    }
}

static void
load_setting_row(struct table_setting_row *r)
{
    const struct setting_row_type &row = r->row;

    switch (row.type) {
        case ROW_CHECKBOX:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(r->wdg), settings[r->setting_name]->v.b);
            break;

        case ROW_HSCALE:
            gtk_range_set_value(GTK_RANGE(r->wdg), (double)settings[r->setting_name]->v.f);
            break;

        default:
            tms_errorf("Unknown row type: %d", row.type);
            break;
    }
}

static void
create_setting_row_widget(struct table_setting_row *r)
{
    const struct setting_row_type &row = r->row;

    switch (row.type) {
        case ROW_CHECKBOX:
            r->wdg = gtk_check_button_new();
            break;

        case ROW_HSCALE:
            r->wdg = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, row.min, row.max, row.step);
            break;

        default:
            tms_errorf("Unknown row type: %d", row.type);
            break;
    }
}

/** --Settings **/
void
save_settings()
{
    P.can_reload_graphics = false;
    P.can_set_settings = false;
    P.add_action(ACTION_RELOAD_GRAPHICS, 0);
    tms_infof("Saving...");

    while (!P.can_set_settings) {
        tms_debugf("Waiting for can_set_settings...");
        SDL_Delay(1);
    }

    char tmp[64];
    settings["enable_shadows"]->v.b = (bool)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings_enable_shadows));
    settings["enable_ao"]->v.b = (bool)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings_enable_ao));
    settings["enable_bloom"]->v.b = (bool)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings_enable_bloom));
    settings["postprocess"]->v.b = (bool)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings_enable_bloom));
    settings["shadow_quality"]->v.u8 = (uint8_t)gtk_spin_button_get_value(settings_shadow_quality);

    /* Graphics */
    for (int x=0; x<settings_num_graphic_rows; ++x) {
        struct table_setting_row *r = &settings_graphic_rows[x];
        save_setting_row(r);
    }

    settings["postprocess"]->v.b = settings["enable_bloom"]->v.b;

    /* Audio */
    for (int x=0; x<settings_num_audio_rows; ++x) {
        struct table_setting_row *r = &settings_audio_rows[x];
        save_setting_row(r);
    }

    /* Controls */
    for (int x=0; x<settings_num_control_rows; ++x) {
        struct table_setting_row *r = &settings_control_rows[x];
        save_setting_row(r);
    }

    /* Interface */
    for (int x=0; x<settings_num_interface_rows; ++x) {
        struct table_setting_row *r = &settings_interface_rows[x];
        save_setting_row(r);
    }

    sm::load_settings();

    strcpy(tmp, get_cb_val(settings_shadow_res));
    char *x = strchr(tmp, 'x');
    if (x == NULL) {
        settings["shadow_map_resx"]->v.i = _tms.window_width;
        settings["shadow_map_resy"]->v.i = _tms.window_height;
    } else {
        char *res_x = (char*)malloc(64);
        char *res_y = (char*)malloc(64);
        int pos = x-tmp;

        strncpy(res_x, tmp, pos);
        strcpy(res_y, tmp+pos+1);
        res_x[pos] = '\0';

        //tms_infof("Setting shadow map to '%s'x'%s'", res_x, res_y);
        settings["shadow_map_resx"]->v.i = atoi(res_x);
        settings["shadow_map_resy"]->v.i = atoi(res_y);

        free(res_x);
        free(res_y);
    }

    strcpy(tmp, get_cb_val(settings_ao_res));
    x = strchr(tmp, 'x');
    if (x != NULL) {
        char *res = (char*)malloc(64);
        int pos = x-tmp;

        strncpy(res, tmp, pos);
        res[pos] = '\0';

        //tms_infof("Setting ao map to '%s'x'%s'", res, res);
        settings["ao_map_res"]->v.i = atoi(res);

        free(res);
    }

    settings["control_type"]->v.u8 = gtk_combo_box_get_active(GTK_COMBO_BOX(settings_control_type));

    if (!settings.save()) {
        tms_errorf("Unable to save settings.");
    } else {
        tms_infof("Successfully saved settings to file.");
    }

    tms_infof("done!");

#ifdef TMS_BACKEND_WINDOWS
    SDL_EventState(SDL_SYSWMEVENT, settings["emulate_touch"]->is_true() ? SDL_ENABLE : SDL_DISABLE);
#endif

    P.can_reload_graphics = true;
}

/* SETTINGS LOAD */
void
on_settings_show(GtkWidget *wdg, void *unused)
{
    char tmp[64];

    gtk_spin_button_set_value(settings_shadow_quality, settings["shadow_quality"]->v.u8);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings_enable_shadows), settings["enable_shadows"]->v.b);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings_enable_ao), settings["enable_ao"]->v.b);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings_enable_bloom), settings["enable_bloom"]->v.b);

    /* Graphics */
    for (int x=0; x<settings_num_graphic_rows; ++x) {
        struct table_setting_row *r = &settings_graphic_rows[x];
        load_setting_row(r);
    }

    /* Audio */
    for (int x=0; x<settings_num_audio_rows; ++x) {
        struct table_setting_row *r = &settings_audio_rows[x];
        load_setting_row(r);
    }

    /* Controls */
    for (int x=0; x<settings_num_control_rows; ++x) {
        struct table_setting_row *r = &settings_control_rows[x];
        load_setting_row(r);
    }

    /* Interface */
    for (int x=0; x<settings_num_interface_rows; ++x) {
        struct table_setting_row *r = &settings_interface_rows[x];
        load_setting_row(r);
    }

    snprintf(tmp, 64, "%dx%d", settings["shadow_map_resx"]->v.i, settings["shadow_map_resy"]->v.i);
    if (settings["shadow_map_resx"]->v.i == _tms.window_width && settings["shadow_map_resy"]->v.i == _tms.window_height) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(settings_shadow_res), 0);
    } else {
        gint index = find_cb_val(settings_shadow_res, tmp);
        if (index != -1) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(settings_shadow_res), index);
        } else {
            gtk_combo_box_text_append_text(settings_shadow_res, tmp);

            index = find_cb_val(settings_shadow_res, tmp);
            if (index != -1) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(settings_shadow_res), index);
            } else {
                tms_errorf("Unable to get index for a value we just appended");
            }
        }
    }

    snprintf(tmp, 64, "%dx%d", settings["ao_map_res"]->v.i, settings["ao_map_res"]->v.i);
    if (settings["ao_map_res"]->v.i == _tms.window_width && settings["ao_map_res"]->v.i == _tms.window_height) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(settings_ao_res), 0);
    } else {
        gint index = find_cb_val(settings_ao_res, tmp);
        if (index != -1) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(settings_ao_res), index);
        } else {
            gtk_combo_box_text_append_text(settings_ao_res, tmp);

            index = find_cb_val(settings_ao_res, tmp);
            if (index != -1) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(settings_ao_res), index);
            } else {
                tms_errorf("Unable to get index for a value we just appended");
            }
        }
    }

    if (settings["control_type"]->v.u8 == 0) {
        strcpy(tmp, "Keyboard");
    } else if (settings["control_type"]->v.u8 == 1) {
        strcpy(tmp, "Keyboard+Mouse");
    } else {
        /* default to keyboard-only controls */
        strcpy(tmp, "Keyboard");
    }

    gint index = find_cb_val(settings_control_type, tmp);
    if (index != -1) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(settings_control_type), index);
    } else {
        gtk_combo_box_text_append_text(settings_control_type, tmp);

        index = find_cb_val(settings_control_type, tmp);
        if (index != -1) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(settings_control_type), index);
        } else {
            tms_errorf("Unable to get index for a value we just appended");
        }
    }
}

/** --Save and Save as copy **/
void
on_save_show(GtkWidget *wdg, void *unused)
{
    char tmp[257];
    memcpy(tmp, W->level.name, W->level.name_len);
    tmp[W->level.name_len] = '\0';
    gtk_entry_set_text(save_entry, tmp);

    gtk_widget_grab_focus(GTK_WIDGET(save_entry));
}

gboolean
on_save_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, save_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(save_window));
    } else if (btn_pressed(w, save_ok, user_data)) {
        if (gtk_entry_get_text_length(save_entry) > 0) {
            const char *name = gtk_entry_get_text(save_entry);
            int name_len = strlen(name);
            if (name_len == 0) {
                ui::message("Your level must have a name.");
                return false;
            }
            W->level.name_len = name_len;
            memcpy(W->level.name, name, name_len);

            tms_infof("set level name to %s", name);

            if (save_type == SAVE_COPY)
                P.add_action(ACTION_SAVE_COPY, 0);
            else
                P.add_action(ACTION_SAVE, 0);
            gtk_widget_hide(GTK_WIDGET(save_window));
        } else {
            gtk_label_set_text(save_status, "You must enter a name!");
        }
    }

    return false;
}

gboolean
on_save_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(GTK_WIDGET(save_window));
    else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(save_cancel))) {
            on_save_btn_click(GTK_WIDGET(save_cancel), NULL, GINT_TO_POINTER(1));
        } else {
            on_save_btn_click(GTK_WIDGET(save_ok), NULL, GINT_TO_POINTER(1));
        }
    }

    return false;
}

/** --Export **/
void
on_export_show(GtkWidget *wdg, void *unused)
{
    gtk_entry_set_text(export_entry, "");

    gtk_widget_grab_focus(GTK_WIDGET(export_entry));
}

gboolean
on_export_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, export_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(export_window));
    } else if (btn_pressed(w, export_ok, user_data)) {
        if (gtk_entry_get_text_length(export_entry) > 0) {
            char *name = strdup(gtk_entry_get_text(export_entry));
            tms_infof("set export name to %s", name);

            P.add_action(ACTION_EXPORT_OBJECT, name);
            gtk_widget_hide(GTK_WIDGET(export_window));
        } else {
            gtk_label_set_text(export_status, "You must enter a name!");
        }
    }

    return false;
}

gboolean
on_export_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(GTK_WIDGET(export_window));
    else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(export_cancel))) {
            on_export_btn_click(GTK_WIDGET(export_cancel), NULL, GINT_TO_POINTER(1));
        } else {
            on_export_btn_click(GTK_WIDGET(export_ok), NULL, GINT_TO_POINTER(1));
        }
    }

    return false;
}

/** --Tips Dialog **/
gboolean
on_tips_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape || key->keyval == GDK_KEY_Return) {
        gtk_widget_hide(w);
        return true;
    }

    return false;
}

/** --Info Dialog **/
void
on_info_show(GtkWidget *wdg, void *unused)
{
    gtk_label_set_text(info_text, _pass_info_descr);
    gtk_label_set_text(info_name, _pass_info_name);
}

gboolean
on_info_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape || key->keyval == GDK_KEY_Return) {
        gtk_widget_hide(w);
        return true;
    }

    return false;
}

/** --Confirm Dialog **/
void
on_confirm_show(GtkWidget *wdg, void *unused)
{
    gtk_label_set_markup(confirm_text, _pass_confirm_text);
    gtk_button_set_label(confirm_button1, _pass_confirm_button1);
    gtk_button_set_label(confirm_button2, _pass_confirm_button2);
    if (_pass_confirm_button3) {
        tms_infof("BUTTON3 EXISTS!!!!!!!!!!!!");
        gtk_button_set_label(confirm_button3, _pass_confirm_button3);
    } else {
        gtk_widget_hide(GTK_WIDGET(confirm_button3));
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(confirm_dna_sandbox_back), settings["dna_sandbox_back"]->v.b);

    switch (confirm_data.confirm_type) {
        case CONFIRM_TYPE_DEFAULT:
            gtk_widget_hide(GTK_WIDGET(confirm_dna_sandbox_back));
            break;

        case CONFIRM_TYPE_BACK_SANDBOX:
            gtk_widget_show(GTK_WIDGET(confirm_dna_sandbox_back));
            break;
    }

    gtk_widget_set_size_request(GTK_WIDGET(confirm_dialog), -1, -1);
}

gboolean
on_confirm_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    switch (key->keyval) {
        case GDK_KEY_Escape:
            gtk_dialog_response(confirm_dialog, GTK_RESPONSE_CANCEL);
            break;

        case GDK_KEY_Return:
            if (!gtk_widget_has_focus(GTK_WIDGET(confirm_button2))) {
                gtk_dialog_response(confirm_dialog, 1);
            }
            break;
    }

    return false;
}

/** --Alert Dialog **/
void
on_alert_show(GtkWidget *wdg, void *unused)
{
    // set text without markup
    // g_object_set(alert_dialog, "text", _alert_text, NULL);

    gtk_message_dialog_set_markup(alert_dialog, _alert_text);
}

gboolean
on_alert_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    switch (key->keyval) {
        case GDK_KEY_Escape:
            gtk_dialog_response(confirm_dialog, GTK_RESPONSE_CANCEL);
            break;

        case GDK_KEY_Return:
            gtk_dialog_response(confirm_dialog, GTK_RESPONSE_ACCEPT);
            break;
    }

    return false;
}

/** --Error Dialog **/
void
on_error_show(GtkWidget *wdg, void *unused)
{
    gtk_label_set_text(error_text, _pass_error_text);
}

gboolean
on_error_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape || key->keyval == GDK_KEY_Return) {
        gtk_widget_hide(w);
        return true;
    }

    return false;
}

/** --Level properties **/
static void
on_level_flag_toggled(GtkToggleButton *btn, gpointer _flag)
{
    bool toggled = gtk_toggle_button_get_active(btn);
    uint64_t flag = VOID_TO_UINT64(_flag);
    tms_debugf("flag: %" PRIu64, flag);

    switch (flag) {
        case LVL_ABSORB_DEAD_ENEMIES:
            gtk_widget_set_sensitive(GTK_WIDGET(lvl_enemy_absorb_time), toggled);
            break;
    }
}

gboolean
on_properties_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(lvl_cancel))) {
            gtk_button_clicked(lvl_cancel);
        } else if (gtk_widget_has_focus(GTK_WIDGET(lvl_descr))) {
            /* do nothing */
        } else {
            gtk_button_clicked(lvl_ok);
        }
    }

    return false;

}

void
refresh_borders()
{
    char tmp[128];
    sprintf(tmp, "%d", W->level.size_x[0]);
    gtk_entry_set_text(lvl_width_left, tmp);

    sprintf(tmp, "%d", W->level.size_x[1]);
    gtk_entry_set_text(lvl_width_right, tmp);

    sprintf(tmp, "%d", W->level.size_y[0]);
    gtk_entry_set_text(lvl_height_down, tmp);

    sprintf(tmp, "%d", W->level.size_y[1]);
    gtk_entry_set_text(lvl_height_up, tmp);

}

/**
 * Get stuff from the currently loaded level and fill in the fields
 **/
void
on_properties_show(GtkWidget *wdg, void *unused)
{
    char *current_descr;
    char current_name[257];
    char tmp[128];

    current_descr = (char*)malloc(W->level.descr_len+1);
    memcpy(current_descr, W->level.descr, W->level.descr_len);
    current_descr[W->level.descr_len] = '\0';
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(lvl_descr);
    gtk_text_buffer_set_text(text_buffer, current_descr, -1);

    memcpy(current_name, W->level.name, W->level.name_len);
    current_name[W->level.name_len] = '\0';
    gtk_entry_set_text(lvl_title, current_name);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lvl_radio_adventure), (W->level.type == LCAT_ADVENTURE));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lvl_radio_custom), (W->level.type == LCAT_CUSTOM));

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(publish_locked), W->level.visibility == LEVEL_LOCKED);

    refresh_borders();

    gtk_spin_button_set_value(lvl_gx, W->level.gravity_x);
    gtk_spin_button_set_value(lvl_gy, W->level.gravity_y);

    /* Gameplay */
    sprintf(tmp, "%u", W->level.final_score);
    gtk_entry_set_text(lvl_score, tmp);

    if (W->level.version >= 7) {
        gtk_widget_set_sensitive(GTK_WIDGET(lvl_show_score), true);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lvl_show_score), W->level.show_score);
        gtk_widget_set_sensitive(GTK_WIDGET(lvl_pause_on_win), true);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lvl_pause_on_win), W->level.pause_on_finish);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(lvl_show_score), false);
        gtk_widget_set_sensitive(GTK_WIDGET(lvl_pause_on_win), false);
    }

    if (W->level.version >= 9) {
        /* TODO: Check current game mode and see if these should be enabled or not.
         * also add an on_click to the type radio buttons which updates this */
        tms_infof("flags: %" PRIu64, W->level.flags);
        for (int x=0; x<num_gtk_level_properties; ++x) {
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_level_properties[x].checkbutton), true);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_level_properties[x].checkbutton), ((uint64_t)(W->level.flags & gtk_level_properties[x].flag) != 0));
        }
    } else {
        for (int x=0; x<num_gtk_level_properties; ++x) {
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_level_properties[x].checkbutton), false);
        }
    }

    char vv[32];

    if (W->level.version == LEVEL_VERSION) {
        snprintf(vv, 31, "%d (latest)", LEVEL_VERSION);
        gtk_button_set_label(GTK_BUTTON(lvl_upgrade), vv);
        gtk_widget_set_sensitive(GTK_WIDGET(lvl_upgrade), false);
    } else {
        snprintf(vv, 31, "%d (Upgrade to %d)", W->level.version, LEVEL_VERSION);
        gtk_button_set_label(GTK_BUTTON(lvl_upgrade), vv);
        gtk_widget_set_sensitive(GTK_WIDGET(lvl_upgrade), true);

    }

    gtk_range_set_value(GTK_RANGE(lvl_enemy_absorb_time), (double)W->level.dead_enemy_absorb_time);
    gtk_range_set_value(GTK_RANGE(lvl_player_respawn_time), (double)W->level.time_before_player_can_respawn);

    gtk_widget_set_sensitive(GTK_WIDGET(lvl_enemy_absorb_time), W->level.flag_active(LVL_ABSORB_DEAD_ENEMIES));

    uint8_t vel_iter = W->level.velocity_iterations;
    uint8_t pos_iter = W->level.position_iterations;
    gtk_range_set_value(GTK_RANGE(lvl_vel_iter), (double)vel_iter);

    gtk_range_set_value(GTK_RANGE(lvl_pos_iter), (double)pos_iter);

    gtk_range_set_value(GTK_RANGE(lvl_prismatic_tol), W->level.prismatic_tolerance);
    gtk_range_set_value(GTK_RANGE(lvl_pivot_tol), W->level.pivot_tolerance);

    gtk_range_set_value(GTK_RANGE(lvl_linear_damping), W->level.linear_damping);
    gtk_range_set_value(GTK_RANGE(lvl_angular_damping), W->level.angular_damping);
    gtk_range_set_value(GTK_RANGE(lvl_joint_friction), W->level.joint_friction);

    gtk_combo_box_set_active(GTK_COMBO_BOX(lvl_bg), W->level.bg);
    new_bg_color = W->level.bg_color;

    {
        GdkRGBA bg_color;
        float r, g, b, a;

        unpack_rgba(W->level.bg_color, &r, &g, &b, &a);

        bg_color.red   = r;
        bg_color.green = g;
        bg_color.blue  = b;
        bg_color.alpha = 1.0;

        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(lvl_bg_color), &bg_color);
    }

    for (const int *ptr = colored_bgs; ; ++ptr) {
        if (*ptr == -1 || *ptr == W->level.bg) {
            gtk_widget_set_visible(GTK_WIDGET(lvl_bg_color), *ptr != -1);
            break;
        }
    }

    free(current_descr);
}

void
on_frequency_show(GtkWidget *wdg, void *unused)
{
    GtkTreeIter iter;
    std::map<uint32_t, entity*> all_entities;
    // <Frequency, <Num Receivers, Num Transmitters> >
    std::map<int, std::pair<int, int> > frequencies;

    /* Reset widgets */
    gtk_spin_button_set_value(frequency_value, 0);
    gtk_list_store_clear(GTK_LIST_STORE(frequency_treemodel));

    /* Fetch current frequency from selection */
    if (G->selection.e && G->selection.e->is_wireless()) {

        gtk_spin_button_set_value(frequency_value, (gdouble)G->selection.e->properties[0].v.i);
    }

    all_entities = W->get_all_entities();
    for (std::map<uint32_t, entity*>::iterator i = all_entities.begin();
            i != all_entities.end(); i++) {
        entity *e = i->second;

        if (e->g_id == O_RECEIVER) { /* Receiver */
            std::pair<std::map<int, std::pair<int, int> >::iterator, bool> ret;
            std::pair<int, int> data = std::make_pair(1, 0);
            ret = frequencies.insert(std::pair<int, std::pair<int, int> >((int)e->properties[0].v.i, data));

            if (!ret.second) {
                ((ret.first)->second).first += 1;
            }
        } else if (e->g_id == O_TRANSMITTER || e->g_id == O_MINI_TRANSMITTER) { /* (Mini) Transmitter */
            std::pair<std::map<int, std::pair<int, int> >::iterator, bool> ret;
            std::pair<int, int> data = std::make_pair(0, 1);
            ret = frequencies.insert(std::pair<int, std::pair<int, int> >((int)e->properties[0].v.i, data));

            if (!ret.second) {
                ((ret.first)->second).second += 1;
            }
        } else if (e->g_id == O_PIXEL && e->properties[4].v.i != 0) {
            /* Pixel */
            std::pair<std::map<int, std::pair<int, int> >::iterator, bool> ret;
            std::pair<int, int> data = std::make_pair(1, 0);
            ret = frequencies.insert(std::pair<int, std::pair<int, int> >((int)e->properties[4].v.i, data));

            if (!ret.second) {
                ((ret.first)->second).first += 1;
            }
        }
    }

    for (std::map<int, std::pair<int, int> >::iterator i = frequencies.begin();
            i != frequencies.end(); ++i) {
        gtk_list_store_append(GTK_LIST_STORE(frequency_treemodel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(frequency_treemodel), &iter,
                FC_FREQUENCY,       i->first,
                FC_RECEIVERS,       (i->second).first,
                FC_TRANSMITTERS,    (i->second).second,
                -1
                );
    }

    gtk_widget_grab_focus(GTK_WIDGET(frequency_value));
}

void
activate_open_state(GtkMenuItem *i, gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(open_state_window));
}

void
activate_open(GtkMenuItem *i, gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(open_window));
}

void
activate_prompt_settings(GtkMenuItem *i, gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(prompt_settings_dialog));
}

void
activate_object(GtkMenuItem *i, gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(object_window));
}

void
activate_export(GtkMenuItem *i, gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(export_window));
}

void
activate_controls(GtkMenuItem *i, gpointer unused)
{
    G->render_controls = true;
}

void
activate_restart_level(GtkMenuItem *i, gpointer unused)
{
    P.add_action(ACTION_RESTART_LEVEL, 0);
}

void
activate_back(GtkMenuItem *i, gpointer unused)
{
    P.add_action(ACTION_BACK, 0);
}

void
activate_save(GtkMenuItem *i, gpointer unused)
{
    bool ask_for_new_name = false;

    if (W->level.name_len == 0 || strcmp(W->level.name, "<no name>") == 0) {
        ask_for_new_name = true;
    }

    if (ask_for_new_name) {
        save_type = SAVE_REGULAR;
        gtk_widget_show_all(GTK_WIDGET(save_window));
    } else {
        P.add_action(ACTION_SAVE, 0);
    }
}

void
activate_save_copy(GtkMenuItem *i, gpointer unused)
{
    save_type = SAVE_COPY;
    gtk_widget_show_all(GTK_WIDGET(save_window));
}

/* When activate_settings is called normally, userdata is an uint8_t with the value 0.
 * That means the graphics should reload and return to the G screen
 * When activate_settings is called via open_dialog(DIALOG_SETTINGS), userdata is 1.
 * That means RELOAD_GRAPHICS should return to the main menu instead. */
void
activate_settings(GtkMenuItem *i, gpointer userdata)
{
    gint result = gtk_dialog_run(settings_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        save_settings();
    }

    gtk_widget_hide(GTK_WIDGET(settings_dialog));
}

void
activate_publish(GtkMenuItem *i, gpointer unused)
{
    gint result = gtk_dialog_run(publish_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        const char *name = gtk_entry_get_text(publish_name);
        int name_len = strlen(name);
        if (name_len == 0) {
            ui::message("You cannot publish a level without a name.");
            activate_publish(0,0);
            return;
        }
        W->level.name_len = name_len;
        memcpy(W->level.name, name, name_len);

        GtkTextIter start, end;
        GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(publish_descr);
        char *descr;

        gtk_text_buffer_get_bounds(text_buffer, &start, &end);

        descr = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
        int descr_len = strlen(descr);

        if (descr_len > 0) {
            W->level.descr_len = descr_len;
            W->level.descr = (char*)realloc(W->level.descr, descr_len+1);

            memcpy(W->level.descr, descr, descr_len);
            descr[descr_len] = '\0';
        } else
            W->level.descr_len = 0;

        W->level.visibility = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(publish_locked)) ? LEVEL_LOCKED : LEVEL_VISIBLE;

        tms_infof("Setting level name to:  %s", name);
        tms_infof("Setting level descr to: %s", descr);

        P.add_action(ACTION_PUBLISH, 0);
    }

    gtk_widget_hide(GTK_WIDGET(publish_dialog));
}

/** --Multi config **/
static void
on_multi_config_show(GtkWidget *wdg, void *unused)
{
    gtk_range_set_value(GTK_RANGE(multi_config_joint_strength), 1.0);

    bool any_entity_locked = false;

    bool enabled_tabs[NUM_MULTI_CONFIG_TABS];
    for (int x=0; x<NUM_MULTI_CONFIG_TABS; ++x) {
        enabled_tabs[x] = false;
    }

    enabled_tabs[TAB_JOINT_STRENGTH]            = true;
    enabled_tabs[TAB_CONNECTION_RENDER_TYPE]    = true;
    enabled_tabs[TAB_MISCELLANEOUS]             = true;

    if (G->state.sandbox && W->is_paused() && !G->state.test_playing) {
        if (G->get_mode() == GAME_MODE_MULTISEL && G->selection.m) {
            for (std::set<entity*>::iterator i = G->selection.m->begin();
                    i != G->selection.m->end(); i++) {
                entity *e = *i;

                if (e->flag_active(ENTITY_IS_PLASTIC)) {
                    enabled_tabs[TAB_PLASTIC_COLOR] = true;
                    enabled_tabs[TAB_PLASTIC_DENSITY] = true;
                }

                if (e->flag_active(ENTITY_IS_LOCKED)) {
                    any_entity_locked = true;
                }
            }
        }
    }

    for (int x=0; x<NUM_MULTI_CONFIG_TABS; ++x) {
        GtkWidget *page = gtk_notebook_get_nth_page(multi_config_nb, x);

        if (!enabled_tabs[x]) {
            gtk_widget_hide(page);
        } else {
            gtk_widget_show(page);
        }
    }

    gtk_widget_set_sensitive(GTK_WIDGET(multi_config_unlock_all), any_entity_locked);
}

static gboolean
on_multi_config_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, multi_config_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(multi_config_window));
    } else if (btn_pressed(w, multi_config_apply, user_data)) {
        tms_debugf("cur tab: %d", multi_config_cur_tab);

        switch (multi_config_cur_tab) {
            case TAB_JOINT_STRENGTH:
                {
                    float val = tclampf(gtk_range_get_value(GTK_RANGE(multi_config_joint_strength)), 0.f, 1.f);
                    P.add_action(ACTION_MULTI_JOINT_STRENGTH, INT_TO_VOID(val * 100.f));
                }
                break;

            case TAB_PLASTIC_COLOR:
                {
                    GdkRGBA color;
                    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(multi_config_plastic_color), &color);

                    tvec4 *vec = (tvec4*)malloc(sizeof(tvec4));
                    vec->r = color.red;
                    vec->g = color.green;
                    vec->b = color.blue;
                    vec->a = 1.0f;

                    P.add_action(ACTION_MULTI_PLASTIC_COLOR, (void*)vec);
                }
                break;

            case TAB_PLASTIC_DENSITY:
                {
                    float val = tclampf(gtk_range_get_value(GTK_RANGE(multi_config_plastic_density)), 0.f, 1.f);
                    P.add_action(ACTION_MULTI_PLASTIC_DENSITY, INT_TO_VOID(val * 100.f));
                }
                break;

            case TAB_CONNECTION_RENDER_TYPE:
                {
                    uint8_t render_type = CONN_RENDER_DEFAULT;

                    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_config_render_type_normal))) {
                        render_type = CONN_RENDER_DEFAULT;
                    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_config_render_type_small))) {
                        render_type = CONN_RENDER_SMALL;
                    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_config_render_type_hide))) {
                        render_type = CONN_RENDER_HIDE;
                    }

                    P.add_action(ACTION_MULTI_CHANGE_CONNECTION_RENDER_TYPE, UINT_TO_VOID(render_type));
                }
                break;

            default:
                tms_errorf("Unknown multi config tab: %d", multi_config_cur_tab);
                return false;
                break;
        }

        gtk_widget_hide(GTK_WIDGET(multi_config_window));
    } else if (btn_pressed(w, multi_config_unlock_all, user_data)) {
        P.add_action(ACTION_MULTI_UNLOCK_ALL, 0);

        gtk_widget_hide(GTK_WIDGET(multi_config_window));
    } else if (btn_pressed(w, multi_config_disconnect_all, user_data)) {
        P.add_action(ACTION_MULTI_DISCONNECT_ALL, 0);

        gtk_widget_hide(GTK_WIDGET(multi_config_window));
    }

    return false;
}

static void
on_multi_config_tab_changed(GtkNotebook *nb, GtkWidget *page, gint tab_num, gpointer unused)
{
    multi_config_cur_tab = tab_num;

    gtk_widget_set_sensitive(GTK_WIDGET(multi_config_apply), (tab_num != TAB_MISCELLANEOUS));
}

/** --Login **/
static void
on_login_show(GtkWidget *wdg, void *unused)
{
    gtk_widget_set_sensitive(GTK_WIDGET(login_btn_log_in), true);

    gtk_entry_set_text(login_username, "");
    gtk_entry_set_text(login_password, "");

    gtk_label_set_text(login_status, "");

    gtk_widget_grab_focus(GTK_WIDGET(login_username));
}

void
on_login_hide(GtkWidget *wdg, void *unused)
{
    tms_infof("login hiding");
    prompt_is_open = 0;
}

gboolean
on_login_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, login_btn_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(login_window));
    } else if (btn_pressed(w, login_btn_log_in, user_data)) {
        if (gtk_entry_get_text_length(login_username) > 0 &&
            gtk_entry_get_text_length(login_password) > 0) {
            struct login_data *data = (struct login_data*)malloc(sizeof(struct login_data));
            strcpy(data->username, gtk_entry_get_text(login_username));
            strcpy(data->password, gtk_entry_get_text(login_password));

            gtk_widget_set_sensitive(GTK_WIDGET(login_btn_log_in), false);
            gtk_label_set_text(login_status, "Logging in...");

            P.add_action(ACTION_LOGIN, (void*)data);
        } else {
            gtk_label_set_text(login_status, "Enter data into both fields.");
        }
    } else if (btn_pressed(w, login_btn_register, user_data)) {
        COMMUNITY_URL("register");
        ui::open_url(url);
    }

    return false;
}

gboolean
on_login_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(login_btn_cancel))) {
            on_login_btn_click(GTK_WIDGET(login_btn_cancel), NULL, GINT_TO_POINTER(1));
        } else {
            on_login_btn_click(GTK_WIDGET(login_btn_log_in), NULL, GINT_TO_POINTER(1));
        }
    }

    return false;
}

void
activate_principiawiki(GtkMenuItem *i, gpointer unused)
{
    ui::open_url("https://principia-web.se/wiki/");
}

void
activate_gettingstarted(GtkMenuItem *i, gpointer unused)
{
    ui::open_url("https://principia-web.se/wiki/Getting_Started");
}

void
activate_login(GtkMenuItem *i, gpointer unused)
{
    prompt_is_open = 1;
    P.focused = 0;
    gtk_widget_show_all(GTK_WIDGET(login_window));
}

void
editor_menu_back_to_menu(GtkMenuItem *i, gpointer unused)
{
    P.add_action(ACTION_GOTO_MAINMENU, 0);
}

static void show_grab_focus(GtkWidget *w, gpointer user_data)
{
    GdkWindow *w_window = gtk_widget_get_window(w);
    GdkDisplay* display = gdk_display_get_default();
    GdkSeat* seat = gdk_display_get_default_seat(display);
    if ((gdk_seat_get_capabilities(seat) & GDK_SEAT_CAPABILITY_KEYBOARD) == 0) {
        tms_warnf("seat has no keyboard capability");
        return;
    }
    while (gdk_seat_grab(
        seat, w_window,
        GDK_SEAT_CAPABILITY_KEYBOARD,
        FALSE,
        NULL, NULL, NULL, NULL
    ) != GDK_GRAB_SUCCESS) {
        SDL_Delay(10);
    }
}

void activate_quickadd(GtkWidget *i, gpointer unused);

gboolean
keypress_quickadd(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    GValue s = {0};
    GValue e = {0};

    g_value_init(&s, G_TYPE_UINT);
    g_value_init(&e, G_TYPE_UINT);

    g_object_get_property(G_OBJECT(w), "cursor-position", &s);
    g_object_get_property(G_OBJECT(w), "selection-bound", &e);

    guint sel = g_value_get_uint(&s)+g_value_get_uint(&e);

    if (key->keyval == GDK_KEY_Escape) {
        gtk_widget_hide(GTK_WIDGET(quickadd_window));
    } else if (key->keyval == GDK_KEY_space
            && sel == strlen(gtk_entry_get_text(GTK_ENTRY(w)))) {
        /* if space is pressed and the whole string is selected,
         * activate it */
        activate_quickadd(w, 0);
        return true;
    }

    gtk_entry_completion_complete(gtk_entry_get_completion(GTK_ENTRY(w)));

    return false;
}

/** --Quickadd **/
static gboolean
match_selected_quickadd(GtkEntryCompletion *widget,
  GtkTreeModel       *model,
  GtkTreeIter        *iter,
  gpointer            user_data)
{
    gtk_widget_hide(GTK_WIDGET(quickadd_window));

    guint _gid;
    guint _type;
    gtk_tree_model_get(model, iter,
                       0, &_gid,
                       2, &_type,
                       -1);

    uint32_t gid = (uint32_t)_gid;
    tms_infof("selected gid %d", gid);

    switch (_type) {
        case LF_MENU:
            P.add_action(ACTION_CONSTRUCT_ENTITY, gid);
            break;

        case LF_ITEM:
            P.add_action(ACTION_CONSTRUCT_ITEM, gid);
            break;

        case LF_DECORATION:
            P.add_action(ACTION_CONSTRUCT_DECORATION, gid);
            break;
    }

    return false;
}

void
refresh_quickadd()
{
    GtkListStore *list = GTK_LIST_STORE(gtk_entry_completion_get_model(gtk_entry_get_completion(quickadd_entry)));
    GtkTreeIter iter;
    int n = 0;
    for (int x=0; x<menu_objects.size(); x++) {
        const struct menu_obj &mo = menu_objects[x];

        gtk_list_store_append(list, &iter);
        gtk_list_store_set(list, &iter,
                0, mo.e->g_id,
                1, mo.e->get_name(),
                2, LF_MENU,
                -1
                );

        if (!mo.e->is_static()) {
            tchest_translations[mo.e->g_id] = n++;
            gtk_combo_box_text_append_text(tchest_entity, mo.e->get_name());
        }
    }
    for (int x=0; x<NUM_ITEMS; ++x) {
        const struct item_option &io = item_options[x];

        char tmp[512];
        snprintf(tmp, 511, "%s (Item)", io.name);

        gtk_list_store_append(list, &iter);
        gtk_list_store_set(list, &iter,
                0, x,
                1, tmp,
                2, LF_ITEM,
                -1
                );
    }
    for (int x=0; x<NUM_DECORATIONS; ++x) {
        const struct decoration_info &di = decorations[x];

        char tmp[512];
        snprintf(tmp, 511, "%s (Decoration)", di.name);

        gtk_list_store_append(list, &iter);
        gtk_list_store_set(list, &iter,
                0, x,
                1, tmp,
                2, LF_DECORATION,
                -1
                );
    }
}

void
activate_quickadd(GtkWidget *i, gpointer unused)
{
    /* there seems to be absolutely no way of retrieving the top completion entry...
     * we have to find it manually */

    const char *search = gtk_entry_get_text(quickadd_entry);
    int len = strlen(search);
    int found_arg = -1;
    int found_score = -10000000;
    int found_lf = -1;

    tms_debugf("Looking for %s", search);

    for (int i=0; i<NUM_LF; ++i) {
        switch (i) {
            case LF_MENU:
                {
                    for (int x=0; x<menu_objects.size(); ++x) {
                        int diff = strncasecmp(search, menu_objects[x].e->get_name(), len);
                        /* Only look for 'exact' matches, meaning they must contain that exact string in the beginning
                         * i.e. 'sub' fits 'sub' and 'sublayer plank' */

                        if (diff == 0) {
                            /* Now we find out what the real difference between the match is */
                            int score = strcasecmp(search, menu_objects[x].e->get_name());

                            if (score == 0) {
                                /* A return value of 0 means it's an exacth match, i.e. 'sub' == 'sub' */
                                found_arg = menu_objects[x].e->g_id;
                                found_score = 0;
                                found_lf = i;
                                break;
                            } else if (score < 0 && score > found_score) {
                                /* Otherwise, we could settle for this half-match, i.e. 'sub' == 'sublayer plank' */
                                found_arg = menu_objects[x].e->g_id;
                                found_score = score;
                                found_lf = i;
                            }
                        }
                    }
                }
                break;

            case LF_ITEM:
                {
                    for (int x=0; x<NUM_ITEMS; ++x) {
                        const struct item_option &io = item_options[x];

                        int diff = strncasecmp(search, io.name, len);
                        /* Only look for 'exact' matches, meaning they must contain that exact string in the beginning
                         * i.e. 'sub' fits 'sub' and 'sublayer plank' */

                        if (diff == 0) {
                            /* Now we find out what the real difference between the match is */
                            int score = strcasecmp(search, io.name);

                            if (score == 0) {
                                /* A return value of 0 means it's an exacth match, i.e. 'sub' == 'sub' */
                                found_arg = x;
                                found_score = 0;
                                found_lf = i;
                                break;
                            } else if (score < 0 && score > found_score) {
                                /* Otherwise, we could settle for this half-match, i.e. 'sub' == 'sublayer plank' */
                                found_arg = x;
                                found_score = score;
                                found_lf = i;
                            }
                        }
                    }

                }
                break;
            case LF_DECORATION:
                {
                    for (int x=0; x<NUM_DECORATIONS; ++x) {
                        int diff = strncasecmp(search, decorations[x].name, len);
                        /* Only look for 'exact' matches, meaning they must contain that exact string in the beginning
                         * i.e. 'sub' fits 'sub' and 'sublayer plank' */

                        if (diff == 0) {
                            /* Now we find out what the real difference between the match is */
                            int score = strcasecmp(search, decorations[x].name);

                            if (score == 0) {
                                /* A return value of 0 means it's an exacth match, i.e. 'sub' == 'sub' */
                                found_arg = x;
                                found_score = 0;
                                found_lf = i;
                                break;
                            } else if (score < 0 && score > found_score) {
                                /* Otherwise, we could settle for this half-match, i.e. 'sub' == 'sublayer plank' */
                                found_arg = x;
                                found_score = score;
                                found_lf = i;
                            }
                        }
                    }

                }
                break;
        }

        if (found_score == 0) break;
    }

    if (found_arg >= 0) {
        switch (found_lf) {
            case LF_MENU:
                P.add_action(ACTION_CONSTRUCT_ENTITY, found_arg);
                break;

            case LF_ITEM:
                P.add_action(ACTION_CONSTRUCT_ITEM, found_arg);
                break;

            case LF_DECORATION:
                P.add_action(ACTION_CONSTRUCT_DECORATION, found_arg);
                break;
        }
    } else {
        tms_infof("'%s' matched no entity name", search);
    }

    gtk_widget_hide(GTK_WIDGET(quickadd_window));
}

gboolean
on_goto_menu_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    GtkAccelGroup *accel_group = gtk_menu_get_accel_group(editor_menu);

    if (key->keyval >= GDK_KEY_1 && key->keyval <= GDK_KEY_9) {
        for (std::deque<struct goto_mark*>::iterator it = editor_menu_marks.begin();
                it != editor_menu_marks.end(); ++it) {
            const struct goto_mark *mark = *it;
            GtkMenuItem *item = mark->menuitem;

            if (mark->key == key->keyval) {
                gtk_menu_item_activate(item);
                gtk_widget_hide(GTK_WIDGET(editor_menu));
                return true;
            }
        }
    }

    return false;
}

gboolean
on_menu_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape) {
        gtk_widget_hide(w);
    } else {
        /* redirect the event to tms? */
        /*
        struct tms_event e;
        e.type = TMS_EV_KEY_PRESS;

        return true;
        */
    }

    return false;
}

gboolean
on_frequency_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, frequency_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(frequency_window));
    } else if (btn_pressed(w, frequency_ok, user_data)) {
        entity *e = G->selection.e;

        if (e && e->is_wireless()) {
            gtk_spin_button_update(frequency_value);

            e->set_property(0, (uint32_t)gtk_spin_button_get_value(frequency_value));
            ui::messagef("Frequency set to %u", e->properties[0].v.i);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            gtk_widget_hide(GTK_WIDGET(frequency_window));
        }
    }

    return false;
}

gboolean
on_frequency_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    switch (key->keyval) {
        case GDK_KEY_Escape:
            gtk_widget_hide(w);
            return false;

        case GDK_KEY_Return:
            if (gtk_widget_has_focus(GTK_WIDGET(frequency_cancel)))
                on_frequency_click(GTK_WIDGET(frequency_cancel), NULL, GINT_TO_POINTER(1));
            else
                on_frequency_click(GTK_WIDGET(frequency_ok), NULL, GINT_TO_POINTER(1));

            return true;
            break;
    }

    return false;
}

const gchar* css_global = R"(
    .display-cell {
        border: none;
        box-shadow: none;
        border-radius: 0;
        background: #101010;
    }

    .display-cell:checked {
        background: #5fbd5a;
    }

    .code-editor {
        font-family: "Cascadia Mono Normal", "Cascadia Mono", "Ubuntu Mono Normal", "Ubuntu Mono", monospace, mono;
        font-size: 1.25em;
    }
)";

void load_gtk_css() {
    //Load global CSS
    {
        GtkCssProvider* css_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(
            css_provider,
            css_global,
            -1, NULL
        );
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }

    //Try to load debug.css in debug builds
    #ifdef DEBUG
    {
        GtkCssProvider* css_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_path (
            css_provider,
            "debug.css",
            NULL
        );
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    #endif
}

int _gtk_loop(void *p)
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) return T_OK;
#endif

    gtk_init(NULL, NULL);

    //Load CSS themes
    load_gtk_css();

    g_object_set(
        gtk_settings_get_default(),
        "gtk-application-prefer-dark-theme", true,
        "gtk-tooltip-timeout", 100,
        NULL
    );

    GtkDialog *dialog;

    /** --Play menu **/
    {
        play_menu = GTK_MENU(gtk_menu_new());

        add_menuitem(play_menu, "Controls", activate_controls);
        add_menuitem(play_menu, "Restart level", activate_restart_level);
        add_menuitem(play_menu, "Back", activate_back);
    }

    /** --Menu **/

    /**
     * menu header: -x, y-
     * Move player here (if adventure, creates a default robot if no player exists)
     * Move selected object here
     * -separator-
     * Go to: 0, 0
     * Go to: Player
     * Go to: Last created entity
     * Go to: Last camera position (before previous go to)
     * Go to: Plank 1543 (see marked entity note below)
     * Go to: Robot 1337
     * -separator-
     * -default menu items, open save, etc-
     *
     *  If on an entity:
     * menu header: -entity id, gid, position, angle-
     * Set as player (if adventure and clicked a creature)
     * Mark entity (marks the entity with a flag and adds it to the Go to list)
     * Unmark entity
     **/
    {
        editor_menu = GTK_MENU(gtk_menu_new());
        editor_menu_go_to_menu = GTK_MENU(gtk_menu_new());

        editor_menu_header = add_menuitem(editor_menu, "HEADER");

        /* --------------------------- */

        editor_menu_move_here_player = add_menuitem(editor_menu, "Move player here", editor_menu_activate);

        editor_menu_move_here_object = add_menuitem(editor_menu, "Move selected object here", editor_menu_activate);

        editor_menu_go_to = add_menuitem_m(editor_menu, "_Go to:");

        GtkAccelGroup *accel_group = gtk_accel_group_new();
        gtk_menu_set_accel_group(editor_menu, accel_group);

        gtk_menu_item_set_submenu(editor_menu_go_to, GTK_WIDGET(editor_menu_go_to_menu));
        {
            editor_menu_marks.push_back(new goto_mark(
                MARK_POSITION,
                "0, 0",
                0,
                tvec2f(0.f, 0.f)
            ));
            editor_menu_marks.push_back(new goto_mark(
                MARK_PLAYER,
                "Player",
                0,
                tvec2f(0.f, 0.f)
            ));
            editor_menu_marks.push_back(editor_menu_last_created);
            editor_menu_marks.push_back(editor_menu_last_cam_pos);

            for (std::deque<struct goto_mark*>::iterator it = editor_menu_marks.begin();
                    it != editor_menu_marks.end(); ++it) {
                struct goto_mark *mark = *it;
                mark->menuitem = add_menuitem(editor_menu_go_to_menu, mark->label, editor_mark_activate, (gpointer)mark);
            }

            refresh_mark_menuitems();
        }

        /* --------------------------- */

        editor_menu_set_as_player = add_menuitem(editor_menu, "Set as player", editor_menu_activate);
        editor_menu_toggle_mark_entity = add_menuitem(editor_menu, "Mark entity", editor_menu_activate);

        /* --------------------------- */

        add_separator(editor_menu);

        editor_menu_lvl_prop = add_menuitem_m(editor_menu, "Level _properties", editor_menu_activate);

        add_menuitem_m(editor_menu, "_New level", activate_new_level);
        editor_menu_save = add_menuitem_m(editor_menu, "_Save", activate_save);
        editor_menu_save_copy = add_menuitem_m(editor_menu, "Save _copy", activate_save_copy);
        add_menuitem_m(editor_menu, "_Open", activate_open);

#ifdef BUILD_PKGMGR
        editor_menu_package_manager = add_menuitem(editor_menu, "Package manager", editor_menu_activate);
#endif

        editor_menu_publish = add_menuitem_m(editor_menu, "P_ublish online", activate_publish);

        editor_menu_settings = add_menuitem_m(editor_menu, "S_ettings", activate_settings);

        editor_menu_login = add_menuitem_m(editor_menu, "_Login", activate_login);

        add_menuitem_m(editor_menu, "_Back to menu", editor_menu_back_to_menu);
        add_menuitem(editor_menu, "Help: Principia Wiki", activate_principiawiki);
        add_menuitem(editor_menu, "Help: Getting Started", activate_gettingstarted);

        //g_signal_connect(editor_menu, "selection-done", G_CALLBACK(on_menu_select), 0);
        g_signal_connect(editor_menu, "key-press-event", G_CALLBACK(on_menu_keypress), 0);
        g_signal_connect(editor_menu_go_to_menu, "key-press-event", G_CALLBACK(on_goto_menu_keypress), 0);
    }

    /** --Open object **/
    {
        object_window = new_window_defaults("Import object", &on_object_show, &on_object_keypress);
        gtk_window_set_default_size(GTK_WINDOW(object_window), 600, 600);
        gtk_widget_set_size_request(GTK_WIDGET(object_window), 600, 600);
        gtk_window_set_resizable(GTK_WINDOW(object_window), true);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkListStore *store;

        store = gtk_list_store_new(OC_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

        object_treemodel = GTK_TREE_MODEL(store);

        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), OC_DATE, GTK_SORT_DESCENDING);

        object_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(object_treemodel));
        gtk_tree_view_set_search_column(object_treeview, OC_NAME);
        g_signal_connect(GTK_WIDGET(object_treeview), "row-activated", G_CALLBACK(activate_object_row), 0);

        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);

        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_box_set_spacing(GTK_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Open button */
        object_btn_open   = GTK_BUTTON(gtk_button_new_with_label("Open"));
        g_signal_connect(object_btn_open, "clicked",
                G_CALLBACK(on_object_btn_click), 0);

        object_btn_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(object_btn_cancel, "clicked",
                G_CALLBACK(on_object_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(object_btn_open));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(object_btn_cancel));

        gtk_box_pack_start(content, GTK_WIDGET(ew), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(object_treeview));
        gtk_container_add(GTK_CONTAINER(object_window), GTK_WIDGET(content));

        add_text_column(object_treeview, "ID", OC_ID);
        add_text_column(object_treeview, "Name", OC_NAME);
        add_text_column(object_treeview, "Version", OC_VERSION);
        add_text_column(object_treeview, "Modified", OC_DATE);
    }

    /** --Open state **/
    {
        open_state_window = new_window_defaults("Load saved game", &on_open_state_show, &on_open_state_keypress);
        gtk_window_set_default_size(GTK_WINDOW(open_state_window), 600, 600);
        gtk_widget_set_size_request(GTK_WIDGET(open_state_window), 600, 600);
        gtk_window_set_resizable(GTK_WINDOW(open_state_window), true);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkListStore *store;

        store = gtk_list_store_new(OSC_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);

        open_state_treemodel = GTK_TREE_MODEL(store);

        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), OSC_DATE, GTK_SORT_DESCENDING);

        open_state_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(open_state_treemodel));
        gtk_tree_view_set_search_column(open_state_treeview, OSC_NAME);
        g_signal_connect(GTK_WIDGET(open_state_treeview), "row-activated", G_CALLBACK(activate_open_state_row), 0);

        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);

        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Open button */
        open_state_btn_open   = GTK_BUTTON(gtk_button_new_with_label("Open"));
        g_signal_connect(open_state_btn_open, "clicked",
                G_CALLBACK(on_open_state_btn_click), 0);

        open_state_btn_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(open_state_btn_cancel, "clicked",
                G_CALLBACK(on_open_state_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(open_state_btn_open));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(open_state_btn_cancel));

        gtk_box_pack_start(content, GTK_WIDGET(ew), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(open_state_treeview));
        gtk_container_add(GTK_CONTAINER(open_state_window), GTK_WIDGET(content));

        add_text_column(open_state_treeview, "Name", OSC_ID);
        add_text_column(open_state_treeview, "Modified", OSC_NAME);
    }

    /** --Open level **/
    {
        open_window = new_window_defaults("Open level", &on_open_show, &on_open_keypress);
        gtk_window_set_default_size(GTK_WINDOW(open_window), 600, 600);
        gtk_widget_set_size_request(GTK_WIDGET(open_window), 600, 600);
        gtk_window_set_resizable(GTK_WINDOW(open_window), true);

        open_menu = GTK_MENU(gtk_menu_new());

        open_menu_information = add_menuitem_m(open_menu, "_Information", open_menu_item_activated);
        open_menu_delete = add_menuitem_m(open_menu, "_Delete", open_menu_item_activated);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkListStore *store;

        store = gtk_list_store_new(OC_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

        open_treemodel = GTK_TREE_MODEL(store);

        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), OC_DATE, GTK_SORT_DESCENDING);

        open_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(open_treemodel));
        gtk_tree_view_set_search_column(open_treeview, OC_NAME);
        g_signal_connect(GTK_WIDGET(open_treeview), "row-activated", G_CALLBACK(activate_open_row), 0);
        g_signal_connect(GTK_WIDGET(open_treeview), "button-press-event", G_CALLBACK(open_row_button_press), 0);

        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);

        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Open button */
        open_btn_open   = GTK_BUTTON(gtk_button_new_with_label("Open"));
        g_signal_connect(open_btn_open, "clicked",
                G_CALLBACK(on_open_btn_click), 0);

        open_btn_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(open_btn_cancel, "clicked",
                G_CALLBACK(on_open_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(open_btn_open));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(open_btn_cancel));

        gtk_box_pack_start(content, GTK_WIDGET(ew), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(open_treeview));
        gtk_container_add(GTK_CONTAINER(open_window), GTK_WIDGET(content));

        add_text_column(open_treeview, "ID", OC_ID);
        add_text_column(open_treeview, "Name", OC_NAME);
        add_text_column(open_treeview, "Version", OC_VERSION);
        add_text_column(open_treeview, "Modified", OC_DATE);
    }

    /** --Package name dialog **/
    {
        pkg_name_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Create new package",
                0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
                NULL, NULL));

        apply_dialog_defaults(pkg_name_dialog);

        pkg_name_ok = GTK_BUTTON(gtk_dialog_add_button(pkg_name_dialog, "_Save", GTK_RESPONSE_ACCEPT));
        gtk_dialog_add_button(pkg_name_dialog, "_Cancel", GTK_RESPONSE_REJECT);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(pkg_name_dialog));
        pkg_name_entry = GTK_ENTRY(gtk_entry_new());

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Enter a name for this package</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(pkg_name_entry), false, false, 0);

        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(pkg_name_dialog, "show", G_CALLBACK(on_pkg_name_show), 0);
    }

    /** --Package level chooser **/
    {
        pkg_lvl_chooser = new_dialog_defaults("Set level ID", &on_pkg_lvl_chooser_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(pkg_lvl_chooser));

        GtkBox *spin = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5));
        pkg_lvl_chooser_lvl_id = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 255, 1, 1, 0)),
                    1, 0));
        gtk_box_pack_start(GTK_BOX(spin), GTK_WIDGET(gtk_label_new("Level ID:")), false, false, 0);
        gtk_box_pack_start(GTK_BOX(spin), GTK_WIDGET(pkg_lvl_chooser_lvl_id), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(spin), 0, 0, 0);

        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Variable chooser **/
    {
        variable_dialog = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
        gtk_container_set_border_width(GTK_CONTAINER(variable_dialog), 10);
        gtk_window_set_default_size(GTK_WINDOW(variable_dialog), 400, 400);
        gtk_widget_set_size_request(GTK_WIDGET(variable_dialog), 400, 400);
        gtk_window_set_title(GTK_WINDOW(variable_dialog), "Variable chooser");
        gtk_window_set_resizable(GTK_WINDOW(variable_dialog), false);
        // gtk_window_set_policy(GTK_WINDOW(variable_dialog),
        //               FALSE,
        //               FALSE, FALSE);
        gtk_window_set_position(variable_dialog, GTK_WIN_POS_CENTER);
        //gtk_window_set_keep_above(GTK_WINDOW(variable_dialog), TRUE);

        g_signal_connect(variable_dialog, "key-press-event", G_CALLBACK(on_variable_keypress), 0);
        g_signal_connect(variable_dialog, "show", G_CALLBACK(on_variable_show), 0);
        g_signal_connect(variable_dialog, "delete-event", G_CALLBACK(on_window_close), 0);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *inner_content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkWidget *l;
        GtkBox *hb;

        hb = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
        l = gtk_label_new("Variable:");
        variable_name = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_max_length(variable_name, 255);
        gtk_entry_set_activates_default(variable_name, true);
        gtk_box_pack_start(hb, l, false, false, 0);
        gtk_box_pack_start(hb, GTK_WIDGET(variable_name), false, false, 0);
        gtk_box_pack_start(inner_content, GTK_WIDGET(hb), false, false, 0);

        hb = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

        /* Reset this */
        variable_reset_this = GTK_BUTTON(gtk_button_new_with_label("Reset this variable"));
        g_signal_connect(variable_reset_this, "clicked", G_CALLBACK(on_variable_btn_click), 0);
        /* Reset all */
        variable_reset_all = GTK_BUTTON(gtk_button_new_with_label("Reset all"));
        g_signal_connect(variable_reset_all, "clicked", G_CALLBACK(on_variable_btn_click), 0);
        /* Ok */
        variable_ok = GTK_BUTTON(gtk_button_new_with_label("Save"));
        g_signal_connect(variable_ok, "clicked", G_CALLBACK(on_variable_btn_click), 0);
        /* Cancel */
        variable_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(variable_cancel, "clicked", G_CALLBACK(on_variable_btn_click), 0);

        gtk_box_pack_start(hb, GTK_WIDGET(variable_reset_this), false, false, 0);
        gtk_box_pack_start(hb, GTK_WIDGET(variable_reset_all), false, false, 0);
        gtk_box_pack_start(inner_content, GTK_WIDGET(hb), false, false, 0);

        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(variable_ok));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(variable_cancel));

        gtk_box_pack_start(content, GTK_WIDGET(inner_content), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(variable_dialog), GTK_WIDGET(content));
    }

    /** --Save and Save as copy **/
    {
        save_window = new_window_defaults("Save level", &on_save_show, &on_save_keypress);
        gtk_window_set_default_size(GTK_WINDOW(save_window), 400, 100);
        gtk_widget_set_size_request(GTK_WIDGET(save_window), 400, 100);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *entries = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *bottom_content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

        /* Name entry */
        save_entry = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_max_length(save_entry, 255);
        gtk_entry_set_activates_default(save_entry, true);

        /* Name label */
        gtk_box_pack_start(GTK_BOX(entries), new_lbl("<b>Enter a name for this level</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(entries), GTK_WIDGET(save_entry), false, false, 0);

        /* Buttons and button box */
        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* OK button */
        save_ok = GTK_BUTTON(gtk_button_new_with_label("Save"));
        g_signal_connect(save_ok, "clicked",
                G_CALLBACK(on_save_btn_click), 0);

        /* Cancel button */
        save_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(save_cancel, "clicked",
                G_CALLBACK(on_save_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(save_ok));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(save_cancel));

        /* Status label */
        save_status = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_xalign(GTK_LABEL(save_status), 0.0f);
        gtk_label_set_yalign(GTK_LABEL(save_status), 0.5f);

        gtk_box_pack_start(bottom_content, GTK_WIDGET(save_status), 1, 1, 0);
        gtk_box_pack_start(bottom_content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_box_pack_start(content, GTK_WIDGET(entries), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(bottom_content), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(save_window), GTK_WIDGET(content));
    }

    /** --Export **/
    {
        export_window = new_window_defaults("Export object", &on_export_show, &on_export_keypress);
        gtk_window_set_default_size(GTK_WINDOW(export_window), 400, 100);
        gtk_widget_set_size_request(GTK_WIDGET(export_window), 400, 100);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *entries = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *bottom_content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

        /* Name entry */
        export_entry = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_max_length(export_entry, 255);
        gtk_entry_set_activates_default(export_entry, true);

        /* Name label */
        gtk_box_pack_start(GTK_BOX(entries), new_lbl("<b>Enter a name for this object</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(entries), GTK_WIDGET(export_entry), false, false, 0);

        /* Buttons and button box */
        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* OK button */
        export_ok = GTK_BUTTON(gtk_button_new_with_label("Save"));
        g_signal_connect(export_ok, "clicked",
                G_CALLBACK(on_export_btn_click), 0);

        /* Cancel button */
        export_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(export_cancel, "clicked",
                G_CALLBACK(on_export_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(export_ok));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(export_cancel));

        /* Status label */
        export_status = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_xalign(GTK_LABEL(export_status), 0.0f);
        gtk_label_set_yalign(GTK_LABEL(export_status), 0.5f);

        gtk_box_pack_start(bottom_content, GTK_WIDGET(export_status), 1, 1, 0);
        gtk_box_pack_start(bottom_content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_box_pack_start(content, GTK_WIDGET(entries), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(bottom_content), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(export_window), GTK_WIDGET(content));
    }

#ifdef BUILD_PKGMGR
    /** --Package manager **/
    {
        package_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
        gtk_container_set_border_width(GTK_CONTAINER(package_window), 10);
        gtk_window_set_default_size(GTK_WINDOW(package_window), 400, 700);
        gtk_widget_set_size_request(GTK_WIDGET(package_window), 400, 700);
        gtk_window_set_title(GTK_WINDOW(package_window), "Package Manager");
        gtk_window_set_resizable(GTK_WINDOW(package_window), false);
        // gtk_window_set_policy(GTK_WINDOW(package_window),
        //               FALSE,
        //               FALSE, FALSE);
        //gtk_window_set_keep_above(GTK_WINDOW(package_window), true);

        g_signal_connect(package_window, "delete-event", G_CALLBACK(on_window_close), 0);
        g_signal_connect(package_window, "show", G_CALLBACK(on_package_manager_show), 0);

        //GtkBox *layout = GTK_BOX(gtk_dialog_get_content_area(package_dialog));
        GtkBox *layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
        gtk_box_set_spacing(GTK_BOX(layout), 5);
        gtk_box_set_homogeneous(GTK_BOX(layout), false);

        gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(gtk_label_new("Your packages:")), 0, 0, 0);

        {
            GtkListStore *store;

            store = gtk_list_store_new(2, G_TYPE_UINT, G_TYPE_STRING);

            pk_pkg_treemodel = GTK_TREE_MODEL(store);

            //gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), OC_DATE, GTK_SORT_DESCENDING);

            pk_pkg_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(pk_pkg_treemodel));
            gtk_tree_view_set_search_column(pk_pkg_treeview, OC_NAME);

            g_signal_connect(GTK_WIDGET(pk_pkg_treeview), "cursor-changed", G_CALLBACK(cursor_changed_pk_pkg), 0);

            GtkWidget *ew = gtk_scrolled_window_new(0,0);
            gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                          GTK_POLICY_AUTOMATIC,
                          GTK_POLICY_AUTOMATIC);

            gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(pk_pkg_treeview));
            gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(ew), 1, 1, 0);

            add_text_column(pk_pkg_treeview, "ID", 0);
            GtkCellRenderer *renderer = add_text_column(pk_pkg_treeview, "Name", 1);

            g_object_set(renderer, "editable", true, NULL);
            g_signal_connect(renderer, "edited", G_CALLBACK(pk_name_edited), 0);
        }

        {
            GtkBox *b = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

            //pk_pkg_delete = GTK_WIDGET(gtk_button_new_with_label("Delete"));
            pk_pkg_create = GTK_WIDGET(gtk_button_new_with_label("Create"));
            pk_pkg_play = GTK_WIDGET(gtk_button_new_with_label("Play"));
            pk_pkg_publish = GTK_WIDGET(gtk_button_new_with_label("Publish"));

            g_signal_connect(pk_pkg_play, "clicked", G_CALLBACK(press_play_pkg), 0);
            g_signal_connect(pk_pkg_create, "clicked", G_CALLBACK(press_create_pkg), 0);
            g_signal_connect(pk_pkg_publish, "clicked", G_CALLBACK(press_publish_pkg), 0);

            //gtk_box_pack_start(GTK_BOX(b), pk_pkg_delete, false, false, 0);
            gtk_box_pack_start(GTK_BOX(b), pk_pkg_create, false, false, 0);
            gtk_box_pack_start(GTK_BOX(b), pk_pkg_play, false, false, 0);
            gtk_box_pack_start(GTK_BOX(b), pk_pkg_publish, false, false, 0);

            gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(b), 0, 0, 0);
        }

        pk_pkg_first_is_menu = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("First level used as level selector"));
        pk_pkg_return_on_finish = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Return to level select on level finish"));

        g_signal_connect(pk_pkg_first_is_menu, "toggled", G_CALLBACK(toggle_first_is_menu), 0);
        g_signal_connect(pk_pkg_return_on_finish, "toggled", G_CALLBACK(toggle_return_on_finish), 0);
        gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(pk_pkg_return_on_finish), 0, 0, 0);
        gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(pk_pkg_first_is_menu), 0, 0, 0);

        GtkBox *spin = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5));
        pk_pkg_unlock_count = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 256, 1, 1, 0)),
                    1, 0));
        g_signal_connect(pk_pkg_unlock_count, "value-changed", G_CALLBACK(value_changed_unlock_count), 0);
        gtk_box_pack_start(GTK_BOX(spin), GTK_WIDGET(gtk_label_new("Unlock count:")), false, false, 0);
        gtk_box_pack_start(GTK_BOX(spin), GTK_WIDGET(pk_pkg_unlock_count), false, false, 0);
        gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(spin), 0, 0, 0);

        gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(gtk_label_new("Levels in selected package:")), 0, 0, 0);

        {
            GtkListStore *store;

            store = gtk_list_store_new(2, G_TYPE_UINT, G_TYPE_STRING);

            pk_lvl_treemodel = GTK_TREE_MODEL(store);

            //gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), OC_DATE, GTK_SORT_DESCENDING);

            pk_lvl_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(pk_lvl_treemodel));
            gtk_tree_view_set_search_column(pk_lvl_treeview, OC_NAME);
            gtk_tree_view_set_reorderable(pk_lvl_treeview, true);

            GtkWidget *ew = gtk_scrolled_window_new(0,0);
            gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                          GTK_POLICY_AUTOMATIC,
                          GTK_POLICY_AUTOMATIC);

            gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(pk_lvl_treeview));
            gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(ew), 1, 1, 0);

            add_text_column(pk_lvl_treeview, "ID", 0);
            add_text_column(pk_lvl_treeview, "Name", 1);

            g_signal_connect(pk_lvl_treemodel, "row-deleted", G_CALLBACK(pk_lvl_row_deleted), 0);
            g_signal_connect(GTK_WIDGET(pk_lvl_treeview), "row-activated", G_CALLBACK(pk_lvl_row_activated), 0);
            g_signal_connect(pk_lvl_treemodel, "row-inserted", G_CALLBACK(pk_lvl_row_inserted), 0);
        }

        {
            GtkBox *b = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

            pk_lvl_add = GTK_WIDGET(gtk_button_new_with_label("Add current level"));
            g_signal_connect(pk_lvl_add, "clicked", G_CALLBACK(press_add_current_level), 0);
            gtk_box_pack_start(GTK_BOX(b), pk_lvl_add, false, false, 0);

            pk_lvl_del = GTK_WIDGET(gtk_button_new_with_label("Remove selected"));
            g_signal_connect(pk_lvl_del, "clicked", G_CALLBACK(press_del_selected), 0);
            gtk_box_pack_start(GTK_BOX(b), pk_lvl_del, false, false, 0);

            pk_lvl_play = GTK_WIDGET(gtk_button_new_with_label("Play selected"));
            g_signal_connect(pk_lvl_play, "clicked", G_CALLBACK(press_play_selected), 0);
            gtk_box_pack_start(GTK_BOX(b), pk_lvl_play, false, false, 0);

            gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(b), 0, 0, 0);
        }

        gtk_container_add(GTK_CONTAINER(package_window), GTK_WIDGET(layout));
        gtk_widget_show_all(GTK_WIDGET(layout));
    }
#endif

    /** --Level properties **/
    {
        properties_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Level properties",
            0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
            NULL, NULL
        ));

        apply_dialog_defaults(properties_dialog);

        g_signal_connect(properties_dialog, "show", G_CALLBACK(on_properties_show), 0);
        g_signal_connect(properties_dialog, "key-press-event", G_CALLBACK(on_properties_keypress), 0);

        GtkBox *layout = GTK_BOX(gtk_dialog_get_content_area(properties_dialog));

        GtkNotebook *nb = GTK_NOTEBOOK(gtk_notebook_new());
        gtk_widget_set_size_request(GTK_WIDGET(nb), 550, 550);
        gtk_notebook_set_tab_pos(nb, GTK_POS_TOP);

        GtkGrid *tbl_info = create_settings_table();
        {
            int y = -1;

            lvl_title = GTK_ENTRY(gtk_entry_new());
            lvl_descr = GTK_TEXT_VIEW(gtk_text_view_new());

            GtkBox* lvl_type_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

            lvl_radio_adventure = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(NULL, "Adventure"));
            lvl_radio_custom = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(gtk_radio_button_get_group(lvl_radio_adventure), "Custom"));

            gtk_container_add(GTK_CONTAINER(lvl_type_box), GTK_WIDGET(lvl_radio_adventure));
            gtk_container_add(GTK_CONTAINER(lvl_type_box), GTK_WIDGET(lvl_radio_custom));

            GtkScrolledWindow *ew = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(0, 0));
            gtk_scrolled_window_set_min_content_height(ew, 64);
            gtk_scrolled_window_set_policy(ew, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
            gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(lvl_descr));

            GtkWidget *fr = gtk_frame_new(NULL);
            gtk_container_add(GTK_CONTAINER(fr), GTK_WIDGET(ew));

            add_setting_row(
                tbl_info, ++y,
                "Name", GTK_WIDGET(lvl_title)
            );

            add_setting_row(
                tbl_info, ++y,
                "Description", GTK_WIDGET(fr)
            );

            add_setting_row(
                tbl_info, ++y,
                "Type", GTK_WIDGET(lvl_type_box)
            );
        }

        GtkGrid *tbl_world = create_settings_table();
        {
            int y = -1;

            lvl_bg = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
            gtk_widget_set_hexpand(GTK_WIDGET(lvl_bg), true);
            g_signal_connect(lvl_bg, "changed", G_CALLBACK(on_lvl_bg_changed), 0);

            for (int x=0; x<num_bgs; x++) {
                gtk_combo_box_text_append_text(lvl_bg, available_bgs[x]);
            }

            lvl_bg_color = GTK_COLOR_BUTTON(gtk_color_button_new());
            gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(lvl_bg_color), false);
            g_signal_connect(lvl_bg_color, "color-set", G_CALLBACK(on_lvl_bg_color_set), 0);

            GtkBox* lvl_bg_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
            gtk_container_add(GTK_CONTAINER(lvl_bg_box), GTK_WIDGET(lvl_bg));
            gtk_container_add(GTK_CONTAINER(lvl_bg_box), GTK_WIDGET(lvl_bg_color));

            lvl_width_left = GTK_ENTRY(gtk_entry_new());
            lvl_width_right = GTK_ENTRY(gtk_entry_new());
            lvl_height_down = GTK_ENTRY(gtk_entry_new());
            lvl_height_up = GTK_ENTRY(gtk_entry_new());

            lvl_autofit = (GtkButton*)gtk_button_new_with_label("Auto-fit borders");
            g_signal_connect(lvl_autofit, "clicked",
                    G_CALLBACK(on_autofit_btn_click), 0);

            lvl_gx = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, -MAX_GRAVITY, MAX_GRAVITY, 1, 1, 0)),
                        1, 0));
            lvl_gy = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, -MAX_GRAVITY, MAX_GRAVITY, 1, 1, 0)),
                        1, 0));

            add_setting_row(
                tbl_world, ++y,
                "Background",
                GTK_WIDGET(lvl_bg_box)
            );

            add_setting_row(
                tbl_world, ++y,
                "Left border",
                GTK_WIDGET(lvl_width_left)
            );

            add_setting_row(
                tbl_world, ++y,
                "Right border",
                GTK_WIDGET(lvl_width_right)
            );

            add_setting_row(
                tbl_world, ++y,
                "Bottom border",
                GTK_WIDGET(lvl_height_down)
            );

            add_setting_row(
                tbl_world, ++y,
                "Top border",
                GTK_WIDGET(lvl_height_up)
            );

            gtk_grid_attach(
                tbl_world,
                GTK_WIDGET(lvl_autofit),
                0, ++y, 3, 1
            );

            add_setting_row(
                tbl_world, ++y,
                "Gravity X",
                GTK_WIDGET(lvl_gx)
            );

            add_setting_row(
                tbl_world, ++y,
                "Gravity Y",
                GTK_WIDGET(lvl_gy)
            );
        }

        GtkGrid *tbl_physics = create_settings_table();
        {
            int y = -1;

            lvl_pos_iter = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 10, 255, 5));
            lvl_vel_iter = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 10, 255, 5));

            lvl_prismatic_tol = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.f, .075f, 0.0125f/2.f));
            lvl_pivot_tol = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.f, .075f, 0.0125f/2.f));

            lvl_linear_damping = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.f, 10.0f, 0.05f));
            lvl_angular_damping = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.f, 10.0f, 0.05f));
            lvl_joint_friction = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.f, 10.0f, 0.05f));

            add_setting_row(
                tbl_physics, ++y,
                "Position iterations",
                GTK_WIDGET(lvl_pos_iter),
                "The amount of position iterations primarily affects dynamic objects. Lower = better performance."
            );

            add_setting_row(
                tbl_physics, ++y,
                "Velocity iterations",
                GTK_WIDGET(lvl_vel_iter),
                "Primarily affects motors and connection. Lower = better performance."
            );

            add_setting_row(
                tbl_physics, ++y,
                "Prismatic tolerance",
                GTK_WIDGET(lvl_prismatic_tol)
            );

            add_setting_row(
                tbl_physics, ++y,
                "Pivot tolerance",
                GTK_WIDGET(lvl_pivot_tol)
            );

            add_setting_row(
                tbl_physics, ++y,
                "Linear damping",
                GTK_WIDGET(lvl_linear_damping)
            );

            add_setting_row(
                tbl_physics, ++y,
                "Angular damping",
                GTK_WIDGET(lvl_angular_damping)
            );

            add_setting_row(
                tbl_physics, ++y,
                "Joint friction",
                GTK_WIDGET(lvl_joint_friction)
            );
        }

        GtkGrid *tbl_gameplay = create_settings_table();
        {
            int y = -1;

            lvl_score = GTK_ENTRY(gtk_entry_new());

            lvl_enemy_absorb_time = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.f, 60.f, 0.1f));
            lvl_player_respawn_time = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.f, 10.f, 0.1f));

            add_setting_row(
                tbl_gameplay, ++y,
                "Final score",
                GTK_WIDGET(lvl_score),
                "What score the player has to reach to win the level."
            );

            add_setting_row(
                tbl_gameplay, ++y,
                "Level version",
                GTK_WIDGET(lvl_upgrade = (GtkButton*) gtk_button_new())
            );
            g_signal_connect(lvl_upgrade, "clicked", G_CALLBACK(on_upgrade_btn_click), 0);

            add_setting_row(
                tbl_gameplay, ++y,
                "Pause on win",
                GTK_WIDGET(lvl_pause_on_win = (GtkCheckButton*) gtk_check_button_new()),
                "Pause the simulation once the win condition has been reached."
            );

            add_setting_row(
                tbl_gameplay, ++y,
                "Display score",
                GTK_WIDGET(lvl_show_score = (GtkCheckButton*) gtk_check_button_new()),
                "Display the score in the top-right corner."
            );

            add_setting_row(
                tbl_gameplay, ++y,
                "Creature absorb time",
                GTK_WIDGET(lvl_enemy_absorb_time),
                "Time before dead creatures are absorbed"
            );

            add_setting_row(
                tbl_gameplay, ++y,
                "Player respawn time",
                GTK_WIDGET(lvl_player_respawn_time),
                "Delay between a player's death and their ability to respawn"
            );

            for (int x=0; x<num_gtk_level_properties; ++x) {
                struct gtk_level_property *prop = &gtk_level_properties[x];
                add_setting_row(
                    tbl_gameplay, ++y,
                    prop->label,
                    GTK_WIDGET(prop->checkbutton = GTK_CHECK_BUTTON(gtk_check_button_new())),
                    prop->help
                );
                g_signal_connect(prop->checkbutton, "toggled", G_CALLBACK(on_level_flag_toggled), UINT_TO_VOID(prop->flag));
            }
        }

        lvl_ok      = GTK_BUTTON(gtk_dialog_add_button(properties_dialog, "_OK", GTK_RESPONSE_ACCEPT));
        lvl_cancel  = GTK_BUTTON(gtk_dialog_add_button(properties_dialog, "_Cancel", GTK_RESPONSE_REJECT));

        GtkWidget *view_info = gtk_viewport_new(0,0);
        GtkWidget *win_info = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (win_info),
                      GTK_POLICY_NEVER,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(view_info), GTK_WIDGET(tbl_info));
        gtk_container_set_border_width(GTK_CONTAINER(tbl_info), 5);
        gtk_container_set_border_width(GTK_CONTAINER(view_info), 0);
        gtk_container_add(GTK_CONTAINER(win_info), GTK_WIDGET(view_info));
        gtk_notebook_append_page(nb, win_info, gtk_label_new("Info"));

        GtkWidget *view_world = gtk_viewport_new(0,0);
        GtkWidget *win_world = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (win_world),
                      GTK_POLICY_NEVER,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(view_world), GTK_WIDGET(tbl_world));
        gtk_container_add(GTK_CONTAINER(win_world), view_world);
        gtk_notebook_append_page(nb, win_world, gtk_label_new("World"));

        GtkWidget *view_physics = gtk_viewport_new(0,0);
        GtkWidget *win_physics = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (win_physics),
                      GTK_POLICY_NEVER,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(view_physics), GTK_WIDGET(tbl_physics));
        gtk_container_add(GTK_CONTAINER(win_physics), view_physics);
        gtk_notebook_append_page(nb, win_physics, gtk_label_new("Physics"));

        GtkWidget *view_gameplay = gtk_viewport_new(0,0);
        GtkWidget *win_gameplay = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (win_gameplay),
                      GTK_POLICY_NEVER,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(view_gameplay), GTK_WIDGET(tbl_gameplay));
        gtk_container_add(GTK_CONTAINER(win_gameplay), view_gameplay);
        gtk_notebook_append_page(nb, win_gameplay, gtk_label_new("Gameplay"));

        gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(nb), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(nb));

        gtk_widget_show_all(GTK_WIDGET(layout));
    }

    /* confirm upgrade version dialog */
    {
        confirm_upgrade_dialog = new_dialog_defaults("Confirm Upgrade");

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(confirm_upgrade_dialog));

        GtkWidget *t = gtk_label_new(0);
        gtk_label_set_markup(GTK_LABEL(t),
        "<b>Are you sure you want to upgrade the version of this level?</b>"
        "\n\n"
        "To get access to new features the version associated with this level "
        "must be upgraded. This action can not be undone. Please save a copy before "
        "upgrading your level."
        "\n\n"
        "By upgrading this level, some object properties such as density, "
        "restitution, friction and applied forces might differ from earlier versions and affect "
        "how your level is simulated."
        );
        gtk_widget_set_size_request(GTK_WIDGET(t), 400, -1);
        gtk_label_set_line_wrap(GTK_LABEL(t), true);
        gtk_box_pack_start(GTK_BOX(content), t, false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Publish **/
    {
        publish_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Publish",
                0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
                "Publish", GTK_RESPONSE_ACCEPT,
                "_Cancel", GTK_RESPONSE_REJECT,
                NULL));

        apply_dialog_defaults(publish_dialog);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(publish_dialog));

        publish_name = GTK_ENTRY(gtk_entry_new());
        publish_descr = GTK_TEXT_VIEW(gtk_text_view_new());
        gtk_text_view_set_wrap_mode(publish_descr, GTK_WRAP_WORD);

        GtkBox *box_locked = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
        publish_locked = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Locked"));

        gtk_box_pack_start(box_locked, GTK_WIDGET(publish_locked), 1, 1, 0);
        gtk_box_pack_start(box_locked, help_widget("Disallow other players from seeing this level outside of packages."), 0, 0, 0);

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Level name:</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(publish_name), false, false, 0);

        GtkWidget *ew = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(
            GTK_SCROLLED_WINDOW(ew),
            GTK_POLICY_AUTOMATIC,
            GTK_POLICY_AUTOMATIC
        );
        gtk_widget_set_size_request(GTK_WIDGET(ew), 400, 150);
        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(publish_descr));

        GtkWidget *fr = gtk_frame_new(NULL);
        gtk_container_add(GTK_CONTAINER(fr), GTK_WIDGET(ew));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Level description:</b>"), false, false, 0);

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(fr), false, false, 0);

        /* Locked box */
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(box_locked), false, false, 0);

        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(publish_dialog, "show", G_CALLBACK(on_publish_show), 0);

        /* TODO: add key-press-events to everything but the cancel-button */
    }

    /** --New level **/
    {
        new_level_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "New level",
                0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
                "Empty Adventure", RESPONSE_EMPTY_ADVENTURE,
                "Adventure", RESPONSE_ADVENTURE,
                "Custom", RESPONSE_CUSTOM,
                NULL));

        apply_dialog_defaults(new_level_dialog);

        /* XXX: Should we add some information about the various level types? */
    }

    /** --Sandbox mode**/
    {
        mode_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Sandbox mode",
                0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
                "Connection Edit", RESPONSE_CONN_EDIT,
                "Multi-Select", RESPONSE_MULTISEL,
                "Terrain Paint", RESPONSE_DRAW,
                NULL));

        apply_dialog_defaults(mode_dialog);

        /* XXX: Should we add some informationa bout the varius modes? */
    }

    /** --Command pad **/
    {
        command_pad_dialog = new_dialog_defaults("Set command", &on_command_pad_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(command_pad_dialog));

        command_pad_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        gtk_combo_box_text_append_text(command_pad_cb, "Stop");
        gtk_combo_box_text_append_text(command_pad_cb, "Start/Stop toggle");
        gtk_combo_box_text_append_text(command_pad_cb, "Left");
        gtk_combo_box_text_append_text(command_pad_cb, "Right");
        gtk_combo_box_text_append_text(command_pad_cb, "Left/Right toggle");
        gtk_combo_box_text_append_text(command_pad_cb, "Jump");
        gtk_combo_box_text_append_text(command_pad_cb, "Aim");
        gtk_combo_box_text_append_text(command_pad_cb, "Attack");
        gtk_combo_box_text_append_text(command_pad_cb, "Layer up");
        gtk_combo_box_text_append_text(command_pad_cb, "Layer down");
        gtk_combo_box_text_append_text(command_pad_cb, "Increase speed");
        gtk_combo_box_text_append_text(command_pad_cb, "Decrease speed");
        gtk_combo_box_text_append_text(command_pad_cb, "Set speed");
        gtk_combo_box_text_append_text(command_pad_cb, "Full health");

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Command</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(command_pad_cb), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Key Listener **/
    {
        dialog = new_dialog_defaults("Key Listener", &on_key_listener_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        key_listener_ls = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_UINT);

        GtkTreeIter iter;
        for (int x=0; x<TMS_KEY__NUM; ++x) {
            const char *s = key_names[x];

            if (s) {
                gtk_list_store_append(key_listener_ls, &iter);
                gtk_list_store_set(key_listener_ls, &iter,
                        0, s,
                        1, x,
                        -1
                        );
            }
        }

        key_listener_cb = GTK_COMBO_BOX(gtk_combo_box_new_with_model(GTK_TREE_MODEL(key_listener_ls)));

        GtkCellRenderer *cell = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(key_listener_cb), cell, TRUE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(key_listener_cb), cell, "text", 0, NULL);

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Key</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(key_listener_cb), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));

        key_listener_dialog = dialog;
    }

    /** --Digital display **/
    {
        digi_dialog = new_dialog_defaults("Display settings", &on_digi_show);

        gtk_widget_set_size_request(GTK_WIDGET(digi_dialog), 200, 450);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(digi_dialog));
        gtk_box_set_spacing(GTK_BOX(content), 7);

        {
            GtkBox *spin = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5));
            digi_wrap = GTK_CHECK_BUTTON(gtk_check_button_new());
            gtk_box_pack_start(GTK_BOX(spin), new_lbl("<b>Wrap around</b>"), false, false, 0);
            gtk_box_pack_start(GTK_BOX(spin), GTK_WIDGET(digi_wrap), false, false, 0);
            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(spin), false, false, 0);
        }
        {
            GtkBox *spin = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5));
            digi_initial = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 40, 1, 1, 0)),
                    1, 0));
            gtk_box_pack_start(GTK_BOX(spin), new_lbl("<b>Initial position</b>"), false, false, 0);
            gtk_box_pack_start(GTK_BOX(spin), GTK_WIDGET(digi_initial), false, false, 0);
            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(spin), false, false, 0);
        }

        {
            digi_label = GTK_LABEL(new_clbl("<b>Symbol 1/36</b>"));

            GtkGrid *tbl_symbol = GTK_GRID(gtk_grid_new());

            gtk_grid_set_row_homogeneous(tbl_symbol, true);
            gtk_grid_set_column_homogeneous(tbl_symbol, true);
            gtk_grid_set_row_spacing(tbl_symbol, 0);
            gtk_grid_set_column_spacing(tbl_symbol, 0);

            for (int y=0; y < 7; y++) {
                for (int x=0; x < 5; x++) {
                    //Create ToggleButton
                    GtkToggleButton* check = GTK_TOGGLE_BUTTON(gtk_toggle_button_new());
                    gtk_toggle_button_set_mode(check, false);
                    digi_check[y][x] = check;

                    //Add .display-cell class
                    GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(check));
                    gtk_style_context_add_class(context, "display-cell");

                    //Add to table
                    gtk_grid_attach(
                        tbl_symbol,
                        GTK_WIDGET(check),
                        x, y, 1, 1
                    );

                    //Connect toggled signal
                    g_signal_connect(
                        check, "toggled",
                        G_CALLBACK(on_digi_toggle),
                        (void*)(uintptr_t)((y * 5) + x)
                    );
                }
            }

            //Create aspect frame and put tbl_symbol into it, to ensure perfect aspect ratio
            GtkAspectFrame* aspect_frame = GTK_ASPECT_FRAME(gtk_aspect_frame_new(NULL, .5f, .5f, 5.f / 7.f, false));
            gtk_container_add(GTK_CONTAINER(aspect_frame), GTK_WIDGET(tbl_symbol));

            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(digi_label), false, false, 0);
            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(aspect_frame), true, true, 0);
        }

        {
            GtkBox *btns = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

            digi_prev = GTK_BUTTON(gtk_button_new_with_label("Previous"));
            gtk_box_pack_start(GTK_BOX(btns), GTK_WIDGET(digi_prev), false, false, 0);
            g_signal_connect(digi_prev, "clicked", G_CALLBACK(on_digi_prev_click), 0);

            digi_next = GTK_BUTTON(gtk_button_new_with_label("Next"));
            gtk_box_pack_start(GTK_BOX(btns), GTK_WIDGET(digi_next), false, false, 0);
            g_signal_connect(digi_next, "clicked", G_CALLBACK(on_digi_next_click), 0);
            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(btns), false, false, 0);

            btns = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5));

            digi_insert = GTK_BUTTON(gtk_button_new_with_label("Insert before"));
            gtk_box_pack_start(GTK_BOX(btns), GTK_WIDGET(digi_insert), false, false, 0);
            g_signal_connect(digi_insert, "clicked", G_CALLBACK(on_digi_insert_click), 0);

            digi_append = GTK_BUTTON(gtk_button_new_with_label("Append"));
            gtk_box_pack_start(GTK_BOX(btns), GTK_WIDGET(digi_append), false, false, 0);
            g_signal_connect(digi_append, "clicked", G_CALLBACK(on_digi_append_click), 0);

            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(btns), false, false, 0);
            btns = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5));

            digi_delete = GTK_BUTTON(gtk_button_new_with_label("Delete current symbol"));
            gtk_box_pack_start(GTK_BOX(btns), GTK_WIDGET(digi_delete), false, false, 0);
            g_signal_connect(digi_delete, "clicked", G_CALLBACK(on_digi_delete_click), 0);

            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(btns), false, false, 0);
        }

        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Sticky **/
    {
        sticky_dialog = new_dialog_defaults("Sticky note", &on_sticky_show);

        gtk_widget_set_size_request(GTK_WIDGET(sticky_dialog), 450, 250);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(sticky_dialog));
        gtk_box_set_spacing(GTK_BOX(content), 7);

        sticky_text = GTK_TEXT_VIEW(gtk_text_view_new());
        GtkTextBuffer *tbuf = gtk_text_view_get_buffer(sticky_text);
        g_signal_connect(G_OBJECT(tbuf), "changed", G_CALLBACK(sticky_text_changed), 0);

        GtkBox *spin = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5));
        sticky_font_size = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 3, 1, 1, 0)),
                    1, 0));

        GtkBox *align = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

        GtkBox *params = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

        sticky_center_x = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Center horizontally"));
        sticky_center_y = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Center vertically"));

        gtk_box_pack_start(GTK_BOX(spin), new_lbl("Font size"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(spin), GTK_WIDGET(sticky_font_size), false, false, 0);

        gtk_box_pack_start(GTK_BOX(align), GTK_WIDGET(sticky_center_x), false, false, 0);
        gtk_box_pack_start(GTK_BOX(align), GTK_WIDGET(sticky_center_y), false, false, 0);

        gtk_box_pack_start(GTK_BOX(params), GTK_WIDGET(spin), false, false, 0);
        gtk_box_pack_start(GTK_BOX(params), GTK_WIDGET(align), false, false, 0);

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(params), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), new_clbl("<b>Sticky text:</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sticky_text), true, true, 0);

        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Item **/
    {
        dialog = new_dialog_defaults("Item", &on_item_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        item_cb = new_item_cb();
        /* Items will be filled on show, to keep a refreshed list of unlocked items. */

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Item type</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(item_cb), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));

        item_dialog = dialog;
    }

    /** --Decoration **/
    {
        dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Decoration",
                0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
                "_OK", GTK_RESPONSE_ACCEPT,
                "_Cancel", GTK_RESPONSE_REJECT,
                NULL));
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
        //gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        decoration_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        for (int x=0; x<NUM_DECORATIONS; x++) {
            gtk_combo_box_text_append_text(decoration_cb, decorations[x].name);
        }

        GtkWidget *t = gtk_label_new(0);
        gtk_label_set_markup(GTK_LABEL(t), "<b>Decoration type</b>");
        gtk_box_pack_start(GTK_BOX(content), t, false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(decoration_cb), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(dialog, "show", G_CALLBACK(on_decoration_show), 0);
        g_signal_connect(dialog, "delete-event", G_CALLBACK(on_window_close), 0);

        decoration_dialog = dialog;
    }

    /** --Resource **/
    {
        dialog = new_dialog_defaults("Resource", &on_resource_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        resource_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        for (int x=0; x<NUM_RESOURCES; x++) {
            gtk_combo_box_text_append_text(resource_cb, resource_data[x].name);
        }

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Resource type</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(resource_cb), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));

        resource_dialog = dialog;
    }

    /** --Vendor **/
    {
        dialog = new_dialog_defaults("Vendor", &on_vendor_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        vendor_amount = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 65535u, 1, 1, 0)),
                    1, 0));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Num. items required</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(vendor_amount), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));

        vendor_dialog = dialog;
    }

    /** --Animal **/
    {
        dialog = new_dialog_defaults("Animal", &on_animal_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        animal_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        for (int x=0; x<NUM_ANIMAL_TYPES; ++x) {
            gtk_combo_box_text_append_text(animal_cb, animal_data[x].name);
        }

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Animal type</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(animal_cb), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));

        animal_dialog = dialog;
    }

    /** --Soundman **/
    {
        dialog = new_dialog_defaults("Sound Manager", &on_soundman_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        soundman_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        for (int x=0; x<SND__NUM; x++) {
            gtk_combo_box_text_append_text(soundman_cb, sm::sound_lookup[x]->name);
        }

        soundman_catch_all = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Catch all"));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Sound type</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(soundman_cb), false, false, 10);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(soundman_catch_all), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));

        soundman_dialog = dialog;
    }

    /** --SFX Emitter 2 **/
    {
        dialog = new_dialog_defaults("SFX Emitter", &on_sfx2_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        sfx2_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        for (int x=0; x<SND__NUM; x++) {
            gtk_combo_box_text_append_text(sfx2_cb, sm::sound_lookup[x]->name);
        }

        sfx2_sub_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());

        g_signal_connect(sfx2_cb, "changed", G_CALLBACK(on_sfx2_cb_changed), 0);

        sfx2_global = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Global sound"));
        sfx2_loop = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Loop"));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Sound</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sfx2_cb), false, false, 10);
        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Sound chunk</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sfx2_sub_cb), false, false, 10);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sfx2_global), false, false, 10);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sfx2_loop), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));

        sfx2_dialog = dialog;
    }

    /** --Faction **/
    {
        dialog = new_dialog_defaults("Set Faction", &on_faction_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        faction_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        for (int x=0; x<NUM_FACTIONS; x++) {
            gtk_combo_box_text_append_text(faction_cb, factions[x].name);
        }

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Faction</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(faction_cb), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));

        faction_dialog = dialog;
    }

    /** --Factory **/
    {
        dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Factory",
                0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
                "_OK", GTK_RESPONSE_ACCEPT,
                NULL));
        factory_cancel = GTK_BUTTON(gtk_dialog_add_button(dialog, "_Cancel", GTK_RESPONSE_REJECT));

        apply_dialog_defaults(dialog);

        gtk_widget_set_size_request(GTK_WIDGET(dialog), 450, 300);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));
        GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
        GtkWidget *l;

        GtkGrid *tbl = GTK_GRID(gtk_grid_new());

        gtk_grid_set_row_spacing(tbl, 5);
        gtk_grid_set_column_spacing(tbl, 5);

        int x = 0;

        l = gtk_label_new("Oil");
        gtk_label_set_xalign(GTK_LABEL(l), 0.0f);
        gtk_label_set_yalign(GTK_LABEL(l), 0.5f);
        factory_oil = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 65535, 1, 1, 0)),
                    1, 0));
        gtk_grid_attach(tbl, l, 0, x, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(factory_oil), 1, x, 1, 1);
        ++x;

        for (; x<NUM_RESOURCES+1; ++x) {
            GtkWidget *l = gtk_label_new(resource_data[x-1].name);
            gtk_label_set_xalign(GTK_LABEL(l), 0.0f);
            gtk_label_set_yalign(GTK_LABEL(l), 0.5f);
            factory_resources[x-1] = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 65535, 1, 1, 0)),
                        1, 0));
            gtk_grid_attach(tbl, l, 0, x, 1, 1);
            gtk_grid_attach(tbl, GTK_WIDGET(factory_resources[x-1]), 1, x, 1, 1);
        }

        l = gtk_label_new("Faction");
        gtk_label_set_xalign(GTK_LABEL(l), 0.0f);
        gtk_label_set_yalign(GTK_LABEL(l), 0.5f);
        factory_faction = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, NUM_FACTIONS-1, 1, 1, 0)),
                    1, 0));
        gtk_grid_attach(tbl, l, 0, x, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(factory_faction), 1, x, 1, 1);
        ++x;

        {
            /*                                        Included        Order        Name,         ID */
            factory_liststore = gtk_list_store_new(4, G_TYPE_BOOLEAN, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);
            GtkTreeModel *model = GTK_TREE_MODEL(factory_liststore);

            factory_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(model));

            GtkCellRenderer *renderer;
            GtkTreeViewColumn *column;
            model = gtk_tree_view_get_model(factory_treeview);

            renderer = gtk_cell_renderer_toggle_new();
            g_signal_connect(renderer, "toggled", G_CALLBACK(factory_enable_toggled), model);

            column = gtk_tree_view_column_new_with_attributes(
                "Enabled",
                renderer,
                "active", 0,
                NULL
            );
            gtk_tree_view_append_column(factory_treeview, column);

            renderer = gtk_cell_renderer_text_new();
            column = gtk_tree_view_column_new_with_attributes(
                "Index",
                renderer,
                "text",
                1,
                NULL
            );
            gtk_tree_view_column_set_sort_column_id(column, 1);
            gtk_tree_view_append_column(factory_treeview, column);

            renderer = gtk_cell_renderer_text_new();
            column = gtk_tree_view_column_new_with_attributes(
                "Recipe",
                renderer,
                "text",
                2,
                NULL
            );
            gtk_tree_view_column_set_sort_column_id(column, 2);
            gtk_tree_view_column_set_expand(column, true);
            gtk_tree_view_append_column(factory_treeview, column);
        }

        GtkWidget *sw = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                GTK_POLICY_AUTOMATIC,
                GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(factory_treeview));

        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(tbl), false, false, 0);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(sw), true, true, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(hbox), true, true, 0);
        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(dialog, "key-press-event", G_CALLBACK(on_factory_keypress), 0);
        g_signal_connect(dialog, "show", G_CALLBACK(on_factory_show), 0);

        factory_dialog = dialog;
    }

    /** --Treasure chest **/
    {
        dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Treasure chest",
            0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
            "_OK", GTK_RESPONSE_ACCEPT,
            NULL
        ));
        tchest_cancel = GTK_BUTTON(gtk_dialog_add_button(dialog, "_Cancel", GTK_RESPONSE_REJECT));

        apply_dialog_defaults(dialog);

        gtk_widget_set_size_request(GTK_WIDGET(dialog), 750, 300);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));
        GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
        GtkWidget *l;

        GtkGrid *tbl = GTK_GRID(gtk_grid_new());
        //gtk_grid_set_row_homogeneous(tbl, true);
        gtk_grid_set_row_spacing(tbl, 5);
        gtk_grid_set_column_spacing(tbl, 5);

        int x = 0;

        tchest_entity = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        gtk_grid_attach(tbl, new_clbl("Entity"), 0, x, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(tchest_entity), 1, x, 1, 1);
        ++x;

        g_signal_connect(tchest_entity, "changed", G_CALLBACK(on_tchest_entity_changed), 0);

        tchest_sub_entity = new_item_cb();
        gtk_grid_attach(tbl, new_clbl("Sub-entity"), 0, x, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(tchest_sub_entity), 1, x, 1, 1);
        ++x;

        tchest_count = GTK_SPIN_BUTTON(gtk_spin_button_new(
            GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 65535, 1, 1, 0)),
            1, 0
        ));
        gtk_grid_attach(tbl, new_clbl("Amount"), 0, x, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(tchest_count), 1, x, 1, 1);
        ++x;

        //spacing
        GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_vexpand(spacer, true);
        gtk_grid_attach(tbl, GTK_WIDGET(spacer), 0, x, 2, 1);
        ++x;

        //Button box (add/remove entity)

        tchest_add_entity = GTK_BUTTON(gtk_button_new_with_label("Add entity"));
        g_signal_connect(
            tchest_add_entity, "clicked",
            G_CALLBACK(on_tchest_btn_click), 0
        );

        tchest_remove_selected = GTK_BUTTON(gtk_button_new_with_label("Remove selected"));
        g_signal_connect(
            tchest_remove_selected, "clicked",
            G_CALLBACK(on_tchest_btn_click), 0
        );

        GtkButtonBox* button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(button_box, GTK_BUTTONBOX_EXPAND);
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(tchest_add_entity));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(tchest_remove_selected));
        gtk_grid_attach(tbl, GTK_WIDGET(button_box), 0, x, 2, 1);
        ++x;

        {
            /*                                        g_id        sub_id      Name,          Count */
            tchest_liststore = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);
            GtkTreeModel *model = GTK_TREE_MODEL(tchest_liststore);

            tchest_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(model));
            g_signal_connect(
                tchest_treeview, "cursor-changed",
                G_CALLBACK(on_tchest_selection_changed), 0
            );
            g_signal_connect(
                tchest_treeview, "columns-changed",
                G_CALLBACK(on_tchest_selection_changed), 0
            );

            GtkCellRenderer *renderer;
            GtkTreeViewColumn *column;
            model = gtk_tree_view_get_model(tchest_treeview);

            renderer = gtk_cell_renderer_text_new();
            column = gtk_tree_view_column_new_with_attributes(
                "Name",
                renderer,
                "text",
                TCHEST_COLUMN_NAME,
                NULL
            );
            gtk_tree_view_column_set_sort_column_id(column, TCHEST_COLUMN_NAME);
            gtk_tree_view_append_column(tchest_treeview, column);

            renderer = gtk_cell_renderer_text_new();
            column = gtk_tree_view_column_new_with_attributes(
                "Amount",
                renderer,
                "text",
                TCHEST_COLUMN_COUNT,
                NULL
            );
            gtk_tree_view_column_set_sort_column_id(column, TCHEST_COLUMN_COUNT);
            gtk_tree_view_append_column(tchest_treeview, column);
        }

        GtkWidget *sw = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(
            GTK_SCROLLED_WINDOW(sw),
            GTK_POLICY_AUTOMATIC,
            GTK_POLICY_AUTOMATIC
        );
        gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(tchest_treeview));

        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(tbl), false, false, 0);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(sw), true, true, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(hbox), true, true, 0);
        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(dialog, "key-press-event", G_CALLBACK(on_tchest_keypress), 0);
        g_signal_connect(dialog, "show", G_CALLBACK(on_tchest_show), 0);

        tchest_dialog = dialog;
    }

    /** --Emitter **/
    {
        emitter_dialog = new_dialog_defaults("Emitter options", &on_emitter_show);
        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(emitter_dialog));

        emitter_auto_absorb = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0f, 60.f, 0.5f));
        g_signal_connect(emitter_auto_absorb, "format-value", G_CALLBACK(format_auto_absorb), 0);

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Absorb entity after emitting</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(emitter_auto_absorb), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --SFX Emitter dialog **/
    {
        sfx_dialog = new_dialog_defaults("SFX Emitter", &on_sfx_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(sfx_dialog));

        sfx_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        for (int x=0; x<NUM_SFXEMITTER_OPTIONS; x++) {
            gtk_combo_box_text_append_text(sfx_cb, sfxemitter_options[x].name);
        }

        sfx_global = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Global sound"));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Sound</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sfx_cb), false, false, 10);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sfx_global), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Event listener dialog **/
    {
        elistener_dialog = new_dialog_defaults("Event listener", &on_elistener_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(elistener_dialog));

        elistener_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        gtk_combo_box_text_append_text(elistener_cb, "Player die");
        gtk_combo_box_text_append_text(elistener_cb, "Enemy die");
        gtk_combo_box_text_append_text(elistener_cb, "Interactive object destroyed");
        gtk_combo_box_text_append_text(elistener_cb, "Player respawn");
        gtk_combo_box_text_append_text(elistener_cb, "Touch/Mouse Click");
        gtk_combo_box_text_append_text(elistener_cb, "Touch/Mouse Release");
        gtk_combo_box_text_append_text(elistener_cb, "Any absorber activated");
        gtk_combo_box_text_append_text(elistener_cb, "Level completed");
        gtk_combo_box_text_append_text(elistener_cb, "Game over");

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Event</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(elistener_cb), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --FX Emitter **/
    {
        fxemitter_dialog = new_dialog_defaults("FX Emitter", &on_fxemitter_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(fxemitter_dialog));

        for (int x=0; x<4; x++) {
            fxemitter_cb[x] = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
            gtk_combo_box_text_append_text(fxemitter_cb[x], "");
            gtk_combo_box_text_append_text(fxemitter_cb[x], "Explosion");
            gtk_combo_box_text_append_text(fxemitter_cb[x], "Highlight");
            gtk_combo_box_text_append_text(fxemitter_cb[x], "Destroy connections");
            gtk_combo_box_text_append_text(fxemitter_cb[x], "Smoke");
            gtk_combo_box_text_append_text(fxemitter_cb[x], "Magic");
            gtk_combo_box_text_append_text(fxemitter_cb[x], "Break");

            gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Effect</b>"), false, false, 0);
            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(fxemitter_cb[x]), false, false, 10);
        }

        GtkBox *slider_container = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
        gtk_box_set_homogeneous(slider_container, true);
        fxemitter_radius = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.125, 5, 0.125));
        gtk_box_pack_start(GTK_BOX(slider_container), new_lbl("<b>Radius</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(slider_container), GTK_WIDGET(fxemitter_radius), true, true, 10);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(slider_container), false, false, 10);

        slider_container = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
        gtk_box_set_homogeneous(slider_container, true);
        fxemitter_count = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 20, 1));
        gtk_box_pack_start(GTK_BOX(slider_container), new_lbl("<b>Count</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(slider_container), GTK_WIDGET(fxemitter_count), true, true, 10);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(slider_container), false, false, 10);

        slider_container = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
        gtk_box_set_homogeneous(slider_container, true);
        fxemitter_interval = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.05, 1, 0.05));
        gtk_box_pack_start(GTK_BOX(slider_container), new_lbl("<b>Interval</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(slider_container), GTK_WIDGET(fxemitter_interval), true, true, 10);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(slider_container), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Cam targeter **/
    {
        dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Cam targeter properties",
                0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
                NULL, NULL));

        apply_dialog_defaults(dialog);

        camtargeter_save = GTK_BUTTON(
                gtk_dialog_add_button(
                    dialog,
                    "_Save", GTK_RESPONSE_ACCEPT)
                );
        camtargeter_cancel = GTK_BUTTON(
                gtk_dialog_add_button(
                    dialog,
                    "_Cancel", GTK_RESPONSE_CANCEL)
                );

        g_signal_connect(dialog, "show", G_CALLBACK(on_camtargeter_show), 0);
        g_signal_connect(dialog, "key-press-event", G_CALLBACK(on_camtargeter_keypress), 0);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        camtargeter_mode = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        gtk_combo_box_text_append_text(camtargeter_mode, "Smooth follow");
        gtk_combo_box_text_append_text(camtargeter_mode, "Snap to object");
        gtk_combo_box_text_append_text(camtargeter_mode, "Relative follow");
        gtk_combo_box_text_append_text(camtargeter_mode, "Linear follow");

        camtargeter_offset_mode = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        gtk_combo_box_text_append_text(camtargeter_offset_mode, "Global");
        gtk_combo_box_text_append_text(camtargeter_offset_mode, "Relative");

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Follow mode</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(camtargeter_mode), false, false, 10);

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Offset mode</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(camtargeter_offset_mode), false, false, 10);

        float min = -150.f;
        float max =  150.f;

        {
            GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
            GtkRange *range = 0;
            GtkEntry *entry = 0;
            GtkWidget *l = 0;

            range = GTK_RANGE(gtk_scale_new(
                GTK_ORIENTATION_HORIZONTAL,
                GTK_ADJUSTMENT(gtk_adjustment_new(0.0, min, max, 0.001, 1.0, 0.0))
            ));

            g_signal_connect(range, "value-changed", G_CALLBACK(camtargeter_value_changed), 0);

            entry = GTK_ENTRY(gtk_entry_new());
            gtk_entry_set_width_chars(entry, 7);

            g_signal_connect(entry, "changed", G_CALLBACK(camtargeter_entry_changed), 0);
            g_signal_connect(entry, "insert-text", G_CALLBACK(camtargeter_insert_text), 0);

            gtk_scale_set_draw_value(GTK_SCALE(range), false);

            camtargeter_x_offset = range;
            camtargeter_x_offset_entry = entry;
            l = gtk_label_new("X offset");

            gtk_box_pack_start(GTK_BOX(hbox), l, false, false, 0);
            gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(range), true, true, 0);
            gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(entry), false, false, 0);

            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(hbox), false, false, 0);
        }

        {
            GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
            GtkRange *range = 0;
            GtkEntry *entry = 0;
            GtkWidget *l = 0;

            range = GTK_RANGE(gtk_scale_new(
                GTK_ORIENTATION_HORIZONTAL,
                GTK_ADJUSTMENT(gtk_adjustment_new(0.0, min, max, 0.001, 1.0, 0.0))
            ));

            g_signal_connect(range, "value-changed", G_CALLBACK(camtargeter_value_changed), 0);

            entry = GTK_ENTRY(gtk_entry_new());
            gtk_entry_set_width_chars(entry, 7);

            g_signal_connect(entry, "changed", G_CALLBACK(camtargeter_entry_changed), 0);
            g_signal_connect(entry, "insert-text", G_CALLBACK(camtargeter_insert_text), 0);

            gtk_scale_set_draw_value(GTK_SCALE(range), false);

            camtargeter_y_offset = range;
            camtargeter_y_offset_entry = entry;
            l = gtk_label_new("Y offset");

            gtk_box_pack_start(GTK_BOX(hbox), l, false, false, 0);
            gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(range), true, true, 0);
            gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(entry), false, false, 0);

            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(hbox), false, false, 0);
        }

        gtk_widget_set_size_request(GTK_WIDGET(dialog), 350, -1);
        gtk_widget_show_all(GTK_WIDGET(content));

        camtargeter_dialog = dialog;
    }

    /** --Quickadd **/
    {
        quickadd_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
        //VX: GTK_WIDGET_SET_FLAGS(quickadd_window, GTK_CAN_FOCUS);
        //VX: GTK_WINDOW(quickadd_window)->type = GTK_WINDOW_TOPLEVEL;
        gtk_window_set_decorated(GTK_WINDOW(quickadd_window), FALSE);
        //VX: gtk_window_set_has_frame(GTK_WINDOW(quickadd_window), FALSE);
        gtk_window_set_type_hint(GTK_WINDOW(quickadd_window), GDK_WINDOW_TYPE_HINT_POPUP_MENU);

        gtk_container_set_border_width(GTK_CONTAINER(quickadd_window), 4);
        gtk_window_set_default_size(GTK_WINDOW(quickadd_window), 200, 20);
        gtk_widget_set_size_request(GTK_WIDGET(quickadd_window), 200, 20);
        gtk_window_set_resizable(GTK_WINDOW(quickadd_window), false);
        // gtk_window_set_policy(GTK_WINDOW(quickadd_window),
        //               FALSE,
        //               FALSE, FALSE);

        quickadd_entry = GTK_ENTRY(gtk_entry_new());

        GtkEntryCompletion *comp = gtk_entry_completion_new();
        GtkListStore *list = gtk_list_store_new(3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT);

        gtk_entry_completion_set_model(comp, GTK_TREE_MODEL(list));
        gtk_entry_completion_set_text_column(comp, 1);
        gtk_entry_completion_set_inline_completion(comp, false);
        gtk_entry_completion_set_inline_selection(comp, true);

#if 0
        /* add all objects from the menu */
        GtkTreeIter iter;
        for (int x=0; x<menu_objects.size(); x++) {
            gtk_list_store_append(list, &iter);
            gtk_list_store_set(list, &iter,
                    0, menu_objects[x].e->g_id,
                    1, menu_objects[x].e->get_name(),
                    1, menu_objects[x].e->get_name(),
                    -1
                    );
        }
#endif

        gtk_entry_set_completion(quickadd_entry, comp);
        gtk_container_add(GTK_CONTAINER(quickadd_window), GTK_WIDGET(quickadd_entry));

        g_signal_connect(comp, "match-selected", G_CALLBACK(match_selected_quickadd), 0);
        g_signal_connect(quickadd_window, "show", G_CALLBACK(show_grab_focus), 0);
        g_signal_connect(quickadd_window, "delete-event", G_CALLBACK(on_window_close), 0);
        g_signal_connect(quickadd_entry, "activate", G_CALLBACK(activate_quickadd), 0);
        g_signal_connect(quickadd_entry, "key-press-event", G_CALLBACK(keypress_quickadd), 0);
    }

    /** --Color Chooser (for Plastic beam & Pixel) **/
    {
        beam_color_dialog = GTK_COLOR_SELECTION_DIALOG(gtk_color_selection_dialog_new("Color"));
        GtkColorSelectionDialog *sel = GTK_COLOR_SELECTION_DIALOG(beam_color_dialog);

        gtk_window_set_position(GTK_WINDOW(beam_color_dialog), GTK_WIN_POS_CENTER);
        //gtk_window_set_keep_above(GTK_WINDOW(beam_color_dialog), TRUE);

        gtk_color_selection_set_has_opacity_control(
            GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(
                GTK_COLOR_SELECTION_DIALOG(sel))), false);

        g_signal_connect(beam_color_dialog, "delete-event", G_CALLBACK(on_window_close), 0);

        GtkWidget *beam_color_dialog_ok_button = gtk_dialog_get_widget_for_response(GTK_DIALOG(beam_color_dialog), GTK_RESPONSE_OK);

        g_signal_connect(gtk_dialog_get_content_area(GTK_DIALOG(beam_color_dialog)),  "key-press-event", G_CALLBACK(on_color_keypress), 0);
        g_signal_connect(beam_color_dialog_ok_button, "key-press-event", G_CALLBACK(on_color_keypress), 0);
        g_signal_connect(beam_color_dialog,           "key-press-event", G_CALLBACK(on_color_keypress), 0);
    }

    /** --Autosave Dialog **/
    {
        dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Autosave prompt",
                0, (GtkDialogFlags)(0),/*GTK_MODAL*/
                "Open", GTK_RESPONSE_YES,
                "Remove", GTK_RESPONSE_NO,
                NULL));

        apply_dialog_defaults(dialog);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        GtkWidget *l = gtk_label_new("Autosave file detected. Open or remove?");
        gtk_box_pack_start(GTK_BOX(content), l, false, false, 0);

        gtk_widget_show_all(GTK_WIDGET(content));

        autosave_dialog = dialog;
    }

    /** --Tips Dialog **/
    {
        tips_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Tips & Tricks",
                0, (GtkDialogFlags)(0),/*GTK_MODAL*/
                "OK", GTK_RESPONSE_CLOSE,
                "Next", GTK_RESPONSE_APPLY,
                "More tips & tricks", GTK_RESPONSE_YES,
                NULL));

        apply_dialog_defaults(tips_dialog);

        gtk_window_set_default_size(GTK_WINDOW(tips_dialog), 425, 400);

        tips_hide = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Don't show this dialog again"));

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(tips_dialog));

        tips_text = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_selectable(tips_text, 1);
        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(tips_text));
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(ew), 1, 1, 3);

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(tips_hide), 0, 0, 3);

        gtk_label_set_line_wrap(GTK_LABEL(tips_text), true);

        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(tips_dialog, "show", G_CALLBACK(on_tips_show), 0);
        g_signal_connect(tips_dialog, "key-press-event", G_CALLBACK(on_tips_keypress), 0);
    }

    /** --Info Dialog **/
    {
        info_dialog = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
        gtk_container_set_border_width(GTK_CONTAINER(info_dialog), 10);
        gtk_window_set_title(GTK_WINDOW(info_dialog), "Info");
        gtk_window_set_resizable(GTK_WINDOW(info_dialog), true);
        gtk_window_set_position(GTK_WINDOW(info_dialog), GTK_WIN_POS_CENTER);
        //gtk_window_set_keep_above(GTK_WINDOW(info_dialog), TRUE);
        gtk_window_set_default_size(GTK_WINDOW(info_dialog), 425, 400);

        info_name = GTK_LABEL(gtk_label_new(0));
        info_text = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_selectable(info_text, 1);
        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(info_text));
        //gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(info_name), 0, 0, 0);
        //gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(ew), 1, 1, 3);
        gtk_container_add(GTK_CONTAINER(info_dialog), GTK_WIDGET(ew));

        gtk_label_set_line_wrap(GTK_LABEL(info_text), true);

        //gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(info_dialog, "show", G_CALLBACK(on_info_show), 0);
        g_signal_connect(info_dialog, "delete-event", G_CALLBACK(on_window_close), 0);

        g_signal_connect(info_dialog, "key-press-event", G_CALLBACK(on_info_keypress), 0);
    }

    /** --Error Dialog **/
    {
        error_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Errors",
                0, (GtkDialogFlags)(0),/*GTK_MODAL*/
                "OK", GTK_RESPONSE_ACCEPT,
                NULL));

        apply_dialog_defaults(error_dialog);

        gtk_window_set_default_size(GTK_WINDOW(error_dialog), 425, 400);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(error_dialog));

        error_text = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_selectable(error_text, 1);
        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(error_text));
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(ew), 1, 1, 3);

        gtk_label_set_line_wrap(GTK_LABEL(error_text), true);

        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(error_dialog, "show", G_CALLBACK(on_error_show), 0);
        g_signal_connect(error_dialog, "delete-event", G_CALLBACK(on_window_close), 0);
        g_signal_connect(error_dialog, "key-press-event", G_CALLBACK(on_error_keypress), 0);
    }

    /** --Confirm Dialog **/
    {
        dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Confirm",
                0, (GtkDialogFlags)(0),/*GTK_MODAL*/
                NULL, NULL));

        apply_dialog_defaults(dialog);

        confirm_button1 = GTK_BUTTON(
                gtk_dialog_add_button(
                    dialog,
                    "Button1", 1)
                );
        confirm_button2 = GTK_BUTTON(
                gtk_dialog_add_button(
                    dialog,
                    "Button2", 2)
                );

        confirm_button3 = GTK_BUTTON(
                gtk_dialog_add_button(
                    dialog,
                    "Button3", 3)
                );

        confirm_dna_sandbox_back = new_check_button("Do not show again");
        //gtk_window_set_default_size(GTK_WINDOW(dialog), 425, 400);

        //GtkBox *aa = GTK_BOX(gtk_dialog_get_action_area(dialog));

        //gtk_dialog_add_action_widget(dialog, GTK_WIDGET(confirm_dna_sandbox_back), -1);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        confirm_text = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_selectable(confirm_text, 1);
        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(confirm_text));
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(ew), 1, 1, 3);

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(confirm_dna_sandbox_back), 0, 0, 3);

        gtk_label_set_line_wrap(GTK_LABEL(confirm_text), true);

        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(dialog, "show", G_CALLBACK(on_confirm_show), 0);
        g_signal_connect(dialog, "key-press-event", G_CALLBACK(on_confirm_keypress), 0);

        confirm_dialog = dialog;
    }

    /** --Alert Dialog **/
    {
        dialog = GTK_DIALOG(gtk_message_dialog_new(
                0, (GtkDialogFlags)(0),
                GTK_MESSAGE_INFO,
                GTK_BUTTONS_CLOSE,
                "Alert"));

        apply_dialog_defaults(dialog);

        g_signal_connect(dialog, "show", G_CALLBACK(on_alert_show), 0);
        g_signal_connect(dialog, "key-press-event", G_CALLBACK(on_alert_keypress), 0);

        alert_dialog = GTK_MESSAGE_DIALOG(dialog);
    }

    /** --Frequency Dialog **/
    {
        frequency_window = new_window_defaults("Set Frequency", &on_frequency_show, &on_frequency_keypress);
        gtk_window_set_default_size(GTK_WINDOW(frequency_window), 325, 300);
        gtk_widget_set_size_request(GTK_WIDGET(frequency_window), 325, 300);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        frequency_value = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 0xFFFFFFFF, 1, 1, 0)),
                    1, 0));

        gtk_box_pack_start(content, new_lbl("<b>Current Frequency:</b>"), false, false, 0);
        gtk_box_pack_start(content, GTK_WIDGET(frequency_value), false, false, 0);

        gtk_box_pack_start(content, new_lbl("<b>Used Frequencies:</b>"), false, false, 0);

        GtkListStore *store;

        store = gtk_list_store_new(FC_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);

        frequency_treemodel = GTK_TREE_MODEL(store);

        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), FC_FREQUENCY, GTK_SORT_ASCENDING);

        GtkWidget *view = gtk_tree_view_new_with_model(frequency_treemodel);
        gtk_tree_view_set_search_column(GTK_TREE_VIEW(view), FC_FREQUENCY);
        g_signal_connect(view, "row-activated", G_CALLBACK(activate_frequency_row), 0);

        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);

        /* Initialize Frequency buttons & button box */
        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);
        frequency_ok = GTK_BUTTON(gtk_button_new_with_label("OK"));
        frequency_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(frequency_ok, "clicked",
                G_CALLBACK(on_frequency_click), 0);
        g_signal_connect(frequency_cancel, "clicked",
                G_CALLBACK(on_frequency_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(frequency_ok));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(frequency_cancel));

        /* Add everything together ????? */
        gtk_container_add(GTK_CONTAINER(ew), view);
        gtk_box_pack_start(content, GTK_WIDGET(ew), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);
        //gtk_container_add(GTK_CONTAINER(content), GTK_WIDGET(button_box));
        gtk_container_add(GTK_CONTAINER(frequency_window), GTK_WIDGET(content));

        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;

        renderer = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
        column = GTK_TREE_VIEW_COLUMN(gtk_tree_view_column_new_with_attributes("Frequency", renderer, "text", FC_FREQUENCY, NULL));
        gtk_tree_view_column_set_sort_column_id(column, FC_FREQUENCY);
        gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

        renderer = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
        column = GTK_TREE_VIEW_COLUMN(gtk_tree_view_column_new_with_attributes("Receivers", renderer, "text", FC_RECEIVERS, NULL));
        gtk_tree_view_column_set_sort_column_id(column, FC_RECEIVERS);
        gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

        renderer = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
        column = GTK_TREE_VIEW_COLUMN(gtk_tree_view_column_new_with_attributes("Transmitters", renderer, "text", FC_TRANSMITTERS, NULL));
        gtk_tree_view_column_set_sort_column_id(column, FC_TRANSMITTERS);
        gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    }

    /** --Frequency range Dialog **/
    {
        freq_range_window = new_window_defaults("Set Frequency Range", &on_freq_range_show, &on_freq_range_keypress);
        gtk_window_set_default_size(GTK_WINDOW(freq_range_window), 325, 300);
        gtk_widget_set_size_request(GTK_WIDGET(freq_range_window), 325, 300);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkBox *tbl_freq_range = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
        gtk_widget_set_halign(GTK_WIDGET(tbl_freq_range), GTK_ALIGN_CENTER);
        {
            freq_range_value = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 0xFFFFFFFF-255, 1, 1, 0)),
                        1, 0));

            freq_range_offset = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 255, 1, 1, 0)),
                        1, 0));

            gtk_box_pack_start(
                tbl_freq_range,
                gtk_label_new("Begin:"),
                false, false, 0
            );
            gtk_box_pack_start(
                tbl_freq_range,
                GTK_WIDGET(freq_range_value),
                false, false, 0
            );

            gtk_box_pack_start(
                tbl_freq_range,
                gtk_label_new("Range:"),
                false, false, 0
            );
            gtk_box_pack_start(
                tbl_freq_range,
                GTK_WIDGET(freq_range_offset),
                false, false, 0
            );
        }

        GValue val = {0};

        g_value_init(&val, G_TYPE_BOOLEAN);
        g_value_set_boolean(&val, TRUE);

        g_object_set_property(G_OBJECT(freq_range_value), "numeric", &val);
        g_object_set_property(G_OBJECT(freq_range_offset), "numeric", &val);

        g_value_unset(&val);

        g_signal_connect(freq_range_value, "value-changed", G_CALLBACK(freq_range_value_changed), 0);
        g_signal_connect(freq_range_value, "changed", G_CALLBACK(freq_range_value_text_changed), 0);
        g_signal_connect(freq_range_offset, "value-changed", G_CALLBACK(freq_range_value_changed), 0);
        g_signal_connect(freq_range_offset, "changed", G_CALLBACK(freq_range_value_text_changed), 0);

        gtk_box_pack_start(content, GTK_WIDGET(tbl_freq_range), false, false, 0);

        freq_range_info = GTK_LABEL(gtk_label_new("Frequencies: 1-100"));
        gtk_box_pack_start(content, GTK_WIDGET(freq_range_info), false, false, 0);

        gtk_box_pack_start(content, new_lbl("<b>Used Frequencies:</b>"), false, false, 0);

        GtkListStore *store;

        store = gtk_list_store_new(FC_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);

        freq_range_treemodel = GTK_TREE_MODEL(store);

        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), FC_FREQUENCY, GTK_SORT_ASCENDING);

        GtkWidget *view = gtk_tree_view_new_with_model(freq_range_treemodel);
        gtk_tree_view_set_search_column(GTK_TREE_VIEW(view), FC_FREQUENCY);
        g_signal_connect(view, "row-activated", G_CALLBACK(activate_freq_range_row), 0);

        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);

        /* Initialize Frequency buttons & button box */
        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);
        freq_range_ok = GTK_BUTTON(gtk_button_new_with_label("OK"));
        freq_range_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(freq_range_ok, "clicked",
                G_CALLBACK(on_freq_range_click), 0);
        g_signal_connect(freq_range_cancel, "clicked",
                G_CALLBACK(on_freq_range_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(freq_range_ok));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(freq_range_cancel));

        /* Add everything together ????? */
        gtk_container_add(GTK_CONTAINER(ew), view);
        gtk_box_pack_start(content, GTK_WIDGET(ew), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);
        //gtk_container_add(GTK_CONTAINER(content), GTK_WIDGET(button_box));
        gtk_container_add(GTK_CONTAINER(freq_range_window), GTK_WIDGET(content));

        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;

        renderer = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
        column = GTK_TREE_VIEW_COLUMN(gtk_tree_view_column_new_with_attributes("Frequency", renderer, "text", FC_FREQUENCY, NULL));
        gtk_tree_view_column_set_sort_column_id(column, FC_FREQUENCY);
        gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

        renderer = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
        column = GTK_TREE_VIEW_COLUMN(gtk_tree_view_column_new_with_attributes("Receivers", renderer, "text", FC_RECEIVERS, NULL));
        gtk_tree_view_column_set_sort_column_id(column, FC_RECEIVERS);
        gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

        renderer = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
        column = GTK_TREE_VIEW_COLUMN(gtk_tree_view_column_new_with_attributes("Transmitters", renderer, "text", FC_TRANSMITTERS, NULL));
        gtk_tree_view_column_set_sort_column_id(column, FC_TRANSMITTERS);
        gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    }

    /** --Multi config **/
    {
        multi_config_window = new_window_defaults("Multi config", &on_multi_config_show);
        gtk_window_set_default_size(GTK_WINDOW(multi_config_window), 600, 350);
        gtk_widget_set_size_request(GTK_WIDGET(multi_config_window), 600, 350);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *entries = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkNotebook *nb = GTK_NOTEBOOK(gtk_notebook_new());
        gtk_notebook_set_tab_pos(nb, GTK_POS_TOP);
        g_signal_connect(nb, "switch-page", G_CALLBACK(on_multi_config_tab_changed), 0);

        /* Buttons and button box */
        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Log in button */
        multi_config_apply = GTK_BUTTON(gtk_button_new_with_label("Apply"));
        g_signal_connect(multi_config_apply, "clicked",
                G_CALLBACK(on_multi_config_btn_click), 0);

        /* Cancel button */
        multi_config_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(multi_config_cancel, "clicked",
                G_CALLBACK(on_multi_config_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(multi_config_apply));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(multi_config_cancel));

        {
            /* Joint strength */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_joint_strength = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05));
            g_signal_connect(multi_config_joint_strength, "format-value", G_CALLBACK(format_joint_strength), 0);

            gtk_box_pack_start(box, GTK_WIDGET(multi_config_joint_strength), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("Settings a new joint might make your selection change it's position/state slightly.\nMake sure you save your level before you press Apply."), 0, 0, 0);

            notebook_append(nb, "Joint strength", box);
        }

        {
            /* Plastic color */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_plastic_color = GTK_COLOR_CHOOSER_WIDGET(gtk_color_chooser_widget_new());

            gtk_box_pack_start(box, GTK_WIDGET(multi_config_plastic_color), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("This will change the color of all plastic objects in your current selection."), 1, 1, 0);

            notebook_append(nb, "Plastic color", box);
        }

        {
            /* Plastic density */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_plastic_density = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05));

            gtk_box_pack_start(box, GTK_WIDGET(multi_config_plastic_density), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("This will change the density of all plastic objects in your current selection."), 1, 1, 0);

            notebook_append(nb, "Plastic density", box);
        }

        {
            /* Connection render type */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_render_type_normal = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(
                        0, "Default"));
            multi_config_render_type_small = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(
                        gtk_radio_button_get_group(multi_config_render_type_normal), "Small"));
            multi_config_render_type_hide = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(
                        gtk_radio_button_get_group(multi_config_render_type_normal), "Hide"));

            gtk_box_pack_start(box, GTK_WIDGET(multi_config_render_type_normal), 0, 0, 0);
            gtk_box_pack_start(box, GTK_WIDGET(multi_config_render_type_small), 0, 0, 0);
            gtk_box_pack_start(box, GTK_WIDGET(multi_config_render_type_hide), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("This will change the render type of all connections in your current selection."), 1, 1, 0);

            notebook_append(nb, "Connection render type", box);
        }

        {
            /* Miscellaneous */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_unlock_all = new_lbtn("Unlock all", &on_multi_config_btn_click);
            multi_config_disconnect_all = new_lbtn("Disconnect all", &on_multi_config_btn_click);

            {
                GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
                gtk_box_pack_start(hbox, GTK_WIDGET(multi_config_unlock_all), 1, 1, 0);
                gtk_box_pack_start(hbox, help_widget("Unlock any previously locked entities.\nOnly active if at least one of the selected entities is locked."), 0, 0, 0);

                gtk_box_pack_start(box, GTK_WIDGET(hbox), 0, 0, 0);
            }
            gtk_box_pack_start(box, GTK_WIDGET(multi_config_disconnect_all), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("Click on any of the buttons above to perform the given action on your current selection."), 1, 1, 0);

            notebook_append(nb, "Miscellaneous", box);
        }

        gtk_box_pack_start(entries, GTK_WIDGET(nb), 1, 1, 0);

        multi_config_nb = nb;

        gtk_box_pack_start(content, GTK_WIDGET(entries), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(multi_config_window), GTK_WIDGET(content));
    }

    /** --Login **/
    {
        login_window = new_window_defaults("Log in", &on_login_show, &on_login_keypress);
        gtk_window_set_default_size(GTK_WINDOW(login_window), 400, 150);
        gtk_widget_set_size_request(GTK_WIDGET(login_window), 400, 150);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *entries = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *bottom_content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

        /* Username entry */
        login_username = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_max_length(login_username, 255);
        gtk_entry_set_activates_default(login_username, true);

        /* Password entry */
        login_password = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_max_length(login_password, 255);
        gtk_entry_set_visibility(login_password, false);

        /* Username label */
        gtk_box_pack_start(entries, new_lbl("<b>Username:</b>"), false, false, 0);
        gtk_box_pack_start(entries, GTK_WIDGET(login_username), false, false, 0);

        /* Password label */
        gtk_box_pack_start(entries, new_lbl("<b>Password:</b>"), false, false, 0);
        gtk_box_pack_start(entries, GTK_WIDGET(login_password), false, false, 0);

        /* Buttons and button box */
        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Log in button */
        login_btn_log_in = GTK_BUTTON(gtk_button_new_with_mnemonic("_Login"));
        g_signal_connect(login_btn_log_in, "clicked",
                G_CALLBACK(on_login_btn_click), 0);

        /* Cancel button */
        login_btn_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(login_btn_cancel, "clicked",
                G_CALLBACK(on_login_btn_click), 0);

        /* Register button */
        login_btn_register = GTK_BUTTON(gtk_button_new_with_label("Register"));
        g_signal_connect(login_btn_register, "clicked",
                G_CALLBACK(on_login_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(login_btn_log_in));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(login_btn_cancel));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(login_btn_register));

        /* Status label */
        login_status = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_xalign(GTK_LABEL(login_status), 0.0f);
        gtk_label_set_yalign(GTK_LABEL(login_status), 0.5f);

        gtk_box_pack_start(bottom_content, GTK_WIDGET(login_status), 1, 1, 0);
        gtk_box_pack_start(bottom_content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_box_pack_start(content, GTK_WIDGET(entries), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(bottom_content), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(login_window), GTK_WIDGET(content));

        g_signal_connect(login_window, "hide", G_CALLBACK(on_login_hide), 0);
    }

    /** --Settings **/
    {
        settings_dialog = new_dialog_defaults("Settings", &on_settings_show);
        gtk_widget_set_size_request(GTK_WIDGET(settings_dialog), 550, -1);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(settings_dialog));

        GtkNotebook *nb = GTK_NOTEBOOK(gtk_notebook_new());
        gtk_notebook_set_tab_pos(nb, GTK_POS_TOP);

        GtkGrid *tbl_graphics;
        {
            GtkGrid *tbl = create_settings_table();

            int y = -1;

            settings_enable_shadows = GTK_CHECK_BUTTON(gtk_check_button_new());
            settings_shadow_quality = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 1, 1, 1, 0)),
                    1,0));
            settings_shadow_res = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
            gtk_combo_box_text_append_text(settings_shadow_res, "(native)");
            gtk_combo_box_text_append_text(settings_shadow_res, "2048x2048");
            gtk_combo_box_text_append_text(settings_shadow_res, "2048x1024");
            gtk_combo_box_text_append_text(settings_shadow_res, "1024x1024");
            gtk_combo_box_text_append_text(settings_shadow_res, "1024x512");
            gtk_combo_box_text_append_text(settings_shadow_res, "512x512");
            gtk_combo_box_text_append_text(settings_shadow_res, "512x256");

            settings_enable_ao = GTK_CHECK_BUTTON(gtk_check_button_new());
            settings_ao_res = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
            gtk_combo_box_text_append_text(settings_ao_res, "512x512");
            gtk_combo_box_text_append_text(settings_ao_res, "256x256");
            gtk_combo_box_text_append_text(settings_ao_res, "128x128");

            settings_enable_bloom = GTK_CHECK_BUTTON(gtk_check_button_new());

            add_setting_row(
                tbl, ++y,
                "Enable shadows",
                GTK_WIDGET(settings_enable_shadows)
            );

            add_setting_row(
                tbl, ++y,
                "Shadow quality",
                GTK_WIDGET(settings_shadow_quality),
                "Shadow quality 0: Sharp\nShadow quality 1: Smooth"
            );

            add_setting_row(
                tbl, ++y,
                "Shadow resolution",
                GTK_WIDGET(settings_shadow_res)
            );

            add_setting_row(
                tbl, ++y,
                "Enable AO",
                GTK_WIDGET(settings_enable_ao)
            );

            add_setting_row(
                tbl, ++y,
                "AO map resolution",
                GTK_WIDGET(settings_ao_res)
            );

            for (int x=0; x<settings_num_graphic_rows; ++x) {
                struct table_setting_row *r = &settings_graphic_rows[x];
                create_setting_row_widget(r);
                add_setting_row(
                    tbl, ++y,
                    r->label,
                    r->wdg,
                    r->help
                );
            }

            tbl_graphics = tbl;
        }

        GtkGrid *tbl_audio;
        {
            GtkGrid *tbl = create_settings_table();
            int y = -1;

            for (int x=0; x<settings_num_audio_rows; ++x) {
                struct table_setting_row *r = &settings_audio_rows[x];

                create_setting_row_widget(r);

                add_setting_row(
                    tbl, ++y,
                    r->label,
                    r->wdg,
                    r->help
                );
            }

            tbl_audio = tbl;
        }

        GtkGrid *tbl_controls;
        {
            GtkGrid *tbl = create_settings_table();

            int y = -1;

            settings_control_type = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
            gtk_combo_box_text_append_text(settings_control_type, "Keyboard");
            gtk_combo_box_text_append_text(settings_control_type, "Keyboard+Mouse");

            add_setting_row(
                tbl, y,
                "Control type",
                GTK_WIDGET(settings_control_type)
            );

            for (int x=0; x<settings_num_control_rows; ++x) {
                struct table_setting_row *r = &settings_control_rows[x];
                create_setting_row_widget(r);
                add_setting_row(
                    tbl, ++y,
                    r->label,
                    r->wdg,
                    r->help
                );
            }

            tbl_controls = tbl;
        }

        GtkGrid *tbl_interface;
        {
            GtkGrid *tbl = create_settings_table();

            int y = -1;

            for (int x=0; x<settings_num_interface_rows; ++x) {
                struct table_setting_row *r = &settings_interface_rows[x];

                create_setting_row_widget(r);

                add_setting_row(
                    tbl, ++y,
                    r->label,
                    r->wdg,
                    r->help
                );
            }

            tbl_interface = tbl;
        }

        gtk_notebook_append_page(nb, GTK_WIDGET(tbl_graphics),  new_lbl("<b>Graphics</b>"));
        gtk_notebook_append_page(nb, GTK_WIDGET(tbl_audio),     new_lbl("<b>Audio</b>"));
        gtk_notebook_append_page(nb, GTK_WIDGET(tbl_controls),  new_lbl("<b>Controls</b>"));
        gtk_notebook_append_page(nb, GTK_WIDGET(tbl_interface), new_lbl("<b>Interface</b>"));

        gtk_widget_show_all(GTK_WIDGET(nb));

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(nb), true, true, 0);

        gtk_notebook_set_current_page(nb,0);
        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Confirm Quit Dialog **/
    {
        confirm_quit_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Confirm Quit",
            0, (GtkDialogFlags)(0),/*GTK_MODAL*/
            NULL, NULL
        ));

        apply_dialog_defaults(confirm_quit_dialog);

        confirm_btn_quit = GTK_BUTTON(gtk_dialog_add_button(confirm_quit_dialog, "_Quit", GTK_RESPONSE_ACCEPT));
        gtk_dialog_add_button(confirm_quit_dialog, "_Cancel", GTK_RESPONSE_REJECT);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(confirm_quit_dialog));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Are you sure you want to quit?\nAny unsaved changes will be lost!</b>"), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(confirm_quit_dialog, "show", G_CALLBACK(on_confirm_quit_show), 0);
    }

    /** --Robot **/
    {
        robot_window = new_window_defaults("Robot settings", &on_robot_show, &on_robot_keypress);

        int y = 0;

        robot_head_equipment = new_item_cb();
        robot_head = new_item_cb();
        robot_back_equipment = new_item_cb();
        robot_front_equipment = new_item_cb();
        robot_feet = new_item_cb();
        robot_bolts = new_item_cb();

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkGrid *tbl = GTK_GRID(gtk_grid_new());
        gtk_grid_set_row_spacing(tbl, 3);
        gtk_grid_set_column_spacing(tbl, 5);

        robot_state_idle = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(
            NULL, "Idle"
        ));
        robot_state_walk = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label_from_widget(
            robot_state_idle, "Walking"
        ));
        robot_state_dead = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label_from_widget(
            robot_state_idle, "Dead"
        ));

        robot_dir_left = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(
            NULL, "Left"
        ));
        robot_dir_random = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label_from_widget(
            robot_dir_left, "Random"
        ));
        robot_dir_right = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label_from_widget(
            robot_dir_left, "Right"
        ));

        gtk_grid_attach(tbl, new_rlbl("Default state"), 0, y, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(robot_state_idle), 1, y, 1, 1);
        y++;
        gtk_grid_attach(tbl, GTK_WIDGET(robot_state_walk), 1, y, 1, 1);
        y++;
        gtk_grid_attach(tbl, GTK_WIDGET(robot_state_dead), 1, y, 1, 1);
        y++;

        robot_roam = GTK_CHECK_BUTTON(gtk_check_button_new());
        gtk_grid_attach(tbl, new_rlbl("Roaming"), 0, y, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(robot_roam), 1, y, 1, 1);
        y++;

        gtk_grid_attach(tbl, gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, y, 2, 1);
        y++;

        gtk_grid_attach(tbl, new_rlbl("Initial direction"), 0, y, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(robot_dir_left), 1, y, 1, 1);
        y++;
        gtk_grid_attach(tbl, GTK_WIDGET(robot_dir_random), 1, y, 1, 1);
        y++;
        gtk_grid_attach(tbl, GTK_WIDGET(robot_dir_right), 1, y, 1, 1);
        y++;

        gtk_grid_attach(tbl, gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, y, 2, 1);
        y++;

        gtk_grid_attach(tbl, new_rlbl("Faction"), 0, y, 1, 1);
        for (int x = 0; x < NUM_FACTIONS; ++x) {
            robot_faction[x] = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(
                x==0 ? 0 : gtk_radio_button_get_group(robot_faction[0]), factions[x].name)
            );
            gtk_grid_attach(tbl, GTK_WIDGET(robot_faction[x]), 1, y, 1, 1);
            y++;
        }

        gtk_grid_attach(tbl, new_rlbl("Head equipment"), 0, y, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(robot_head_equipment), 1, y, 1, 1);
        y++;

        gtk_grid_attach(tbl, new_rlbl("Head"), 0, y, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(robot_head), 1, y, 1, 1);
        y++;

        gtk_grid_attach(tbl, new_rlbl("Back equipment"), 0, y, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(robot_back_equipment), 1, y, 1, 1);
        y++;

        gtk_grid_attach(tbl, new_rlbl("Front equipment"), 0, y, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(robot_front_equipment), 1, y, 1, 1);
        y++;

        gtk_grid_attach(tbl, new_rlbl("Feet"), 0, y, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(robot_feet), 1, y, 1, 1);
        y++;

        gtk_grid_attach(tbl, new_rlbl("Bolt set"), 0, y, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(robot_bolts), 1, y, 1, 1);
        y++;

        {
            /*                                         equipped        name           ID */
            robot_ls_equipment = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_INT);
            GtkTreeModel *model = GTK_TREE_MODEL(robot_ls_equipment);

            robot_tv_equipment = GTK_TREE_VIEW(gtk_tree_view_new_with_model(model));

            GtkCellRenderer *renderer;
            GtkTreeViewColumn *column;
            model = gtk_tree_view_get_model(robot_tv_equipment);

            renderer = gtk_cell_renderer_toggle_new();
            g_signal_connect(renderer, "toggled", G_CALLBACK(robot_item_toggled), model);
            column = gtk_tree_view_column_new_with_attributes(
                "Equipped",
                renderer,
                "active",
                ROBOT_COLUMN_EQUIPPED,
                NULL
            );
            gtk_tree_view_column_set_sizing(GTK_TREE_VIEW_COLUMN(column), GTK_TREE_VIEW_COLUMN_FIXED);
            gtk_tree_view_append_column(robot_tv_equipment, column);

            renderer = gtk_cell_renderer_text_new();
            column = gtk_tree_view_column_new_with_attributes(
                "Item",
                renderer,
                "text",
                ROBOT_COLUMN_ITEM,
                NULL
            );
            gtk_tree_view_column_set_expand(column, true);
            gtk_tree_view_column_set_sort_column_id(column, 2);
            gtk_tree_view_append_column(robot_tv_equipment, column);
        }

        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        robot_btn_ok = GTK_BUTTON(gtk_button_new_with_label("OK"));
        g_signal_connect(
            robot_btn_ok, "clicked",
            G_CALLBACK(on_robot_btn_click), NULL
        );

        robot_btn_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(
            robot_btn_cancel, "clicked",
            G_CALLBACK(on_robot_btn_click), NULL
        );

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(robot_btn_ok));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(robot_btn_cancel));

        GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

        GtkScrolledWindow *robot_tv_equipment_scroll = GTK_SCROLLED_WINDOW(
            gtk_scrolled_window_new(NULL, NULL)
        );
        gtk_container_add(
            GTK_CONTAINER(robot_tv_equipment_scroll),
            GTK_WIDGET(robot_tv_equipment)
        );
        gtk_scrolled_window_set_propagate_natural_width(
            robot_tv_equipment_scroll, true
        );
        gtk_scrolled_window_set_policy(
            robot_tv_equipment_scroll,
            GTK_POLICY_NEVER,
            GTK_POLICY_AUTOMATIC
        );

        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(tbl), true, true, 0);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(robot_tv_equipment_scroll), true, true, 0);

        gtk_box_pack_start(content, GTK_WIDGET(hbox), true, true, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(robot_window), GTK_WIDGET(content));
    }

    /** --Puzzle play **/
    {
        puzzle_play_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Play method",
            0, (GtkDialogFlags)(0),/*GTK_MODAL*/
            "Test play", PUZZLE_TEST_PLAY,
            "Simulate", PUZZLE_SIMULATE,
            "_Cancel", GTK_RESPONSE_CANCEL,
            NULL
        ));

        apply_dialog_defaults(puzzle_play_dialog);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(puzzle_play_dialog));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("Do you want to test-play the level, or just simulate it?"), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Published **/
    {
        published_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Level published!",
            0, (GtkDialogFlags)(0),/*GTK_MODAL*/
            "Go to level page", GTK_RESPONSE_ACCEPT,
            "_Cancel", GTK_RESPONSE_REJECT,
            NULL
        ));

        apply_dialog_defaults(published_dialog);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(published_dialog));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("Your level has been successfully published on the community website."), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), new_lbl("To view your level, or submit it to a running contest, please click the button below."), false, false, 0);

        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Community **/
    {
        dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Back to main menu?",
            0, (GtkDialogFlags)(0),/*GTK_MODAL*/
            "Yes", GTK_RESPONSE_ACCEPT,
            "No", GTK_RESPONSE_REJECT,
            NULL
        ));

        apply_dialog_defaults(dialog);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("Do you want to return to the main menu?"), false, false, 0);

        gtk_widget_show_all(GTK_WIDGET(content));

        community_dialog = dialog;
    }

    /** --Jumper **/
    {
        jumper_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Jumper",
            0, (GtkDialogFlags)(0),/*GTK_MODAL*/
            NULL, NULL
        ));

        apply_dialog_defaults(jumper_dialog);

        jumper_save = GTK_BUTTON(gtk_dialog_add_button(
            jumper_dialog,
            "_Save", GTK_RESPONSE_ACCEPT
        ));
        jumper_cancel = GTK_BUTTON(gtk_dialog_add_button(
            jumper_dialog,
            "_Cancel", GTK_RESPONSE_CANCEL
        ));

        g_signal_connect(jumper_dialog, "show", G_CALLBACK(on_jumper_show), 0);
        g_signal_connect(jumper_dialog, "key-press-event", G_CALLBACK(on_jumper_keypress), 0);

        gtk_widget_set_size_request(GTK_WIDGET(jumper_dialog), 350, -1);
        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(jumper_dialog));

        GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

        jumper_value = GTK_RANGE(gtk_scale_new(
            GTK_ORIENTATION_HORIZONTAL,
            GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1.0, 0.001, 0.1, 0.0))
        ));
        gtk_scale_set_digits(GTK_SCALE(jumper_value), 4);

        g_signal_connect(jumper_value, "value-changed", G_CALLBACK(jumper_value_changed), 0);

        jumper_value_entry = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_width_chars(jumper_value_entry, 7);

        g_signal_connect(jumper_value_entry, "changed", G_CALLBACK(jumper_value_entry_changed), 0);
        g_signal_connect(jumper_value_entry, "insert-text", G_CALLBACK(jumper_value_entry_insert_text), 0);

        gtk_scale_set_draw_value(GTK_SCALE(jumper_value), false);

        gtk_box_pack_start(GTK_BOX(hbox), new_clbl("Value"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(jumper_value), true, true, 0);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(jumper_value_entry), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(hbox), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --cursorfield **/
    {
        cursorfield_dialog = new_dialog_defaults("Cursor field", &on_cursorfield_show);

        gtk_widget_set_size_request(GTK_WIDGET(cursorfield_dialog), 350, -1);
        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(cursorfield_dialog));

        GtkGrid *tbl_settings = GTK_GRID(gtk_grid_new());
        {
            int y = 0;

            cursorfield_right = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -3, 3, .1));
            cursorfield_up = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -3, 3, .1));
            cursorfield_left = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -3, 3, .1));
            cursorfield_down = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -3, 3, .1));

            gtk_widget_set_hexpand(GTK_WIDGET(cursorfield_right), true);
            gtk_widget_set_hexpand(GTK_WIDGET(cursorfield_up), true);
            gtk_widget_set_hexpand(GTK_WIDGET(cursorfield_left), true);
            gtk_widget_set_hexpand(GTK_WIDGET(cursorfield_down), true);

            gtk_grid_attach(tbl_settings, new_rlbl("Lower X"), 0, y, 1, 1);
            gtk_grid_attach(tbl_settings, GTK_WIDGET(cursorfield_left), 1, y, 1, 1);
            y++;

            gtk_grid_attach(tbl_settings, new_rlbl("Upper X"), 0, y, 1, 1);
            gtk_grid_attach(tbl_settings, GTK_WIDGET(cursorfield_right), 1, y, 1, 1);
            y++;

            gtk_grid_attach(tbl_settings, new_rlbl("Lower Y"), 0, y, 1, 1);
            gtk_grid_attach(tbl_settings, GTK_WIDGET(cursorfield_down), 1, y, 1, 1);
            y++;

            gtk_grid_attach(tbl_settings, new_rlbl("Upper Y"), 0, y, 1, 1);
            gtk_grid_attach(tbl_settings, GTK_WIDGET(cursorfield_up), 1, y, 1, 1);
        }

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(tbl_settings), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --escript **/
    {
        escript_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
        gtk_window_set_title(escript_window, "Lua Script");
        gtk_widget_set_size_request(GTK_WIDGET(escript_window), 800, 560);
        gtk_window_set_resizable(escript_window, true);
        // gtk_window_set_policy(escript_window,
        //               FALSE,
        //               FALSE, FALSE);
        gtk_window_set_position(GTK_WINDOW(escript_window), GTK_WIN_POS_CENTER);
        //gtk_window_set_keep_above(GTK_WINDOW(escript_window), TRUE);

        escript_statusbar = GTK_STATUSBAR(gtk_statusbar_new());

        g_signal_connect(escript_window, "show", G_CALLBACK(on_escript_show), 0);
        g_signal_connect(escript_window, "key-press-event", G_CALLBACK(on_escript_keypress), 0);
        g_signal_connect(escript_window, "delete-event", G_CALLBACK(on_window_close), 0);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkWidget *cb;

        {
            cb = gtk_check_button_new_with_label("Use external editor");
            g_signal_connect(cb, "toggled", G_CALLBACK(on_escript_external_editor_toggled), 0);
            gtk_widget_set_tooltip_text(cb, "Check this file if you want to edit the Lua from an external editor");
            escript_use_external_editor = GTK_CHECK_BUTTON(cb);
        }

        // GtkTextTagTable *tt = gtk_text_tag_table_new();
        // escript_buffer = gtk_source_buffer_new(tt);

        //escript_buffer = GTK_SOURCE_BUFFER(gtk_source_buffer_new(NULL));



        // escript_tt_function = gtk_text_buffer_create_tag(GTK_TEXT_BUFFER(escript_buffer),
        //                                                  "function",
        //                                                  "foreground", "#ff00ff",
        //                                                  NULL);

        //g_signal_connect(escript_buffer, "mark-set", G_CALLBACK(on_escript_mark_set), 0);

        //escript_code = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(escript_buffer));

        //gtk_widget_modify_font(GTK_WIDGET(escript_code), pango_font_description_from_string("monospace"));

        /*
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(escript_code);
        gtk_text_buffer_create_tag(buffer, "font")
        */

#ifdef USE_GTK_SOURCE_VIEW
        {

            //Create GtkSourceLanguageManager and get lua GtkSourceLanguage
            GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default();
            GtkSourceLanguage *l = gtk_source_language_manager_guess_language(lm, ".lua", NULL);
            if (l == NULL) tms_warnf("ESCRIPT: no lang definition for lua");

            //Create GtkSourceStyleSchemeManager and get Adwaita GtkSourceStyleScheme
            GtkSourceStyleSchemeManager *sm = gtk_source_style_scheme_manager_get_default();
            GtkSourceStyleScheme *s = gtk_source_style_scheme_manager_get_scheme(sm, "cobalt");
            if (s == NULL) tms_warnf("ESCRIPT: source theme not found");

            //Create new escript_buffer
            escript_buffer = GTK_TEXT_BUFFER(gtk_source_buffer_new(NULL));
            g_signal_connect(GTK_SOURCE_BUFFER(escript_buffer), "mark-set", G_CALLBACK(on_escript_mark_set), 0);

            //Enable highlighting for the buffer
            gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(escript_buffer), l);
            gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(escript_buffer), TRUE);
            gtk_source_buffer_set_style_scheme(GTK_SOURCE_BUFFER(escript_buffer), s);

            //Create GtkSourceView
            escript_code = gtk_source_view_new_with_buffer(GTK_SOURCE_BUFFER(escript_buffer));

            //Enable fancy-ass stuf
            gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(GTK_SOURCE_VIEW(escript_code)), TRUE);
            gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(escript_code), TRUE);
            gtk_source_view_set_indent_on_tab(GTK_SOURCE_VIEW(escript_code), TRUE);
            gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(escript_code), 4);
            gtk_source_view_set_indent_width(GTK_SOURCE_VIEW(escript_code), -1);
            gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(escript_code), TRUE);
            gtk_source_view_set_smart_backspace(GTK_SOURCE_VIEW(escript_code), TRUE);
            gtk_source_view_set_smart_home_end(GTK_SOURCE_VIEW(escript_code), GTK_SOURCE_SMART_HOME_END_BEFORE);
            gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(escript_code), TRUE);
        }
#else
        escript_buffer = GTK_TEXT_BUFFER(gtk_text_buffer_new(NULL));
        escript_code = gtk_text_view_new_with_buffer(GTK_TEXT_BUFFER(escript_buffer));
#endif

        //Connect mark-set
        g_signal_connect(GTK_TEXT_BUFFER(escript_buffer), "mark-set", G_CALLBACK(on_escript_mark_set), 0);

        //Add .code-editor class
        GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(escript_code));
        gtk_style_context_add_class(context, "code-editor");

        escript_external_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL,0));
        escript_external_file_path = GTK_LABEL(new_clbl("Placeholder"));

        gtk_box_pack_start(escript_external_box, GTK_WIDGET(escript_external_file_path), false, false, 5);
        gtk_box_pack_start(escript_external_box, new_clbl("Open the file path above with your favourite code editor and edit the code there.\nBefore you press play in Principia, remember to save the external file!\nThe file will be created when you press the Save-button."), false, false, 5);

        GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
        gtk_box_pack_start(GTK_BOX(hbox), new_lbl("  "), false, false, 0);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(escript_use_external_editor), false, false, 0);

        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(escript_code));

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(ew), true, true, 5);

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(escript_external_box), false, false, 0);

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(hbox), false, false, 5);

        GtkButtonBox *buttonbox = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(buttonbox), 5);

        /* Save */
        escript_save = GTK_BUTTON(gtk_button_new_with_label("Save"));
        g_signal_connect(escript_save, "clicked",
                G_CALLBACK(on_escript_btn_click), 0);

        /* Cancel */
        escript_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(escript_cancel, "clicked",
                G_CALLBACK(on_escript_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(buttonbox), GTK_WIDGET(escript_save));
        gtk_container_add(GTK_CONTAINER(buttonbox), GTK_WIDGET(escript_cancel));

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(buttonbox), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(escript_statusbar), false, false, 0);

        gtk_container_add(GTK_CONTAINER(escript_window), GTK_WIDGET(content));

        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Shape Extruder **/
    {
        shapeextruder_dialog = new_dialog_defaults("Shape Extruder", &on_shapeextruder_show);

        gtk_widget_set_size_request(GTK_WIDGET(shapeextruder_dialog), 350, -1);
        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(shapeextruder_dialog));

        GtkGrid *tbl_settings = GTK_GRID(gtk_grid_new());
        {
            int y = 0;

            shapeextruder_right = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 2, .01));
            shapeextruder_up = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 2, .01));
            shapeextruder_left = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 2, .01));
            shapeextruder_down = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 2, .01));

            gtk_widget_set_hexpand(GTK_WIDGET(shapeextruder_right), true);
            gtk_widget_set_hexpand(GTK_WIDGET(shapeextruder_up), true);
            gtk_widget_set_hexpand(GTK_WIDGET(shapeextruder_left), true);
            gtk_widget_set_hexpand(GTK_WIDGET(shapeextruder_down), true);

            gtk_grid_attach(tbl_settings, new_rlbl("Right"), 0, y, 1, 1);
            gtk_grid_attach(tbl_settings, GTK_WIDGET(shapeextruder_right), 1, y, 1, 1);
            y++;

            gtk_grid_attach(tbl_settings, new_rlbl("Up"), 0, y, 1, 1);
            gtk_grid_attach(tbl_settings, GTK_WIDGET(shapeextruder_up), 1, y, 1, 1);
            y++;

            gtk_grid_attach(tbl_settings, new_rlbl("Left"), 0, y, 1, 1);
            gtk_grid_attach(tbl_settings, GTK_WIDGET(shapeextruder_left), 1, y, 1, 1);
            y++;

            gtk_grid_attach(tbl_settings, new_rlbl("Down"), 0, y, 1, 1);
            gtk_grid_attach(tbl_settings, GTK_WIDGET(shapeextruder_down), 1, y, 1, 1);
        }

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(tbl_settings), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Polygon **/
    {
        dialog = new_dialog_defaults("Polygon", &on_polygon_show);

        gtk_widget_set_size_request(GTK_WIDGET(dialog), 350, -1);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        GtkGrid *tbl_settings = create_settings_table();
        {
            int y = -1;

            polygon_sublayer_depth = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 4, 1));
            polygon_front_align = GTK_CHECK_BUTTON(gtk_check_button_new());

            add_setting_row(
                tbl_settings, ++y,
                "Sublayer depth",
                GTK_WIDGET(polygon_sublayer_depth)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Front align",
                GTK_WIDGET(polygon_front_align),
                "Sublayer depth from front instead of back"
            );
        }

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(tbl_settings), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));

        polygon_dialog = dialog;
    }

    /** --Synth **/
    {
        synth_dialog = new_dialog_defaults("Synthesizer", &on_synth_show, &on_synth_keypress);

        gtk_widget_set_size_request(GTK_WIDGET(synth_dialog), 350, -1);
        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(synth_dialog));

        GtkGrid *tbl_settings = create_settings_table();
        {
            int y = -1;

            synth_hz_low = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 440*8, 20, .1, 0)),
                        50, 0));

            synth_hz_high = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 440*8, 20, .1, 0)),
                        50, 0));

            synth_bitcrushing = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 64, 1));

            synth_pulse_width = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1., .01f));

            synth_freq_vibrato_hz = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 32, 1));
            synth_freq_vibrato_extent = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1., .01));

            synth_vol_vibrato_hz = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 32, 1));
            synth_vol_vibrato_extent = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, .01));

            synth_waveform = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());

            for (int x=0; x<NUM_WAVEFORMS; ++x) {
                gtk_combo_box_text_append_text(synth_waveform, speaker_options[x]);
            }

            add_setting_row(
                tbl_settings, ++y,
                "Base frequency",
                GTK_WIDGET(synth_hz_low)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Peak frequency",
                GTK_WIDGET(synth_hz_high)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Waveform",
                GTK_WIDGET(synth_waveform)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Pulse width",
                GTK_WIDGET(synth_pulse_width)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Bitcrushing",
                GTK_WIDGET(synth_bitcrushing)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Volume vibrato Hz",
                GTK_WIDGET(synth_vol_vibrato_hz)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Volume vibrato extent",
                GTK_WIDGET(synth_vol_vibrato_extent)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Freq vibrato Hz",
                GTK_WIDGET(synth_freq_vibrato_hz)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Freq vibrato extent",
                GTK_WIDGET(synth_freq_vibrato_extent)
            );
        }

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(tbl_settings), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Rubber **/
    {
        dialog = new_dialog_defaults("Rubber properties", &on_rubber_show, &on_rubber_keypress);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));
        GtkWidget *l;
        GtkBox *vb;

        l = gtk_label_new("Restitution"); vb = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        rubber_restitution = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.1));
        gtk_box_pack_start(vb, l, false, false, 0);
        gtk_box_pack_start(vb, GTK_WIDGET(rubber_restitution), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(vb), false, false, 0);

        l = gtk_label_new("Friction"); vb = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        rubber_friction = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1.0, 10.0, 0.1));
        gtk_box_pack_start(vb, l, false, false, 0);
        gtk_box_pack_start(vb, GTK_WIDGET(rubber_friction), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(vb), false, false, 0);

        gtk_widget_show_all(GTK_WIDGET(content));

        rubber_dialog = dialog;
    }

    /** --Timer **/
    {
        dialog = new_dialog_defaults("Timer", &on_timer_show, &on_timer_keypress);

        gtk_widget_set_size_request(GTK_WIDGET(dialog), 250, -1);
        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        GtkGrid *tbl_settings = create_settings_table();
        {
            int y = -1;

            timer_seconds = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 360, 1, 1, 0)),
                        50, 0));
            g_signal_connect(timer_seconds, "value-changed", G_CALLBACK(timer_time_changed), 0);

            timer_milliseconds = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 999, 16, 16, 0)),
                        50, 0));
            g_signal_connect(timer_milliseconds, "value-changed", G_CALLBACK(timer_time_changed), 0);

            timer_num_ticks = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 255, 1, 1, 0)),
                        1, 0));

            timer_time = GTK_LABEL(gtk_label_new("0.0s"));
            gtk_label_set_xalign(GTK_LABEL(timer_time), 0.0f);
            gtk_label_set_yalign(GTK_LABEL(timer_time), 0.5f);

            timer_use_system_time = GTK_CHECK_BUTTON(gtk_check_button_new());

            add_setting_row(
                tbl_settings, ++y,
                "Time between ticks",
                GTK_WIDGET(timer_time)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Seconds",
                GTK_WIDGET(timer_seconds)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Milliseconds",
                GTK_WIDGET(timer_milliseconds)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Number of ticks",
                GTK_WIDGET(timer_num_ticks),
                "0 = Infinite ticks"
            );

            add_setting_row(
                tbl_settings, ++y,
                "Use system time",
                GTK_WIDGET(timer_use_system_time),
                "Use system time for ticks, instead of in-game time."
            );
        }

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(tbl_settings), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));

        timer_dialog = dialog;
    }

    /** --Sequencer **/
    {
        sequencer_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
        //gtk_container_set_border_width(GTK_CONTAINER(sequencer_window), 10);
        gtk_window_set_default_size(GTK_WINDOW(sequencer_window), 400, 400);
        gtk_widget_set_size_request(GTK_WIDGET(sequencer_window), 400, 400);
        gtk_window_set_title(GTK_WINDOW(sequencer_window), "Sequencer settings");
        gtk_window_set_resizable(GTK_WINDOW(sequencer_window), false);
        // gtk_window_set_policy(GTK_WINDOW(sequencer_window),
        //               FALSE,
        //               FALSE, FALSE);

        g_signal_connect(sequencer_window, "show", G_CALLBACK(on_sequencer_show), 0);
        g_signal_connect(sequencer_window, "key-press-event", G_CALLBACK(on_sequencer_keypress), 0);
        g_signal_connect(sequencer_window, "delete-event", G_CALLBACK(on_window_close), 0);

        gtk_window_set_position(GTK_WINDOW(sequencer_window), GTK_WIN_POS_CENTER);
        //gtk_window_set_keep_above(GTK_WINDOW(sequencer_window), TRUE);
        gtk_widget_set_size_request(GTK_WIDGET(sequencer_window), 250, -1);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        sequencer_save   = GTK_BUTTON(gtk_button_new_with_label("Save"));
        g_signal_connect(sequencer_save, "clicked",
                G_CALLBACK(on_sequencer_click), 0);

        sequencer_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(sequencer_cancel, "clicked",
                G_CALLBACK(on_sequencer_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(sequencer_save));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(sequencer_cancel));

        GtkGrid *table = create_settings_table();
        {
            int y = -1;

            sequencer_sequence = GTK_ENTRY(gtk_entry_new());
            g_signal_connect(sequencer_sequence, "focus-out-event", G_CALLBACK(sequencer_sequence_focus_out), 0);
            //hack: update in realtime:
            g_signal_connect(sequencer_sequence, "changed", G_CALLBACK(sequencer_sequence_focus_out), 0);

            sequencer_seconds = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 360, 1, 1, 0)),
                        50, 0));
            g_signal_connect(sequencer_seconds, "value-changed", G_CALLBACK(sequencer_time_changed), 0);

            sequencer_milliseconds = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 950, 50, 50, 0)),
                        50, 0));
            g_signal_connect(sequencer_milliseconds, "value-changed", G_CALLBACK(sequencer_time_changed), 0);

            sequencer_wrap_around = GTK_CHECK_BUTTON(gtk_check_button_new());

            sequencer_state = GTK_LABEL(gtk_label_new("0.0s"));
            gtk_label_set_xalign(GTK_LABEL(sequencer_state), 0.0f);
            gtk_label_set_yalign(GTK_LABEL(sequencer_state), 0.5f);

            gtk_grid_attach(table, GTK_WIDGET(sequencer_state), 0, ++y, 3, 1);

            add_setting_row(
                table, ++y,
                "Sequence",
                GTK_WIDGET(sequencer_sequence)
            );

            add_setting_row(
                table, ++y,
                "Seconds",
                GTK_WIDGET(sequencer_seconds)
            );

            add_setting_row(
                table, ++y,
                "Milliseconds",
                GTK_WIDGET(sequencer_milliseconds)
            );

            add_setting_row(
                table, ++y,
                "Wrap Around",
                GTK_WIDGET(sequencer_wrap_around)
            );
        }

        gtk_box_pack_start(content, GTK_WIDGET(table), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(sequencer_window), GTK_WIDGET(content));
    }

    /** --Prompt settings dialog **/
    {
        prompt_settings_dialog = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
        gtk_container_set_border_width(GTK_CONTAINER(prompt_settings_dialog), 10);
        gtk_window_set_default_size(GTK_WINDOW(prompt_settings_dialog), 400, 400);
        gtk_widget_set_size_request(GTK_WIDGET(prompt_settings_dialog), 400, 400);
        gtk_window_set_title(GTK_WINDOW(prompt_settings_dialog), "Prompt settings");
        gtk_window_set_resizable(GTK_WINDOW(prompt_settings_dialog), false);
        // gtk_window_set_policy(GTK_WINDOW(prompt_settings_dialog),
        //               FALSE,
        //               FALSE, FALSE);
        gtk_window_set_position(prompt_settings_dialog, GTK_WIN_POS_CENTER);
        //gtk_window_set_keep_above(GTK_WINDOW(prompt_settings_dialog), TRUE);

        g_signal_connect(prompt_settings_dialog, "key-press-event", G_CALLBACK(on_prompt_keypress), 0);
        g_signal_connect(prompt_settings_dialog, "show", G_CALLBACK(on_prompt_show), 0);
        g_signal_connect(prompt_settings_dialog, "delete-event", G_CALLBACK(on_window_close), 0);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *inner_content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkWidget *l;
        GtkBox *hb;

        l = gtk_label_new("Message");
        prompt_message = GTK_TEXT_VIEW(gtk_text_view_new());
        gtk_box_pack_start(inner_content, l, false, false, 0);
        gtk_box_pack_start(inner_content, GTK_WIDGET(prompt_message), true, true, 0);

        l = gtk_label_new("Leave a button text empty if you don't want to use it.");
        gtk_box_pack_start(inner_content, l, false, false, 0);

        hb = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
        l = gtk_label_new("Button 1:");
        prompt_b1 = GTK_ENTRY(gtk_entry_new());
        gtk_box_pack_start(hb, l, false, false, 0);
        gtk_box_pack_start(hb, GTK_WIDGET(prompt_b1), false, false, 0);
        gtk_box_pack_start(inner_content, GTK_WIDGET(hb), false, false, 0);

        hb = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
        l = gtk_label_new("Button 2:");
        prompt_b2 = GTK_ENTRY(gtk_entry_new());
        gtk_box_pack_start(hb, l, false, false, 0);
        gtk_box_pack_start(hb, GTK_WIDGET(prompt_b2), false, false, 0);
        gtk_box_pack_start(inner_content, GTK_WIDGET(hb), false, false, 0);

        hb = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
        l = gtk_label_new("Button 3:");
        prompt_b3 = GTK_ENTRY(gtk_entry_new());
        gtk_box_pack_start(hb, l, false, false, 0);
        gtk_box_pack_start(hb, GTK_WIDGET(prompt_b3), false, false, 0);
        gtk_box_pack_start(inner_content, GTK_WIDGET(hb), false, false, 0);

        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Save */
        prompt_save = GTK_BUTTON(gtk_button_new_with_label("Save"));
        g_signal_connect(prompt_save, "clicked",
                G_CALLBACK(on_prompt_btn_click), 0);

        /* Cancel */
        prompt_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(prompt_cancel, "clicked",
                G_CALLBACK(on_prompt_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(prompt_save));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(prompt_cancel));

        gtk_box_pack_start(content, GTK_WIDGET(inner_content), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(prompt_settings_dialog), GTK_WIDGET(content));
    }

    gdk_threads_add_idle(_sig_ui_ready, 0);


    gtk_main();


    return T_OK;
}

static gboolean
_sig_ui_ready(gpointer unused)
{
    SDL_LockMutex(ui_lock);
    ui_ready = SDL_TRUE;
    SDL_CondSignal(ui_cond);
    SDL_UnlockMutex(ui_lock);

    return false;
}

void ui::init()
{
    ui_lock = SDL_CreateMutex();
    ui_cond = SDL_CreateCond();
    ui_ready = SDL_FALSE;

    SDL_Thread *gtk_thread;

    gtk_thread = SDL_CreateThread(_gtk_loop, "_gtk_loop", 0);

    if (gtk_thread == NULL) {
        tms_errorf("SDL_CreateThread failed: %s", SDL_GetError());
    }
}

static gboolean
_open_play_menu(gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(play_menu));
    gtk_menu_popup(play_menu, 0, 0, 0, 0, 0, gtk_get_current_event_time());

    return false;
}

static gboolean
_open_sandbox_menu(gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(editor_menu));
    gtk_menu_popup(editor_menu, 0, 0, 0, 0, 0, gtk_get_current_event_time());

    if (G->state.sandbox) {
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_save),      true);
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_save_copy), true);
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_publish),   true);
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_lvl_prop),  true);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_save),      false);
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_save_copy), false);
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_publish),   false);
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_lvl_prop),  false);
    }

    if (W->is_paused()) {
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_move_here_object), G->selection.e != 0);

        char tmp[256];

        if (G->selection.e) {
            snprintf(tmp, 255, "- id:%u, g_id:%u, pos:%.2f/%.2f, angle:%.2f -",
                    G->selection.e->id, G->selection.e->g_id,
                    G->selection.e->get_position().x,
                    G->selection.e->get_position().y,
                    G->selection.e->get_angle()
                    );
            gtk_widget_hide(GTK_WIDGET(editor_menu_move_here_player));
            gtk_widget_hide(GTK_WIDGET(editor_menu_go_to));
            if (!G->selection.e->is_creature()) {
                gtk_widget_hide(GTK_WIDGET(editor_menu_set_as_player));
            }

            bool is_marked = false;

            for (std::deque<struct goto_mark*>::iterator it = editor_menu_marks.begin();
                    it != editor_menu_marks.end(); ++it) {
                struct goto_mark *mark = *it;

                if (mark != editor_menu_last_created && mark->type == MARK_ENTITY && mark->id == G->selection.e->id) {
                    is_marked = true;
                    break;
                }
            }

            char mark_entity[256];

            if (is_marked) {
                snprintf(mark_entity, 255, "Un_mark entity");
            } else {
                snprintf(mark_entity, 255, "_Mark entity");
            }
            gtk_menu_item_set_label(editor_menu_toggle_mark_entity, mark_entity);
            gtk_menu_item_set_use_underline(editor_menu_toggle_mark_entity, true);
        } else {
            b2Vec2 pos = G->get_last_cursor_pos(0);
            snprintf(tmp, 255, "- %.2f/%.2f -", pos.x, pos.y);

            gtk_widget_hide(GTK_WIDGET(editor_menu_set_as_player));
            gtk_widget_hide(GTK_WIDGET(editor_menu_toggle_mark_entity));

            if (!W->is_adventure()) {
                gtk_widget_hide(GTK_WIDGET(editor_menu_move_here_player));
            }
        }

        gtk_menu_item_set_label(editor_menu_header, tmp);

    } else {
        gtk_widget_hide(GTK_WIDGET(editor_menu_header));
        gtk_widget_hide(GTK_WIDGET(editor_menu_move_here_player));
        gtk_widget_hide(GTK_WIDGET(editor_menu_move_here_object));
        gtk_widget_hide(GTK_WIDGET(editor_menu_go_to));
        gtk_widget_hide(GTK_WIDGET(editor_menu_set_as_player));
        gtk_widget_hide(GTK_WIDGET(editor_menu_toggle_mark_entity));
    }

    // Disable the Login button if the user is already logged in.
    gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_login), (P.user_id == 0));

    // Disable the Publish button if the user is not logged in.
    gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_publish), (P.user_id != 0));

    return false;
}

static gboolean
_open_quickadd(gpointer unused)
{
    gtk_window_set_position(quickadd_window, GTK_WIN_POS_MOUSE);
    //gtk_entry_set_text(quickadd_entry, "");
    //gtk_entry_
    gtk_widget_show_all(GTK_WIDGET(quickadd_window));
    gtk_widget_grab_focus(GTK_WIDGET(quickadd_entry));
    //gtk_window_set_keep_above(GTK_WINDOW(quickadd_window), TRUE);
    tms_infof("open quickadd");

    return false;
}

/** --Color Chooser (for Plastic beam & Pixel) **/
static gboolean
_open_beam_color(gpointer unused)
{
    /* set current chooser to beam/pixel current color */
    GtkColorSelection *sel = GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(beam_color_dialog));

    entity *e = G->selection.e;
    if (e) {
        GdkRGBA color;
        tvec4 c = e->get_color();
        color.red   = c.r;
        color.green = c.g;
        color.blue  = c.b;
        if (e->g_id == O_PIXEL) {
            color.alpha = (double)(e->properties[4].v.i8) / 255.0;
            gtk_color_selection_set_has_opacity_control(sel, true);
        } else {
            color.alpha = 1.;
            gtk_color_selection_set_has_opacity_control(sel, false);
        }
        gtk_color_selection_set_current_rgba(sel, &color);
#if 0 // XXX: Unused painter tool
    } else if (W->is_adventure() && adventure::player && adventure::is_player_alive()) {
        robot_parts::tool *t = adventure::player->get_tool();
        if (t && t->get_arm_type() == TOOL_PAINTER) {
            GdkRGBA color;
            color.red   = t->properties[0].v.f;
            color.green = t->properties[1].v.f;
            color.blue  = t->properties[2].v.f;
            color.alpha = 1.0;

            gtk_color_chooser_set_use_alpha(sel, false);
            gtk_color_chooser_set_rgba(sel, &color);
        }
#endif
    }

    if (gtk_dialog_run(GTK_DIALOG(beam_color_dialog)) == GTK_RESPONSE_OK) {
        //GtkColorSelection *sel = GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(beam_color_dialog)));

        GdkRGBA color;
        gtk_color_selection_get_current_rgba(sel, &color);

        entity *e = G->selection.e;

        if (e) {
            e->set_color4(color.red, color.green, color.blue);

            if (e->g_id == O_PIXEL) {
                e->set_property(4, (uint8_t)(color.alpha * 255.));
            }
#if 0 // XXX: Unused painter tool
        } else if (W->is_adventure() && adventure::player && adventure::is_player_alive()) {
            robot_parts::tool *t = adventure::player->get_tool();
            if (t && t->tool_id == TOOL_PAINTER) {
                t->set_property(0, (float)color.red / (float)0xffff);
                t->set_property(1, (float)color.green / (float)0xffff);
                t->set_property(2, (float)color.blue / (float)0xffff);
                ((robot_parts::painter*)t)->update_appearance();
            }
#endif
        }
    }

    gtk_widget_hide(GTK_WIDGET(beam_color_dialog));

    return false;
}

static gboolean
_open_polygon_color(gpointer unused)
{
    return _open_beam_color(unused);
}

static gboolean
_open_pixel_color(gpointer unused)
{
    return _open_beam_color(unused);
}

static gboolean
_open_save_window(gpointer unused)
{
    activate_save(NULL, 0);

    return false;
}

static gboolean
_open_publish_dialog(gpointer unused)
{
    activate_publish(NULL, 0);

    return false;
}

static gboolean
_open_login_dialog(gpointer unused)
{
    activate_login(NULL, 0);

    return false;
}

static gboolean
_open_prompt_dialog(gpointer unused)
{
    if (W->is_adventure() && adventure::player) {
        adventure::player->stop_moving(DIR_LEFT);
        adventure::player->stop_moving(DIR_RIGHT);
    }

    base_prompt *bp = G->current_prompt->get_base_prompt();
    if (G->current_prompt && G->current_prompt->is_prompt_compatible() && bp) {

        GtkDialog *d = GTK_DIALOG(gtk_message_dialog_new(
                    0, (GtkDialogFlags)(0),
                    GTK_MESSAGE_OTHER,
                    GTK_BUTTONS_NONE,
                    0));
        //gtk_window_set_decorated(GTK_WINDOW(d), FALSE);
        //gtk_window_set_has_frame(GTK_WINDOW(d), FALSE);

        g_object_set(d, "text", *bp->message, NULL);

        gtk_window_set_deletable(GTK_WINDOW(d), FALSE);
        gtk_window_set_position(GTK_WINDOW(d), GTK_WIN_POS_CENTER);
        //gtk_window_set_keep_above(GTK_WINDOW(d), TRUE);

        if (W && W->level.version >= LEVEL_VERSION_1_2_3) {
            for (int x=0; x<3; ++x) {
                const struct base_prompt::prompt_button &btn = bp->buttons[x];
                if (*btn.len && *btn.buf) {
                    gtk_dialog_add_button(d, *btn.buf, x+1);
                }
            }
        } else {
            int n=0;
            for (int x=0; x<3; ++x) {
                const struct base_prompt::prompt_button &btn = bp->buttons[x];
                if (*btn.len && *btn.buf) {
                    gtk_dialog_add_button(d, *btn.buf, ++n);
                }
            }
        }

        cur_prompt = d;
        prompt_is_open = 1;

        P.focused = false;

        int response = PROMPT_RESPONSE_NONE;

        do {
            response = (int)gtk_dialog_run(d);

            switch (response) {
                case PROMPT_RESPONSE_A:
                case PROMPT_RESPONSE_B:
                case PROMPT_RESPONSE_C:
                    if (G->current_prompt) {
                        bp = G->current_prompt->get_base_prompt();

                        if (bp) {
                            tms_debugf("setting prompt response from here");
                            bp->set_response(response);
                        }
                    }
                    break;

                default:
                    response = PROMPT_RESPONSE_NONE;
                    tms_warnf("no response given.");
                    break;
            }
        } while (response == PROMPT_RESPONSE_NONE);

        P.focused = true;

        gtk_widget_hide(GTK_WIDGET(d));
    }

    cur_prompt = 0;
    prompt_is_open = 0;

    return false;
}

static gboolean
_open_prompt_settings_dialog(gpointer unused)
{
    activate_prompt_settings(NULL, 0);

    return false;
}

static gboolean
_open_export(gpointer unused)
{
    activate_export(NULL, 0);

    return false;
}

static gboolean
_open_open_state_dialog(gpointer unused)
{
    activate_open_state(NULL, 0);

    return false;
}

static gboolean
_open_open_dialog(gpointer unused)
{
    activate_open(NULL, 0);

    return false;
}

static gboolean
_open_multiemitter_dialog(gpointer unused)
{
    object_window_multiemitter = true;
    activate_object(NULL, 0);

    return false;
}

static gboolean
_open_object_dialog(gpointer unused)
{
    object_window_multiemitter = false;
    activate_object(NULL, 0);

    return false;
}

static gboolean
_open_new_level_dialog(gpointer unused)
{
    activate_new_level(NULL, 0);

    return false;
}

static gboolean
_open_mode_dialog(gpointer unused)
{
    activate_mode_dialog(NULL, 0);

    return false;
}

static gboolean
_open_autosave(gpointer unused)
{
    gtk_widget_hide(GTK_WIDGET(autosave_dialog));
    gint result = gtk_dialog_run(autosave_dialog);
    gtk_widget_hide(GTK_WIDGET(autosave_dialog));

    if (result == GTK_RESPONSE_YES) {
        P.add_action(ACTION_OPEN_AUTOSAVE, 0);
    } else if (result == GTK_RESPONSE_NO) {
        P.add_action(ACTION_REMOVE_AUTOSAVE, 0);
    }

    return false;
}

static gboolean
_open_tips_dialog(gpointer unused)
{
    do {
         gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tips_hide), settings["hide_tips"]->v.b);

        gtk_widget_hide(GTK_WIDGET(tips_dialog));
        gint result = gtk_dialog_run(tips_dialog);
        gtk_widget_hide(GTK_WIDGET(tips_dialog));

        settings["hide_tips"]->v.b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tips_hide));

        if (result == GTK_RESPONSE_APPLY) {
            tms_infof("reshowing tips");
            continue;
        }

        if (result == GTK_RESPONSE_YES)
            ui::open_url("https://principia-web.se/wiki/");

        break;
    } while (true);

    return false;
}

static gboolean
_open_info_dialog(gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(info_dialog));

    return false;
}

static gboolean
_open_error_dialog(gpointer unused)
{
    gtk_widget_hide(GTK_WIDGET(error_dialog));
    gtk_dialog_run(error_dialog);
    gtk_widget_hide(GTK_WIDGET(error_dialog));

    return false;
}

/** --Confirm Dialog **/
static gboolean
_open_confirm_dialog(gpointer unused)
{
    gtk_widget_hide(GTK_WIDGET(confirm_dialog));
    P.focused = false;
    int r = gtk_dialog_run(confirm_dialog);
    P.focused = true;

    switch (r) {
        case 1:
            // button 1 pressed
            if (confirm_action1 != ACTION_IGNORE) {
                P.add_action(confirm_action1, confirm_action1_data);
            }
            break;

        case 2:
            // button 2 pressed
            if (confirm_action2 != ACTION_IGNORE) {
                P.add_action(confirm_action2, confirm_action2_data);
            }
            break;

        case 3:
            // button 2 pressed
            if (confirm_action3 != ACTION_IGNORE) {
                P.add_action(confirm_action3, confirm_action3_data);
            }
            break;
    }

    switch (confirm_data.confirm_type) {
        case CONFIRM_TYPE_BACK_SANDBOX:
            settings["dna_sandbox_back"]->v.b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(confirm_dna_sandbox_back));
            settings.save();
            break;
    }

    gtk_widget_hide(GTK_WIDGET(confirm_dialog));

    return false;
}

/** --Emitter **/
static gboolean
_open_emitter_dialog(gpointer unused)
{
    int result = gtk_dialog_run(emitter_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && (e->g_id == O_EMITTER || e->g_id == O_MINI_EMITTER)) {
            e->properties[6].v.f = gtk_range_get_value(GTK_RANGE(emitter_auto_absorb));

            ui::message("Emitter properties saved!");

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(emitter_dialog));

    return false;
}

/** --Alert Dialog **/
static gboolean
_open_alert_dialog(gpointer unused)
{
    gtk_widget_hide(GTK_WIDGET(alert_dialog));
    P.focused = false;
    gtk_dialog_run(GTK_DIALOG(alert_dialog));
    P.focused = true;

    gtk_widget_hide(GTK_WIDGET(alert_dialog));

    return false;
}

static gboolean
_open_frequency_window(gpointer unused)
{
    activate_frequency(NULL, 0);

    return false;
}

static gboolean
_open_freq_range_window(gpointer unused)
{
    activate_freq_range(NULL, 0);

    return false;
}

static gboolean
_open_pkg_lvl_chooser_window(gpointer unused)
{
    gint result = gtk_dialog_run(pkg_lvl_chooser);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && (e->g_id == O_PKG_WARP || e->g_id == O_PKG_STATUS)) {
            e->properties[0].v.i8 = gtk_spin_button_get_value(pkg_lvl_chooser_lvl_id);
            tms_infof("New lvl id: %d", e->properties[0].v.i8);
        }
    }

    gtk_widget_hide(GTK_WIDGET(pkg_lvl_chooser));

    return false;
}

static gboolean
_open_robot_window(gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(robot_window));

    return false;
}

static gboolean
_open_puzzle_play(gpointer unused)
{
    gint result = gtk_dialog_run(puzzle_play_dialog);

    switch (result) {
        case PUZZLE_SIMULATE:
        case PUZZLE_TEST_PLAY:
            P.add_action(ACTION_PUZZLEPLAY, (void*)(intptr_t)result);
            break;
    }

    gtk_widget_hide(GTK_WIDGET(puzzle_play_dialog));

    return false;
}

/** --Published **/
static gboolean
_open_published(gpointer unused)
{
    gint result = gtk_dialog_run(published_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        COMMUNITY_URL("level/%d", W->level.community_id);
        ui::open_url(url);
    }

    gtk_widget_hide(GTK_WIDGET(published_dialog));

    return false;
}

/** --Community **/
static gboolean
_open_community(gpointer unused)
{
    gint result = gtk_dialog_run(community_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        P.add_action(ACTION_GOTO_MAINMENU, 0);
    }

    gtk_widget_hide(GTK_WIDGET(community_dialog));

    return false;
}

/** --Sequencer **/
static gboolean
_open_sequencer(gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(sequencer_window));

    return false;
}

/** --Jumper **/
static gboolean
_open_jumper(gpointer unused)
{
    gint result = gtk_dialog_run(jumper_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_JUMPER) {
            float v = gtk_range_get_value(jumper_value);
            if (v < 0.f) v = 0.f;
            else if (v > 1.f) v = 1.f;
            e->properties[0].v.f = v;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
            ((jumper*)e)->update_color();
        }
    }

    gtk_widget_hide(GTK_WIDGET(jumper_dialog));

    return false;
}

/** --cursorfield **/
static gboolean
_open_cursorfield(gpointer unused)
{
    gint result = gtk_dialog_run(cursorfield_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_CURSOR_FIELD) {
            e->properties[0].v.f = gtk_range_get_value(cursorfield_right);
            e->properties[1].v.f = gtk_range_get_value(cursorfield_up);
            e->properties[2].v.f = gtk_range_get_value(cursorfield_left);
            e->properties[3].v.f = gtk_range_get_value(cursorfield_down);

            if (e->properties[0].v.f < e->properties[2].v.f) e->properties[0].v.f = e->properties[2].v.f+.5f;
            if (e->properties[0].v.f > 3.f) {
                e->properties[0].v.f = 3.f;
                e->properties[2].v.f = e->properties[0].v.f - .5f;
            }
            if (e->properties[1].v.f < e->properties[3].v.f) e->properties[1].v.f = e->properties[1].v.f+.5f;
            if (e->properties[1].v.f > 3.f) {
                e->properties[1].v.f = 3.f;
                e->properties[3].v.f = e->properties[1].v.f - .5f;
            }
        }
    }
    gtk_widget_hide(GTK_WIDGET(cursorfield_dialog));

    return false;
}

/** --escript **/
static gboolean
_open_escript(gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(escript_window));

    return false;
}

/** --Shape extruder **/
static gboolean
_open_shapeextruder(gpointer unused)
{
    gint result = gtk_dialog_run(shapeextruder_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_SHAPE_EXTRUDER) {
            e->properties[0].v.f = gtk_range_get_value(shapeextruder_right);
            e->properties[1].v.f = gtk_range_get_value(shapeextruder_up);
            e->properties[2].v.f = gtk_range_get_value(shapeextruder_left);
            e->properties[3].v.f = gtk_range_get_value(shapeextruder_down);
        }
    }
    gtk_widget_hide(GTK_WIDGET(shapeextruder_dialog));

    return false;
}

/** --Synthesizer **/
static gboolean
_open_synth(gpointer unused)
{
    gint result = gtk_dialog_run(synth_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_SYNTHESIZER) {
            float low = gtk_spin_button_get_value(synth_hz_low);
            float high = gtk_spin_button_get_value(synth_hz_high);
            float pw = gtk_range_get_value(synth_pulse_width);
            float vb = gtk_range_get_value(synth_vol_vibrato_hz);
            float fb = gtk_range_get_value(synth_freq_vibrato_hz);
            float vbe = gtk_range_get_value(synth_vol_vibrato_extent);
            float fbe = gtk_range_get_value(synth_freq_vibrato_extent);
            float bitcrushing = gtk_range_get_value(synth_bitcrushing);

            if (high < low) high = low;

            e->properties[0].v.f = low;
            e->properties[1].v.f = high;

            int index = gtk_combo_box_get_active(GTK_COMBO_BOX(synth_waveform));

            e->properties[2].v.i = index;

            e->properties[3].v.f = bitcrushing;
            e->properties[4].v.f = vb;
            e->properties[5].v.f = fb;

            e->properties[6].v.f = vbe;
            e->properties[7].v.f = fbe;

            e->properties[8].v.f = pw;
        }
    }

    gtk_widget_hide(GTK_WIDGET(synth_dialog));

    return false;
}

/** --Rubber **/
static gboolean
_open_rubber(gpointer unused)
{
    gint result = gtk_dialog_run(rubber_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && (e->g_id == O_WHEEL || e->g_id == O_RUBBER_BEAM)) {
            float restitution = gtk_range_get_value(GTK_RANGE(rubber_restitution));
            float friction = gtk_range_get_value(GTK_RANGE(rubber_friction));

            e->properties[1].v.f = restitution;
            e->properties[2].v.f = friction;

            if (e->g_id == O_RUBBER_BEAM) {
                ((beam*)e)->do_update_fixture = true;
            } else {
                ((wheel*)e)->do_update_fixture = true;
            }

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(rubber_dialog));

    return false;
}

/** --Timer **/
static gboolean
_open_timer(gpointer unused)
{
    gint result = gtk_dialog_run(timer_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_TIMER) {
            uint8_t num_ticks = gtk_spin_button_get_value(timer_num_ticks);
            int seconds = gtk_spin_button_get_value(timer_seconds);
            int milliseconds = gtk_spin_button_get_value(timer_milliseconds);
            uint32_t full_time = (seconds*1000) + milliseconds;
            if (full_time < TIMER_MIN_TIME)
                full_time = TIMER_MIN_TIME;

            e->properties[0].v.i = full_time;
            e->properties[1].v.i8 = num_ticks;
            e->properties[2].v.i = (uint32_t)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(timer_use_system_time));

            ui::message("Timer properties saved!");
            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(timer_dialog));

    return false;
}

static gboolean
_open_settings(gpointer unused)
{
    activate_settings(0, 0);

    return false;
}

static gboolean
_open_multi_config(gpointer unused)
{
    g_object_set(
        G_OBJECT(multi_config_plastic_color),
        "show-editor", FALSE,
        NULL
    );

    gtk_widget_show_all(GTK_WIDGET(multi_config_window));

    return false;
}

static gboolean
_open_variable_window(gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(variable_dialog));

    return false;
}

static gboolean
_open_command_pad_window(gpointer unused)
{
    gint result = gtk_dialog_run(command_pad_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_COMMAND_PAD) {
            command *pad = static_cast<command*>(e);
            char tmp[64];

            strcpy(tmp, get_cb_val(command_pad_cb));
            if (strcmp(tmp, "Stop") == 0) {
                pad->set_command(COMMAND_STOP);
            } else if (strcmp(tmp, "Start/Stop toggle") == 0) {
                pad->set_command(COMMAND_STARTSTOP);
            } else if (strcmp(tmp, "Left") == 0) {
                pad->set_command(COMMAND_LEFT);
            } else if (strcmp(tmp, "Right") == 0) {
                pad->set_command(COMMAND_RIGHT);
            } else if (strcmp(tmp, "Left/Right toggle") == 0) {
                pad->set_command(COMMAND_LEFTRIGHT);
            } else if (strcmp(tmp, "Jump") == 0) {
                pad->set_command(COMMAND_JUMP);
            } else if (strcmp(tmp, "Aim") == 0) {
                pad->set_command(COMMAND_AIM);
            } else if (strcmp(tmp, "Attack") == 0) {
                pad->set_command(COMMAND_ATTACK);
            } else if (strcmp(tmp, "Layer up") == 0) {
                pad->set_command(COMMAND_LAYERUP);
            } else if (strcmp(tmp, "Layer down") == 0) {
                pad->set_command(COMMAND_LAYERDOWN);
            } else if (strcmp(tmp, "Increase speed") == 0) {
                pad->set_command(COMMAND_INCRSPEED);
            } else if (strcmp(tmp, "Decrease speed") == 0) {
                pad->set_command(COMMAND_DECRSPEED);
            } else if (strcmp(tmp, "Set speed") == 0) {
                pad->set_command(COMMAND_SETSPEED);
            } else if (strcmp(tmp, "Full health") == 0) {
                pad->set_command(COMMAND_HEALTH);
            } else {
                tms_infof("unknown command: %s", tmp);
            }

            ui::message("Command pad properties saved!");
            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(command_pad_dialog));

    return false;
}

static gboolean
_open_digi_window(gpointer unused)
{
    gint result = gtk_dialog_run(digi_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;
        if (e && (e->g_id == O_PASSIVE_DISPLAY || e->g_id == O_ACTIVE_DISPLAY)) {
            e->properties[0].v.i8 = (uint8_t)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(digi_wrap));
            e->properties[1].v.i8 = gtk_spin_button_get_value(digi_initial)-1;

            char str[DISPLAY_MAX_SYMBOLS*35+1];
            int ss=0;

            for (int s=0; s<num_digi_symbols; s++) {
                for (int y=0; y<7; y++) {
                    for (int x=0; x<5; x++) {
                        if (symbols[s] & (1ull << (uint64_t)(y*5+x)))
                            str[ss] = '1';
                        else
                            str[ss] = '0';
                        ss++;
                    }
                }
            }

            str[ss] = '\0';
            e->set_property(2, str);
            ((display*)e)->load_symbols();
        }
    }

    gtk_widget_hide(GTK_WIDGET(digi_dialog));

    return false;
}

static gboolean
_open_sticky_window(gpointer unused)
{
    gint result = gtk_dialog_run(sticky_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;
        if (e && e->g_id == O_STICKY_NOTE) {
            e->properties[1].v.i8 = (uint8_t) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sticky_center_x));
            e->properties[2].v.i8 = (uint8_t) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sticky_center_y));
            e->properties[3].v.i8 = gtk_spin_button_get_value(sticky_font_size);
        }
        GtkTextIter start, end;
        GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(sticky_text);
        char *text;

        gtk_text_buffer_get_bounds(text_buffer, &start, &end);

        text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);

        P.add_action(ACTION_SET_STICKY_TEXT, text);
    }

    gtk_widget_hide(GTK_WIDGET(sticky_dialog));

    return false;
}

static gboolean
_open_fxemitter_window(gpointer unused)
{
    gint result = gtk_dialog_run(fxemitter_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_FX_EMITTER) {
            for (int x=0; x<4; x++) {
                int index = gtk_combo_box_get_active(GTK_COMBO_BOX(fxemitter_cb[x]));

                if (index == 0) {
                    e->properties[3+x].v.i = FX_INVALID;
                } else {
                    e->properties[3+x].v.i = index - 1;
                }
            }

            e->properties[0].v.f = (float)gtk_range_get_value(GTK_RANGE(fxemitter_radius));
            e->properties[1].v.i = (uint32_t)gtk_range_get_value(GTK_RANGE(fxemitter_count));
            e->properties[2].v.f = (float)gtk_range_get_value(GTK_RANGE(fxemitter_interval));
        }
    }

    gtk_widget_hide(GTK_WIDGET(fxemitter_dialog));

    return false;
}

static gboolean
_open_sfx_window(gpointer unused)
{
    gint result = gtk_dialog_run(sfx_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_SFX_EMITTER) {
            e->set_property(0, (uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(sfx_cb)));
            e->set_property(1, (uint8_t)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sfx_global)));
        }
    }

    gtk_widget_hide(GTK_WIDGET(sfx_dialog));

    return false;
}

static gboolean
_open_sfx2_window(gpointer unused)
{
    gint result = gtk_dialog_run(sfx2_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_SFX_EMITTER) {
            e->set_property(0, (uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(sfx2_cb)));
            e->set_property(1, (uint8_t)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sfx2_global)));
            e->set_property(3, (uint8_t)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sfx2_loop)));

            uint32_t active_sub = (uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(sfx2_sub_cb));

            if (active_sub == 0) {
                e->properties[2].v.i = SFX_CHUNK_RANDOM;
            } else {
                e->properties[2].v.i = active_sub - 1;
            }
        }
    }

    gtk_widget_hide(GTK_WIDGET(sfx2_dialog));

    return false;
}

/** --Item **/
static gboolean
_open_item(gpointer unused)
{
    gint result = gtk_dialog_run(item_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_ITEM) {
            ((item*)e)->set_item_type((uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(item_cb)));
            ((item*)e)->do_recreate_shape = true;
            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(item_dialog));

    return false;
}

/** --Decoration **/
static gboolean
_open_decoration(gpointer unused)
{
    GtkDialog *d = decoration_dialog;

    gint result = gtk_dialog_run(d);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_DECORATION) {
            ((decoration*)e)->set_decoration_type((uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(decoration_cb)));
            ((decoration*)e)->do_recreate_shape = true;
            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(d));

    return false;
}

/** --Key Listener **/
static gboolean
_open_key_listener(gpointer unused)
{
    gint result = gtk_dialog_run(key_listener_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_KEY_LISTENER) {
            GtkTreeIter iter;

            if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(key_listener_cb), &iter)) {
                GValue val = {0, };
                gtk_tree_model_get_value(GTK_TREE_MODEL(key_listener_ls),
                                            &iter,
                                            1, &val
                                            );

                uint32_t key = g_value_get_uint(&val);

                tms_debugf("Key: %u: %s", key, key_names[key]);

                e->properties[0].v.i = key;
                P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
                P.add_action(ACTION_RESELECT, 0);
            }
        }
    }

    gtk_widget_hide(GTK_WIDGET(key_listener_dialog));

    return false;
}

/** --Faction **/
static gboolean
_open_faction(gpointer unused)
{
    GtkDialog *d = faction_dialog;

    gint result = gtk_dialog_run(d);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && (e->g_id == O_GUARDPOINT)) {
            ((anchor*)e)->set_faction((uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(faction_cb)));
            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(d));

    return false;
}

/** --Resource **/
static gboolean
_open_resource(gpointer unused)
{
    GtkDialog *d = resource_dialog;

    gint result = gtk_dialog_run(d);

	if (result == GTK_RESPONSE_ACCEPT) {
		entity *e = G->selection.e;

		if (e && e->g_id == O_RESOURCE) {
			((resource*)e)->set_resource_type((uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(resource_cb)));
			P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
			P.add_action(ACTION_RESELECT, 0);
		}
	}

    gtk_widget_hide(GTK_WIDGET(d));

    return false;
}

/** --Vendor **/
static gboolean
_open_vendor(gpointer unused)
{
    GtkDialog *d = vendor_dialog;

    gint result = gtk_dialog_run(d);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_VENDOR) {
            gtk_spin_button_update(vendor_amount);
            e->properties[2].v.i = gtk_spin_button_get_value(vendor_amount);
            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(d));

    return false;
}

/** --Animal **/
static gboolean
_open_animal(gpointer unused)
{
    GtkDialog *d = animal_dialog;

    gint result = gtk_dialog_run(d);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_ANIMAL) {
            W->add_action(e->id, ACTION_SET_ANIMAL_TYPE, UINT_TO_VOID((uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(animal_cb))));

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(d));

    return false;
}

/** --Soundman **/
static gboolean
_open_soundman(gpointer unused)
{
    GtkDialog *d = soundman_dialog;

    gint result = gtk_dialog_run(d);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_SOUNDMAN) {
            e->properties[0].v.i = (uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(soundman_cb));

            if (e->properties[0].v.i >= SND__NUM) e->properties[0].v.i = SND__NUM-1;

            e->properties[1].v.i8 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(soundman_catch_all)) ? 1 : 0;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(d));

    return false;
}

/** --Polygon **/
static gboolean
_open_polygon(gpointer unused)
{
    GtkDialog *d = polygon_dialog;

    gint result = gtk_dialog_run(d);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_PLASTIC_POLYGON) {
            ((polygon*)e)->do_recreate_shape = true;

            e->properties[1].v.i8 = (uint8_t)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(polygon_front_align));
            e->properties[0].v.i8 = (uint8_t)gtk_range_get_value(GTK_RANGE(polygon_sublayer_depth))-1;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(d));

    return false;
}

/** --Factory **/
static gboolean
_open_factory(gpointer unused)
{
    GtkDialog *d = factory_dialog;

    gint result = gtk_dialog_run(d);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && IS_FACTORY(e->g_id)) {
            factory *f = static_cast<factory*>(e);

            gtk_spin_button_update(factory_oil);
            gtk_spin_button_update(factory_faction);

            f->properties[1].v.i = gtk_spin_button_get_value(factory_oil);
            f->properties[2].v.i = gtk_spin_button_get_value(factory_faction);
            for (int x=0; x<NUM_RESOURCES; ++x) {
                gtk_spin_button_update(factory_resources[x]);
                f->properties[FACTORY_NUM_EXTRA_PROPERTIES+x].v.i = gtk_spin_button_get_value(factory_resources[x]);
            }

            GtkTreeModel *model = GTK_TREE_MODEL(factory_liststore);
            GtkTreeIter iter;
            int x = 0;
            std::stringstream ss;

            if (gtk_tree_model_get_iter_first(model, &iter)) {
                do {
                    GValue val = {0, };
                    GValue val_id = {0, };
                    gtk_tree_model_get_value(model, &iter, FACTORY_COLUMN_ENABLED, &val);
                    gtk_tree_model_get_value(model, &iter, FACTORY_COLUMN_RECIPE_ID, &val_id);
                    gboolean enabled = g_value_get_boolean(&val);
                    gint id = g_value_get_int(&val_id);
                    if (enabled == TRUE) {
                        if (x != 0) {
                            ss << ';';
                        }

                        ss << id;

                        ++ x;
                    }
                } while (gtk_tree_model_iter_next(model, &iter));
            }

            f->set_property(0, ss.str().c_str());
            tms_debugf("Recipe string: %s", f->properties[0].v.s.buf);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(d));

    return false;
}

/** --Treasure chest **/
static gboolean
_open_treasure_chest(gpointer unused)
{
    GtkDialog *d = tchest_dialog;

    gint result = gtk_dialog_run(d);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_TREASURE_CHEST) {
            treasure_chest *tc = static_cast<treasure_chest*>(e);

            GtkTreeModel *model = GTK_TREE_MODEL(tchest_liststore);
            GtkTreeIter iter;
            int x = 0;
            std::stringstream ss;

            if (gtk_tree_model_get_iter_first(model, &iter)) {
                do {
                    GValue val_g_id = {0, };
                    GValue val_sub_id = {0, };
                    GValue val_count = {0, };
                    gtk_tree_model_get_value(model, &iter, TCHEST_COLUMN_G_ID, &val_g_id);
                    gtk_tree_model_get_value(model, &iter, TCHEST_COLUMN_SUB_ID, &val_sub_id);
                    gtk_tree_model_get_value(model, &iter, TCHEST_COLUMN_COUNT, &val_count);

                    gint g_id = g_value_get_int(&val_g_id);
                    gint sub_id = g_value_get_int(&val_sub_id);
                    gint count = g_value_get_int(&val_count);

                    if (x > 0) {
                        ss << ";";
                    }

                    ss << g_id << ":" << sub_id << ":" << count;

                    ++x;
                } while (gtk_tree_model_iter_next(model, &iter));
            }

            tc->set_property(0, ss.str().c_str());
            tms_debugf("TC Loot string: %s", tc->properties[0].v.s.buf);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(d));

    return false;
}

static gboolean
_open_elistener_window(gpointer unused)
{
    gint result = gtk_dialog_run(elistener_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_EVENT_LISTENER) {
            e->set_property(0, (uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(elistener_cb)));
        }
    }

    gtk_widget_hide(GTK_WIDGET(elistener_dialog));

    return false;
}

/** --Cam targeter **/
static gboolean
_open_camtargeter_window(gpointer unused)
{
    gint result = gtk_dialog_run(camtargeter_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_CAM_TARGETER) {
            e->properties[1].v.i8 = gtk_combo_box_get_active(GTK_COMBO_BOX(camtargeter_mode));
            e->properties[2].v.i8 = gtk_combo_box_get_active(GTK_COMBO_BOX(camtargeter_offset_mode));

            float v = gtk_range_get_value(camtargeter_x_offset);
            if (v < -150.f) v = -150.f;
            else if (v > 150.f) v = 150.f;
            e->properties[3].v.f = v;

            v = gtk_range_get_value(camtargeter_y_offset);
            if (v < -150.f) v = -150.f;
            else if (v > 150.f) v = 150.f;
            e->properties[4].v.f = v;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(camtargeter_dialog));

    return false;
}

/** --Confirm Quit Dialog **/
static gboolean
_open_confirm_quit(gpointer unused)
{
    if (gtk_dialog_run(confirm_quit_dialog) == GTK_RESPONSE_ACCEPT) {
        tms_infof("Quitting!");
        _tms.state = TMS_STATE_QUITTING;
    } else {
        tms_infof("not quitting.");
    }

    gtk_widget_hide(GTK_WIDGET(confirm_quit_dialog));

    return false;
}

static gboolean
_close_all_dialogs(gpointer unused)
{
    gtk_widget_hide(GTK_WIDGET(play_menu));
    gtk_widget_hide(GTK_WIDGET(editor_menu));
    gtk_widget_hide(GTK_WIDGET(open_window));
    gtk_widget_hide(GTK_WIDGET(open_state_window));
    gtk_widget_hide(GTK_WIDGET(object_window));
    gtk_widget_hide(GTK_WIDGET(save_window));
    gtk_widget_hide(GTK_WIDGET(properties_dialog));
    gtk_widget_hide(GTK_WIDGET(beam_color_dialog));
    gtk_widget_hide(GTK_WIDGET(publish_dialog));
    gtk_widget_hide(GTK_WIDGET(new_level_dialog));
    gtk_widget_hide(GTK_WIDGET(mode_dialog));
    gtk_widget_hide(GTK_WIDGET(quickadd_window));
    gtk_widget_hide(GTK_WIDGET(frequency_window));
    gtk_widget_hide(GTK_WIDGET(settings_dialog));
    gtk_widget_hide(GTK_WIDGET(confirm_quit_dialog));
    gtk_widget_hide(GTK_WIDGET(command_pad_dialog));
    gtk_widget_hide(GTK_WIDGET(sticky_dialog));
    gtk_widget_hide(GTK_WIDGET(fxemitter_dialog));
    gtk_widget_hide(GTK_WIDGET(freq_range_window));
    gtk_widget_hide(GTK_WIDGET(puzzle_play_dialog));
    gtk_widget_hide(GTK_WIDGET(timer_dialog));
    gtk_widget_hide(GTK_WIDGET(escript_window));
    gtk_widget_hide(GTK_WIDGET(synth_dialog));
    gtk_widget_hide(GTK_WIDGET(prompt_settings_dialog));
    gtk_widget_hide(GTK_WIDGET(shapeextruder_dialog));
    gtk_widget_hide(GTK_WIDGET(cursorfield_dialog));
    gtk_widget_hide(GTK_WIDGET(jumper_dialog));
    gtk_widget_hide(GTK_WIDGET(rubber_dialog));
    gtk_widget_hide(GTK_WIDGET(login_window));
    gtk_widget_hide(GTK_WIDGET(autosave_dialog));
    gtk_widget_hide(GTK_WIDGET(community_dialog));
    gtk_widget_hide(GTK_WIDGET(published_dialog));
    //if (cur_prompt) gtk_widget_hide(GTK_WIDGET(cur_prompt));
    return false;
}

static gboolean
_close_absolutely_all_dialogs(gpointer unused)
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) return false;
#endif

    _close_all_dialogs(0);
    gtk_widget_hide(GTK_WIDGET(info_dialog));
    gtk_widget_hide(GTK_WIDGET(package_window));

    return false;
}

static void wait_ui_ready()
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) return;
#endif

    SDL_LockMutex(ui_lock);
    if (!ui_ready) {
        SDL_CondWaitTimeout(ui_cond, ui_lock, 4000);
        if (!ui_ready) tms_fatalf("Could not initialise game (GTK not ready)");
    }
    SDL_UnlockMutex(ui_lock);
}

void ui::open_url(const char *url)
{
#if SDL_VERSION_ATLEAST(2,0,14)
    tms_infof("open url (SDL): %s", url);
    SDL_OpenURL(url);
#else
    #error "SDL2 2.0.14+ is required for this platform"
#endif
}

void
ui::open_dialog(int num, void *data/*=0*/)
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) {
        /* Send default response to any prompt that pops up */
        if (num == DIALOG_PROMPT) {
            if (G->current_prompt) {
                base_prompt *bp = G->current_prompt->get_base_prompt();
                if (bp) {
                    SDL_Delay(40);
                    bp->set_response(PROMPT_RESPONSE_A);
                }
            }
        }

        return;
    }
#endif

    wait_ui_ready();

    switch (num) {
        case DIALOG_SANDBOX_MENU:
			editor_menu_on_entity = 0;
			if (data) {
				editor_menu_on_entity = VOID_TO_UINT8(data);
			}

			gdk_threads_add_idle(_open_sandbox_menu, 0);
            break;

        case DIALOG_OPEN_AUTOSAVE:  gdk_threads_add_idle(_open_autosave, 0); break;
        case DIALOG_EXPORT:         gdk_threads_add_idle(_open_export, 0); break;
        case DIALOG_PLAY_MENU:      gdk_threads_add_idle(_open_play_menu, 0); break;
        case DIALOG_QUICKADD:       gdk_threads_add_idle(_open_quickadd, 0); break;
        case DIALOG_BEAM_COLOR:     gdk_threads_add_idle(_open_beam_color, 0); break;
        case DIALOG_SHAPEEXTRUDER:  gdk_threads_add_idle(_open_shapeextruder, 0); break;
        case DIALOG_CURSORFIELD:    gdk_threads_add_idle(_open_cursorfield, 0); break;
        case DIALOG_ESCRIPT:        gdk_threads_add_idle(_open_escript, 0); break;
        case DIALOG_JUMPER:         gdk_threads_add_idle(_open_jumper, 0); break;
        case DIALOG_PIXEL_COLOR:    gdk_threads_add_idle(_open_pixel_color, 0); break;
        case DIALOG_POLYGON_COLOR:  gdk_threads_add_idle(_open_polygon_color, 0); break;
        case DIALOG_SAVE:           gdk_threads_add_idle(_open_save_window, 0); break;
        case DIALOG_OPEN:           gdk_threads_add_idle(_open_open_dialog, 0); break;

        case DIALOG_OPEN_STATE:
            if (data && VOID_TO_UINT8(data) == 1) {
                open_state_no_testplaying = true;
            } else {
                open_state_no_testplaying = false;
            }

            gdk_threads_add_idle(_open_open_state_dialog, 0);
            break;

        case DIALOG_OPEN_OBJECT:    gdk_threads_add_idle(_open_object_dialog, 0); break;
        case DIALOG_MULTIEMITTER:   gdk_threads_add_idle(_open_multiemitter_dialog, 0); break;
        case DIALOG_EMITTER:        gdk_threads_add_idle(_open_emitter_dialog, 0); break;
        case DIALOG_NEW_LEVEL:      gdk_threads_add_idle(_open_new_level_dialog, 0); break; /* XXX: */
        case DIALOG_SANDBOX_MODE:   gdk_threads_add_idle(_open_mode_dialog, 0); break; /* XXX: */
        case DIALOG_SET_FREQUENCY:  gdk_threads_add_idle(_open_frequency_window, 0); break;
        case DIALOG_CONFIRM_QUIT:   gdk_threads_add_idle(_open_confirm_quit, 0); break;
        case DIALOG_SET_COMMAND:    gdk_threads_add_idle(_open_command_pad_window, 0); break;
        case DIALOG_STICKY:         gdk_threads_add_idle(_open_sticky_window, 0); break;
        case DIALOG_DIGITALDISPLAY: gdk_threads_add_idle(_open_digi_window, 0); break;
        case DIALOG_FXEMITTER:      gdk_threads_add_idle(_open_fxemitter_window, 0); break;
        case DIALOG_EVENTLISTENER:  gdk_threads_add_idle(_open_elistener_window, 0); break;
        case DIALOG_SFXEMITTER:     gdk_threads_add_idle(_open_sfx_window, 0); break;
        case DIALOG_SFXEMITTER_2:   gdk_threads_add_idle(_open_sfx2_window, 0); break;
        case DIALOG_CAMTARGETER:    gdk_threads_add_idle(_open_camtargeter_window, 0); break;
        case DIALOG_SET_FREQ_RANGE: gdk_threads_add_idle(_open_freq_range_window, 0); break;
        case DIALOG_SET_PKG_LEVEL:  gdk_threads_add_idle(_open_pkg_lvl_chooser_window, 0); break;
        case DIALOG_ROBOT:          gdk_threads_add_idle(_open_robot_window, 0); break;
        case DIALOG_PUZZLE_PLAY:    gdk_threads_add_idle(_open_puzzle_play, 0); break;
        case DIALOG_TIMER:          gdk_threads_add_idle(_open_timer, 0); break;
        case DIALOG_SYNTHESIZER:    gdk_threads_add_idle(_open_synth, 0); break;
        case DIALOG_SEQUENCER:      gdk_threads_add_idle(_open_sequencer, 0); break;
        case DIALOG_SETTINGS:       gdk_threads_add_idle(_open_settings, 0); break;
        case DIALOG_VARIABLE:       gdk_threads_add_idle(_open_variable_window, 0); break;
        case DIALOG_ITEM:           gdk_threads_add_idle(_open_item, 0); break;
        case DIALOG_DECORATION:     gdk_threads_add_idle(_open_decoration, 0); break;
        case DIALOG_SET_FACTION:    gdk_threads_add_idle(_open_faction, 0); break;
        case DIALOG_RESOURCE:       gdk_threads_add_idle(_open_resource, 0); break;
        case DIALOG_VENDOR:         gdk_threads_add_idle(_open_vendor, 0); break;
        case DIALOG_FACTORY:        gdk_threads_add_idle(_open_factory, 0); break;
        case DIALOG_TREASURE_CHEST: gdk_threads_add_idle(_open_treasure_chest, 0); break;
        case DIALOG_RUBBER:         gdk_threads_add_idle(_open_rubber, 0); break;
        case DIALOG_PUBLISHED:      gdk_threads_add_idle(_open_published, 0); break;
        case DIALOG_COMMUNITY:      gdk_threads_add_idle(_open_community, 0); break;
        case DIALOG_ANIMAL:         gdk_threads_add_idle(_open_animal, 0); break;
        case DIALOG_SOUNDMAN:       gdk_threads_add_idle(_open_soundman, 0); break;
        case DIALOG_POLYGON:        gdk_threads_add_idle(_open_polygon, 0); break;
        case DIALOG_KEY_LISTENER:   gdk_threads_add_idle(_open_key_listener, 0); break;
        case DIALOG_MULTI_CONFIG:   gdk_threads_add_idle(_open_multi_config, 0); break;

        case CLOSE_ALL_DIALOGS:     gdk_threads_add_idle(_close_all_dialogs, 0); break;
        case CLOSE_ABSOLUTELY_ALL_DIALOGS: gdk_threads_add_idle(_close_absolutely_all_dialogs, 0); break;

        case DIALOG_PUBLISH:        gdk_threads_add_idle(_open_publish_dialog, 0); break;
        case DIALOG_LOGIN:          gdk_threads_add_idle(_open_login_dialog, 0); break;

        case DIALOG_PROMPT:
            if (G) {
                G->reset_touch(false);
            }
            //gdk_threads_add_idle(_open_prompt_dialog, 0);
            gdk_threads_add_timeout(40, _open_prompt_dialog, 0);
            break;
        case DIALOG_PROMPT_SETTINGS: gdk_threads_add_idle(_open_prompt_settings_dialog, 0); break;

        default:
            tms_warnf("Unhandled dialog ID: %d", num);
            break;
    }

    gdk_display_flush(gdk_display_get_default());
}

void ui::open_sandbox_tips()
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) return;
#endif

    wait_ui_ready();

    gdk_threads_add_idle(_open_tips_dialog, 0);

    gdk_display_flush(gdk_display_get_default());
}

void
ui::open_help_dialog(const char *title, const char *description)
{
    wait_ui_ready();

    /* title and description are constant static strings in
     * object facotyr, should be safe to use directly
     * from any thread */
    _pass_info_name = const_cast<char*>(title);
    _pass_info_descr = const_cast<char*>(description);
    gdk_threads_add_idle(_open_info_dialog, 0);

    gdk_display_flush(gdk_display_get_default());
}

void
ui::set_next_action(int action_id)
{
    tms_infof("set_next_Actino: %d", action_id);
    ui::next_action = action_id;
}

void
ui::emit_signal(int num, void *data/*=0*/)
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) return;
#endif

    wait_ui_ready();

    /* XXX this stuff probably needs to be added to gdk_threads_idle_add()! */

    switch (num) {
        case SIGNAL_LOGIN_SUCCESS:
            P.add_action(ui::next_action, 0);
            ui::next_action = 0;
            gtk_widget_hide(GTK_WIDGET(login_window));
            break;

        case SIGNAL_LOGIN_FAILED:
            gtk_label_set_text(login_status, "An error occured.");
            gtk_widget_set_sensitive(GTK_WIDGET(login_btn_log_in), true);
            return;

        case SIGNAL_QUICKADD_REFRESH:
            refresh_quickadd();
            break;

        case SIGNAL_REFRESH_BORDERS:
            refresh_borders();
            break;

        case SIGNAL_ENTITY_CONSTRUCTED:
            editor_menu_last_created->id = VOID_TO_UINT32(data);
            break;
    }

    ui::next_action = ACTION_IGNORE;
}

void
ui::quit()
{
    /* TODO: add proper quit stuff here */
    _tms.state = TMS_STATE_QUITTING;
}

void
ui::open_error_dialog(const char *error_msg)
{
    wait_ui_ready();

    _pass_error_text = strdup(error_msg);
    gdk_threads_add_idle(_open_error_dialog, 0);

    gdk_display_flush(gdk_display_get_default());
}

void
ui::confirm(const char *text,
        const char *button1, principia_action action1,
        const char *button2, principia_action action2,
        const char *button3/*=0*/, principia_action action3/*=ACTION_IGNORE*/,
        struct confirm_data _confirm_data/*=none*/
        )
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) {
        P.add_action(action1.action_id, 0);
        return;
    }
#endif

    wait_ui_ready();

    _pass_confirm_text    = strdup(text);
    _pass_confirm_button1 = strdup(button1);
    _pass_confirm_button2 = strdup(button2);
    if (button3) {
        _pass_confirm_button3 = strdup(button3);
    } else {
        _pass_confirm_button3 = 0;
    }

    confirm_action1 = action1.action_id;
    confirm_action2 = action2.action_id;
    confirm_action3 = action3.action_id;

    confirm_action1_data = action1.action_data;
    confirm_action2_data = action2.action_data;
    confirm_action3_data = action3.action_data;

    confirm_data = _confirm_data;

    gdk_threads_add_idle(_open_confirm_dialog, 0);

    gdk_display_flush(gdk_display_get_default());
}

void
ui::alert(const char *text, uint8_t alert_type/*=ALERT_INFORMATION*/)
{
    wait_ui_ready();

    if (_alert_text) {
        free(_alert_text);
    }

    _alert_type = alert_type;
    _alert_text = strdup(text);

    gdk_threads_add_idle(_open_alert_dialog, 0);

    gdk_display_flush(gdk_display_get_default());
}

#pragma GCC diagnostic pop

#endif
