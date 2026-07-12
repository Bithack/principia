#include "imgui.hh"
#include "main.hh"
#include "world.hh"

namespace UiPublish {
    static bool do_open = false;

    static std::string name = "";
    static std::string description = "";
    static bool locked = false;

    void publish() {
        W->level.name_len = name.length();
        memcpy(W->level.name, name.c_str(), W->level.name_len);

        if (description.length() > 0) {
            W->level.descr_len = description.length();
            W->level.descr = (char *)realloc(W->level.descr, W->level.descr_len + 1);

            memcpy(W->level.descr, description.c_str(), W->level.descr_len);
            W->level.descr[W->level.descr_len] = '\0';
        } else
            W->level.descr_len = 0;

        W->level.visibility = locked ? LEVEL_LOCKED : LEVEL_VISIBLE;

        tms_debugf("Setting level name to:  %s", name.c_str());
        tms_debugf("Setting level descr to: %s", description.c_str());

        P.add_action(ACTION_PUBLISH, 0);

        ImGui::CloseCurrentPopup();
    }

    void open() {
        do_open = true;

        name = std::string(W->level.name, W->level.name_len);
        description = std::string(W->level.descr, W->level.descr_len);
        locked = W->level.visibility == LEVEL_LOCKED;
    }

    void layout() {
        handle_do_open(&do_open, "Publish");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Publish", REF_TRUE, MODAL_FLAGS)) {

            ImGui::Text("Name:");
            // make the input label 100% width
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##name", &name);

            ImGui::Text("Description:");
            ImGui::InputTextMultiline("##description", &description, UI(350, 170), ImGuiInputTextFlags_WordWrap);

            ImGui::Checkbox("Locked", &locked);
            ImGui::SetItemTooltip("Disallow other players from seeing this level on the community site.");

            ImGui_ButtonBarPadding();

            bool publish_disabled = name.length() == 0 || name.length() > 255;
            if (publish_disabled)
                ImGui::BeginDisabled();

            if (ImGui::Button("Publish", UI(70., 0.)))
                publish();

            if (publish_disabled)
                ImGui::EndDisabled();

            ImGui::SameLine();

            if (ImGui_CancelButton())
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
    }
}
