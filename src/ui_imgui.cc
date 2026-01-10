#ifdef PRINCIPIA_BACKEND_IMGUI

#include "ui_imgui.hh"

#include "game.hh"
#include "main.hh"
#include "misc.hh"
#include "settings.hh"
#include "ui.hh"

#include "tms/backend/print.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include "ui_imgui_impl_tms.hh"

#include <cstdio>
#include <cstdlib>

// Misc helper functions

ImVec4 rgba(uint32_t color) {
    float components[4]; //ABGR
    for (int i = 0; i < 4; i++) {
        components[i] = (float)(color & 0xFF) / 255.;
        color >>= 8;
    }
    return ImVec4(components[3], components[2], components[1], components[0]);
}

bool lax_search(const std::string& where, const std::string& what) {
    return std::search(
        where.begin(), where.end(),
        what.begin(), what.end(),
        [](char lhs, char rhs) { return std::tolower(lhs) == std::tolower(rhs); }
    ) != where.end();
}

void ImGui_CenterNextWindow() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
        ImGuiCond_Always, //ImGuiCond_Appearing,
        ImVec2(0.5f, 0.5f)
    );
}

void ImGui_BeginScaleFont(float scale) {
    ImGui::GetFont()->Scale = scale;
    ImGui::PushFont(ImGui::GetFont());
}

void ImGui_EndScaleFont() {
    ImGui::GetFont()->Scale = 1.;
    ImGui::PopFont();
    ImGui::GetFont()->Scale = 1.;
}

void handle_do_open(bool *do_open, const char* name) {
    if (*do_open) {
        *do_open = false;
        ImGui::OpenPopup(name);
    }
}



// FILE LOADING //
std::vector<uint8_t> *load_ass(const char *path) {
    tms_infof("(imgui-backend) loading asset from %s...", path);

    FILE_IN_ASSET(true);
    FILE *file = (FILE*) _fopen(path, "rb");
    tms_assertf(file, "file not found");

    _fseek(file, 0, SEEK_END);
    size_t size = _ftell(file);
    tms_debugf("buf size %d", (int) size);
    void *buffer = malloc(size + 1);

    _fseek(file, 0, SEEK_SET);
    _fread(buffer, 1, size, file);
    _fclose(file);

    uint8_t *typed_buffer = (uint8_t*) buffer;
    std::vector<uint8_t> *vec = new std::vector<uint8_t>(typed_buffer, typed_buffer + size);
    free(buffer);

    return vec;
}

/// PFONT ///

static struct PFont im_load_ttf(const char *path, float size_pixels) {
    std::vector<uint8_t>* buf = load_ass(path);

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    if (size_pixels <= 16.) {
        font_cfg.OversampleH = 3;
    }

    ImFont *font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(buf->data(), buf->size(), size_pixels, &font_cfg);

    struct PFont pfont;
    pfont.fontbuffer = buf;
    pfont.font = font;

    return pfont;
}

struct PFont ui_font;
struct PFont ui_font_mono;

static void load_fonts() {
    //TODO free existing fonts

    float size_pixels = 12.f;
    size_pixels *= settings["uiscale"]->v.f;
    size_pixels = roundf(size_pixels);

    tms_infof("font size %fpx", size_pixels);

    ui_font = im_load_ttf("data/fonts/Roboto-Bold.ttf", size_pixels);
    ui_font_mono = im_load_ttf("data/fonts/SourceCodePro-Medium.ttf", size_pixels + 2);

}

static void update_imgui_ui_scale() {
    float scale_factor = settings["uiscale"]->v.f;
    ImGui::GetStyle().ScaleAllSizes(scale_factor);

    //ImGui::GetIO().FontGlobalScale = roundf(9. * scale_factor) / 9.;
}

static void principia_style() {
    ImGui::StyleColorsDark();
    ImGuiStyle *style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    //Rounding
    style->FrameRounding  = style->GrabRounding  = 2.3f;
    style->WindowRounding = style->PopupRounding = style->ChildRounding = 3.0f;

    //style->FrameBorderSize = .5;

    //TODO style
    //colors[ImGuiCol_WindowBg]    = rgba(0xfdfdfdff);
    //colors[ImGuiCol_ScrollbarBg] = rgba(0x767676ff);
    //colors[ImGuiCol_ScrollbarGrab] = rgba(0x767676ff);
    //colors[ImGuiCol_ScrollbarGrabActive] = rgba(0xb1b1b1);
}

static bool init_ready = false;

// UI helpers implemented here to avoid multiple definitions when header is included
void ui_init() {
    UiLevelManager::init();
    UiLuaEditor::init();
    //UiQuickadd::init();
    UiSynthesizer::init();
}

//On debug builds, open imgui demo window by pressing Shift+F9
#ifdef DEBUG
static bool show_demo = false;
static void ui_demo_layout() {
    if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F9) && ImGui::GetIO().KeyShift) {
        show_demo ^= 1;
    }
    if (show_demo) {
        ImGui::ShowDemoWindow(&show_demo);
    }
}
#endif

static void ui_layout() {
#ifdef DEBUG
    ui_demo_layout();
#endif
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
}

// Non-header ui::* definitions
void ui::init() {
    tms_assertf(!init_ready, "ui::init called twice");

    //create context
#ifdef DEBUG
    IMGUI_CHECKVERSION();
#endif
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    //set flags
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigInputTrickleEventQueue = false;
    io.ConfigWindowsResizeFromEdges = true; //XXX: not active until custom cursors are implemented...
    io.ConfigDragClickToInputText = true;
    //Disable saving state/logging
    io.IniFilename = NULL;
    io.LogFilename = NULL;

    //style
    principia_style();

    //update scale
    update_imgui_ui_scale();

    //load fonts
    load_fonts();

    //ensure gl ctx exists
    tms_assertf(_tms._window != NULL, "window does not exist yet");
    tms_assertf(SDL_GL_GetCurrentContext() != NULL, "no gl ctx");

    //init
    if (!ImGui_ImplOpenGL3_Init()) {
        tms_fatalf("(imgui-backend) gl impl init failed");
    }

    if (ImGui_ImplTMS_Init() != T_OK) {
        tms_fatalf("(imgui-backend) tms impl init failed");
    }

    //call ui_init
    ui_init();

    init_ready = true;
}

void ui::render() {
    if (settings["render_gui"]->is_false()) return;

    tms_assertf(init_ready, "ui::render called before ui::init");
    tms_assertf(GImGui != NULL, "gimgui is null. is imgui ready?");

    ImGuiIO& io = ImGui::GetIO();

    //start frame
    if (ImGui_ImplTms_NewFrame() <= 0) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::PushFont(ui_font.font);

    //layout
    ui_layout();

    ImGui::PopFont();

    //render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ui::open_dialog(int num, void *data) {
    tms_assertf(init_ready, "ui::open_dialog called before ui::init");
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
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplTMS_Shutdown();
    ImGui::DestroyContext();
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
