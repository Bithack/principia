#include "ui_imgui.hh"

namespace UiFrequency {
static uint32_t __always_zero = 0;

    static bool do_open = false;
    static bool range = false;
    static entity *entity_ptr;

    static bool this_is_tx = false;
    static uint32_t* this_freq_range_start;
    static uint32_t* this_freq_range_size;

    typedef struct {
        //only used by update_freq_space, now freq_user
        size_t index;
        entity *ent;
        bool is_tx;
        //this is an inclusive range
        //a.k.a range_start..=range_end
        uint32_t range_start; uint32_t range_end;
    } FreqUsr;

    static uint32_t max_freq = 0;
    static std::vector<FreqUsr> freq_space;

    //emulating std::optional from c++17 (rust Option) with std::pair<T, bool>
    static std::pair<FreqUsr, bool> freq_user(entity *e) {
        FreqUsr usr;
        usr.ent = e;
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

    //Update freq_space and max_freq
    //freq_space does not include the current tx
    static void update_freq_space() {
        freq_space.clear();
        max_freq = 0;
        uint32_t counter = 0;
        std::map<uint32_t, entity*> all_entities = W->get_all_entities();
        for (auto i = all_entities.begin(); i != all_entities.end(); i++) {
            entity *e = i->second;
            //Skip currently selected entity
            if (e == entity_ptr) continue;
            auto x = freq_user(e);
            if (x.second) {
                max_freq = (std::max)(max_freq, x.first.range_start);
                freq_space.push_back(x.first);
            }
        }
        std::sort(freq_space.begin(), freq_space.end(), [](const FreqUsr& a, const FreqUsr& b) {
            //return a.ent->id < b.ent->id;
            //XXX: sort by range_start looks better
            return a.range_start < b.range_start;
        });
        for (size_t i = 0; i < freq_space.size(); i++) freq_space[i].index = i;
    }

    void open(bool is_range, entity *e) {
        do_open = true;

        range = is_range;
        entity_ptr = e;

        this_is_tx =
            (e->g_id == O_TRANSMITTER) ||
            (e->g_id == O_MINI_TRANSMITTER) ||
            (e->g_id == O_BROADCASTER);

        //XXX: pixel->properties[4].v.i (need i8) is incorrect but this menu is currently unused for pixel anyway
        this_freq_range_start = (e->g_id == O_PIXEL) ? &e->properties[4].v.i : &e->properties[0].v.i;
        this_freq_range_size = range ? &e->properties[1].v.i : &__always_zero;

        update_freq_space();
    }

    void layout() {
        handle_do_open(&do_open, "Frequency");
        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Frequency", REF_TRUE, MODAL_FLAGS)) {
            //XXX: This assumes int is 4 bytes
            ImGui::DragInt("Frequency", (int*) this_freq_range_start, .1, 0, 10000);
            (*this_freq_range_size)++;
            if (range) ImGui::SliderInt("Range", (int*) this_freq_range_size, 1, 30);
            (*this_freq_range_size)--;

            ImGui::Text(
                range ? "%s on frequencies: %d-%d" : "%s on frequency: %d",
                this_is_tx ? "Transmitting" : "Listening",
                *this_freq_range_start,
                *this_freq_range_start + *this_freq_range_size
            );

            ImVec2 size = ImVec2(0., 133.);
            if (ImGui::BeginChild(ImGui::GetID("###x-table-frame"), size, ImGuiChildFlags_NavFlattened, FRAME_FLAGS)) {
                if (ImGui::BeginTable("###x-table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_NoSavedSettings)) {
                    ImGui::TableSetupColumn("###", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Object");
                    ImGui::TableSetupColumn("Frequency");
                    ImGui::TableHeadersRow();
                    for (const FreqUsr& usr: freq_space) {
                        ImGui::TableNextRow();
                        ImGui::TableSetBgColor(
                            ImGuiTableBgTarget_RowBg0,
                            (
                                (usr.is_tx != this_is_tx) &&
                                (this_is_tx ? (
                                    (usr.range_start >= *this_freq_range_start) &&
                                    (usr.range_end <= (*this_freq_range_start + *this_freq_range_size))
                                ) : (
                                    (*this_freq_range_start >= usr.range_start) &&
                                    ((*this_freq_range_start + *this_freq_range_size) <= usr.range_end)
                                ))
                            ) ? (usr.is_tx ? ImColor(48, 255, 48, 48) : ImColor(255, 48, 48, 48))
                                : ImColor(0,0,0,0)
                        );
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", usr.is_tx ? "^" : "v");
                        ImGui::TableNextColumn();
                        ImGui::Text("%s (id: %d)", usr.ent->get_name(), usr.ent->id);
                        ImGui::SetItemTooltip("Click to set frequency\nShift + click to select object");
                        if (ImGui::IsItemClicked()) {
                            if (ImGui::GetIO().KeyShift) {
                                G->lock();
                                b2Vec2 xy = usr.ent->get_position();
                                float z = G->cam->_position.z;
                                G->cam->set_position(xy.x, xy.y, z);
                                G->selection.reset();
                                G->selection.select(usr.ent);
                                G->unlock();
                                //ImGui::CloseCurrentPopup();
                            } else {
                                *this_freq_range_start = usr.range_start;
                                if (range) *this_freq_range_size = usr.range_end - usr.range_start;
                            }
                        }
                        ImGui::TableNextColumn();
                        if (usr.range_end == usr.range_start) {
                            ImGui::Text("%d", usr.range_start);
                        } else {
                            ImGui::Text("%d-%d", usr.range_start, usr.range_end);
                        }
                    }
                    //ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, 0);
                    ImGui::EndTable();
                }
            }
            ImGui::EndChild();
            ImGui::EndPopup();
        }
    }
}
