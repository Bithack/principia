#include <string>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_opengl3.h"
#include "ui_imgui_impl_tms.hh"

int prompt_is_open = 0;

void ui::init() {
  //create context
#ifdef DEBUG
  IMGUI_CHECKVERSION();
#endif
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  
  //set flags
  io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
  io.ConfigInputTrickleEventQueue = false;
  io.ConfigWindowsResizeFromEdges = true; //XXX: not active until custom cursors are implemented...
  io.ConfigDragClickToInputText = true;

  //set PlatformHandleRaw
  ImGuiViewport* main_viewport = ImGui::GetMainViewport();
  main_viewport->PlatformHandleRaw = nullptr;
#if defined(TMS_BACKEND_WINDOWS)
  SDL_SysWMinfo info;
  if (SDL_GetWindowWMInfo((SDL_Window*) _tms._window, &info)) {
    main_viewport->PlatformHandleRaw = (void*)info.info.win.window;
  }
#endif

  //style
  ImGui::StyleColorsDark();

  //init
  tms_assertf(_tms._window != NULL, "window does not exist yet");
  tms_assertf(SDL_GL_GetCurrentContext() != NULL, "no gl ctx");
  tms_assertf(ImGui_ImplOpenGL3_Init(), "gl ctx init failed");
  tms_assertf(ImGui_ImplTMS_Init() == T_OK, "tms init failed");
}

static void ui_layout() {
  static bool show_demo_window = true;
  ImGui::ShowDemoWindow(&show_demo_window);
}

void ui::render() {
  ImGuiIO& io = ImGui::GetIO();

  //update window size
  int w, h;
  SDL_GetWindowSize((SDL_Window*) _tms._window, &w, &h);
  if ((w != 0) && (h != 0)) {
    int display_w, display_h;
    SDL_GL_GetDrawableSize((SDL_Window*) _tms._window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float) w, (float) h);
    io.DisplayFramebufferScale = ImVec2((float) display_w / w, (float) display_h / h);
  } else {
    tms_errorf("window size is 0");
    return;
  }

  //start frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();

  //layout
  ui_layout();

  //render
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ui::open_dialog(int num, void *data) {
  //TODO
  tms_errorf("ui::open_dialog not implemented yet");
}

void ui::open_sandbox_tips() {
  //TODO
  tms_errorf("ui::open_sandbox_tips not implemented yet");
}

void ui::open_url(const char *url) {
  //TODO
  tms_errorf("ui::open_url not implemented yet");
}

void ui::open_help_dialog(const char* title, const char* description, bool enable_markup) {
  //TODO
  tms_errorf("ui::open_help_dialog not implemented yet");
}

void ui::emit_signal(int num, void *data){
  //TODOs
  tms_errorf("ui::emit_signal not implemented yet");
}

void ui::quit() {
  //TODO
  tms_errorf("ui::quit not implemented yet");
}

void ui::set_next_action(int action_id) {
  //TODO
  tms_errorf("ui::set_next_action not implemented yet");
}

void ui::open_error_dialog(const char *error_msg) {
  //TODO
  tms_errorf("ui::open_error_dialog not implemented yet");
}

void ui::confirm(
  const char *text,
  const char *button1, principia_action action1,
  const char *button2, principia_action action2,
  const char *button3, principia_action action3,
  struct confirm_data _confirm_data
) {
  //TODO
  P.add_action(action1.action_id, 0);
  tms_errorf("ui::confirm not implemented yet");
}

void ui::alert(const char* text, uint8_t type) {
  //TODO
  tms_errorf("ui::alert not implemented yet");
}
