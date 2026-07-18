#include "entity.hh"
#include "game.hh"
#include "imgui.h"
#include "imgui.hh"
#include "sfxemitter.hh"
#include "soundmanager.hh"

namespace UiSfxEmitter {
    static bool do_open = false;

    static int sound_id;
    static int sound_sub_id;
    static bool global_sound;
    static bool loop;

    void apply_properties() {
        entity* e = G->selection.e;
        if (!e || e->g_id != O_SFX_EMITTER)
            return;

        e->properties[0].v.i = sound_id;
        e->properties[1].v.i8 = global_sound ? 1 : 0;
        e->properties[2].v.i = (sound_sub_id == 0) ? SFX_CHUNK_RANDOM : sound_sub_id - 1;
        e->properties[3].v.i8 = loop ? 1 : 0;

        ImGui::CloseCurrentPopup();
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (!e || e->g_id != O_SFX_EMITTER)
            return;

        sound_id = e->properties[0].v.i;
        global_sound = (e->properties[1].v.i8 == 1);
        loop = (e->properties[3].v.i8 == 1);
        sound_sub_id = (e->properties[2].v.i == SFX_CHUNK_RANDOM) ? 0 : e->properties[2].v.i + 1;
    }

    void layout() {
        handle_do_open(&do_open, "SFX Emitter");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("SFX Emitter", REF_TRUE, MODAL_FLAGS)) {

            ImGui::TextUnformatted("Sound");
            if (ImGui::BeginCombo("##sound", sm::sound_lookup[sound_id]->name)) {
                for (int x=0; x<SND__NUM; x++) {
                    bool is_selected = (sound_id == x);
                    if (ImGui::Selectable(sm::sound_lookup[x]->name, is_selected)) {
                        sound_id = x;
                        sound_sub_id = 0;
                    }

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::TextUnformatted("Sound chunk");

            const sm_sound *snd = sm::get_sound_by_id(sound_id);
            if (snd->num_chunks == 1)
                ImGui::BeginDisabled();

            const char *preview_value;
            if (snd->num_chunks == 1)
                preview_value = "-";
            else if (sound_sub_id == 0)
                preview_value = "Random";
            else
                preview_value = snd->chunks[sound_sub_id-1].name;

            if (ImGui::BeginCombo("##sound_chunk", preview_value)) {
                bool is_selected = (sound_sub_id == 0);
                if (ImGui::Selectable("Random", is_selected))
                    sound_sub_id = 0;

                if (is_selected)
                    ImGui::SetItemDefaultFocus();

                for (int x=0; x<snd->num_chunks; ++x) {
                    is_selected = (sound_sub_id == x+1);
                    if (ImGui::Selectable(snd->chunks[x].name, is_selected))
                        sound_sub_id = x+1;

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (snd->num_chunks == 1)
                ImGui::EndDisabled();

            ImGui::Checkbox("Global sound", &global_sound);

            ImGui::Checkbox("Loop", &loop);

            ImGui_ButtonBar(apply_properties);
            ImGui::EndPopup();
        }
    }
}
