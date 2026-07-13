#include "fxemitter.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiFXEmitter {
    static bool do_open = false;
    static float radius = 1.f;
    static int count = 5;
    static float interval = .3f;
    static uint32_t effects[4] = {FX_EXPLOSION, FX_INVALID, FX_INVALID, FX_INVALID};

    static const char *effect_names[] = {
        "[None]",
        "Explosion",
        "Highlight",
        "Destroy connections",
        "Smoke",
        "Magic",
        "Break"
    };

    void apply_properties() {
        entity *e = G->selection.e;
        if (e && e->g_id == O_FX_EMITTER) {
            e->properties[0].v.f = radius;
            e->properties[1].v.i = count;
            e->properties[2].v.f = interval;
            for (int n = 0; n < 4; ++n) {
                e->properties[3 + n].v.i = effects[n] == 0 ? FX_INVALID : effects[n] - 1;
            }

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);

            ImGui::CloseCurrentPopup();
        }
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (e && e->g_id == O_FX_EMITTER) {
            radius = e->properties[0].v.f;
            count = e->properties[1].v.i;
            interval = e->properties[2].v.f;
            for (int n = 0; n < 4; ++n) {
                effects[n] = e->properties[3 + n].v.i;
                if (effects[n] == FX_INVALID)
                    effects[n] = 0;
                else
                    effects[n]++;
            }
        }
    }

    void layout() {
        handle_do_open(&do_open, "FX Emitter");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("FX Emitter", REF_TRUE, MODAL_FLAGS)) {
            ImGui::Text("Effects:");

            for (int n = 0; n < 4; ++n) {
                ImGui::PushID(n);
                if (ImGui::BeginCombo("##effect", effect_names[effects[n]])) {
                    for (int i = 0; i < sizeof(effect_names) / sizeof(effect_names[0]); ++i) {
                        bool is_selected = (effects[n] == i);
                        if (ImGui::Selectable(effect_names[i], is_selected))
                            effects[n] = i;

                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopID();
            }

            ImGui::Separator();

            ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
            ImGui::SliderFloat("Radius", &radius, 0.1f, 5.f, "%.1f", flags);
            ImGui::SliderInt("Count", &count, 1, 20, "%d", flags);
            ImGui::SliderFloat("Interval", &interval, 0.05f, 1.f, "%.2f", flags);

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
