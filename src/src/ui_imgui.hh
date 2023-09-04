#include <string>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
#include "imgui.h"
//#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "imgui_impl_opengl3.h"
#include "ui_imgui_impl_tms.hh"

//STUFF
static bool __ec9f6917_ref;
#define REF_TRUE &(__ec9f6917_ref = true)
#define REF_FALSE &(__ec9f6917_ref = false)

#define MODAL_FLAGS (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)

//HELPER FUNCTIONS

static bool lax_search(const std::string& where, const std::string& what) {
  return std::search(
    where.begin(), where.end(),
    what.begin(), what.end(),
    [](char lhs, char rhs) { return std::tolower(lhs) == std::tolower(rhs); }
  ) != where.end();
}

static void ImGui_CenterNextWindow() {
  ImGuiIO& io = ImGui::GetIO();
  ImGui::SetNextWindowPos(
    ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
    ImGuiCond_Always, //ImGuiCond_Appearing,
    ImVec2(0.5f, 0.5f)
  );
}

namespace UiSandboxMenu  { static void open(); static void layout(); }
namespace UiLevelManager { static void open(); static void layout(); }
namespace UiLogin { static void open(); static void layout(); static void complete_login(int signal); }

namespace UiSandboxMenu {
  static bool do_open = false;
  static b2Vec2 sb_position = b2Vec2_zero;

  static void open() {
    do_open = true;
    sb_position = G->get_last_cursor_pos(0);
  }

  static void layout() {
    if (do_open) {
      do_open = false;
      ImGui::OpenPopup("sandbox_menu");
    }
    if (ImGui::BeginPopup("sandbox_menu", ImGuiWindowFlags_NoMove)) {
      //True if current level can be saved as a copy
      //Saves can only be created if current level state is sandbox
      bool is_sandbox = G->state.sandbox;

      //True if already saved and the save can be updated
      //Saves can only be updated if:
      // - Current level state is sandbox
      // - Level is local (and not an auto-save)
      // - Level is already saved
      bool can_update_save =
          G->state.sandbox &&
          (W->level_id_type == LEVEL_LOCAL) &&
          (W->level.local_id != 0); //&& W->level.name_len;

      //Info panel

      //Cursor:
      ImGui::Text("Cursor: (%.2f, %.2f)", sb_position.x, sb_position.y);
      ImGui::Separator();

      //Selected object info:
      if (G->selection.e) {
        //If an object is selected, display it's info...
        //XXX: some of this stuff does the same things as principia ui items...
        //---- consider removal?
        entity* sent = G->selection.e;
        b2Vec2 sent_pos = sent->get_position();
        ImGui::Text("%s (g_id: %d)", sent->get_name(), sent->g_id);
        ImGui::Text("ID: %d", sent->id);
        ImGui::Text("Position: (%.2f, %.2f)", sent_pos.x, sent_pos.y);
        // if ((sent->dialog_id > 0) && ImGui::MenuItem("Configure...")) {
        //   ui::open_dialog(sent->dialog_id);
        // }
        if (ImGui::MenuItem("> Move to cursor")) {
          G->selection.e->set_position(sb_position);
        };
        ImGui::Separator();
      }


      //"Level properties"
      if (ImGui::MenuItem("Level properties")) {
        //TODO
      }
      
      //"Publish online"
      if (is_sandbox) {
        ImGui::BeginDisabled(!P.user_id);
        ImGui::MenuItem("Publish online");
        ImGui::EndDisabled();
        ImGui::SetItemTooltip("Upload your level to %s", P.community_host);
      }

      //"Save": update current save
      if (can_update_save && ImGui::MenuItem("Save")) {
        //XXX: temporarily change text to "Saved" (green)?
        P.add_action(ACTION_SAVE, 0);
        ImGui::CloseCurrentPopup();
      }

      //"Save as...": create a new save
      if (is_sandbox && ImGui::MenuItem("Save as...")) {
        //UiSaveAs::open();
        ImGui::CloseCurrentPopup();
      }

      //"Open...": open the Level Manager
      if (ImGui::MenuItem("Open...")) {
        UiLevelManager::open();
        ImGui::CloseCurrentPopup();
      }

      ImGui::Separator();
      
      //"User menu": This menu is basically useless/just a placeholder
      if (P.user_id && P.username) {
        ImGui::PushID("##UserMenu");
        if (ImGui::BeginMenu(P.username)) {
          if (ImGui::MenuItem("Manage account")) {
            char tmp[1024];
            snprintf(tmp, 1023, "https://%s/user/%s", P.community_host, P.username);
            ui::open_url(tmp);
          }
          if (ImGui::MenuItem("Log out")) {
            //TODO actually log out
            P.user_id = 0;
            P.username = nullptr;
            P.add_action(ACTION_REFRESH_HEADER_DATA, 0);
          }
          ImGui::EndMenu();
        };
        ImGui::PopID();
      } else {
        if (ImGui::MenuItem("Log in")) {
          UiLogin::open();
        };
      }

      if (ImGui::MenuItem("Settings")) {
        //TODO
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Back to menu")) {
        P.add_action(ACTION_GOTO_MAINMENU, 0);
      };

      ImGui::EndMenu();
    }
  }
};


