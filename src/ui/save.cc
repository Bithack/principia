#include "ui_imgui.hh"

namespace UiSave {
static bool do_open = false;
    static std::string level_name{""};

    void open() {
        do_open = true;
        size_t sz = (std::min)((int) W->level.name_len, LEVEL_NAME_LEN_HARD_LIMIT);
        level_name = std::string((const char*) &W->level.name, sz);
        if (level_name == std::string{LEVEL_NAME_PLACEHOLDER}) {
            level_name = "";
        }
    }

    void layout() {
        handle_do_open(&do_open, "###sas");
        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Save as...###sas", REF_TRUE, MODAL_FLAGS)) {
            ImGuiStyle& style = ImGui::GetStyle();

            ImGui::TextUnformatted("Level name:");

            //Level name input field
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            bool activate = ImGui::InputTextWithHint(
                "###levelname",
                LEVEL_NAME_PLACEHOLDER,
                &level_name,
                ImGuiInputTextFlags_EnterReturnsTrue
            );

            //Validation
            bool invalid = level_name.length() > LEVEL_NAME_LEN_SOFT_LIMIT;

            //Char counter, X/250
            float cpy = ImGui::GetCursorPosY();
            ImGui::SetCursorPosY(cpy + style.FramePadding.y);
            ImGui::TextColored(
                invalid ? ImColor(255, 0, 0) : ImColor(1.f, 1.f, 1.f, style.DisabledAlpha),
                "%zu/%d", level_name.length(), LEVEL_NAME_LEN_SOFT_LIMIT
            );
            ImGui::SetCursorPosY(cpy);

            //Save button, right-aligned
            const char *save_str = "Save";
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - (ImGui::CalcTextSize(save_str).x + style.FramePadding.x * 2.));
            ImGui::BeginDisabled(invalid);
            if (ImGui::Button(save_str)  || (activate && !invalid)) {
                size_t sz = (std::min)((int) level_name.length(), LEVEL_NAME_LEN_SOFT_LIMIT);
                memcpy((char*) &W->level.name, level_name.c_str(), sz);
                W->level.name_len = sz;
                P.add_action(ACTION_SAVE_COPY, 0);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();

            ImGui::EndPopup();
        }
    }
}
