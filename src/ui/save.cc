#include "imgui.hh"
#include "main.hh"
#include "ui.hh"
#include "world.hh"

namespace UiSave {
    static bool do_open = false;
    static bool copy = false;
    static char level_name[256];

    void apply_properties() {
        size_t name_len = strlen(level_name);
        if (name_len == 0) {
            ui::message("Your level must have a name.");
            return;
        }
        W->level.name_len = name_len;
        memcpy(W->level.name, level_name, name_len);

        if (copy)
            P.add_action(ACTION_SAVE_COPY, 0);
        else
            P.add_action(ACTION_SAVE, 0);

        ImGui::CloseCurrentPopup();
    }

    void open(bool copy_flag) {
        do_open = true;
        copy = copy_flag;

        size_t name_len = W->level.name_len;
        memcpy(level_name, W->level.name, name_len);
        level_name[name_len] = '\0';
        if (strcmp(level_name, LEVEL_NAME_PLACEHOLDER) == 0)
            level_name[0] = '\0';
    }

    void layout() {
        handle_do_open(&do_open, "###save");
        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal(copy ? "Save as...###save" : "Save###save", REF_TRUE, MODAL_FLAGS)) {
            ImGui_CloseOnEsc();

            ImGui::TextUnformatted("Level name:");

            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();

            bool activate = ImGui::InputTextWithHint(
                "###levelname",
                LEVEL_NAME_PLACEHOLDER,
                level_name,
                IM_ARRAYSIZE(level_name),
                ImGuiInputTextFlags_EnterReturnsTrue
            );

            bool disabled = level_name[0] == '\0';

            ImGui_ButtonBarPadding();

            if (disabled)
                ImGui::BeginDisabled();

            if (ImGui_SaveButton() || (!disabled && activate))
                apply_properties();

            if (disabled)
                ImGui::EndDisabled();

            ImGui::SameLine();

            if (ImGui_CancelButton())
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
    }
}
