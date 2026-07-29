#include "game.hh"
#include "main.hh"
#include "ui.hh"
#include <SDL3/SDL.h>
#include <tms/cpp.hh>

#if !defined(SDL_PLATFORM_ANDROID) && !defined(NO_UI)

#include "imgui_internal.h"
#include "imgui.hh"
#include "ui_imgui.hh"

static ImguiDriver imgui_driver;

void ui::init() {
    imgui_driver = ImguiDriver();
    imgui_driver.init();
}

void ui::open_dialog(int num, void *data/*=0*/) {
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
            ImGui::ClosePopupsOverWindow(nullptr, true);
            break;
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
}

void ui::open_sandbox_tips() {
    UiTips::open();
}

void ui::set_next_action(int action_id) {
    tms_infof("set_next_Actino: %d", action_id);
    ui::next_action = action_id;
}

void ui::emit_signal(int num, void *data/*=0*/) {

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

bool ui::is_blocking() {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard || io.WantCaptureMouse;
}

#endif
