#include "imgui.hh"
#include "main.hh"
#include "ui.hh"
#include "world.hh"

namespace UiPublished {
    static bool do_open = false;

    void open() {
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "Level published!");

        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(UI(400., 0.));
        if (ImGui::BeginPopupModal("Level published!", REF_TRUE, MODAL_FLAGS)) {

            ImGui::TextWrapped("Your level has been successfully published on the community website."
                "\n\nTo view your level on the community site, please click the button below.");

            ImGui_ButtonBarPadding();

            if (ImGui::Button("Go to level page", UI(120., 0.))) {
                COMMUNITY_URL("level/%d", W->level.community_id);
                ui::open_url(url);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Close", UI(70., 0.)))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
    }
}
