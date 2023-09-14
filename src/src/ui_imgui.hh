#ifdef __UI_IMGUI_H_GUARD
#error please do not include this file directly
#endif
#define __UI_IMGUI_H_GUARD

//NOLINTBEGIN(misc-definitions-in-headers)

#include "ui.hh"
#include "game.hh"
#include "settings.hh"
#include "soundmanager.hh"
#include "loading_screen.hh"
#include "misc.hh"
#include "speaker.hh"
#include "tms/backend/print.h"
//---
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <bits/chrono.h>
//---
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
//---
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_opengl3.h"
#include "TextEditor.h"
//---
#include "ui_imgui_impl_tms.hh"

//CONFIG

// **Experimental/Broken**
//Allow changing the UI scale without reloading the game
//Currently only affects:
// * non-tms-widget stuff like the sidebar and various uis
// * ImGui dialogs
//disabled because it may break some widget size calculations for not much benefit.
//XXX: the game needs a way to reinit G->wdg_* stuff for this to become viable
//...which is out of scope of this pr
#define UI_UISCALE_NO_RELOAD false

//Apply UI scale setting to ImGui guis
//Seems to work ¯\_(ツ)_/¯
//Usage WITHOUT a TTF font is experimental
#define UI_UISCALE_IMGUI true

//Use TTF font instead of the default one
#define UI_USE_TTF_FONT true
#define UI_TTF_FONT "data-shared" SLASH "fonts" SLASH "Roboto-Bold.ttf"
#define UI_TTF_FONT_MONO "data-shared" SLASH "fonts" SLASH "SourceCodePro-Medium.ttf"
#define UI_BASE_FONT_SIZE 12.f /* only applies to ttf fonts! */

//--------------------------------------------

//STUFF
static uint64_t __ref;
#define REF_FZERO ((float*) &(__ref = 0))
#define REF_IZERO ((int*) &(__ref = 0))
#define REF_TRUE ((bool*) &(__ref = 1))
#define REF_FALSE ((bool*) &(__ref = 0))

//constants
#define MODAL_FLAGS (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)
#define POPUP_FLAGS (ImGuiWindowFlags_NoMove)

//HELPER FUNCTIONS

//I stole this one from some random SO post...
template<typename ... Args>
std::string string_format(const std::string& format, Args ... args) {
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;
  if( size_s <= 0 ){ throw std::runtime_error("Error during formatting."); }
  auto size = static_cast<size_t>(size_s);
  std::unique_ptr<char[]> buf(new char[size]);
  std::snprintf(buf.get(), size, format.c_str(), args ...);
  return std::string(buf.get(), buf.get() + size - 1);
}

//converts 0xRRGGBBAA-encoded u32 to ImVec4
static ImVec4 rgba(uint32_t color) {
  float components[4]; //ABGR
  for (int i = 0; i < 4; i++) {
    components[i] = (float)(color & 0xFF) / 255.;
    color >>= 8;
  }
  return ImVec4(components[3], components[2], components[1], components[0]);
}

//check if string should be filtered by a search query
static bool lax_search(const std::string& where, const std::string& what) {
  return std::search(
    where.begin(), where.end(),
    what.begin(), what.end(),
    [](char lhs, char rhs) { return std::tolower(lhs) == std::tolower(rhs); }
  ) != where.end();
}

//imgui helper: Center next imgui window
static void ImGui_CenterNextWindow() {
  ImGuiIO& io = ImGui::GetIO();
  ImGui::SetNextWindowPos(
    ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
    ImGuiCond_Always, //ImGuiCond_Appearing,
    ImVec2(0.5f, 0.5f)
  );
}

//if &do_open, *do_open = false, and open popup with name
static void handle_do_open(bool *do_open, const char* name) {
  if (*do_open) {
    *do_open = false;
    ImGui::OpenPopup(name);
  }
}

/* forward */ 
static void update_imgui_ui_scale();

struct PFont {
  void* fontbuffer;
  ImFont* font;
};

static struct PFont im_load_ttf(const char *path, float size_pixels);

static struct PFont ui_font;
static struct PFont ui_font_mono;

static void reload_fonts();

/* forward */
enum class MessageType { 
  Message,
  Error
};

/* forward */ 
namespace UiSandboxMenu  { static void open(); static void layout(); }
namespace UiPlayMenu { static void open(); static void layout(); }
namespace UiLevelManager { static void open(); static void layout(); }
namespace UiLogin { static void open(); static void layout(); static void complete_login(int signal); }
namespace UiMessage { static void open(const char* msg, MessageType typ = MessageType::Message); static void layout(); }
namespace UiSettings { static void open(); static void layout(); }
namespace UiLuaEditor { static void init(); static void open(entity *e = G->selection.e); static void layout(); }
namespace UiTips {  static void open(); static void layout(); }
namespace UiSandboxMode  { static void open(); static void layout(); }
namespace UiQuickadd { /*static void init();*/ static void open(); static void layout(); }
namespace UiSynthesizer { static void init(); static void open(entity *e = G->selection.e); static void layout(); }
namespace UiObjColorPicker { static void open(entity *e = G->selection.e); static void layout(); }

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

namespace UiSandboxMenu {
  static bool do_open = false;
  static b2Vec2 sb_position = b2Vec2_zero;

  static void open() {
    do_open = true;
    sb_position = G->get_last_cursor_pos(0);
  }

  static void layout() {
    handle_do_open(&do_open, "sandbox_menu");
    if (ImGui::BeginPopup("sandbox_menu", POPUP_FLAGS)) {
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
        if (ImGui::MenuItem("Move to cursor")) {
          G->selection.e->set_position(sb_position);
        };
        ImGui::Separator();
      }


      //"Level properties"
      if (ImGui::MenuItem("Level properties")) {
        //TODO
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
        if (ImGui::MenuItem("Log in...")) {
          UiLogin::open();
        };
      }
      //"Publish online"
      if (is_sandbox) {
        ImGui::BeginDisabled(!P.user_id);
        ImGui::MenuItem("Publish online");
        ImGui::EndDisabled();
        ImGui::SetItemTooltip("Upload your level to %s", P.community_host);
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Settings...")) {
        UiSettings::open();
      }

      if (ImGui::MenuItem("Back to menu")) {
        P.add_action(ACTION_GOTO_MAINMENU, 0);
      };

      ImGui::EndMenu();
    }
  }
};

namespace UiPlayMenu {
  static bool do_open = false;

  static void open() {
    do_open = true;
  }

