#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiPromptSettings {
    static bool do_open = false;

    static char prompt_text[701];
    static char response_text[3][51];

    void apply_properties() {
        entity *e = G->selection.e;
        if (e && e->g_id == O_PROMPT) {
            for (int i = 0; i < 3; ++i)
                e->set_property(i, response_text[i]);

            e->set_property(3, prompt_text);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_PROMPT) {
            for (int i = 0; i < 3; ++i) {
                strncpy(response_text[i], e->properties[i].v.s.buf, sizeof(response_text[i]) - 1);
                response_text[i][sizeof(response_text[i]) - 1] = '\0';
            }

            strncpy(prompt_text, e->properties[3].v.s.buf, sizeof(prompt_text) - 1);
            prompt_text[sizeof(prompt_text) - 1] = '\0';
        }
    }

    void layout() {
        handle_do_open(&do_open, "Prompt##prompt_settings");

        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(UI(300, .0));
        if (ImGui::BeginPopupModal("Prompt##prompt_settings", REF_TRUE, MODAL_FLAGS)) {

            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::InputTextMultiline("##message", prompt_text, sizeof(prompt_text), ImVec2(0, ImGui::GetTextLineHeight() * 8),
                ImGuiInputTextFlags_WordWrap);

            ImGui::Separator();

            ImGui::TextWrapped("Leave a button text empty if you don't want to use it.");

            ImGui::InputTextWithHint("##response1", "Button 1", response_text[0], sizeof(response_text[0]));
            ImGui::InputTextWithHint("##response2", "Button 2", response_text[1], sizeof(response_text[1]));
            ImGui::InputTextWithHint("##response3", "Button 3", response_text[2], sizeof(response_text[2]));

            ImGui_ButtonBarPadding();

            bool disabled = prompt_text[0] == '\0'
                || (prompt_text[0] == '\0')
                || (response_text[0][0] == '\0' && response_text[1][0] == '\0' && response_text[2][0] == '\0');

            if (disabled)
                ImGui::BeginDisabled();

            if (ImGui_SaveButton() || (!disabled && ImGui_SaveKeybind()))
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
