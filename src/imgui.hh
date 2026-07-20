#pragma once

#if defined(PRINCIPIA_BACKEND_IMGUI)

#include "imgui.h"
#include "imgui_stdlib.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

//STUFF

static uint64_t __ref;
#define REF_FZERO ((float*) &(__ref = 0))
#define REF_IZERO ((int*) &(__ref = 0))
#define REF_TRUE ((bool*) &(__ref = 1))
#define REF_FALSE ((bool*) &(__ref = 0))

//constants
#define FRAME_FLAGS ImGuiWindowFlags_NoSavedSettings
#define MODAL_FLAGS (FRAME_FLAGS | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)
#define POPUP_FLAGS (FRAME_FLAGS | ImGuiWindowFlags_NoMove)
#define LEVEL_NAME_LEN_SOFT_LIMIT 250
#define LEVEL_NAME_LEN_HARD_LIMIT 254
#define LEVEL_NAME_PLACEHOLDER (const char*)"<no name>"

//Unroll ImVec4 components
#define IM_XY(V) (V).x, (V).y
#define IM_ZW(V) (V).z, (V).w
#define IM_XYZ(V) (V).x, (V).y, (V).z
#define IM_XYZW(V) (V).x, (V).y, (V).z, (V).w

struct PFont {
    std::vector<uint8_t> *fontbuffer;
    ImFont *font;
};

extern struct PFont ui_font;
extern struct PFont ui_font_mono;

extern float imgui_ui_scale;

// HELPER FUNCTIONS

inline float UI(float v) {
    return v * imgui_ui_scale;
}

inline ImVec2 UI(float x, float y) {
    return ImVec2(x * imgui_ui_scale, y * imgui_ui_scale);
}

#define ImGui_ButtonBar(func) \
	ImGui_ButtonBarPadding(); \
\
	if (ImGui_SaveButton()) \
		func(); \
\
	ImGui::SameLine(); \
\
	if (ImGui_CancelButton()) \
		ImGui::CloseCurrentPopup()

inline void ImGui_ButtonBarPadding() {
	ImGui::Dummy(UI(0.0f, 5.0f));
}

inline bool ImGui_SaveButton() {
	return ImGui::Button("Save", UI(70., 0.));
}

inline bool ImGui_CancelButton() {
	return ImGui::Button("Cancel", UI(70., 0.));
}

template<typename ... Args>
inline std::string string_format(const std::string& format, Args ... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;

	if (size_s <= 0)
		return "(string_format error)";

	auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1);
}

//check if string should be filtered by a search query
bool lax_search(const std::string& where, const std::string& what);

//imgui helper: Center next imgui window
void ImGui_CenterNextWindow();

//if &do_open, *do_open = false, and open popup with name
void handle_do_open(bool *do_open, const char* name);

class ImguiDriver {
public:
	void init();
	void pre_render();
	void post_render();
	void quit();
};

#endif
