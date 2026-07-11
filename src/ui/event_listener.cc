#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiEventListener {
    static bool do_open = false;
    static int selected_event = 0;
    static const char *events[] = {
        "Player die",
        "Enemy die",
        "Interactive object destroyed",
        "Player respawn",
        "Touch/Mouse Click",
        "Touch/Mouse Release",
        "Any absorber activated",
        "Level completed",
        "Game over"
    };

    void apply_properties() {
        entity *e = G->selection.e;

        if (e && e->g_id == O_EVENT_LISTENER) {
            e->properties[0].v.i = selected_event;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_EVENT_LISTENER)
            selected_event = e->properties[0].v.i;
    }

    void layout() {
        handle_do_open(&do_open, "Event listener");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Event listener", REF_TRUE, MODAL_FLAGS)) {
            if (ImGui::BeginCombo("##event", events[selected_event])) {
                for (int i = 0; i < sizeof(events) / sizeof(events[0]); ++i) {
                    bool is_selected = (selected_event == i);
                    if (ImGui::Selectable(events[i], is_selected))
                        selected_event = i;

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