  static void layout() {
    handle_do_open(&do_open, "play_menu");
    if (ImGui::BeginPopup("play_menu", POPUP_FLAGS)) {
      if (ImGui::MenuItem("Controls")) {
        G->render_controls = true;
      }
      if (ImGui::MenuItem("Restart")) {
        P.add_action(ACTION_RESTART_LEVEL, 0);
      }
      if (ImGui::MenuItem("Back")) {
        P.add_action(ACTION_BACK, 0);
      }
      ImGui::EndMenu();
    }
  }
}

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
    handle_do_open(&do_open, "Level Manager");
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
        if (ImGui::IsWindowAppearing()) {
          ImGui::SetKeyboardFocusHere();
        }
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
            if (strcmp(version_str, "unknown_version") == 0) {
              version_str = "unknown";
            } else if (strcmp(version_str, "old_level") == 0) {
              version_str = "old";
            }
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
    No,
    LoggingIn,
    ResultSuccess,
    ResultFailure
  };

  static bool do_open = false;
  static std::string username{""};
  static std::string password{""};
  static LoginStatus login_status = LoginStatus::No;

  static void complete_login(int signal) {
    switch (signal) {
      case SIGNAL_LOGIN_SUCCESS:
        login_status = LoginStatus::ResultSuccess;
        break;
      case SIGNAL_LOGIN_FAILED:
        login_status = LoginStatus::ResultFailure;
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
    login_status = LoginStatus::No;
  }

  static void layout() {
    handle_do_open(&do_open, "Log in");
    ImGui_CenterNextWindow();
    //Only allow closing the window if a login attempt is not in progress
    bool *allow_closing = (login_status != LoginStatus::LoggingIn) ? REF_TRUE : NULL;
    if (ImGui::BeginPopupModal("Log in", allow_closing, MODAL_FLAGS)) {
      if (login_status == LoginStatus::ResultSuccess) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
      }

      // currently, passwords have >= 15 chars req, but the limit used to be lower (at >0 ?)...
      // ...so disable the check for now, we don't want to lock out users out of their accounts :p
      bool req_username_len = username.length() > 0;
      bool req_pass_len = password.length() > 0; //password.length() >= 15;

      ImGui::BeginDisabled(
        (login_status == LoginStatus::LoggingIn) ||
        (login_status == LoginStatus::ResultSuccess)
      );

      if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
      }
      bool activate = false;
      activate |= ImGui::InputTextWithHint("###username", "Username", &username, ImGuiInputTextFlags_EnterReturnsTrue);
      activate |= ImGui::InputTextWithHint("###password", "Password", &password, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_Password);

      ImGui::EndDisabled();

      bool can_submit =
        (login_status != LoginStatus::LoggingIn) &&
        (login_status != LoginStatus::ResultSuccess) &&
        (req_pass_len && req_username_len);
      ImGui::BeginDisabled(!can_submit);
      if (ImGui::Button("Log in...") || (can_submit && activate)) {
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
        case LoginStatus::ResultFailure:
          ImGui::TextColored(ImVec4(1., 0., 0., 1.), "Login failed"); // Login attempt failed
          break;
        default:
          break;
      }

      ImGui::EndPopup();
    }
  }
}

namespace UiMessage {
  static bool do_open = false;
  static std::string message {""};
  static MessageType msg_type = MessageType::Error;
  
  static void open(const char* msg, MessageType typ /*=MessageType::Message*/) {
    do_open = true;
    msg_type = typ;
    message.assign(msg);
  }

  static void layout() {
    handle_do_open(&do_open, "###info-popup");
    ImGui_CenterNextWindow();
    const char* typ;
    switch (msg_type) {
      case MessageType::Message:
        typ = "Message###info-popup";
        break;
      
      case MessageType::Error:
        typ = "Error###info-popup";
        break;
    }
    ImGui::SetNextWindowSize(ImVec2(400., 0.));
    if (ImGui::BeginPopupModal(typ, NULL, MODAL_FLAGS)) {
      ImGui::TextWrapped("%s", message.c_str());
      if (ImGui::Button("Close")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Copy to clipboard")) {
        ImGui::SetClipboardText(message.c_str());
      }
      ImGui::EndPopup();
    }
  }
}

namespace UiSettings {
  static bool do_open = false;

  enum class IfDone {
    Nothing,
    Exit,
    Reload,
  };

  static IfDone if_done = IfDone::Nothing;
  static bool is_saving = false;

  static std::unordered_map<const char*, setting*> local_settings;

  static const char* copy_settings[] = {
    //GRAPHICS
    "is_very_shitty",
    "texture_quality",
    "enable_shadows",
    "shadow_quality",
    "shadow_map_resx",
    "shadow_map_resy",
    "enable_ao",
    "ao_map_res",
    "postprocess",
    "enable_bloom",
    "vsync",
    "gamma_correct",
    //VOLUME
    "volume",
    "muted",
    //CONTROLS
    "touch_controls",
    "jail_cursor",
    "cam_speed_modifier",
    "smooth_cam",
    "zoom_speed",
    "smooth_zoom",
    //"smooth_menu",
    //INTERFACE
    "hide_tips",
    "display_grapher_value",
    "display_object_id",
    "display_fps",
    "uiscale",
    "first_adventure", "tutorial",
    "menu_speed",
    "smooth_menu",
    "emulate_touch",
    "rc_lock_cursor",
#ifdef DEBUG
    "debug",
#endif
    NULL
  };

  static void on_before_apply() {
    tms_infof("Preparing to reload stuff later...");
#if defined(UI_UISCALE_NO_RELOAD)
    if (UI_UISCALE_NO_RELOAD) {
      _tms.xppcm /= settings["uiscale"]->v.f;
      _tms.yppcm /= settings["uiscale"]->v.f;
    }
#endif
  }

  static void on_after_apply() {
    tms_infof("Now, reloading some stuff (as promised!)...");

    //Reload sound manager settings to apply new volume
    sm::load_settings();

    //Reload gui to apply the new ui scale
  #if defined(UI_UISCALE_NO_RELOAD)
    if (UI_UISCALE_NO_RELOAD) {
      _tms.xppcm *= settings["uiscale"]->v.f;
      _tms.yppcm *= settings["uiscale"]->v.f;
      //need something like G->recreate_widgets() to make this work
      G->refresh_gui();
      G->refresh_widgets();
      //also update imgui widget size
      #if defined(UI_UISCALE_IMGUI)
      if (UI_UISCALE_IMGUI) {
        update_imgui_ui_scale();
        reload_fonts();
      }
      #endif
    }
    #endif
  }

