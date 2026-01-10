#include "ui_imgui.hh"

namespace UiPlayMenu {
    static bool do_open = false;

    void open() {
        do_open = true;
    }

    void layout() {
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
