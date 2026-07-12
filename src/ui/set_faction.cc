#include "anchor.hh"
#include "entity.hh"
#include "faction.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiSetFaction {
    static bool do_open = false;
    static int selected_faction = 0;

    void apply_properties() {
        entity* e = G->selection.e;
        if (e && e->g_id == O_GUARDPOINT) {
            ((anchor *)e)->set_faction(selected_faction);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
        ImGui::CloseCurrentPopup();
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_GUARDPOINT)
            selected_faction = e->properties[0].v.i;
    }

    void layout() {
        handle_do_open(&do_open, "Set Faction");
        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Set Faction", REF_TRUE, MODAL_FLAGS)) {
            if (ImGui::BeginCombo("##faction", factions[selected_faction].name)) {
                for (int i = 0; i < sizeof(factions) / sizeof(factions[0]); ++i) {
                    bool is_selected = (selected_faction == i);
                    if (ImGui::Selectable(factions[i].name, is_selected))
                        selected_faction = i;

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
