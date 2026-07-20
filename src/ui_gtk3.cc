/**
 * Note: The GTK3 dialog backend is deprecated and will be replaced with the
 * Imgui backend on desktop platforms when it is finished. Don't spend any
 * more time than absolutely necessary on this backend.
 */

#include "game.hh"
#include "main.hh"
#include "ui.hh"
#include <SDL3/SDL.h>
#include <tms/cpp.hh>

#if !defined(SDL_PLATFORM_ANDROID) && !defined(NO_UI)

#ifndef PRINCIPIA_BACKEND_IMGUI

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

bool   ui_ready = false;
SDL_Condition  *ui_cond;
SDL_Mutex *ui_lock;
static gboolean _sig_ui_ready(gpointer unused);

static gboolean on_window_close(GtkWidget *w, void *unused) {
    P.focused = true;
    gtk_widget_hide(w);
    return true;
}

static GtkCellRenderer *add_text_column(GtkTreeView *tv, const char *title, int id) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
    column = GTK_TREE_VIEW_COLUMN(gtk_tree_view_column_new_with_attributes(title, renderer, "text", id, NULL));

    gtk_tree_view_column_set_sort_column_id(column, id);
    gtk_tree_view_append_column(tv, column);

    return renderer;
}

static void apply_dialog_defaults(
    void *w,
    GtkCallback on_show=0,
    gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)=0
) {
    gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);
    gtk_window_set_keep_above(GTK_WINDOW(w), TRUE);
    g_signal_connect(w, "delete-event", G_CALLBACK(on_window_close), 0);

    if (on_show)
        g_signal_connect(w, "show", G_CALLBACK(on_show), 0);

    if (on_keypress)
        g_signal_connect(w, "key-press-event", G_CALLBACK(on_keypress), 0);
}

static GtkWindow *new_window_defaults(const char *title, GtkCallback on_show=0, gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)=0) {
    GtkWidget *r = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(r), 10);
    gtk_window_set_title(GTK_WINDOW(r), title);
    gtk_window_set_resizable(GTK_WINDOW(r), false);

    apply_dialog_defaults(r, on_show, on_keypress);

    return GTK_WINDOW(r);
}

bool btn_pressed(GtkWidget *ref, GtkButton *btn, gpointer user_data) {
    return (
        ref == GTK_WIDGET(btn) &&
        (
            ((gtk_widget_get_state_flags(ref) & GTK_STATE_ACTIVE) != 0) ||
            GPOINTER_TO_INT(user_data) == 1
        )
    );
}


int _gtk_loop(void *p) {
    gtk_init(NULL, NULL);

    g_object_set(
        gtk_settings_get_default(),
        "gtk-application-prefer-dark-theme", true,
        "gtk-tooltip-timeout", 100,
        NULL
    );

    gdk_threads_add_idle(_sig_ui_ready, 0);

    gtk_main();

    return T_OK;
}

static gboolean _sig_ui_ready(gpointer unused) {
    SDL_LockMutex(ui_lock);
    ui_ready = true;
    SDL_SignalCondition(ui_cond);
    SDL_UnlockMutex(ui_lock);

    return false;
}

static gboolean _close_all_dialogs(gpointer unused) {
    return false;
}

static void wait_ui_ready() {
    SDL_LockMutex(ui_lock);
    if (!ui_ready) {
        SDL_WaitConditionTimeout(ui_cond, ui_lock, 4000);
        if (!ui_ready) tms_fatalf("Could not initialise game (GTK not ready)");
    }
    SDL_UnlockMutex(ui_lock);
}

#endif

#include "imgui.hh"
#include "ui_imgui.hh"

static ImguiDriver imgui_driver;

void ui::init() {
#ifndef PRINCIPIA_BACKEND_IMGUI
    ui_lock = SDL_CreateMutex();
    ui_cond = SDL_CreateCondition();
    ui_ready = false;

    SDL_Thread *gtk_thread;

    gtk_thread = SDL_CreateThread(_gtk_loop, "_gtk_loop", 0);

    if (gtk_thread == NULL)
        tms_errorf("SDL_CreateThread failed: %s", SDL_GetError());
#endif

    imgui_driver = ImguiDriver();
    imgui_driver.init();
}

