#include "sequencer.hh"
#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiSequencer {
    static bool do_open = false;

    static std::string sequence;
    static int seconds = 0;
    static int milliseconds = 0;
    static bool wrap_around = true;

    void apply_properties() {
        entity *e = G->selection.e;
        if (e && e->g_id == O_SEQUENCER) {
            int total_ms = seconds * 1000 + milliseconds;
            if (total_ms < SEQUENCER_MIN_TIME)
                total_ms = SEQUENCER_MIN_TIME;

            e->set_property(0, sequence.c_str());
            e->properties[1].v.i = total_ms;
            e->properties[2].v.i8 = wrap_around ? 1 : 0;

            ((sequencer *)e)->refresh_sequence();

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    int get_steps() {
        int steps = 0;
        for (char c : sequence)
            if (c == '1' || c == '0')
                steps++;

        return steps;
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_SEQUENCER) {
            seconds = floor((float)(e->properties[1].v.i) / 1000.f);
            milliseconds = (float)(e->properties[1].v.i % 1000);

            sequence = e->properties[0].v.s.buf;
            wrap_around = e->properties[2].v.i8 != 0;
        }
    }

    void layout() {
        handle_do_open(&do_open, "Sequencer");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Sequencer", REF_TRUE, MODAL_FLAGS)) {
            if ((seconds * 1000 + milliseconds) < SEQUENCER_MIN_TIME) {
                seconds = 0;
                milliseconds = SEQUENCER_MIN_TIME;
            }

            ImGui::Text("%d.%03ds. %d steps", seconds, milliseconds, get_steps());

            ImGui::InputText("Sequence", &sequence, ImGuiInputTextFlags_CharsNoBlank);

            ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
            ImGui::DragInt("Seconds", &seconds, .1, 0, 360, "%d", flags);
            ImGui::DragInt("Milliseconds", &milliseconds, .1, 0, 950, "%d", flags);

            ImGui::Checkbox("Wrap Around", &wrap_around);

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