  static void save_thread() {
    tms_debugf("inside save_thread()");
    tms_infof("Waiting for can_set_settings...");
    while (!P.can_set_settings) {
      tms_debugf("Waiting for can_set_settings...");
      SDL_Delay(1);
    }
    tms_debugf("Ok, ready, saving...");
    on_before_apply();
    for (size_t i = 0; copy_settings[i] != NULL; i++) {
      tms_infof("writing setting %s", copy_settings[i])
      memcpy(settings[copy_settings[i]], local_settings[copy_settings[i]], sizeof(setting));
    }
    tms_assertf(settings.save(), "Unable to save settings.");
    on_after_apply();
    tms_infof("Successfully saved settings, returning...");
    P.can_reload_graphics = true;
    is_saving = false;
    tms_debugf("save_thread() completed");
  }

  static void save_settings() {
    tms_infof("Saving settings...");
    is_saving = true;
    P.can_reload_graphics = false;
    P.can_set_settings = false;
    P.add_action(ACTION_RELOAD_GRAPHICS, 0);
    std::thread thread(save_thread);
    thread.detach();
  }

  static void read_settings() {
    tms_infof("Reading settings...");
    for (auto& it: local_settings) {
      tms_debugf("free %s", it.first);
      free((void*) local_settings[it.first]);
    }
    local_settings.clear();
    for (size_t i = 0; copy_settings[i] != NULL; i++) {
      tms_debugf("reading setting %s", copy_settings[i]);
      setting *heap_setting = new setting;
      memcpy(heap_setting, settings[copy_settings[i]], sizeof(setting));
      local_settings[copy_settings[i]] = heap_setting;
    }
  }
  
  static void open() {
    do_open = true;
    is_saving = false;
    if_done = IfDone::Nothing;
    read_settings();
  }

  static void im_resolution_picker(
    std::string friendly_name,
    const char *setting_x,
    const char *setting_y,
    const char* items[],
    int32_t items_x[],
    int32_t items_y[]
  ) {
    int item_count = 0;
    while (items[item_count] != NULL) { item_count++; }
    item_count++; //to overwrite the terminator
    
    std::string cust = string_format("%dx%d", local_settings[setting_x]->v.i, local_settings[setting_y]->v.i);
    items_x[item_count - 1] = local_settings[setting_x]->v.i;
    items_y[item_count - 1] = local_settings[setting_y]->v.i;
    items[item_count - 1] = cust.c_str();

    int item_current = item_count - 1;
    for (int i = 0; i < item_count; i++) {
      if (
        (items_x[i] == local_settings[setting_x]->v.i) &&
        (items_y[i] == local_settings[setting_y]->v.i)
      ) {
        item_current = i;
        break;
      }
    }
    
    ImGui::PushID(friendly_name.c_str());
    ImGui::TextUnformatted(friendly_name.c_str());
    ImGui::Combo("###combo", &item_current, items, (std::max)(item_count - 1, item_current + 1));
    ImGui::PopID();

    local_settings[setting_x]->v.i = items_x[item_current];
    local_settings[setting_y]->v.i = items_y[item_current];
  }

