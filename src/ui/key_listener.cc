#include "key_listener.hh"
#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiKeyListener {
    static bool do_open = false;
    static int selected_item = 0;

    void apply_properties() {
        entity *e = G->selection.e;

        if (e && e->g_id == O_KEY_LISTENER) {
            e->properties[0].v.i = selected_item;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_KEY_LISTENER)
            selected_item = e->properties[0].v.i;
    }

    void layout() {
        handle_do_open(&do_open, "Key listener");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Key listener", REF_TRUE, MODAL_FLAGS)) {
            if (ImGui::BeginCombo("##item", key_names[selected_item])) {
                for (int i = 0; i < TMS_KEY__NUM; ++i) {
                    const char *name = key_names[i];

                    if (!name)
                        continue;

                    bool is_selected = (selected_item == i);
                    if (ImGui::Selectable(key_names[i], is_selected))
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
