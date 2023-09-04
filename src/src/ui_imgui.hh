#include <string>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_opengl3.h"
#include "ui_imgui_impl_tms.hh"

// class UiBase {
//   public:
//     static inline void init() {}
//     static inline void open() {}
//     static inline void layout() {}
// };

// class UiOpenable: public UiBase {
//   protected:
//     static bool is_open;
  
//   public:
//     static inline void open() {
//       is_open = true;
//     }
// };

// class UiPopup: public UiBase {
//   private:
//     static bool do_open;

//   protected:
//     static const char* name;
//     static inline void popup_layout() {};

//   public:
//     static inline void open() {
//       do_open = true;
//     }
//     static inline void layout() {
//       if (do_open) {
//         do_open = false;
//         ImGui::OpenPopup(name);
//       }
//       if (ImGui::BeginPopup(name, ImGuiWindowFlags_NoMove)) {
//         popup_layout();
//         ImGui::EndPopup();
//       }
//     }
// };

// // Implementation

// class UiDemoWindow: public UiOpenable {
//   public:
//     static inline void layout() {
//       if (is_open) ImGui::ShowDemoWindow(&is_open);
//     }
// };

// class UiSandboxMenu: public UiPopup {
//   public:
//     static inline void popup_layout() {
//       //True if current level can be saved as a copy
//       //Saves can only be created if current level state is sandbox
//       bool is_sandbox = G->state.sandbox;

//       //True if already saved and the save can be updated
//       //Saves can only be updated if:
//       // - Current level state is sandbox
//       // - Level is local (and not an auto-save)
//       // - Level is already saved
//       bool can_update_save =
//           G->state.sandbox &&
//           (W->level_id_type == LEVEL_LOCAL) &&
//           (W->level.local_id != 0); //&& W->level.name_len;

//       //TODO info panel

//       //"Level properties"
//       ImGui::MenuItem("Level properties");
      
//       //"Publish online"
//       if (is_sandbox) {
//         ImGui::BeginDisabled(!P.user_id);
//         ImGui::MenuItem("Publish online");
//         ImGui::EndDisabled();
//         ImGui::SetItemTooltip("Upload your level to %s", P.community_host);
//       }

//       //"Save": update current save
//       if (can_update_save && ImGui::MenuItem("Save")) {
//         //temporarily change text to "Saved" (green)?
//         P.add_action(ACTION_SAVE, 0);
//         ImGui::CloseCurrentPopup();
//       }

//       //"Save as...": create a new save
//       if (is_sandbox && ImGui::MenuItem("Save as...")) {
//         //UiSaveAs::open();
//         ImGui::CloseCurrentPopup();
//       }

//       //"Open...": open the Level Manager
//       if (ImGui::MenuItem("Open...")) {
//         //UiSaveManager::open();
//         ImGui::CloseCurrentPopup();
//       }

//       ImGui::Separator();
//     }
// };

static void ui_init() {
  // UiDemoWindow::init();
  // UiSandboxMenu::init();
}

static void ui_layout() {
  // UiDemoWindow::layout();
  // UiSandboxMenu::layout();
}

//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

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
  switch (num) {
    case DIALOG_SANDBOX_MENU:
      UiSandboxMenu::open();
      break;
    default:
      tms_errorf("dialog %d not implemented yet", num);
  }
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
  ImGui_ImplOpenGL3_Shutdown();
  ImGui::DestroyContext();
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
