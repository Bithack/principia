#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiPkgLvlSelector {
    static bool do_open = false;
    static int pkg_level = 1;

    void apply_properties() {
        entity *e = G->selection.e;
        if (e && (e->g_id == O_PKG_WARP || e->g_id == O_PKG_STATUS)) {
            e->properties[0].v.i8 = (uint8_t)pkg_level;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
        ImGui::CloseCurrentPopup();
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && (e->g_id == O_PKG_WARP || e->g_id == O_PKG_STATUS))
            pkg_level = e->properties[0].v.i8;
    }

    void layout() {
        handle_do_open(&do_open, "Set level ID");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Set level ID", REF_TRUE, MODAL_FLAGS)) {
            ImGui::Text("Level ID:");
            ImGui::DragInt("##lvlid", &pkg_level, .1, 0, 255, "%d",
                ImGuiSliderFlags_AlwaysClamp);

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
