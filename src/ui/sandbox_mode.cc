#include "ui_imgui.hh"

namespace UiSandboxMode {
    static bool do_open = false;

    void open() {
        do_open = true;
    }

    void layout() {
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
