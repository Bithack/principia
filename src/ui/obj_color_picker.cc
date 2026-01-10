#include "ui_imgui.hh"

namespace UiObjColorPicker {
    static bool do_open = false;
    static entity *entity_ptr;
    static float ref_color[4];

    static bool use_alpha = false;
    std::string wintitle{"Color"};

    void open(bool alpha, entity *entity) {
        do_open = true;
        entity_ptr = entity;
        tvec4 color = entity->get_color();
        ref_color[0] = color.r;
        ref_color[1] = color.g;
        ref_color[2] = color.b;
        ref_color[3] = use_alpha ? color.a : 1.f;
        use_alpha = alpha;
        tms_infof("opening color picker for %s", entity->get_name());
        wintitle = string_format("%s###beam-color", entity_ptr->get_name());
    }

    void layout() {
        handle_do_open(&do_open, "###beam-color");
        ImGui_CenterNextWindow();

        if (ImGui::BeginPopupModal(wintitle.c_str(), REF_TRUE, MODAL_FLAGS)) {
            tvec4 color = entity_ptr->get_color();
            float color_arr[4] = {
                color.r,
                color.g,
                color.b,
                use_alpha ? color.a : 1.f
            };
            ImGuiColorEditFlags flags =
                (use_alpha ? (
                    ImGuiColorEditFlags_AlphaPreviewHalf |
                    ImGuiColorEditFlags_AlphaBar
                ) : ImGuiColorEditFlags_NoAlpha)
                | ImGuiColorEditFlags_NoDragDrop
                | ImGuiColorEditFlags_PickerHueWheel; //TODO: decide! (color wheel/square)
            if (ImGui::ColorPicker4("Color", (float*) &color_arr, flags, (const float*) &ref_color)) {
                entity_ptr->set_color4(color_arr[0], color_arr[1], color_arr[2], color_arr[3]);
            }

            //*SPECIAL CASE*: Pixel frequency
            //This used to be controlled by the alpha value
            //but a separate slider is more user-friendly
            if (entity_ptr->g_id == O_PIXEL) {
                ImGui::Separator();
                ImGui::SliderInt(
                    "Frequency",
                    (int*) &entity_ptr->properties[4].v.i8,
                    0, 255,
                    (entity_ptr->properties[4].v.i8 == 0) ? "<none>" : "%d"
                );
            }

            ImGui::EndPopup();
        }
    }
}
