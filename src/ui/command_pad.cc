#include "command.hh"
#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiCommandPad {
    static bool do_open = false;
    static int selected_command = 0;
    static const char *cpad_type_names[] = {
        "Stop",
        "Start/Stop toggle",
        "Left",
        "Right",
        "Left/Right toggle",
        "Jump",
        "Aim",
        "Attack",
        "Layer up",
        "Layer down",
        "Increase speed",
        "Decrease speed",
        "Set speed",
        "Full health"
    };

    void apply_properties() {
        entity* e = G->selection.e;
        if (e && e->g_id == O_COMMAND_PAD) {
            ((command *)e)->set_command(selected_command);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
        ImGui::CloseCurrentPopup();
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_COMMAND_PAD) {
            selected_command = e->properties[0].v.i;
        }
    }

    void layout() {
        handle_do_open(&do_open, "Command Pad");
        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Command Pad", REF_TRUE, MODAL_FLAGS)) {
            if (ImGui::BeginCombo("##command", cpad_type_names[selected_command])) {

                for (int i = 0; i < sizeof(cpad_type_names) / sizeof(cpad_type_names[0]); ++i) {
                    bool is_selected = (selected_command == i);
                    if (ImGui::Selectable(cpad_type_names[i], is_selected)) {
                        selected_command = i;
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