  static void layout() {
    handle_do_open(&do_open, "Settings");
    ImGui_CenterNextWindow();
    //TODO unsaved changes indicator
    if (ImGui::BeginPopupModal("Settings", is_saving ? NULL : REF_TRUE, MODAL_FLAGS)) {
      if ((if_done == IfDone::Exit) && !is_saving) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
      } else if ((if_done == IfDone::Reload) && !is_saving) {
        if_done = IfDone::Nothing;
        read_settings();
      }
      if (ImGui::BeginTabBar("###settings-tabbbar")) {
        if (ImGui::BeginTabItem("Graphics")) {
          // ImGui::BeginTable("###graphics-settings", 2);
          // ImGui::TableNextColumn();

          ImGui::SeparatorText("Textures");
          ImGui::TextUnformatted("Texture quality");
          const char* quality_str[] = {"Low", "Medium", "High"};
          int *tex_q_ptr = (int*) &local_settings["texture_quality"]->v.u8;
          ImGui::SliderInt("###texture_quality", tex_q_ptr, 0, 2, quality_str[*tex_q_ptr]);
          ImGui::SetItemTooltip("Decrease texture resolution to reduce VRAM usage and improve performace");

          ImGui::SeparatorText("Shadows");
          ImGui::Checkbox("Enable shadows", (bool*) &local_settings["enable_shadows"]->v.b);
          ImGui::BeginDisabled(!local_settings["enable_shadows"]->v.b);
          ImGui::Checkbox("Smooth shadows", (bool*) &local_settings["shadow_quality"]->v.u8);
          {
            const char* resolutions[] = { "4096x4096", "4096x2048", "2048x2048", "2048x1024", "1024x1024", "1024x512", "512x512", "512x256", NULL };
            int32_t values_x[] = { 4096, 4096, 2048, 2048, 1024, 1024, 512, 512, -1 };
            int32_t values_y[] = { 4096, 2048, 2048, 1024, 1024, 512,  512, 256, -1 };
            im_resolution_picker(
              "Shadow resolution",
              "shadow_map_resx",
              "shadow_map_resy",
              resolutions,
              values_x,
              values_y
            );
          }
          ImGui::EndDisabled();

          ImGui::SeparatorText("Ambient Occlusion");
          ImGui::Checkbox("Enable AO", (bool*) &local_settings["enable_ao"]->v.b);
          ImGui::SetItemTooltip("Adds subtle shading behind objects");
          ImGui::BeginDisabled(!local_settings["enable_ao"]->v.b);
          {
            const char* resolutions[] = { "512x512", "256x256", "128x128", NULL };
            int32_t values[] = { 512, 256, 128, -1 };
            im_resolution_picker(
              "AO resolution",
              "ao_map_res",
              "ao_map_res",
              resolutions,
              values,
              values
            );
          }
          ImGui::EndDisabled();
          
          ImGui::SeparatorText("Post-processing");

          // ImGui::Checkbox("Enable post-processing", (bool*) &local_settings["postprocess"]->v.b);
          // ImGui::BeginDisabled(!local_settings["postprocess"]->v.b);
          // ImGui::Checkbox("Enable bloom", local_settings["postprocess"]->v.b ? ((bool*) &local_settings["enable_bloom"]->v.b) : REF_FALSE);
          // ImGui::SetItemTooltip("Adds a subtle glow effect to bright objects");
          // ImGui::EndDisabled();ImGui::Checkbox("Enable post-processing", (bool*) &local_settings["postprocess"]->v.b);

          //XXX: Post-processing always enables bloom, so these two settings basically do the same thing
          bool is_bloom_enabled = local_settings["enable_bloom"]->v.b && local_settings["postprocess"]->v.b;
          if (ImGui::Checkbox("Enable bloom", &is_bloom_enabled)) {
            local_settings["postprocess"]->v.b = is_bloom_enabled;
            local_settings["enable_bloom"]->v.b = is_bloom_enabled;
          }
          ImGui::SetItemTooltip("Adds a subtle glow effect to bright objects");

          ImGui::Checkbox("Gamma correction", (bool*) &local_settings["gamma_correct"]->v.b);
          ImGui::SetItemTooltip("Adjusts the brightness and contrast to ensure accurate color representation");

          ImGui::SeparatorText("Display");

          //VSync option has no effect on Android
          #ifdef TMS_BACKEND_PC
          ImGui::Checkbox("Enable V-Sync", (bool*) &local_settings["vsync"]->v.b);
          ImGui::SetItemTooltip("Helps eliminate screen tearing by limiting the refresh rate.\nMay introduce a slight input delay.");
          #endif

          ImGui::Checkbox("Safe mode", (bool*) &local_settings["is_very_shitty"]->v.b);
          ImGui::SetItemTooltip("a.k.a Very Shitty Mode\nDisables most visual effects");

          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Sound")) {
          ImGui::SeparatorText("Volume");
          ImGui::BeginDisabled(local_settings["muted"]->v.b);
          ImGui::SliderFloat(
            "###volume-slider",
            local_settings["muted"]->v.b ? REF_FZERO : ((float*) &local_settings["volume"]->v.f),
            0.f, 1.f
          );
          if (ImGui::IsItemDeactivatedAfterEdit()) {
            float volume = sm::volume;
            sm::volume = local_settings["volume"]->v.f;
            sm::play(&sm::click, sm::position.x, sm::position.y, rand(), 1., false, 0, true);
            sm::volume = volume;
          }
          ImGui::EndDisabled();
          
          ImGui::Checkbox("Mute", (bool*) &local_settings["muted"]->v.b);
          
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Controls")) {
          ImGui::EndTabItem();

          ImGui::SeparatorText("Camera");

          ImGui::TextUnformatted("Camera speed");
          ImGui::SliderFloat("###Camera-speed", (float*) &local_settings["cam_speed_modifier"]->v.f, 0.1, 15.);

          ImGui::Checkbox("Smooth camera", (bool*) &local_settings["smooth_cam"]->v.b);

          ImGui::TextUnformatted("Zoom speed");
          ImGui::SliderFloat("###Camera-zoom-speed", (float*) &local_settings["zoom_speed"]->v.f, 0.1, 3.);

          ImGui::Checkbox("Smooth zoom", (bool*) &local_settings["smooth_zoom"]->v.b);

          ImGui::SeparatorText("Menu");

          ImGui::TextUnformatted("Menu scroll speed");
          ImGui::SliderFloat("###Menu-speed", (float*) &local_settings["menu_speed"]->v.f, 1., 15.);

          ImGui::Checkbox("Smooth menu scrolling", (bool*) &local_settings["smooth_menu"]->v.b);

          ImGui::SeparatorText("Mouse");

          ImGui::Checkbox("Enable cursor jail", (bool*) &local_settings["jail_cursor"]->v.b);
          ImGui::SetItemTooltip("Lock the cursor inside the the game window while playing a level");

          ImGui::Checkbox("Enable RC cursor lock", (bool*) &local_settings["rc_lock_cursor"]->v.b);
          ImGui::SetItemTooltip("Lock the cursor while controlling RC widgets");

          ImGui::SeparatorText("Touchscreen");
          
          ImGui::Checkbox("Enable on-screen controls", (bool*) &local_settings["touch_controls"]->v.b);
          ImGui::SetItemTooltip("Enable touch-friendly on-screen controls");

          ImGui::Checkbox("Emulate touch", (bool*) &local_settings["emulate_touch"]->v.b);
          ImGui::SetItemTooltip("Enable this if you use an external device other than a mouse to control Principia, such as a Wacom pad.");
        }
        if (ImGui::BeginTabItem("Interface")) {
          ImGui::SeparatorText("UI");

          ImGui::TextUnformatted("UI Scale (requires restart)");
          std::string display_value = string_format("%.01f", local_settings["uiscale"]->v.f);
          ImGui::SliderFloat("###uiScale", &local_settings["uiscale"]->v.f, 0.2, 2., display_value.c_str());
          local_settings["uiscale"]->v.f = (int)(local_settings["uiscale"]->v.f * 10) * 0.1f;

          ImGui::SeparatorText("Help & Tips");

          ImGui::Checkbox("Do not show tips", (bool*) &local_settings["hide_tips"]->v.b);
          //Should this be in debug?
          ImGui::BeginDisabled((local_settings["tutorial"]->v.u32 & 0b1111111) == 0b1111111);
          if (ImGui::Button("Skip tutorial")) {
            local_settings["first_adventure"]->v.b = false;
            local_settings["tutorial"]->v.u32 = 0xFFFFFFFF;
          }
          ImGui::EndDisabled();

          ImGui::SeparatorText("Advanced");

          ImGui::Checkbox("Display grapher values", (bool*) &local_settings["display_grapher_value"]->v.b);

          ImGui::Checkbox("Display object IDs", (bool*) &local_settings["display_object_id"]->v.b);

          ImGui::TextUnformatted("Display FPS");
          ImGui::Combo("###displayFPS", (int*) &local_settings["display_fps"]->v.u8, "Off\0On\0Graph\0Graph (Raw)\0", 4);
          
          #ifdef DEBUG
          ImGui::SeparatorText("Debug");
          ImGui::Checkbox("debug (f1)", (bool*) &local_settings["debug"]->v.b);
          #endif

          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Gameplay")) {
          ImGui::TextUnformatted("WIP");
          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();

        //This assumes separator height == 1. which results in actual height of 0
        float button_area_height = 
          ImGui::GetStyle().ItemSpacing.y + //Separator spacing
          (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.); // Buttons
        if (ImGui::GetContentRegionAvail().y > button_area_height) {
          ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y - button_area_height);
        }
        ImGui::Separator();
        ImGui::BeginDisabled(is_saving);
        bool do_save = false;
        if (ImGui::Button("Apply")) {
          if_done = IfDone::Reload;
          save_settings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
          if_done = IfDone::Exit;
          save_settings();
        }
        ImGui::EndDisabled();
      }
      ImGui::EndPopup();
    }
  }
}

namespace UiLuaEditor {
  static bool do_open = false;
  static entity *entity_ptr;
  static bool has_unsaved_changes = false;

  static TextEditor editor;
	
