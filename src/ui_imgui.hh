#pragma once

#include "entity.hh"
#include "game.hh"
#include "main.hh"
#include <cstdint>
#include <string>
#include <vector>
#include <cstdio>
#include <memory>
#include <stdexcept>

#include "ui.hh"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

//--------------------------------------------

//STUFF

static uint64_t __ref;
#define REF_FZERO ((float*) &(__ref = 0))
#define REF_IZERO ((int*) &(__ref = 0))
#define REF_TRUE ((bool*) &(__ref = 1))
#define REF_FALSE ((bool*) &(__ref = 0))

//constants
#define FRAME_FLAGS ImGuiWindowFlags_NoSavedSettings
#define MODAL_FLAGS (FRAME_FLAGS | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)
#define POPUP_FLAGS (FRAME_FLAGS | ImGuiWindowFlags_NoMove)
#define LEVEL_NAME_LEN_SOFT_LIMIT 250
#define LEVEL_NAME_LEN_HARD_LIMIT 254
#define LEVEL_NAME_PLACEHOLDER (const char*)"<no name>"

//Unroll ImVec4 components
#define IM_XY(V) (V).x, (V).y
#define IM_ZW(V) (V).z, (V).w
#define IM_XYZ(V) (V).x, (V).y, (V).z
#define IM_XYZW(V) (V).x, (V).y, (V).z, (V).w

//HELPER FUNCTIONS

// static void unpack_rgba(uint32_t color, float *r, float *g, float *b, float *a) {
//   int _r = (color >> 24) & 0xFF;
//   int _g = (color >> 16) & 0xFF;
//   int _b = (color >>  8) & 0xFF;
//   int _a = (color      ) & 0xFF;
//   *r = _r / 255.f;
//   *g = _g / 255.f;
//   *b = _b / 255.f;
//   *a = _a / 255.f;
// }

template<typename ... Args>
inline std::string string_format(const std::string& format, Args ... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;
    if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1);
}

//converts 0xRRGGBBAA-encoded u32 to ImVec4
ImVec4 rgba(uint32_t color);

//check if string should be filtered by a search query
bool lax_search(const std::string& where, const std::string& what);

//imgui helper: Center next imgui window
void ImGui_CenterNextWindow();

void ImGui_BeginScaleFont(float scale);
void ImGui_EndScaleFont();

//if &do_open, *do_open = false, and open popup with name
void handle_do_open(bool *do_open, const char* name);

/* forward */
enum class MessageType {
    Message,
    Error,
    LevelInfo
};

/// PFONT ///

struct PFont {
    std::vector<uint8_t> *fontbuffer;
    ImFont *font;
};

extern struct PFont ui_font;
extern struct PFont ui_font_mono;

/* forward */
namespace UiSandboxMenu  { void open(); void layout(); }
namespace UiPlayMenu { void open(); void layout(); }
namespace UiLevelManager { void init(); void open(); void layout(); }
namespace UiLogin { void open(); void layout(); void complete_login(int signal); }
namespace UiMessage { void open(const char* msg, MessageType typ = MessageType::Message); void layout(); }
namespace UiSettings { void open(); void layout(); }
namespace UiLuaEditor { void init(); void open(entity *e = G->selection.e); void layout(); }
namespace UiTips {  void open(); void layout(); }
namespace UiSandboxMode  { void open(); void layout(); }
namespace UiQuickadd { /*static void init();*/ void open(); void layout(); }
namespace UiSynthesizer { void init(); void open(entity *e = G->selection.e); void layout(); }
namespace UiObjColorPicker { void open(bool alpha = false, entity *e = G->selection.e); void layout(); }
namespace UiLevelProperties { void open(); void layout(); }
namespace UiSave { void open(); void layout(); }
namespace UiNewLevel { void open(); void layout(); }
namespace UiFrequency { void open(bool is_range, entity *e = G->selection.e); void layout(); }
namespace UiConfirm { void open(const char* text, const char* button1, principia_action action1, const char* button2, principia_action action2, const char* button3, principia_action action3, struct confirm_data  _confirm_data); void layout(); }
namespace UiAnimal { void open(); void layout(); }
namespace UiRobot { void open(); void layout(); }
namespace UiSticky { void open(); void layout(); }
namespace UiTreasureChest { void open(); void layout(); }
namespace UiPolygon { void open(); void layout(); }
namespace UiRubber { void open(); void layout(); }
namespace UiDecoration { void open(); void layout(); }
namespace UiVariable { void open(); void layout(); }
