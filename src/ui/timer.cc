#include "timer.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"
#include "world.hh"

namespace UiTimer {
    static bool do_open = false;

    static int seconds = 0;
    static int milliseconds = 0;
    static int ticks = 0;
    static bool use_system_time = false;

    void apply_properties() {
        entity *e = G->selection.e;
        if (e && e->g_id == O_TIMER) {
            int total_ms = seconds * 1000 + milliseconds;
            if (total_ms < TIMER_MIN_TIME)
                total_ms = TIMER_MIN_TIME;

            e->properties[0].v.i = total_ms;
            e->properties[1].v.i8 = ticks;
            e->properties[2].v.i = use_system_time ? 1 : 0;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_TIMER) {
            float s = floor((float)(e->properties[0].v.i) / 1000.f);
            float ms = (float)(e->properties[0].v.i % 1000);

            seconds = (int)s;
            milliseconds = (int)ms;
            ticks = e->properties[1].v.i8;
            use_system_time = e->properties[2].v.i != 0;
        }
    }

    void layout() {
        handle_do_open(&do_open, "Timer");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Timer", REF_TRUE, MODAL_FLAGS)) {

            if ((seconds * 1000 + milliseconds) < TIMER_MIN_TIME) {
                seconds = 0;
                milliseconds = TIMER_MIN_TIME;
            }

            ImGui::Text("Time between ticks:   %d.%03ds", seconds, milliseconds);

            ImGui::SetNextItemWidth(UI(110));
            ImGui::DragInt("Seconds", &seconds, .1, 0, 360, "%d",
                ImGuiSliderFlags_AlwaysClamp);

            ImGui::SetNextItemWidth(UI(110));
            ImGui::DragInt("Milliseconds", &milliseconds, .1, 0, 999, "%d",
                ImGuiSliderFlags_AlwaysClamp);

            ImGui::SetNextItemWidth(UI(110));
            ImGui::DragInt("Number of ticks", &ticks, .1, 0, 255, "%d",
                ImGuiSliderFlags_AlwaysClamp);
            ImGui::SetItemTooltip("0 = Infinite ticks");

            ImGui::Checkbox("Use system time", &use_system_time);
            ImGui::SetItemTooltip("Use system time for ticks, instead of in-game time.");

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
