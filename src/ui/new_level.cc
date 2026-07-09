#include "imgui.hh"
#include "main.hh"
#include "pkgman.hh"

namespace UiNewLevel {
static bool do_open = false;

    void open() {
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "###new-level");
        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("New level###new-level", REF_TRUE, MODAL_FLAGS)) {

            if (ImGui::Button("Custom")) {
                P.add_action(ACTION_NEW_LEVEL, LCAT_CUSTOM);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Adventure")) {
                P.add_action(ACTION_NEW_LEVEL, LCAT_ADVENTURE);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Procedural Adventure")) {
                P.add_action(ACTION_NEW_GENERATED_LEVEL, LCAT_ADVENTURE);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Puzzle")) {
                P.add_action(ACTION_NEW_LEVEL, LCAT_PUZZLE);
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}
