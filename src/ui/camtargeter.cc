#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"
#include "object_factory.hh"

namespace UiCamTargeter {
    static bool do_open = false;
    static uint8_t follow_mode = 0;
    static uint8_t offset_mode = 0;
    static float x_offset = 0.f;
    static float y_offset = 0.f;
    static const char *follow_mode_names[] = {"Smooth follow", "Snap to object", "Relative follow", "Linear follow"};

    void apply_properties() {
        entity *e = G->selection.e;

        if (e && e->g_id == O_CAM_TARGETER) {
            e->properties[1].v.i8 = follow_mode;
            e->properties[2].v.i8 = offset_mode;
            e->properties[3].v.f = tclampf(x_offset, -150.f, 150.f);
            e->properties[4].v.f = tclampf(y_offset, -150.f, 150.f);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_CAM_TARGETER) {
            follow_mode = e->properties[1].v.i8;
            offset_mode = e->properties[2].v.i8;
            x_offset = e->properties[3].v.f;
            y_offset = e->properties[4].v.f;
        }
    }

    void layout() {
        handle_do_open(&do_open, "Cam Targeter");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Cam Targeter", REF_TRUE, MODAL_FLAGS)) {
            ImGui::Text("Follow mode:");

            if (ImGui::BeginCombo(" ", follow_mode_names[follow_mode])) {
                for (int i = 0; i < sizeof(follow_mode_names) / sizeof(follow_mode_names[0]); ++i) {
                    bool is_selected = (follow_mode == i);
                    if (ImGui::Selectable(follow_mode_names[i], is_selected))
                        follow_mode = i;

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }

            ImGui::Text("Offset mode:");

            int _offset_mode = offset_mode;
            ImGui::RadioButton("Global", &_offset_mode, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Local", &_offset_mode, 1);
            offset_mode = (uint8_t)_offset_mode;

            ImGui::DragFloat("X offset", &x_offset, .1, -150.f, 150.f, "%.2f");
            ImGui::DragFloat("Y offset", &y_offset, .1, -150.f, 150.f, "%.2f");

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
