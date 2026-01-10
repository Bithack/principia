#include "ui_imgui.hh"

namespace UiRubber {
static bool do_open = false;
    static float restitution = 0.5f;
    static float friction = 1.8f;

    void open() {
        entity* e = G->selection.e;
        restitution = e->properties[1].v.f;
        friction = e->properties[2].v.f;
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "Rubber");
        if (ImGui::BeginPopupModal("Rubber", REF_TRUE, MODAL_FLAGS)) {

            ImGui::SliderFloat("Restitution", &restitution, 0.0f, 1.0f);
            ImGui::SliderFloat("Friction", &friction, 1.0f, 10.0f);

            ImGui::Spacing();
            ImGui::SeparatorText("");
            if (ImGui::Button("Apply")) {
                entity* e = G->selection.e;
                if (e && (e->g_id == O_WHEEL || e->g_id == O_RUBBER_BEAM)) {
                    e->properties[1].v.f = restitution;
                    e->properties[2].v.f = friction;
                    P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
                    P.add_action(ACTION_RESELECT, 0);
                    do_open = false;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}
