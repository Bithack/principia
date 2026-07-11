#include "item.hh"
#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiItem {
    static bool do_open = false;
    static int selected_item = 0;

    void apply_properties() {
        entity *e = G->selection.e;

        if (e && e->g_id == O_ITEM) {
            ((item *)e)->set_item_type((uint8_t)selected_item);
            ((item *)e)->do_recreate_shape = true;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_ITEM)
            selected_item = e->properties[0].v.i;
    }

    void layout() {
        handle_do_open(&do_open, "Item");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Item", REF_TRUE, MODAL_FLAGS)) {
            if (ImGui::BeginCombo("##item", item::get_ui_name(selected_item))) {
                for (int i = 0; i < NUM_ITEMS; ++i) {
                    bool is_selected = (selected_item == i);
                    if (ImGui::Selectable(item::get_ui_name(i), is_selected))
                        selected_item = i;

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
