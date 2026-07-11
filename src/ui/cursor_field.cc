#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"
#include "object_factory.hh"

namespace UiCursorField {
    static bool do_open = false;
    static float val_right = 0.f;
    static float val_up = 0.f;
    static float val_left = 0.f;
    static float val_down = 0.f;

    void apply_properties() {
        entity *e = G->selection.e;

        if (e && e->g_id == O_CURSOR_FIELD) {
            e->properties[0].v.f = val_right;
            e->properties[1].v.f = val_up;
            e->properties[2].v.f = val_left;
            e->properties[3].v.f = val_down;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_CURSOR_FIELD) {
            val_right = e->properties[0].v.f;
            val_up = e->properties[1].v.f;
            val_left = e->properties[2].v.f;
            val_down = e->properties[3].v.f;
        }
    }

    void layout() {
        handle_do_open(&do_open, "Cursor Field");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Cursor Field", REF_TRUE, MODAL_FLAGS)) {
            ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
            ImGui::SliderFloat("Lower X", &val_left, -3.f, 3.f, "%.1f", flags);
            ImGui::SliderFloat("Upper X", &val_right, -3.f, 3.f, "%.1f", flags);
            ImGui::SliderFloat("Lower Y", &val_down, -3.f, 3.f, "%.1f", flags);
            ImGui::SliderFloat("Upper Y", &val_up, -3.f, 3.f, "%.1f", flags);

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
