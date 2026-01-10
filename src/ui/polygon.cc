#include "polygon.hh"
#include "ui_imgui.hh"

namespace UiPolygon {
	static bool do_open = false;
    static int sublayer_depth = 1;
    static bool front_align = false;

    void open() {
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "Polygon");
        ImGui::SetNextWindowSize(ImVec2(350, 0));

        if (ImGui::BeginPopupModal("Polygon", nullptr, MODAL_FLAGS)) {
            entity *e = G->selection.e;

            ImGui::Text("Sublayer Depth");
            ImGui::SliderInt("##depth", &sublayer_depth, 1, 4);

            ImGui::Checkbox("Front Align", &front_align);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Sublayer depth from front instead of back");
            }

            ImGui::Spacing();
            ImGui::SeparatorText("");
            if (ImGui::Button("Apply")) {
                if (e && e->g_id == O_PLASTIC_POLYGON) {
                    ((polygon*)e)->do_recreate_shape = true;

                    e->properties[1].v.i8 = static_cast<uint8_t>(front_align ? 1 : 0);
                    e->properties[0].v.i8 = static_cast<uint8_t>(sublayer_depth - 1);

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