  static void init() {
    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
    editor.SetPalette(TextEditor::GetDarkPalette());
    editor.SetTabSize(2);
    editor.SetShowWhitespaces(false);
  }

  static void flash_controller() {
    tms_infof("Flashing controller");

    //get ptr to len and buf, freeing the old buf if present
    uint32_t *len = &entity_ptr->properties[0].v.s.len;
    char **buf = &entity_ptr->properties[0].v.s.buf;
    if (*buf) free(*buf);

    //get code
    std::string code = editor.GetText();
    
    //get code ptr and len
    const char *src = code.c_str();
    *len = code.size();

    //trim trailing newlines
    while (*len && (src[*len - 1] == '\n')) --*len;

    //create a new buffer and copy the data
    //principia lua code is not zero terminated
    *buf = (char*) malloc(*len);
    memcpy(*buf, src, *len);

    has_unsaved_changes = false;
  }

  static void reload_code() {
    uint32_t len = entity_ptr->properties[0].v.s.len;
    char *buf = entity_ptr->properties[0].v.s.buf;
    // char *code = (char*) malloc(len + 1);
    // memcpy(code, buf, len);
    // code[len] = '\0';
    tms_infof("code len %d", len);
    std::string code = std::string(buf, len);
    editor.SetText(code);
    tms_infof("buf load success");
    has_unsaved_changes = false;
  }

  static void open(entity *entity /*= G->selection.e*/) {
    do_open = true;
    entity_ptr = entity;
    reload_code();
  }

  static void layout() {
    //TODO better ui design

    ImGuiIO& io = ImGui::GetIO();
    handle_do_open(&do_open, "Code editor");
    ImGui_CenterNextWindow();
    ImGui::SetNextWindowSize(ImVec2(800, 600));
    //has_unsaved_changes ? NULL : REF_TRUE
    if (ImGui::BeginPopupModal("Code editor", REF_TRUE, MODAL_FLAGS | (has_unsaved_changes ? ImGuiWindowFlags_UnsavedDocument : 0))) {
      ImGui::BeginDisabled(!has_unsaved_changes);
      // if (ImGui::Button("Save and exit (Alt+S)") | (io.KeyAlt && ImGui::IsKeyReleased(ImGuiKey_S))) {
      //   flash_controller();
      //   ImGui::CloseCurrentPopup();
      //   ImGui::EndPopup();
      //   return;
      // }
      //ImGui::SameLine();
      if (ImGui::Button("Save (Ctrl+S)") | (io.KeyCtrl && ImGui::IsKeyReleased(ImGuiKey_S))) {
        flash_controller();
        reload_code();
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      if (ImGui::Button("Close")) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
      }

      #if defined(UI_USE_TTF_FONT)
      if (UI_USE_TTF_FONT) ImGui::PushFont(ui_font_mono.font);
      #endif

      editor.Render("TextEditor");

      #if defined(UI_USE_TTF_FONT)
      if (UI_USE_TTF_FONT) ImGui::PopFont();
      #endif

      if (editor.IsTextChanged()) {
        has_unsaved_changes = true;
      }
      ImGui::EndPopup();
    }
  }
}

namespace UiTips {
  static bool do_open = false;

  static void open() {
    ctip = rand() % num_tips;
    do_open = true;
  }

  static void layout() {
    //TODO: optimize for uiscale!
    handle_do_open(&do_open, "Tips and tricks");
    ImGui_CenterNextWindow();
    ImGui::SetNextWindowSize(ImVec2(400, 200));
    if (ImGui::BeginPopupModal("Tips and tricks", REF_TRUE, MODAL_FLAGS)) {
      ImGuiStyle &style = ImGui::GetStyle();
      float font_size = ImGui::GetFontSize();
      ImVec2 frame_padding = style.FramePadding;
      ImVec2 content_region = ImGui::GetContentRegionMax();

      ImGui::TextWrapped("%s", tips[ctip]);

      //Align at the bottom of the window
      ImGui::SetCursorPosY(content_region.y - (font_size + (2. * frame_padding.y)));
      if (ImGui::Button("<###tips-prev")) {
        if (--ctip < 0) ctip = num_tips - 1;
      }
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(.8, .8, .8, 1), "Tip %d/%d", ctip + 1, num_tips);
      ImGui::SameLine();
      if (ImGui::Button(">###tips-next")) {
        ctip = (ctip + 1) % num_tips;
      }
      ImGui::SameLine();
      if (ImGui::Checkbox("Don't show again", (bool*) &settings["hide_tips"]->v.b)) {
        settings.save();
      }
      ImGui::SameLine();
      const char* close_text = "Close";
      const ImVec2 close_text_size = ImGui::CalcTextSize(close_text);
      ImGui::SetCursorPosX(content_region.x - (close_text_size.x + frame_padding.x * 2.));
      if (ImGui::Button(close_text)) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }
}

namespace UiSandboxMode {
  static bool do_open = false;

  static void open() {
    do_open = true;
  }

  static void layout() {
    handle_do_open(&do_open, "sandbox_mode");
    if (ImGui::BeginPopup("sandbox_mode", POPUP_FLAGS)) {
      if (ImGui::MenuItem("Multiselect")) {
        G->lock();
        G->set_mode(GAME_MODE_MULTISEL);
        G->unlock();
      }
      if (ImGui::MenuItem("Connection edit")) {
        G->lock();
        G->set_mode(GAME_MODE_CONN_EDIT);
        G->unlock();
      } 
      if (ImGui::MenuItem("Terrain paint")) {
        G->lock();
        G->set_mode(GAME_MODE_DRAW);
        G->unlock();
      }
      ImGui::EndPopup();
    }
  }
}

namespace UiQuickadd {
  static bool do_open = false;
  static std::string query{""};

  enum class ItemCategory {
    MenuObject,
    //TODO other categories (like items, animals etc) like in gtk3
  };
  struct SearchItem {
    ItemCategory cat;
    uint32_t id;
  };
  
  static bool is_haystack_inited = false;
  static std::vector<SearchItem> haystack;
  static std::vector<size_t> search_results; //referencing idx in haystack
  //last known best item, will be used in case there are no search results
  static size_t last_viable_solution;

  static std::string resolve_item_name(SearchItem item) {
    std::string name;
    switch (item.cat) {
      case ItemCategory::MenuObject: {
        const struct menu_obj &obj = menu_objects[item.id];
        name = obj.e->get_name(); // XXX: get_real_name??
        break;
      }
    }
    return name;
  }

