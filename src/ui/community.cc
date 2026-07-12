#include "imgui.hh"
#include "main.hh"

namespace UiCommunity {
    static bool do_open = false;

    void open() {
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "Back to main menu?");
        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(UI(280, .0));

        if (ImGui::BeginPopupModal("Back to main menu?", NULL, MODAL_FLAGS)) {
            ImGui::TextWrapped("Do you want to return to the main menu?");

            ImGui::Dummy(UI(0.0f, 25.0f));

            if (ImGui::Button("Yes", UI(70., 0.))) {
                P.add_action(ACTION_GOTO_MAINMENU, 0);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("No", UI(70., 0.))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}
