#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiVendor {
    static bool do_open = false;
    static int items_required = 0;

    void apply_properties() {
        entity *e = G->selection.e;
        if (e && e->g_id == O_VENDOR) {
            e->properties[2].v.i = items_required;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_VENDOR)
            items_required = e->properties[2].v.i;
    }

    void layout() {
        handle_do_open(&do_open, "Vendor");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Vendor", REF_TRUE, MODAL_FLAGS)) {
            ImGui::Text("Num. items required");

            ImGui::DragInt("##numitems", &items_required, .1, 1, 65535, "%d",
                ImGuiSliderFlags_AlwaysClamp);

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