  //TODO fuzzy search and scoring
  static std::vector<uint32_t> low_confidence;
  static void search() {
    search_results.clear();
    low_confidence.clear();
    for (int i = 0; i < haystack.size(); i++) {
      SearchItem item = haystack[i];
      std::string name = resolve_item_name(item);
      if (lax_search(name, query)) {
        search_results.push_back(i);
      } else if (lax_search(query, name)) {
        low_confidence.push_back(i);
      }
    }
    //Low confidence results are pushed after regular ones
    //Low confidence = no query in name, but name is in query.
    //So while searching for "Thick plank"...
    // Thick plank is a regular match
    // Plank is a low confidence match
    for (int i = 0; i < low_confidence.size(); i++) {
      search_results.push_back(low_confidence[i]);
    }
    tms_infof(
      "search \"%s\" %d/%d matched, (%d low confidence)", 
      query.c_str(), 
      (int) search_results.size(), 
      (int) haystack.size(),
      (int) low_confidence.size()
    );
    if (search_results.size() > 0) {
      last_viable_solution = search_results[0];
    }
  }

  // HAYSTACK IS LAZY-INITED!
  // WE CAN'T CALL THIS RIGHT AWAY 
  // AS MENU OBJECTS ARE INITED *AFTER* UI!!!
  static void init_haystack() {
    if (is_haystack_inited) return;
    is_haystack_inited = true;

    //Setup haystack
    haystack.clear();
    haystack.reserve(menu_objects.size());
    for (int i = 0; i < menu_objects.size(); i++) {
      SearchItem itm;
      itm.cat = ItemCategory::MenuObject;
      itm.id = i;
      haystack.push_back(itm);
    }
    tms_infof("init qs haystack with size %d", (int) haystack.size());
    tms_debugf("DEBUG: menu obj cnt %d", (int) menu_objects.size());

    //for opt. reasons
    search_results.reserve(haystack.size());
    low_confidence.reserve(haystack.size());
  }

  static void open() {
    do_open = true;
    query = "";
    search_results.clear();
    init_haystack();
    search();
  }

  static void activate_item(SearchItem item) {
    switch (item.cat) {
      case ItemCategory::MenuObject: {
        p_gid g_id = menu_objects[item.id].e->g_id;
        P.add_action(ACTION_CONSTRUCT_ENTITY, g_id);
        break;
      }
    }
  }

  static void layout() {
    handle_do_open(&do_open, "quickadd");
    if (ImGui::BeginPopup("quickadd", POPUP_FLAGS)) {
      if (ImGui::IsKeyReleased(ImGuiKey_Escape)) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
      }
      if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
      }

      if (ImGui::InputTextWithHint(
        "###qs-search",
        "Search for components", 
        &query, 
        ImGuiInputTextFlags_EnterReturnsTrue
      )) {
        if (search_results.size() > 0) {
          activate_item(haystack[search_results[0]]);
        } else {
          tms_infof("falling back to last best solution, can't just refuse to do anything!");
          activate_item(haystack[last_viable_solution]);
        }
        ImGui::CloseCurrentPopup();
      };
      if (ImGui::IsItemEdited()) {
        search();
      }

      if (ImGui::BeginListBox("###listbox")) {
        for (int i = 0; i < search_results.size(); i++) {
          ImGui::PushID(i);
          SearchItem item = haystack[search_results[i]];
          if (ImGui::Selectable(resolve_item_name(item).c_str())) {
            activate_item(item);
            ImGui::CloseCurrentPopup();
          }
          ImGui::PopID();
        }
        if (search_results.size() == 0) {
          SearchItem item = haystack[last_viable_solution];
          if (ImGui::Selectable(resolve_item_name(item).c_str())) {
            activate_item(item);
            ImGui::CloseCurrentPopup();
          }
        }
        ImGui::EndListBox();
      }
      ImGui::EndPopup();
    }
  }
}

namespace UiSynthesizer {
  static bool do_open = false;
  static entity *entity_ptr = nullptr;

  static std::chrono::steady_clock::time_point init_time;

  #define SYNTH_GRAPH_SIZE ImVec2(400., 100.)
  //Points must be multiple of 400
  #define SYNTH_GRAPH_POINTS_0 400
  #define SYNTH_GRAPH_POINTS_1 800
  #define SYNTH_GRAPH_POINTS_2 1600
  #define SYNTH_GRAPH_VX 0.01f
  #define SYNTH_GRAPH_VY 1.f

  // static const char *graph_type_names[] = { "Sine" };
  //static const graph_type_fns = {};
  enum {
    WAVEFORM_SINE,
    WAVEFORM_SQR,
    WAVEFORM_PULSE,
    WAVEFORM_SAWTOOTH,
    WAVEFORM_TRIANGLE,
    WAVEFORM_ETC
  };

  static void init() {
    init_time = std::chrono::steady_clock::now();
  }

  static void open(entity *entity /*= G->selection.e*/) {
    do_open = true;
    entity_ptr = entity;
  }
  
