#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"
#include "object_factory.hh"
#include "ui.hh"
#include "world.hh"
#include <map>

namespace UiFrequency {
    static bool do_open = false;
    static bool range = false;

    static bool this_is_tx = false;
    static uint32_t freq_range_start;
    static int freq_range_size;

    typedef struct {
        bool is_tx;
        //this is an inclusive range
        //a.k.a range_start..=range_end
        uint32_t range_start; uint32_t range_end;
    } FreqUsr;

    static std::map<uint32_t, std::pair<int,int>> freq_counts; // freq -> (tx_count, rx_count)

    static std::pair<FreqUsr, bool> freq_user(entity *e) {
        FreqUsr usr;
        if ((e->g_id == O_TRANSMITTER) || (e->g_id == O_MINI_TRANSMITTER)) {
            usr.is_tx = true;
            usr.range_start = usr.range_end = e->properties[0].v.i;
        } else if (e->g_id == O_BROADCASTER) {
            usr.is_tx = true;
            usr.range_start = e->properties[0].v.i;
            usr.range_end = e->properties[0].v.i + e->properties[1].v.i;
        } else if (e->g_id == O_RECEIVER) {
            usr.is_tx = false;
            usr.range_start = usr.range_end = e->properties[0].v.i;
        } else if ((e->g_id == O_PIXEL) && (e->properties[4].v.i8 != 0)) {
            usr.is_tx = false;
            usr.range_start = usr.range_end = (uint32_t)(int8_t)e->properties[4].v.i8;
        } else {
            //usr is not fully inited~
            return std::pair<FreqUsr, bool>(usr, false);
        }

        return std::pair<FreqUsr, bool>(usr, true);
    }

    // Build frequency tallies (exclude current entity)
    static void update_freq_space() {
        freq_counts.clear();

        std::map<uint32_t, entity*> all_entities = W->get_all_entities();
        for (auto i = all_entities.begin(); i != all_entities.end(); ++i) {
            entity *e = i->second;

            auto x = freq_user(e);
            if (!x.second)
                continue;

            const FreqUsr &usr = x.first;
            for (uint32_t f = usr.range_start; f <= usr.range_end; ++f) {
                if (usr.is_tx)
                    freq_counts[f].first++;
                else
                    freq_counts[f].second++;
            }
        }
    }

    void apply_properties() {
        entity *e = G->selection.e;

        if (e) {
            e->properties[0].v.i = freq_range_start;
            if (e->g_id == O_BROADCASTER) {
                e->properties[1].v.i = freq_range_size;
                ui::messagef("Frequency set to %u (+%u)", freq_range_start, freq_range_size);
            } else
                ui::messagef("Frequency set to %u", freq_range_start);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
        ImGui::CloseCurrentPopup();
    }

    void open(bool is_range) {
        entity *e = G->selection.e;
        do_open = true;

        range = is_range;

        this_is_tx =
            (e->g_id == O_TRANSMITTER) ||
            (e->g_id == O_MINI_TRANSMITTER) ||
            (e->g_id == O_BROADCASTER);

        freq_range_start = e->properties[0].v.i;
        freq_range_size = e->properties[1].v.i;

        update_freq_space();
    }

    void layout() {
        handle_do_open(&do_open, "Frequency");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Frequency", REF_TRUE, MODAL_FLAGS)) {
            unsigned int max_freq = 4294967040; // 2^32 - 256
            ImGui::DragScalar("Frequency", ImGuiDataType_U32, &freq_range_start, .1, 0, &max_freq);
            freq_range_size++;
            if (range)
                ImGui::SliderInt("Range", &freq_range_size, 1, 255, "%d", ImGuiSliderFlags_AlwaysClamp);
            freq_range_size--;

            ImGui::Text(
                range ? "%s on frequencies: %d-%d" : "%s on frequency: %d",
                this_is_tx ? "Transmitting" : "Listening",
                freq_range_start,
                freq_range_start + freq_range_size
            );

            ImVec2 size = ImVec2(0., 250.);
            if (ImGui::BeginChild(ImGui::GetID("###x-table-frame"), size, ImGuiChildFlags_NavFlattened, FRAME_FLAGS)) {
                const auto &counts = freq_counts;

                if (ImGui::BeginTable("###x-table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_NoSavedSettings)) {
                    ImGui::TableSetupColumn("Frequency", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Receivers", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Transmitters", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableHeadersRow();

                    for (const auto &p : counts) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", p.first);
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", p.second.second);
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", p.second.first);
                    }

                    ImGui::EndTable();
                }
            }
            ImGui::EndChild();

            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            if (ImGui::Button("Save"))
                apply_properties();

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
    }
}
