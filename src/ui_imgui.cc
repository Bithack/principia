#include "imgui.hh"
#include "ui_imgui.hh"
#include "ui.hh"

#ifdef PRINCIPIA_BACKEND_IMGUI

static ImguiDriver imgui_driver;

void ui::init() {
    imgui_driver = ImguiDriver();
    imgui_driver.init();
}

void ui::render() {
    imgui_driver.pre_render();

    UiSandboxMenu::layout();
    UiPlayMenu::layout();
    UiLevelManager::layout();
    UiVariable::layout();
    UiLogin::layout();
    UiMessage::layout();
    UiSettings::layout();
    UiLuaEditor::layout();
    UiTips::layout();
    UiSandboxMode::layout();
    UiQuickadd::layout();
    UiSynthesizer::layout();
    UiObjColorPicker::layout();
    UiLevelProperties::layout();
    UiSave::layout();
    UiNewLevel::layout();
    UiFrequency::layout();
    UiConfirm::layout();
    UiAnimal::layout();
    UiRobot::layout();
    UiSticky::layout();
    UiTreasureChest::layout();
    UiPolygon::layout();
    UiRubber::layout();
    UiDecoration::layout();

    imgui_driver.post_render();
}

void ui::open_dialog(int num, void *data) {
    switch (num) {
        //XXX: this gets called after opening the sandbox menu, closing it immediately
        case CLOSE_ABSOLUTELY_ALL_DIALOGS:
        case CLOSE_ALL_DIALOGS:
            tms_infof("XXX: CLOSE_ALL_DIALOGS/CLOSE_ABSOLUTELY_ALL_DIALOGS (200/201) are intentionally ignored");
            break;
        case DIALOG_SANDBOX_MENU:
            UiSandboxMenu::open();
            break;
        case DIALOG_PLAY_MENU:
            UiPlayMenu::open();
            break;
        case DIALOG_OPEN:
            UiLevelManager::open();
            break;
        case DIALOG_VARIABLE:
            UiVariable::open();
            break;
        case DIALOG_LOGIN:
            UiLogin::open();
            break;
        case DIALOG_SETTINGS:
            UiSettings::open();
            break;
        case DIALOG_ESCRIPT:
            UiLuaEditor::open();
            break;
        case DIALOG_SANDBOX_MODE:
            UiSandboxMode::open();
            break;
        case DIALOG_QUICKADD:
            UiQuickadd::open();
            break;
        case DIALOG_SYNTHESIZER:
            UiSynthesizer::open();
            break;
        case DIALOG_BEAM_COLOR:
        case DIALOG_POLYGON_COLOR:
        case DIALOG_PIXEL_COLOR:
            UiObjColorPicker::open();
            break;
        case DIALOG_LEVEL_PROPERTIES:
            UiLevelProperties::open();
            break;
        case DIALOG_SAVE:
        case DIALOG_SAVE_COPY:
            UiSave::open();
            break;
        case DIALOG_NEW_LEVEL:
            UiNewLevel::open();
            break;
        case DIALOG_SET_FREQUENCY:
            UiFrequency::open(false);
            break;
        case DIALOG_SET_FREQ_RANGE:
            UiFrequency::open(true);
            break;
        case DIALOG_LEVEL_INFO:
            UiMessage::open((char *)data, MessageType::LevelInfo);
            break;
        case DIALOG_ANIMAL:
            UiAnimal::open();
            break;
        case DIALOG_ROBOT:
            UiRobot::open();
            break;
        case DIALOG_STICKY:
            UiSticky::open();
            break;
        case DIALOG_TREASURE_CHEST:
            UiTreasureChest::open();
            break;
        case DIALOG_POLYGON:
            UiPolygon::open();
            break;
        case DIALOG_RUBBER:
            UiRubber::open();
            break;
        case DIALOG_DECORATION:
            UiDecoration::open();
            break;
        default:
            tms_errorf("dialog %d not implemented yet", num);
    }
}

void ui::open_sandbox_tips() {
    UiTips::open();
}

void ui::emit_signal(int num, void *data){
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
    }
}

void ui::quit() {
    imgui_driver.quit();
}

void ui::set_next_action(int action_id) {
    tms_infof("set_next_action %d", action_id);
    ui::next_action = action_id;
}

void ui::open_error_dialog(const char *error_msg) {
    UiMessage::open(error_msg, MessageType::Error);
}

void ui::confirm(
    const char *text,
    const char *button1, principia_action action1,
    const char *button2, principia_action action2,
    const char *button3, principia_action action3,
    struct confirm_data _confirm_data
) {
    UiConfirm::open(text, button1, action1, button2, action2, button3, action3, _confirm_data);
}

void ui::alert(const char* text, uint8_t type) {
    UiMessage::open(text, MessageType::Message);
}

#endif