  static void layout() {
    handle_do_open(&do_open, "Synthesizer");
    ImGui_CenterNextWindow();
    if (ImGui::BeginPopupModal("Synthesizer", REF_TRUE, MODAL_FLAGS)) {
      ImDrawList *draw_list = ImGui::GetWindowDrawList();

      float *low_hz = &entity_ptr->properties[0].v.f;
      float *high_hz = &entity_ptr->properties[1].v.f;
      uint32_t *waveform = &entity_ptr->properties[2].v.i;
      float *bit_crushing = &entity_ptr->properties[3].v.f;
      float *vol_vibrato = &entity_ptr->properties[4].v.f;
      float *freq_vibrato = &entity_ptr->properties[5].v.f;
      float *vol_vibrato_ext = &entity_ptr->properties[6].v.f;
      float *freq_vibrato_ext = &entity_ptr->properties[7].v.f;
      float *pulse_width = &entity_ptr->properties[8].v.f;

      if (*waveform < WAVEFORM_ETC) {
        //Render graph
        ImVec2 p_min = ImGui::GetCursorScreenPos();
        ImVec2 p_max = ImVec2(p_min.x + SYNTH_GRAPH_SIZE.x, p_min.y + SYNTH_GRAPH_SIZE.y);
        ImVec2 size = SYNTH_GRAPH_SIZE;

        ImGui::PushID("synth-graph");
        ImGui::Dummy(SYNTH_GRAPH_SIZE);
        ImGui::PopID();

        draw_list->PushClipRect(p_min, p_max);

        //Background
        draw_list->AddRectFilled(p_min, p_max, ImColor(0, 0, 0));

        //Center h. line
        draw_list->AddLine(
          ImVec2(0., p_min.y + (size.y / 2)),
          ImVec2(p_max.x, p_min.y + (size.y / 2)),
          ImColor(128, 128, 128),
          2.
        );

        //Graph
        {
          double time_seconds;
          {
            auto now = std::chrono::steady_clock::now();
            time_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(now - init_time).count();
          }

          float freq = *low_hz;
          
          // if ((*freq_vibrato > 0.) && (*freq_vibrato_ext > 0.)) {
          //   freq *= 1. - (sin(2. * M_PI * time_seconds * *freq_vibrato) * SYNTH_GRAPH_VX) * *freq_vibrato_ext;
          // }

          static const float scr_spd_inv = 10.;
          float x_offset = (time_seconds * SYNTH_GRAPH_VX) / scr_spd_inv;
          float x = 0.;

          int points = (freq > 1000.) ? ((freq > 2000.) ? SYNTH_GRAPH_POINTS_2 : SYNTH_GRAPH_POINTS_1) : SYNTH_GRAPH_POINTS_0;
          
          float bcc = 0;

          float prev_draw_x, prev_draw_y;
          for (int i = 0; i < points; i++) {
            float y;
            float sx = (x + x_offset) * freq;
            float wave = sinf(sx * 2. * M_PI);
            switch (*waveform) {
              case WAVEFORM_SINE:
                y = wave;
                break;
              case WAVEFORM_SQR:
                //XXX: is this correct?
                y = ((int)(sx) & 1) ? 1 : -1;
                break;
              case WAVEFORM_PULSE:
                if (*pulse_width >= 1.) {
                  y = 1.;
                } else if (*pulse_width <= 0.) {
                  y = -1.;
                } else {
                  y = (((wave + 1.f) / 2.f) >= (1.f - *pulse_width)) ? 1.f : -1.f;
                }
                break;
              case WAVEFORM_SAWTOOTH:
                y = fmod(sx * 2., 2.) - 1.;
                break;
              case WAVEFORM_TRIANGLE:
                // y = fmod(sx * freq, 1.);
                // if (y > 0.5) y = .5 - y;
                // y = (y - .25) * 4.;
                y = 4.0 * fabs(fmod(sx, 1.0) - 0.5) -1.;
                break;
              case WAVEFORM_ETC:
                break;
            }
            if (*vol_vibrato > 0.) {
              //y *= .5 - .5 * sin(M_PI * 2. * *vol_vibrato * (((float) i / points) + x_offset)) * *vol_vibrato_ext;
              //Limited to 15 hz to prevent epilepsy and stuff
              y *= .5 - .5 * sin(M_PI * 2. * (std::min)(*vol_vibrato, 15.f) * (time_seconds / 2.)) * *vol_vibrato_ext;
            }

            float draw_x = p_min.x + ((x / SYNTH_GRAPH_VX) * size.x);
            float draw_y = p_min.y + (((y / SYNTH_GRAPH_VY) * -.5) + .5) * size.y;

            if ((int) *bit_crushing > 0) {
              if (bcc != 0) draw_y = prev_draw_y;
              bcc += 1600.f / points;
              if (bcc >= (*bit_crushing + 1)) bcc = 0;
            }
            
            if (i != 0) draw_list->AddLine(
              ImVec2(prev_draw_x, prev_draw_y),
              ImVec2(draw_x, draw_y),
              ImColor(255, 0, 0),
              2.f
            );

            prev_draw_x = draw_x;
            prev_draw_y = draw_y;

            x += SYNTH_GRAPH_VX / (float) points;
          }
        }
        
        //Numbers
        draw_list->AddText(
          ImVec2(p_min.x, p_max.y - ImGui::GetFontSize()),
          ImColor(128, 128, 128),
          "0.0"
        );
        static const std::string end = string_format("%.02f", SYNTH_GRAPH_VX);
        draw_list->AddText(
          ImVec2(p_max.x - ImGui::CalcTextSize(end.c_str()).x, p_max.y - ImGui::GetFontSize()),
          ImColor(128, 128, 128),
          end.c_str()
        );
        draw_list->PopClipRect();
      } else {
        // ImGui::TextColored(ImColor(255, 0, 0), "Unable to [preview this waveform");
        // ImGui::SetCursorPosY(ImGui::GetStyle().ItemSpacing.y);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::PushID("synth-graph-dummy");
        ImGui::Dummy(SYNTH_GRAPH_SIZE);
        ImGui::PopID();
        draw_list->AddText(p, ImColor(255, 0, 0), "Unable to preview this waveform");
      }

      //Controls

      ImGui::SeparatorText("Waveform");
      
      //Waveform
      if (ImGui::BeginCombo("Waveform", (*waveform < NUM_WAVEFORMS) ? speaker_options[*waveform]: "???")) {
        for (int i = 0; i < NUM_WAVEFORMS; i++) {
          ImGui::PushID(i);
          if (ImGui::Selectable(speaker_options[i])) {
            *waveform = i;
          }
          ImGui::PopID();
        }
        ImGui::EndCombo();
      }

      //Pulse -> Pulse width
      if (*waveform == WAVEFORM_PULSE) {
        ImGui::SliderFloat("Pulse width", pulse_width, 0., 1.);
      }

      //Frequency
      ImGui::SeparatorText("Frequency");

      //Base freq
      int hz_int = (int) roundf(*low_hz);
      if (ImGui::SliderInt("Base requency", &hz_int, 100, 3520)) {
        *low_hz = (float) hz_int;
        *high_hz = (std::max)(*high_hz, *low_hz);
      }

      //Max freq
      hz_int = (int) roundf(*high_hz);
      if (ImGui::SliderInt("Max frequency", &hz_int, 100, 3520)) {
        *high_hz = (float) hz_int;
        *low_hz = (std::min)(*high_hz, *low_hz);
      }

      //Vibrato
      ImGui::SeparatorText("Volume vibrato");

      ImGui::SliderFloat("Frequency###vol-freq", vol_vibrato, 0., 32.);
      ImGui::SliderFloat("Extent###vol-ext", vol_vibrato_ext, 0., 1.);

      //Freq vibrato
      ImGui::SeparatorText("Frequency vibrato");

      ImGui::SliderFloat("Frequency###freq-freq", freq_vibrato, 0., 32.);
      ImGui::SliderFloat("Extent###freq-ext", freq_vibrato_ext, 0., 1.);

      //Bitcrush
      ImGui::SeparatorText("Bitcrushing");

      int bc_int = (int) *bit_crushing;
      if (ImGui::SliderInt("Bitcrushing", &bc_int, 0, 64)) {
        *bit_crushing = (float) bc_int;
      }

      ImGui::TextDisabled("Visualization may be inaccurate");

      ImGui::EndPopup();
    }
  }
}

