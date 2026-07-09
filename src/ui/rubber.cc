#include "beam.hh"
#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"
#include "wheel.hh"

namespace UiRubber {
static bool do_open = false;
    static float restitution = 0.5f;
    static float friction = 1.8f;

    void apply_properties() {
        entity* e = G->selection.e;
        if (e && (e->g_id == O_WHEEL || e->g_id == O_RUBBER_BEAM)) {
            e->properties[1].v.f = restitution;
            e->properties[2].v.f = friction;

            if (e->g_id == O_RUBBER_BEAM)
                ((beam*)e)->do_update_fixture = true;
            else
                ((wheel*)e)->do_update_fixture = true;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    void open() {
        entity* e = G->selection.e;
        restitution = e->properties[1].v.f;
        friction = e->properties[2].v.f;
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "Rubber");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Rubber", REF_TRUE, MODAL_FLAGS)) {

            ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
            ImGui::SliderFloat("Restitution", &restitution, 0.0f, 1.0f, "%.2f", flags);
            ImGui::SliderFloat("Friction", &friction, 1.0f, 10.0f, "%.2f", flags);

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
