#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"
#include "soundmanager.hh"

namespace UiSoundManager {
    static bool do_open = false;
    static int selected_sound = 0;
    static bool catch_all = false;

    void apply_properties() {
        entity* e = G->selection.e;
        if (e && e->g_id == O_SOUNDMAN) {
            e->properties[0].v.i = selected_sound;
            e->properties[1].v.i8 = catch_all ? 1 : 0;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_SOUNDMAN) {
            selected_sound = e->properties[0].v.i;
            catch_all = e->properties[1].v.i8 != 0;
        }
    }

    void layout() {
        handle_do_open(&do_open, "Sound Manager");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Sound Manager", REF_TRUE, MODAL_FLAGS)) {

            if (ImGui::BeginCombo("##sound", sm::sound_lookup[selected_sound]->name)) {
                for (int i = 0; i < sizeof(sm::sound_lookup) / sizeof(sm::sound_lookup[0]); ++i) {
                    bool is_selected = (selected_sound == i);
                    if (ImGui::Selectable(sm::sound_lookup[i]->name, is_selected))
                        selected_sound = i;

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Checkbox("Catch all", &catch_all);

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
