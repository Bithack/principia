#include "ui_imgui.hh"

namespace UiSticky {
	static bool do_open = false;
    static bool center_x = true;
    static bool center_y = true;
    static int font_size = 2;
    static std::string text = "Hello!";

    void open() {
        entity* e = G->selection.e;
        text = e->properties[0].v.s.buf ? e->properties[0].v.s.buf : "Hello!";
        center_x = e->properties[1].v.i8 != 0;
        center_y = e->properties[2].v.i8 != 0;
        font_size = e->properties[3].v.i8;
        do_open = true;
    }

    void layout() {


        handle_do_open(&do_open, "Sticky Note");
        if (ImGui::BeginPopupModal("Sticky Note", nullptr, MODAL_FLAGS)) {
            ImGui::Checkbox("Center X", &center_x);
            ImGui::SameLine();
            ImGui::Checkbox("Center Y", &center_y);
            ImGui::Spacing();
            ImGui::SliderInt("Font Size", &font_size, 0, 3);
            ImGui::SeparatorText("Text");
            ImGui::InputTextMultiline("##text", &text, ImVec2(300, ImGui::GetTextLineHeight() * 10));

            ImGui::Spacing();
            ImGui::SeparatorText("");
            if (ImGui::Button("Apply")) {
                entity* e = G->selection.e;
                if (e->properties[0].v.s.buf) {
                    free(e->properties[0].v.s.buf);
                }
                e->properties[0].v.s.buf = strdup(text.c_str());
                e->properties[1].v.i8 = static_cast<uint8_t>(center_x);
                e->properties[2].v.i8 = static_cast<uint8_t>(center_y);
                e->properties[3].v.i8 = static_cast<uint8_t>(font_size);

                P.add_action(ACTION_SET_STICKY_TEXT, text.c_str());
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
