#include "ui.hh"
#include "game.hh"
#include "settings.hh"
#include "soundmanager.hh"
#include "loading_screen.hh"
//----------------------------------
#include <string>
#include <thread>
#include <unordered_map>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
#include "imgui.h"
//#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "imgui_impl_opengl3.h"
#include "ui_imgui_impl_tms.hh"
#include "TextEditor.h"

//CONFIG

//Allow changing the UI scale without reloading the game
//XXX: the game needs a way to reinit G->wdg_* stuff for this to become viable
//...which is out of scope of this backend
//Currently only affects non-tms-widget stuff like the sidebar and various uis
//but may breeak some widget size calculations...
#define EXPERIMENTAL_UISCALE_NO_RELOAD false

//STUFF
static uint64_t __ref;
#define REF_FZERO ((float*) &(__ref = 0))
#define REF_IZERO ((int*) &(__ref = 0))
#define REF_TRUE ((bool*) &(__ref = 1))
#define REF_FALSE ((bool*) &(__ref = 0))

#define MODAL_FLAGS (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)

//HELPER FUNCTIONS

//some random SO post
template<typename ... Args>
std::string string_format(const std::string& format, Args ... args) {
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;
  if( size_s <= 0 ){ throw std::runtime_error("Error during formatting."); }
  auto size = static_cast<size_t>(size_s);
  std::unique_ptr<char[]> buf(new char[size]);
  std::snprintf(buf.get(), size, format.c_str(), args ...);
  return std::string(buf.get(), buf.get() + size - 1);
}

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

static void handle_do_open(bool *do_open, const char* name) {
  if (*do_open) {
    *do_open = false;
    ImGui::OpenPopup(name);
  }
}

namespace UiSandboxMenu  { static void open(); static void layout(); }
namespace UiPlayMenu { static void open(); static void layout(); }
namespace UiLevelManager { static void open(); static void layout(); }
namespace UiLogin { static void open(); static void layout(); static void complete_login(int signal); }
namespace UiMessage {
  enum class MessageType { Message, Error };
  static void open(const char* msg, MessageType typ = MessageType::Message);
  static void layout();
}
namespace UiSettings { static void open(); static void layout(); }
namespace UiLuaEditor { static void init(); static void open(entity *e = G->selection.e); static void layout(); }
namespace UiTips {  static void open(); static void layout(); }

namespace UiSandboxMenu {
  static bool do_open = false;
  static b2Vec2 sb_position = b2Vec2_zero;

  static void open() {
    do_open = true;
    sb_position = G->get_last_cursor_pos(0);
  }

  static void layout() {
    handle_do_open(&do_open, "sandbox_menu");
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
    if (ImGui::BeginPopup("play_menu", ImGuiWindowFlags_NoMove)) {
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
#if defined(EXPERIMENTAL_UISCALE_NO_RELOAD)
    if (EXPERIMENTAL_UISCALE_NO_RELOAD) {
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
#if defined(EXPERIMENTAL_UISCALE_NO_RELOAD)
    if (EXPERIMENTAL_UISCALE_NO_RELOAD) {
      _tms.xppcm *= settings["uiscale"]->v.f;
      _tms.yppcm *= settings["uiscale"]->v.f;
      //need something like G->recreate_widgets() to make this work
      G->refresh_gui();
      G->refresh_widgets();
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
          ImGui::SliderInt("###texture_quality", (int*) &local_settings["texture_quality"]->v.u8, 0, 2);

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
        ImGui::EndTabBar();

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
      editor.Render("TextEditor");
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
    handle_do_open(&do_open, "Tips and tricks");
    ImGui_CenterNextWindow();
    ImGui::SetNextWindowSize(ImVec2(400, 200));
    if (ImGui::BeginPopupModal("Tips and tricks", REF_TRUE, MODAL_FLAGS)) {
      ImGui::TextWrapped("%s", tips[ctip]);
      //Align at the bottom of the window
      //TODO do not hardcode y
      ImGui::SetCursorPosY(ImGui::GetWindowSize().y - 27);
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
      ImGui::SetCursorPosX(ImGui::GetWindowSize().x - ImGui::CalcTextSize(close_text).x - 15);
      if (ImGui::Button(close_text)) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }
}

static void ui_init() {
  UiLuaEditor::init();
}

static void ui_layout() {
  UiSandboxMenu::layout();
  UiPlayMenu::layout();
  UiLevelManager::layout();
  UiLogin::layout();
  UiMessage::layout();
  UiSettings::layout();
  UiLuaEditor::layout();
  UiTips::layout();
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
  io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NavEnableKeyboard;
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
  UiMessage::open(error_msg, UiMessage::MessageType::Error);
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
  UiMessage::open(text, UiMessage::MessageType::Message);
}

//ї