void ui::open_dialog(int num, void *data/*=0*/) {
#ifndef PRINCIPIA_BACKEND_IMGUI
    wait_ui_ready();
#endif

    switch (num) {
        case DIALOG_SANDBOX_MENU:
            UiSandboxMenu::open();
            break;
        case DIALOG_LEVEL_PROPERTIES:
            UiLevelProperties::open();
            break;
        case DIALOG_EXPORT:
            UiExport::open();
            break;
        case DIALOG_PLAY_MENU:
            UiPlayMenu::open();
            break;
        case DIALOG_QUICKADD:
            UiQuickadd::open();
            break;
        case DIALOG_SHAPEEXTRUDER:
            UiShapeExtruder::open();
            break;
        case DIALOG_CURSORFIELD:
            UiCursorField::open();
            break;
        case DIALOG_ESCRIPT:
            UiLuaEditor::open();
            break;
        case DIALOG_JUMPER:
            UiJumper::open();
            break;
        case DIALOG_BEAM_COLOR:
        case DIALOG_PIXEL_COLOR:
        case DIALOG_POLYGON_COLOR:
            UiObjColorPicker::open();
            break;
        case DIALOG_SAVE:
            UiSave::open(false);
            break;
        case DIALOG_SAVE_COPY:
            UiSave::open(true);
            break;
        case DIALOG_OPEN:
            UiLevelManager::open();
            break;
        case DIALOG_OPEN_STATE:
            UiOpenState::open(data && VOID_TO_UINT8(data) == 1);
            break;
        case DIALOG_OPEN_OBJECT:
            UiOpenObject::open(false);
            break;
        case DIALOG_MULTIEMITTER:
            UiOpenObject::open(true);
            break;
        case DIALOG_EMITTER:
            UiEmitter::open();
            break;
        case DIALOG_NEW_LEVEL:
            UiNewLevel::open();
            break;
        case DIALOG_SANDBOX_MODE:
            UiSandboxMode::open();
            break;
        case DIALOG_SET_FREQUENCY:
            UiFrequency::open(false);
            break;
        case DIALOG_CONFIRM_QUIT:
            UiConfirmQuit::open();
            break;
        case DIALOG_SET_COMMAND:
            UiCommandPad::open();
            break;
        case DIALOG_STICKY:
            UiSticky::open();
            break;
        case DIALOG_DIGITALDISPLAY:
            UiDigitalDisplay::open();
            break;
        case DIALOG_FXEMITTER:
            UiFXEmitter::open();
            break;
        case DIALOG_EVENTLISTENER:
            UiEventListener::open();
            break;
        case DIALOG_SFXEMITTER:
            UiSfxEmitterLegacy::open();
            break;
        case DIALOG_SFXEMITTER_2:
            UiSfxEmitter::open();
            break;
        case DIALOG_CAMTARGETER:
            UiCamTargeter::open();
            break;
        case DIALOG_SET_FREQ_RANGE:
            UiFrequency::open(true);
            break;
        case DIALOG_SET_PKG_LEVEL:
            UiPkgLvlSelector::open();
            break;
        case DIALOG_ROBOT:
            UiRobot::open();
            break;
        case DIALOG_TIMER:
            UiTimer::open();
            break;
        case DIALOG_SYNTHESIZER:
            UiSynthesizer::open();
            break;
        case DIALOG_SEQUENCER:
            UiSequencer::open();
            break;
        case DIALOG_SETTINGS:
            UiSettings::open();
            break;
        case DIALOG_VARIABLE:
            UiVariable::open();
            break;
        case DIALOG_ITEM:
            UiItem::open();
            break;
        case DIALOG_DECORATION:
            UiDecoration::open();
            break;
        case DIALOG_SET_FACTION:
            UiSetFaction::open();
            break;
        case DIALOG_RESOURCE:
            UiResource::open();
            break;
        case DIALOG_VENDOR:
            UiVendor::open();
            break;
        case DIALOG_FACTORY:
            UiFactory::open();
            break;
        case DIALOG_TREASURE_CHEST:
            UiTreasureChest::open();
            break;
        case DIALOG_RUBBER:
            UiRubber::open();
            break;
        case DIALOG_PUBLISHED:
            UiPublished::open();
            break;
        case DIALOG_COMMUNITY:
            UiCommunity::open();
            break;
        case DIALOG_ANIMAL:
            UiAnimal::open();
            break;
        case DIALOG_SOUNDMAN:
            UiSoundManager::open();
            break;
        case DIALOG_POLYGON:
            UiPolygon::open();
            break;
        case DIALOG_KEY_LISTENER:
            UiKeyListener::open();
            break;
        case DIALOG_MULTI_CONFIG:
            UiMultiConfig::open();
            break;
        case CLOSE_ABSOLUTELY_ALL_DIALOGS:
        case CLOSE_ALL_DIALOGS:
#ifndef PRINCIPIA_BACKEND_IMGUI
            gdk_threads_add_idle(_close_all_dialogs, 0);
            break;
#else
            tms_infof("XXX: CLOSE_ALL_DIALOGS/CLOSE_ABSOLUTELY_ALL_DIALOGS (200/201) are intentionally ignored");
            break;
#endif
        case DIALOG_PUBLISH:
            UiPublish::open();
            break;
        case DIALOG_LOGIN:
            UiLogin::open();
            break;
        case DIALOG_LEVEL_INFO:
            UiMessage::open((char *)data, MessageType::LevelInfo);
            break;
        case DIALOG_PROMPT:
            if (G)
                G->reset_touch(false);
            UiPrompt::open();
            break;
        case DIALOG_PROMPT_SETTINGS:
            UiPromptSettings::open();
            break;
        default:
            tms_warnf("Unhandled dialog ID: %d", num);
            break;
    }

#ifndef PRINCIPIA_BACKEND_IMGUI
    gdk_display_flush(gdk_display_get_default());
#endif
}

