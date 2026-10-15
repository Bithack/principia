#include "game.hh"
#include "imgui.hh"
#include "main.hh"
#include "ui.hh"

namespace UiPlayMenu {
    static bool do_open = false;

    void open() {
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "play_menu");
        if (ImGui::BeginPopup("play_menu", POPUP_FLAGS)) {
            if (W->level.flag_active(LVL_ALLOW_QUICKSAVING))
                if (ImGui::MenuItem("Save state"))
                    P.add_action(ACTION_SAVE_STATE, 0);

            if (ImGui::MenuItem("Load state")) {
                ImGui::CloseCurrentPopup();
                ui::open_dialog(DIALOG_OPEN_STATE);
            }

            if (W->is_adventure())
                if (ImGui::MenuItem("Self-destruct"))
                    P.add_action(ACTION_SELF_DESTRUCT, 0);

            if (!_tms.touch_controls)
                if (ImGui::MenuItem("Controls"))
                    G->render_controls = true;

            int source = W->level_id_type;
            if (source == LEVEL_DB)
                if (ImGui::MenuItem("Restart"))
                    P.add_action(ACTION_RESTART_LEVEL, 0);

#ifdef SDL_PLATFORM_ANDROID
            if (source == LEVEL_DB) {
                if (ImGui::MenuItem("Back to community")) {
                    ImGui::CloseCurrentPopup();
                    COMMUNITY_URL("level/%d", W->level.community_id);
                    ui::open_url(url);
                }
            } else
#endif
            {
                if (ImGui::MenuItem("Back"))
                    P.add_action(ACTION_BACK, 0);
            }

            ImGui::EndPopup();
        }
    }
}
