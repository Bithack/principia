#include "ui.hh"
#include "ui_imgui.hh"

namespace UiConfirm {
	static bool do_open = false;
    const char *confirm_text;

    const char *confirm_button1;
    const char *confirm_button2;
    const char *confirm_button3;

    int confirm_action1;
    int confirm_action2;
    int confirm_action3;

    void *confirm_action1_data = 0;
    void *confirm_action2_data = 0;
    void *confirm_action3_data = 0;

    struct confirm_data confirm_data(CONFIRM_TYPE_DEFAULT);

    void open(const char *text,
              const char *button1, principia_action action1,
              const char *button2, principia_action action2,
              const char *button3, principia_action action3,
              struct confirm_data _confirm_data) {

        confirm_text = strdup(text);

        confirm_button1 = strdup(button1);
        confirm_button2 = strdup(button2);
        if (button3) {
            confirm_button3 = strdup(button3);
        } else {
            confirm_button3 = 0;
        }

        confirm_action1 = action1.action_id;
        confirm_action2 = action2.action_id;
        confirm_action3 = action3.action_id;

        confirm_action1_data = action1.action_data;
        confirm_action2_data = action2.action_data;
        confirm_action3_data = action3.action_data;

        confirm_data = _confirm_data;

        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "Confirm");
        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(ImVec2(400, .0));

        if (ImGui::BeginPopupModal("Confirm", NULL, MODAL_FLAGS)) {
            ImGui::TextWrapped("%s", confirm_text);

            ImGui::Dummy(ImVec2(0.0f, 40.0f));

            if (ImGui::Button(confirm_button1)) {
                P.add_action(confirm_action1, confirm_action1_data);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(confirm_button2)) {
                P.add_action(confirm_action2, confirm_action2_data);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (confirm_button3 != 0) {
                if (ImGui::Button(confirm_button3)) {
                    P.add_action(confirm_action3, confirm_action3_data);
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
    }
}