namespace UiLevelManager {
  struct lvlinfo_ext {
    lvlinfo info;
    uint32_t id;
    int type;
  };

  static bool do_open = false;
  static std::string search_query{""};

  static lvlfile *level_list = nullptr;
  static int level_list_type = LEVEL_LOCAL;

  static lvlinfo_ext *level_metadata = nullptr;

  static void update_level_info(int id_type, uint32_t id) {
    if (level_metadata) {
      //Check if data needs to be reloaded
      if ((level_metadata->id == id) && (level_metadata->type == id_type)) return;

      //Dealloc current data
      level_metadata->info.~lvlinfo();
      free(level_metadata);
    }
    
    level_metadata = new lvlinfo_ext;

    //Update meta
    level_metadata->id = id;
    level_metadata->type = id_type;
    
    //Read level info
    lvledit lvl;
    if (lvl.open(id_type, id)) {
      level_metadata->info = lvl.lvl;
      if (level_metadata->info.descr_len && level_metadata->info.descr) {
        level_metadata->info.descr = strdup(level_metadata->info.descr);
      }
    } else {
      delete level_metadata;
      level_metadata = nullptr;
    }
  }

  static void reload_level_list() {
    //Recursively deallocate the linked list
    while (level_list) {
      lvlfile* next = level_list->next;
      delete level_list;
      level_list = next;
    }
    //Get a new list of levels
    level_list = pkgman::get_levels(level_list_type);
  }

  static void open() {
    do_open = true;
    search_query = "";
    level_list_type = LEVEL_LOCAL;
    reload_level_list();
  }

  static void layout() {
    ImGuiIO& io = ImGui::GetIO();
    if (do_open) {
      do_open = false;
      ImGui::OpenPopup("Level Manager");
    }
    ImGui_CenterNextWindow();
    ImGui::SetNextWindowSize(ImVec2(800., 0.));
    if (ImGui::BeginPopupModal("Level Manager", REF_TRUE, MODAL_FLAGS)) {
      bool any_level_found = false;
      
      //Top action bar
      {
        //Level type selector
        //Allows switching between local and DB levels
        static const char* items[] = { "Local", "Downloaded" };
        if (ImGui::Combo("##id-lvltype", &level_list_type, items, IM_ARRAYSIZE(items))) {
          reload_level_list();
        }

        //"Get more levels" button
        ImGui::SameLine();
        if ((level_list_type == LEVEL_DB) && ImGui::Button("Get more levels"))
          ui::open_url((std::string("https://") + P.community_host).c_str());

        //Align stuff to the right
        //lvlname width + padding
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (200. + 10.));
        
        //Actual level name field
        ImGui::PushItemWidth(200.);
        ImGui::InputTextWithHint("##LvlmanLevelName", "Search levels", &search_query);
        ImGui::PopItemWidth();
      }

      ImGui::Separator();

      //Actual level list
      ImGui::BeginChild("save_list_child", ImVec2(0., 500.), false);
      if (ImGui::BeginTable("save_list", 5, ImGuiTableFlags_Borders)) {
        //Setup table columns
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Last modified", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();

        lvlfile *level = level_list;
        while (level) {
          //Search (lax_search is used to ignore case)
          if ((search_query.length() > 0) && !(
            lax_search(level->name, search_query) ||
            (std::to_string(level->id).find(search_query) != std::string::npos)
          )) {
            //Just skip levels we don't like
            level = level->next;
            continue;
          }

          //This is required to prevent ID conflicts
          ImGui::PushID(level->id);

          //Start laying out the table row...
          ImGui::TableNextRow();

          //ID
          if (ImGui::TableNextColumn()) {
            ImGui::Text("%d", level->id);
          }

          //Name
          if (ImGui::TableNextColumn()) {
            ImGui::SetNextItemWidth(999.);
            ImGui::LabelText("##levelname", "%s", level->name);

            //Display description if hovered
            if (ImGui::BeginItemTooltip()) {
              update_level_info(level->id_type, level->id);
              if (!level_metadata) {
                ImGui::TextColored(ImVec4(1.,.3,.3,1.), "Failed to load level metadata");
              } else if (level_metadata->info.descr_len && level_metadata->info.descr) {
                ImGui::PushTextWrapPos(400);
                ImGui::TextWrapped("%s", level_metadata->info.descr);
                ImGui::PopTextWrapPos();
              } else {
                ImGui::TextColored(ImVec4(.6,.6,.6,1.), "<no description>");
              }
              ImGui::EndTooltip();
            }
          }

          //Modified date
          if (ImGui::TableNextColumn()) {
            ImGui::TextUnformatted(level->modified_date);
          }

          //Version
          if (ImGui::TableNextColumn()) {
            const char* version_str = level_version_string(level->version);
            if (version_str == "unknown_version") version_str = "unknown";
            if (version_str == "old_level") version_str = "old";
            ImGui::Text("%s (%d)", version_str, level->version);
          }

          //Actions
          if (ImGui::TableNextColumn()) {
            // Delete level ---
            // To prevent accidental level deletion,
            // Shift must be held while clicking the button
            bool allow_delete = io.KeyShift;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, allow_delete ? 1. : .6);
            if (ImGui::Button("Delete##delete-sandbox-level")) {
              G->lock();
              if (allow_delete && G->delete_level(level->id_type, level->id, level->save_id)) {
                //If deleting current local level, remove it's local_id
                //This disables the "save" option
                if ((level->id_type == LEVEL_LOCAL) && (level->id == W->level.local_id)) {
                  W->level.local_id = 0;
                }
                //Reload the list of levels
                reload_level_list();
              }
              G->unlock();
            }
            ImGui::PopStyleVar();
            if (!allow_delete) ImGui::SetItemTooltip("Hold Shift to unlock");

            //TODO "Play" button

            // Open level ---
            // Principia's ACTION_OPEN signal only supports loading local levels,
            // so we have to lock the game and load the level manually...
            // this is completely fine unless the gui is multithreaded
            ImGui::SameLine();
            if (ImGui::Button("Open level")) {
              if (level->id_type == LEVEL_LOCAL) {
                //Use ACTION_OPEN if possible
                P.add_action(ACTION_OPEN, level->id);
              } else {
                //Otherwise, load the level and switch the screen manually
                G->lock();
                G->open_sandbox(level->id_type, level->id);
                G->resume_action = GAME_RESUME_OPEN;
                tms::set_screen(G);
                P.s_loading_screen->set_next_screen(G);
                G->unlock();
              }
              ImGui::CloseCurrentPopup();
            }
          }

          level = level->next;
          any_level_found = true;

          ImGui::PopID();
        }
        ImGui::EndTable();
        ImGui::EndChild();
      }
      if (!any_level_found) {
        ImGui::TextUnformatted("No levels found");
      }
      ImGui::EndPopup();
    }
  }
};

