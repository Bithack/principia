#include "const.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiResource {
    static bool do_open = false;
    static int selected_index = 0;

    void apply_properties() {
        entity *e = G->selection.e;
        if (e && e->g_id == O_RESOURCE) {
            ((resource *)e)->set_resource_type((uint8_t)selected_index);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_RESOURCE)
            selected_index = e->properties[0].v.i;
    }

    void layout() {
        handle_do_open(&do_open, "Resource");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Resource", REF_TRUE, MODAL_FLAGS)) {
            if (ImGui::BeginCombo("##resource", resource_data[selected_index].name)) {
                for (int i = 0; i < NUM_RESOURCES; ++i) {
                    bool is_selected = (selected_index == i);
                    if (ImGui::Selectable(resource_data[i].name, is_selected))
                        selected_index = i;

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
