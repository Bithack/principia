#include "game.hh"
#include "imgui.hh"
#include "sfxemitter.hh"

namespace UiSfxEmitterLegacy {
    static bool do_open = false;

    static int sound_id;
    static bool global_sound;

    void apply_properties() {
        entity* e = G->selection.e;
        if (!e || e->g_id != O_SFX_EMITTER)
            return;

        e->properties[0].v.i = sound_id;
        e->properties[1].v.i8 = global_sound ? 1 : 0;

        ImGui::CloseCurrentPopup();
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (!e || e->g_id != O_SFX_EMITTER)
            return;

        sound_id = e->properties[0].v.i;
        global_sound = (e->properties[1].v.i8 == 1);
    }

    void layout() {
        handle_do_open(&do_open, "SFX Emitter (legacy)");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("SFX Emitter (legacy)", REF_TRUE, MODAL_FLAGS)) {

            ImGui::TextUnformatted("Sound");
            if (ImGui::BeginCombo("##sound", sfxemitter_options[sound_id].name)) {
                for (int x = 0; x < NUM_SFXEMITTER_OPTIONS; x++) {
                    bool is_selected = (sound_id == x);
                    if (ImGui::Selectable(sfxemitter_options[x].name, is_selected))
                        sound_id = x;

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Checkbox("Global sound", &global_sound);

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