namespace UiLogin {
  enum class LoginStatus {
    None,
    LoggingIn,
    Success,
    Failure
  };

  static bool do_open = false;
  static std::string username{""};
  static std::string password{""};
  static LoginStatus login_status = LoginStatus::None;

  static void complete_login(int signal) {
    switch (signal) {
      case SIGNAL_LOGIN_SUCCESS:
        login_status = LoginStatus::Success;
        break;
      case SIGNAL_LOGIN_FAILED:
        login_status = LoginStatus::Failure;
        P.user_id = 0;
        P.username = nullptr;
        username = "";
        password = "";
        break;
    }
  }
  
  static void open() {
    do_open = true;
    username = "";
    password = "";
    login_status = LoginStatus::None;
  }

  static void layout() {
    if (do_open) {
      do_open = false;
      ImGui::OpenPopup("Log in");
    }
    ImGui_CenterNextWindow();
    //Only allow closing the window if a login attempt is not in progress
    bool *allow_closing = (login_status != LoginStatus::LoggingIn) ? REF_TRUE : NULL;
    if (ImGui::BeginPopupModal("Log in", allow_closing, MODAL_FLAGS)) {
      if (login_status == LoginStatus::Success) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
      }

      ImGui::BeginDisabled((login_status == LoginStatus::LoggingIn) || (login_status == LoginStatus::Success));

      ImGui::InputTextWithHint("###username", "Username", &username);
      ImGui::InputTextWithHint("###password", "Password", &password, ImGuiInputTextFlags_Password);
      
      if (ImGui::Button("Log in...")) {
        login_status = LoginStatus::LoggingIn;
        login_data *data = new login_data;
        strncpy(data->username, username.c_str(), 256);
        strncpy(data->password, password.c_str(), 256);
        P.add_action(ACTION_LOGIN, data);
      }

      ImGui::EndDisabled();
      
      ImGui::SameLine();

      switch (login_status) {
        case LoginStatus::LoggingIn:
          ImGui::TextUnformatted("Logging in...");
          break;
        case LoginStatus::Failure:
          ImGui::TextColored(ImVec4(1., 0., 0., 1.), "Login failed"); // Login attempt failed
          break;
        //default:
        //  ImGui::TextUnformatted(" ");
      }

      ImGui::EndPopup();
    }
  }
}

static void ui_layout() {
  UiSandboxMenu::layout();
  UiLevelManager::layout();
  UiLogin::layout();
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
    case DIALOG_OPEN:
      UiLevelManager::open();
      break;
    case DIALOG_LOGIN:
      UiLogin::open();
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
  switch (num) {
    case SIGNAL_LOGIN_SUCCESS:
    case SIGNAL_LOGIN_FAILED:
      UiLogin::complete_login(num);
      break;
  }
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
