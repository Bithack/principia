#include "imgui.hh"
#include "misc.hh"
#include "settings.hh"
#include <vector>

#if defined(PRINCIPIA_BACKEND_IMGUI) || defined(UI_IMGUI_IN_GTK)

// imgui_impl_tms.cc
IMGUI_IMPL_API bool ImGui_ImplSDL3_Init();
IMGUI_IMPL_API void ImGui_ImplSDL3_Shutdown();
IMGUI_IMPL_API void ImGui_ImplSDL3_NewFrame();

#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

bool lax_search(const std::string& where, const std::string& what) {
    return std::search(
        where.begin(), where.end(),
        what.begin(), what.end(),
        [](char lhs, char rhs) { return std::tolower(lhs) == std::tolower(rhs); }
    ) != where.end();
}

void ImGui_CenterNextWindow() {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
}

// unused, do we need this?
void ImGui_BeginScaleFont(float scale) {
    ImGui::GetFont()->Scale = scale;
    ImGui::PushFont(ImGui::GetFont());
}

// unused, do we need this?
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

    _fseek(file, 0, SDL_IO_SEEK_END);
    size_t size = _ftell(file);
    tms_debugf("buf size %d", (int) size);
    void *buffer = malloc(size + 1);

    _fseek(file, 0, SDL_IO_SEEK_SET);
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
    style->FramePadding = ImVec2(10, 5);
    style->ItemSpacing  = ImVec2(8, 6);
    style->FrameRounding  = style->GrabRounding  = 2.3f;
    style->WindowRounding = style->PopupRounding = style->ChildRounding = 3.0f;

    //style->FrameBorderSize = .5;

    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.05f, 0.05f, 0.05f, 0.35f);
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

void ImguiDriver::init() {

    //create context
#ifdef DEBUG
    IMGUI_CHECKVERSION();
#endif
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    //set flags
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    //io.ConfigInputTrickleEventQueue = false;
    io.ConfigWindowsResizeFromEdges = true; //XXX: not active until custom cursors are implemented...
    io.ConfigDragClickToInputText = true;

    //Disable saving state/logging
    io.IniFilename = NULL;
    io.LogFilename = NULL;

    principia_style();
    update_imgui_ui_scale();
    load_fonts();

    if (!ImGui_ImplOpenGL3_Init()) {
        tms_fatalf("(imgui-backend) gl impl init failed");
    }

    ImGui_ImplSDL3_Init();
}

void ImguiDriver::pre_render() {
    if (settings["render_gui"]->is_false()) return;

    tms_assertf(GImGui != NULL, "gimgui is null. is imgui ready?");

    ImGuiIO& io = ImGui::GetIO();

    //start frame
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::PushFont(ui_font.font);

#ifdef DEBUG
    ui_demo_layout();
#endif
}

void ImguiDriver::post_render() {
    ImGui::PopFont();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImguiDriver::quit() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

#endif
