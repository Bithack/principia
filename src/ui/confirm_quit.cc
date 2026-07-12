#include "imgui.hh"
#include <tms/cpp.hh>

namespace UiConfirmQuit {
    static bool do_open = false;

    void open() {
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "Confirm Quit");
        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(UI(280., .0));

        if (ImGui::BeginPopupModal("Confirm Quit", NULL, MODAL_FLAGS)) {
            ImGui::TextWrapped("Are you sure you want to quit?\n\nAny unsaved changes will be lost!");

            ImGui::Dummy(UI(0.0f, 25.0f));

            if (ImGui::Button("Yes", UI(70., 0.))) {
                _tms.state = TMS_STATE_QUITTING;
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