void ui::open_sandbox_tips() {
    UiTips::open();
}

void ui::set_next_action(int action_id) {
    tms_infof("set_next_Actino: %d", action_id);
    ui::next_action = action_id;
}

void ui::emit_signal(int num, void *data/*=0*/) {
#ifndef PRINCIPIA_BACKEND_IMGUI
    wait_ui_ready();
#endif

    switch (num) {
        case SIGNAL_LOGIN_SUCCESS:
            UiLogin::complete_login(num);
            if (ui::next_action != ACTION_IGNORE) {
                P.add_action(ui::next_action, 0);
                ui::next_action = ACTION_IGNORE;
            }
            break;

        case SIGNAL_LOGIN_FAILED:
            ui::next_action = ACTION_IGNORE;
            UiLogin::complete_login(num);
            break;

        case SIGNAL_REFRESH_BORDERS:
            UiLevelProperties::reload_border_sizes();
            break;
    }

    ui::next_action = ACTION_IGNORE;
}

void ui::quit() {
    /* TODO: add proper quit stuff here */
    _tms.state = TMS_STATE_QUITTING;

    imgui_driver.quit();
}

void ui::open_error_dialog(const char *error_msg) {
    UiMessage::open(error_msg, MessageType::Error);
}

void ui::confirm(const char *text,
        const char *button1, principia_action action1,
        const char *button2, principia_action action2,
        const char *button3/*=0*/, principia_action action3/*=ACTION_IGNORE*/,
        struct confirm_data _confirm_data/*=none*/
        ) {

    UiConfirm::open(text, button1, action1, button2, action2, button3, action3, _confirm_data);
}

void ui::alert(const char *text, uint8_t alert_type/*=ALERT_INFORMATION*/) {
    UiMessage::open(text, MessageType::Message);
}

void ui::render() {
    imgui_driver.pre_render();

    UiSandboxMenu::layout();
    UiSandboxMode::layout();
    UiQuickadd::layout();
    UiPlayMenu::layout();
    UiNewLevel::layout();
    UiLogin::layout();
    UiAnimal::layout();
    UiRubber::layout();
    UiObjColorPicker::layout();
    UiPolygon::layout();
    UiVariable::layout();
    UiSticky::layout();
    UiJumper::layout();
    UiDecoration::layout();
    UiEmitter::layout();
    UiCommandPad::layout();
    UiFrequency::layout();
    UiPkgLvlSelector::layout();
    UiEventListener::layout();
    UiResource::layout();
    UiItem::layout();
    UiSave::layout();
    UiLevelManager::layout();
    UiShapeExtruder::layout();
    UiCursorField::layout();
    UiSettings::layout();
    UiSetFaction::layout();
    UiConfirm::layout();
    UiMessage::layout();
    UiKeyListener::layout();
    UiTips::layout();
    UiPublish::layout();
    UiPublished::layout();
    UiTimer::layout();
    UiCommunity::layout();
    UiConfirmQuit::layout();
    UiLuaEditor::layout();
    UiCamTargeter::layout();
    UiVendor::layout();
    UiFXEmitter::layout();
    UiPrompt::layout();
    UiPromptSettings::layout();
    UiSoundManager::layout();
    UiSequencer::layout();
    UiExport::layout();
    UiTreasureChest::layout();
    UiLevelProperties::layout();
    UiRobot::layout();
    UiDigitalDisplay::layout();
    UiFactory::layout();
    UiSfxEmitter::layout();
    UiSfxEmitterLegacy::layout();
    UiSynthesizer::layout();
    UiMultiConfig::layout();
    UiOpenState::layout();
    UiOpenObject::layout();

    imgui_driver.post_render();
}

#endif
