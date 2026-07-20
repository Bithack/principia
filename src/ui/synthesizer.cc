#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"
#include "speaker.hh"

namespace UiSynthesizer {
    static bool do_open = false;

    static float low_hz;
    static float high_hz;
    static uint32_t waveform;
    static float bit_crushing;
    static float vol_vibrato;
    static float freq_vibrato;
    static float vol_vibrato_ext;
    static float freq_vibrato_ext;
    static float pulse_width;

    void apply_properties() {
        entity *e = G->selection.e;
        if (!e || e->g_id != O_SYNTHESIZER)
            return;

        e->properties[0].v.f = low_hz;
        e->properties[1].v.f = high_hz;
        e->properties[2].v.i = waveform;
        e->properties[3].v.f = bit_crushing;
        e->properties[4].v.f = vol_vibrato;
        e->properties[5].v.f = freq_vibrato;
        e->properties[6].v.f = vol_vibrato_ext;
        e->properties[7].v.f = freq_vibrato_ext;
        e->properties[8].v.f = pulse_width;

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);

        ImGui::CloseCurrentPopup();
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_SYNTHESIZER) {
            low_hz = e->properties[0].v.f;
            high_hz = e->properties[1].v.f;
            waveform = e->properties[2].v.i;
            bit_crushing = e->properties[3].v.f;
            vol_vibrato = e->properties[4].v.f;
            freq_vibrato = e->properties[5].v.f;
            vol_vibrato_ext = e->properties[6].v.f;
            freq_vibrato_ext = e->properties[7].v.f;
            pulse_width = e->properties[8].v.f;
        }
    }

    void layout() {
        handle_do_open(&do_open, "Synthesizer");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Synthesizer", REF_TRUE, MODAL_FLAGS)) {
            ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;

            //Base freq
            int hz_int = (int) roundf(low_hz);
            if (ImGui::InputInt("Base frequency", &hz_int, 20, 100)) {
                low_hz = (float) hz_int;
                high_hz = (std::max)(high_hz, low_hz);
            }

            //Max freq
            hz_int = (int) roundf(high_hz);
            if (ImGui::InputInt("Peak frequency", &hz_int, 20, 100)) {
                high_hz = (float) hz_int;
                low_hz = (std::min)(high_hz, low_hz);
            }

            low_hz = tclampf(low_hz, 0.f, 440.f*8.f);
            high_hz = tclampf(high_hz, 0.f, 440.f*8.f);

            //Waveform
            if (ImGui::BeginCombo("Waveform", speaker_options[waveform])) {
                for (int i = 0; i < NUM_WAVEFORMS; i++) {
                    ImGui::PushID(i);
                    if (ImGui::Selectable(speaker_options[i]))
                        waveform = i;

                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }

            //Pulse -> Pulse width
            if (waveform != 2)
                ImGui::BeginDisabled();

            ImGui::SliderFloat("Pulse width", &pulse_width, 0., 1., "%.2f", flags);

            if (waveform != 2)
                ImGui::EndDisabled();

            //Vibrato
            ImGui::SliderFloat("Volume vibrato Hz###vol-freq", &vol_vibrato, 0., 32., "%.0f", flags);
            ImGui::SliderFloat("Volume vibrato extent###vol-ext", &vol_vibrato_ext, 0., 1., "%.2f", flags);

            //Freq vibrato
            ImGui::SliderFloat("Freq vibrato Hz###freq-freq", &freq_vibrato, 0., 32., "%.0f", flags);
            ImGui::SliderFloat("Freq vibrato extent###freq-ext", &freq_vibrato_ext, 0., 1., "%.2f", flags);

            //Bitcrush
            int bc_int = (int)bit_crushing;
            if (ImGui::SliderInt("Bitcrushing", &bc_int, 0, 64, "%d", flags))
                bit_crushing = (float)bc_int;

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
