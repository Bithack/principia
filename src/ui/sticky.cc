#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiSticky {
	static bool do_open = false;
    static bool center_x = true;
    static bool center_y = true;
    static int font_size = 2;
    static std::string text = "Hello!";

    void apply_properties() {
        entity* e = G->selection.e;
        if (!e || e->g_id != O_STICKY_NOTE) {
            tms_errorf("sticky: no entity selected");
            return;
        }

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

    void open() {
        do_open = true;

        entity* e = G->selection.e;
        if (!e || e->g_id != O_STICKY_NOTE) {
            tms_errorf("sticky: no entity selected");
            return;
        }

        text = e->properties[0].v.s.buf ? e->properties[0].v.s.buf : "Hello!";
        center_x = e->properties[1].v.i8 != 0;
        center_y = e->properties[2].v.i8 != 0;
        font_size = e->properties[3].v.i8;
    }

    void layout() {
        handle_do_open(&do_open, "Sticky Note");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Sticky Note", REF_TRUE, MODAL_FLAGS)) {
            ImGui::Checkbox("Center X", &center_x);
            ImGui::SameLine();
            ImGui::Checkbox("Center Y", &center_y);
            ImGui::Spacing();

            ImGui::SliderInt("Font Size", &font_size, 0, 3, "%d", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SeparatorText("Text");
            ImGui::InputTextMultiline("##text", &text, ImVec2(UI(210), ImGui::GetTextLineHeight() * 8));

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