namespace UiObjColorPicker {
  static bool do_open = false;
  static entity *entity_ptr;
  static float ref_color[4];

  static void open(entity *entity) {
    do_open = true;
    entity_ptr = entity;
    tvec4 color = entity->get_color();
    ref_color[0] = color.r;
    ref_color[1] = color.g;
    ref_color[2] = color.b;
    ref_color[3] = color.a;
    tms_infof("opening color picker for %s", entity->get_name());
  }

  static void layout() {
    handle_do_open(&do_open, "Color###beam-color");
    ImGui_CenterNextWindow();
    if (ImGui::BeginPopupModal("Color###beam-color", REF_TRUE, MODAL_FLAGS)) {
      tvec4 color = entity_ptr->get_color();
      float color_arr[4] = {color.r, color.g, color.b, color.a};
      ImGuiColorEditFlags flags = 0
        | ImGuiColorEditFlags_AlphaPreview
        | ImGuiColorEditFlags_AlphaBar
        | ImGuiColorEditFlags_NoDragDrop
        | ImGuiColorEditFlags_PickerHueWheel //TODO: decide!
      ;
      if (ImGui::ColorPicker4("Color", (float*) &color_arr, flags, (const float*) &ref_color)) {
        color.r = color_arr[0];
        color.g = color_arr[1];
        color.b = color_arr[2];
        color.a = color_arr[3];
        entity_ptr->set_color(color);
      }
      ImGui::EndPopup();
    }
  }
}

static void ui_init() {
  UiLuaEditor::init();
  //UiQuickadd::init();
  UiSynthesizer::init();
}

static void ui_layout() {
#ifdef DEBUG
  ui_demo_layout();
#endif
  UiSandboxMenu::layout();
  UiPlayMenu::layout();
  UiLevelManager::layout();
  UiLogin::layout();
  UiMessage::layout();
  UiSettings::layout();
  UiLuaEditor::layout();
  UiTips::layout();
  UiSandboxMode::layout();
  UiQuickadd::layout();
  UiSynthesizer::layout();
  UiObjColorPicker::layout();
}

//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

#if defined(TMS_BACKEND_PC) && !defined(NO_UI)
int prompt_is_open = 0;
#endif 

static void update_imgui_ui_scale() {
#ifdef UI_UISCALE_IMGUI
  if (UI_UISCALE_IMGUI) {
    float scale_factor = settings["uiscale"]->v.f;
    ImGui::GetStyle().ScaleAllSizes(scale_factor);

    #ifdef UI_USE_TTF_FONT
    if(!UI_USE_TTF_FONT) {
    #endif
      ImGui::GetIO().FontGlobalScale = roundf(9. * scale_factor) / 9.;
    #ifdef UI_USE_TTF_FONT
    }
    #endif
  }
#endif
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

static struct PFont im_load_ttf(const char *path, float size_pixels) {
  tms_infof("loading ui font from %s...", path);

  FILE_IN_ASSET(true);
  FILE *file = (FILE*) _fopen(path, "rb");
  tms_assertf(file, "font file not found");

  _fseek(file, 0, SEEK_END);
  size_t size = _ftell(file);
  tms_infof("buf size %d", (int) size);
  void *fontbuffer = malloc(size + 1);

  _fseek(file, 0, SEEK_SET);
  _fread(fontbuffer, 1, size, file);
  _fclose(file);
  
  ImFontConfig font_cfg;
  font_cfg.FontDataOwnedByAtlas = false;
  if (size_pixels <= 16.) {
    font_cfg.OversampleH = 3;
  }

  ImFont *font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontbuffer, size, size_pixels, &font_cfg);

  struct PFont pfont;
  pfont.fontbuffer = fontbuffer;
  pfont.font = font;

  return pfont;
}

void reload_fonts() {
  #if defined(UI_USE_TTF_FONT)
  if (UI_USE_TTF_FONT) {
    //TODO free existing fonts

    float size_pixels = UI_BASE_FONT_SIZE;
    #ifdef UI_UISCALE_IMGUI
    if (UI_UISCALE_IMGUI) size_pixels *= settings["uiscale"]->v.f;
    #endif
    size_pixels = roundf(size_pixels);

    tms_infof("font size %fpx", size_pixels);

    ui_font = im_load_ttf(UI_TTF_FONT, size_pixels);
    ui_font_mono = im_load_ttf(UI_TTF_FONT_MONO, size_pixels + 2);
  }
  #endif
}

void ui::init() {
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
  principia_style();

  //update scale
  update_imgui_ui_scale();

  //load font
  reload_fonts();

  //ensure gl ctx exists
  tms_assertf(_tms._window != NULL, "window does not exist yet");
  tms_assertf(SDL_GL_GetCurrentContext() != NULL, "no gl ctx");

  //init
  tms_assertf(ImGui_ImplOpenGL3_Init(), "gl impl init failed");
  tms_assertf(ImGui_ImplTMS_Init() == T_OK, "tms impl init failed");
  
  //call ui_init
  ui_init();
}

void ui::render() {
  if (settings["render_gui"]->is_false()) return;

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
  
  #if defined(UI_USE_TTF_FONT)
  if (UI_USE_TTF_FONT) ImGui::PushFont(ui_font.font);
  #endif

  //layout
  ui_layout();

  #if defined(UI_USE_TTF_FONT)
  if (UI_USE_TTF_FONT) ImGui::PopFont();
  #endif

  //render
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
    default:
      tms_errorf("dialog %d not implemented yet", num);
  }
}

void ui::open_sandbox_tips() {
  UiTips::open();
}

void ui::open_url(const char *url) {
  tms_infof("open url: %s", url);
  #if SDL_VERSION_ATLEAST(2,0,14)
    SDL_OpenURL(url);
  #elif defined(TMS_BACKEND_LINUX)
    #warning "Please upgrade to SDL 2.0.14"
    if (fork() == 0) {
      execlp("xdg-open", "xdg-open", url, NULL);
      _exit(0);
    }
  #else
    #error "SDL2 2.0.14+ is required"
  #endif
}

void ui::open_help_dialog(const char* title, const char* description, bool enable_markup) {
  //TODO
  tms_errorf("ui::open_help_dialog not implemented yet");
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
  //TODO
  P.add_action(action1.action_id, 0);
  tms_errorf("ui::confirm not implemented yet");
}

void ui::alert(const char* text, uint8_t type) {
  //TODO handle type, e.g. error, warning.
  //these are currently unused by principia, but should be handled regardless
  UiMessage::open(text, MessageType::Message);
}

//NOLINTEND(misc-definitions-in-headers)
//ї
