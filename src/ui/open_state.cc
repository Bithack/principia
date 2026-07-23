#include "game.hh"
#include "imgui.hh"
#include "main.hh"
#include "menu-play.hh"
#include "pkgman.hh"

// TODO: Add delete button for deleting states (needs a new method to delete states in game)
// Also maybe bring over the search stuff and debloat the StateEntry struct as not all the fields are useful for states

namespace UiOpenState {
    struct StateEntry {
        uint32_t id;
        uint32_t save_id;
        uint32_t id_type;

        std::string name;
        std::string modified;
    };

    static bool do_open = false;
    static std::vector<StateEntry> entries;
    static bool open_state_no_testplaying = false;
    static uint32_t pending_delete_id = 0;

    static void refresh() {
        entries.clear();

        lvlfile *level = pkgman::get_levels(LEVEL_LOCAL_STATE);

        while (level) {
            entries.push_back({
                level->id,
                level->save_id,
                static_cast<uint32_t>(level->id_type),
                level->name,
                level->modified_date,
            });

            lvlfile *next = level->next;
            delete level;
            level = next;
        }
    }

    static void open_state(const StateEntry &entry) {
        uint32_t *info = (uint32_t*)malloc(sizeof(uint32_t) * 3);

        info[0] = entry.id_type;
        info[1] = entry.id;
        info[2] = entry.save_id;

        if (open_state_no_testplaying) {
            G->state.test_playing = false;
            G->screen_back = P.s_menu_play;
        }

        P.add_action(ACTION_OPEN_STATE, info);

        ImGui::CloseCurrentPopup();
    }

    void open(bool _open_state_no_testplaying) {
        open_state_no_testplaying = _open_state_no_testplaying;
        do_open = true;

        refresh();
    }

    void layout() {
        handle_do_open(&do_open, "Open state");

        ImGui_CenterNextWindow();

        ImGui::SetNextWindowSize(UI(600., 0.));
        if (ImGui::BeginPopupModal("Open state", REF_TRUE, MODAL_FLAGS)) {

            ImGui_CloseOnEsc();

            ImGui::BeginChild("state_list_child", UI(0., 350.), ImGuiChildFlags_NavFlattened, FRAME_FLAGS);

            if (ImGui::BeginTable("state_list", 3, ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Last modified", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed);

                ImGui::TableHeadersRow();

                bool any_found = false;

                for (StateEntry &entry : entries) {
                    any_found = true;

                    ImGui::PushID(entry.save_id);

                    ImGui::TableNextRow();

                    // Name
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(entry.name.c_str());

                    // Modified
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(entry.modified.c_str());

                    // Actions
                    ImGui::TableNextColumn();

                    if (ImGui::Button("Open state"))
                        open_state(entry);

                    ImGui::PopID();
                }

                ImGui::EndTable();

                if (!any_found)
                    ImGui::TextDisabled("No saved states found.");
            }

            ImGui::EndChild();

            ImGui::EndPopup();
        }
    }
}
