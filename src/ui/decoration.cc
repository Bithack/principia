#include "decorations.hh"
#include "ui_imgui.hh"

namespace UiDecoration {
	static bool do_open = false;
    static int selected_index = 0;

    void open() {
        selected_index = 0;
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "Decoration type");
        if (ImGui::BeginPopupModal("Decoration type", nullptr, MODAL_FLAGS)) {
            if (ImGui::BeginCombo(" ", decorations[selected_index].name)) {
                for (int i = 0; i < NUM_DECORATIONS; ++i) {
                    bool is_selected = (selected_index == i);
                    if (ImGui::Selectable(decorations[i].name, is_selected)) {
                        selected_index = i;
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Spacing();
            ImGui::SeparatorText("");
            if (ImGui::Button("Apply")) {
                entity* e = G->selection.e;
                if (e && e->g_id == O_DECORATION) {
                    ((decoration*)e)->set_decoration_type((uint32_t)selected_index);
                    ((decoration*)e)->do_recreate_shape = true;

                    P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
                    P.add_action(ACTION_RESELECT, 0);
                }
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}
