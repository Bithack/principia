#include "speaker.hh"
#include "ui_imgui.hh"
#include <chrono>

namespace UiSynthesizer {
    static bool do_open = false;
    static entity *entity_ptr = nullptr;

    static std::chrono::steady_clock::time_point init_time;

    #define SYNTH_GRAPH_SIZE ImVec2(400., 100.)
    //Points must be multiple of 400
    #define SYNTH_GRAPH_POINTS_0 400
    #define SYNTH_GRAPH_POINTS_1 800
    #define SYNTH_GRAPH_POINTS_2 1600
    #define SYNTH_GRAPH_VX 0.01f
    #define SYNTH_GRAPH_VY 1.f

    // static const char *graph_type_names[] = { "Sine" };
    //static const graph_type_fns = {};
    enum {
        WAVEFORM_SINE,
        WAVEFORM_SQR,
        WAVEFORM_PULSE,
        WAVEFORM_SAWTOOTH,
        WAVEFORM_TRIANGLE,
        WAVEFORM_ETC
    };

    void init() {
        init_time = std::chrono::steady_clock::now();
    }

    void open(entity *entity /*= G->selection.e*/) {
        do_open = true;
        entity_ptr = entity;
    }

    void layout() {
        handle_do_open(&do_open, "Synthesizer");
        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Synthesizer", REF_TRUE, MODAL_FLAGS)) {
            ImDrawList *draw_list = ImGui::GetWindowDrawList();

            float *low_hz = &entity_ptr->properties[0].v.f;
            float *high_hz = &entity_ptr->properties[1].v.f;
            uint32_t *waveform = &entity_ptr->properties[2].v.i;
            float *bit_crushing = &entity_ptr->properties[3].v.f;
            float *vol_vibrato = &entity_ptr->properties[4].v.f;
            float *freq_vibrato = &entity_ptr->properties[5].v.f;
            float *vol_vibrato_ext = &entity_ptr->properties[6].v.f;
            float *freq_vibrato_ext = &entity_ptr->properties[7].v.f;
            float *pulse_width = &entity_ptr->properties[8].v.f;

            if (*waveform < WAVEFORM_ETC) {
                //Render graph
                ImVec2 p_min = ImGui::GetCursorScreenPos();
                ImVec2 p_max = ImVec2(p_min.x + SYNTH_GRAPH_SIZE.x, p_min.y + SYNTH_GRAPH_SIZE.y);
                ImVec2 size = SYNTH_GRAPH_SIZE;

                ImGui::PushID("synth-graph");
                ImGui::Dummy(SYNTH_GRAPH_SIZE);
                ImGui::PopID();

                draw_list->PushClipRect(p_min, p_max);

                //Background
                draw_list->AddRectFilled(p_min, p_max, ImColor(0, 0, 0));

                //Center h. line
                draw_list->AddLine(
                    ImVec2(0., p_min.y + (size.y / 2)),
                    ImVec2(p_max.x, p_min.y + (size.y / 2)),
                    ImColor(128, 128, 128),
                    2.
                );

                //Graph
                {
                    double time_seconds;
                    {
                        auto now = std::chrono::steady_clock::now();
                        time_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(now - init_time).count();
                    }

                    float freq = *low_hz;

                    // if ((*freq_vibrato > 0.) && (*freq_vibrato_ext > 0.)) {
                    //   freq *= 1. - (sin(2. * M_PI * time_seconds * *freq_vibrato) * SYNTH_GRAPH_VX) * *freq_vibrato_ext;
                    // }

                    static const float scr_spd_inv = 10.;
                    float x_offset = (time_seconds * SYNTH_GRAPH_VX) / scr_spd_inv;
                    float x = 0.;

                    int points = (freq > 1000.) ? ((freq > 2000.) ? SYNTH_GRAPH_POINTS_2 : SYNTH_GRAPH_POINTS_1) : SYNTH_GRAPH_POINTS_0;

                    float bcc = 0;

                    float prev_draw_x, prev_draw_y;
                    for (int i = 0; i < points; i++) {
                        float y;
                        float sx = (x + x_offset) * freq;
                        float wave = sinf(sx * 2. * M_PI);
                        switch (*waveform) {
                            case WAVEFORM_SINE:
                                y = wave;
                                break;
                            case WAVEFORM_SQR:
                                //XXX: is this correct?
                                y = ((int)(sx) & 1) ? 1 : -1;
                                break;
                            case WAVEFORM_PULSE:
                                if (*pulse_width >= 1.) {
                                    y = 1.;
                                } else if (*pulse_width <= 0.) {
                                    y = -1.;
                                } else {
                                    y = (((wave + 1.f) / 2.f) >= (1.f - *pulse_width)) ? 1.f : -1.f;
                                }
                                break;
                            case WAVEFORM_SAWTOOTH:
                                y = fmod(sx * 2., 2.) - 1.;
                                break;
                            case WAVEFORM_TRIANGLE:
                                // y = fmod(sx * freq, 1.);
                                // if (y > 0.5) y = .5 - y;
                                // y = (y - .25) * 4.;
                                y = 4.0 * fabs(fmod(sx, 1.0) - 0.5) -1.;
                                break;
                            case WAVEFORM_ETC:
                                break;
                        }
                        if (*vol_vibrato > 0.) {
                            //y *= .5 - .5 * sin(M_PI * 2. * *vol_vibrato * (((float) i / points) + x_offset)) * *vol_vibrato_ext;
                            //Limited to 15 hz to prevent epilepsy and stuff
                            y *= .5 - .5 * sin(M_PI * 2. * (std::min)(*vol_vibrato, 15.f) * (time_seconds / 2.)) * *vol_vibrato_ext;
                        }

                        float draw_x = p_min.x + ((x / SYNTH_GRAPH_VX) * size.x);
                        float draw_y = p_min.y + (((y / SYNTH_GRAPH_VY) * -.5) + .5) * size.y;

                        if ((int) *bit_crushing > 0) {
                            if (bcc != 0) draw_y = prev_draw_y;
                            bcc += 1600.f / points;
                            if (bcc >= (*bit_crushing + 1)) bcc = 0;
                        }

                        if (i != 0) draw_list->AddLine(
                            ImVec2(prev_draw_x, prev_draw_y),
                            ImVec2(draw_x, draw_y),
                            ImColor(255, 0, 0),
                            2.f
                        );

                        prev_draw_x = draw_x;
                        prev_draw_y = draw_y;

                        x += SYNTH_GRAPH_VX / (float) points;
                    }
                }

                //Numbers
                draw_list->AddText(
                    ImVec2(p_min.x, p_max.y - ImGui::GetFontSize()),
                    ImColor(128, 128, 128),
                    "0.0"
                );
                static const std::string end = string_format("%.02f", SYNTH_GRAPH_VX);
                draw_list->AddText(
                    ImVec2(p_max.x - ImGui::CalcTextSize(end.c_str()).x, p_max.y - ImGui::GetFontSize()),
                    ImColor(128, 128, 128),
                    end.c_str()
                );
                draw_list->PopClipRect();
            } else {
                // ImGui::TextColored(ImColor(255, 0, 0), "Unable to [preview this waveform");
                // ImGui::SetCursorPosY(ImGui::GetStyle().ItemSpacing.y);
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImGui::PushID("synth-graph-dummy");
                ImGui::Dummy(SYNTH_GRAPH_SIZE);
                ImGui::PopID();
                draw_list->AddText(p, ImColor(255, 0, 0), "Unable to preview this waveform");
            }

            //Controls

            ImGui::SeparatorText("Waveform");

            //Waveform
            if (ImGui::BeginCombo("Waveform", (*waveform < NUM_WAVEFORMS) ? speaker_options[*waveform]: "???")) {
                for (int i = 0; i < NUM_WAVEFORMS; i++) {
                    ImGui::PushID(i);
                    if (ImGui::Selectable(speaker_options[i])) {
                        *waveform = i;
                    }
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }

            //Pulse -> Pulse width
            if (*waveform == WAVEFORM_PULSE) {
                ImGui::SliderFloat("Pulse width", pulse_width, 0., 1.);
            }

            //Frequency
            ImGui::SeparatorText("Frequency");

            //Base freq
            int hz_int = (int) roundf(*low_hz);
            if (ImGui::SliderInt("Base requency", &hz_int, 100, 3520)) {
                *low_hz = (float) hz_int;
                *high_hz = (std::max)(*high_hz, *low_hz);
            }

            //Max freq
            hz_int = (int) roundf(*high_hz);
            if (ImGui::SliderInt("Max frequency", &hz_int, 100, 3520)) {
                *high_hz = (float) hz_int;
                *low_hz = (std::min)(*high_hz, *low_hz);
            }

            //Vibrato
            ImGui::SeparatorText("Volume vibrato");

            ImGui::SliderFloat("Frequency###vol-freq", vol_vibrato, 0., 32.);
            ImGui::SliderFloat("Extent###vol-ext", vol_vibrato_ext, 0., 1.);

            //Freq vibrato
            ImGui::SeparatorText("Frequency vibrato");

            ImGui::SliderFloat("Frequency###freq-freq", freq_vibrato, 0., 32.);
            ImGui::SliderFloat("Extent###freq-ext", freq_vibrato_ext, 0., 1.);

            //Bitcrush
            ImGui::SeparatorText("Bitcrushing");

            int bc_int = (int) *bit_crushing;
            if (ImGui::SliderInt("Bitcrushing", &bc_int, 0, 64)) {
                *bit_crushing = (float) bc_int;
            }

            ImGui::TextDisabled("Visualization may be inaccurate");

            ImGui::EndPopup();
        }
    }
}
