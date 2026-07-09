#include "polygon.hh"
#include "imgui.hh"
#include "ui_imgui.hh"

namespace UiPolygon {
	static bool do_open = false;
    static int sublayer_depth = 1;
    static bool front_align = false;

    void apply_properties() {
        entity *e = G->selection.e;
        if (e && e->g_id == O_PLASTIC_POLYGON) {
            ((polygon*)e)->do_recreate_shape = true;

            e->properties[1].v.i8 = static_cast<uint8_t>(front_align ? 1 : 0);
            e->properties[0].v.i8 = static_cast<uint8_t>(sublayer_depth - 1);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
        ImGui::CloseCurrentPopup();
    }

    void open() {
        do_open = true;
        entity* e = G->selection.e;
        if (e && e->g_id == O_PLASTIC_POLYGON) {
            sublayer_depth = (e->properties[0].v.i8 & 0x7) + 1;
            front_align = (e->properties[1].v.i8 & 0x1) != 0;
        }
    }

    void layout() {
        handle_do_open(&do_open, "Polygon");
        ImGui::SetNextWindowSize(ImVec2(350, 0));

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Polygon", REF_TRUE, MODAL_FLAGS)) {

            ImGui::Text("Sublayer Depth");
            ImGui::SliderInt("##depth", &sublayer_depth, 1, 4, "%d", ImGuiSliderFlags_AlwaysClamp);

            ImGui::Checkbox("Front Align", &front_align);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Sublayer depth from front instead of back");

            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            if (ImGui::Button("Save", ImVec2(80, 0)))
                apply_properties();

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(80, 0)))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
    }
}
