#include "game.hh"
#include "imgui.hh"
#include "main.hh"
#include "pkgman.hh"

namespace UiLevelManager {
    struct lvlinfo_ext {
        lvlinfo info;
        uint32_t id;
    };

    static int pending_delete_id = 0;
    static bool do_open = false;
    static std::string search_query{""};

    static lvlfile *level_list = nullptr;

    static lvlinfo_ext *level_metadata = nullptr;

    static int update_level_info(int id_type, uint32_t id) {
        if (level_metadata) {
            // Check if data needs to be reloaded
            if (level_metadata->id == id)
                return 0;

            // Dealloc current data
            level_metadata->info.~lvlinfo();
            delete level_metadata;
        }

        level_metadata = new lvlinfo_ext;

        // Update meta
        level_metadata->id = id;

        // Read level info
        lvledit lvl;
        if (lvl.open(id_type, id)) {
            level_metadata->info = lvl.lvl;
            if (level_metadata->info.descr_len && level_metadata->info.descr)
                level_metadata->info.descr = strdup(level_metadata->info.descr);

            return 1;
        } else {
            delete level_metadata;
            level_metadata = nullptr;
            return -1;
        }
    }

    static void reload_level_list() {
        //Recursively deallocate the linked list
        while (level_list) {
            lvlfile* next = level_list->next;
            delete level_list;
            level_list = next;
        }

        level_list = pkgman::get_levels(LEVEL_LOCAL);
    }

    void open() {
        do_open = true;
        search_query = "";
        reload_level_list();
    }

    void layout() {
        handle_do_open(&do_open, "Level Manager");

        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(UI(700., 0.));
        if (ImGui::BeginPopupModal("Level Manager", REF_TRUE, MODAL_FLAGS)) {
            bool any_level_found = false;

            //Top action bar
            {
                //Align stuff to the right
                //lvlname width + padding
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - UI(200.));

                //Actual level name field
                ImGui::PushItemWidth(UI(200.));
                if (ImGui::IsWindowAppearing())
                    ImGui::SetKeyboardFocusHere();

                ImGui::InputTextWithHint("##LvlmanLevelName", "Search levels", &search_query);
                ImGui::PopItemWidth();
            }

            ImGui::Separator();

            //Actual level list
            ImGui::BeginChild("save_list_child", UI(0., 400.), ImGuiChildFlags_NavFlattened, FRAME_FLAGS);
            if (ImGui::BeginTable("save_list", 5, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
                //Setup table columns
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Last modified", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                lvlfile *prev_level = nullptr;
                lvlfile *level = level_list;
                int num_levels = 0;
                int skipped_levels = 0;
                while (level) {
                    bool pending_reload = false;

                    //Search (lax_search is used to ignore case)
                    if ((search_query.length() > 0) && !(
                        lax_search(level->name, search_query) ||
                        (std::to_string(level->id).find(search_query) != std::string::npos)
                    )) {
                        // Just skip levels we don't like
                        level = level->next;
                        continue;
                    }

                    num_levels++;
                    if (num_levels > 5000) {
                        skipped_levels++;
                        level = level->next;
                        continue;
                    }

                    //This is required to prevent ID conflicts
                    ImGui::PushID(level->id);

                    //Start laying out the table row...
                    ImGui::TableNextRow();

                    //ID
                    if (ImGui::TableNextColumn()) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%d", level->id);
                    }

                    //Name
                    if (ImGui::TableNextColumn()) {
                        ImGui::SetNextItemWidth(999.);
                        ImGui::Text("%s", level->name);

                        //Display description if hovered
                        if (ImGui::BeginItemTooltip()) {
                            update_level_info(level->id_type, level->id);

                            if (level_metadata && level_metadata->info.descr_len && level_metadata->info.descr) {
                                ImGui::PushTextWrapPos(400);
                                ImGui::TextWrapped("%s", level_metadata->info.descr);
                                ImGui::PopTextWrapPos();
                            } else
                                ImGui::TextColored(ImVec4(.6,.6,.6,1.), "<no description>");

                            ImGui::EndTooltip();
                        }
                    }

                    //Modified date
                    if (ImGui::TableNextColumn())
                        ImGui::Text("%s", level->modified_date);

                    //Version
                    if (ImGui::TableNextColumn()) {
                        const char *version_str = level_version_string(level->version);
                        ImGui::Text("%s", version_str);
                    }

                    //Actions
                    if (ImGui::TableNextColumn()) {
                        if (ImGui::Button(pending_delete_id == level->id ? "Confirm?##delete-sandbox-level" : "Delete##delete-sandbox-level", UI(70., 0.))) {
                            if (pending_delete_id != level->id) {
                                pending_delete_id = level->id;
                            } else {
                                pending_delete_id = 0;
                                if (G->delete_level(level->id_type, level->id, level->save_id)) {
                                    //If deleting current local level, remove it's local_id
                                    //This disables the "save" option
                                    if ((level->id_type == LEVEL_LOCAL) && (level->id == W->level.local_id))
                                        W->level.local_id = 0;

                                    pending_reload = true;
                                }
                            }
                        }

                        if (pending_delete_id == level->id) {
                            ImGui::SetItemTooltip("Double-click to confirm deletion of the level. (Esc to cancel)");

                            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                                pending_delete_id = 0;
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("Open level")) {
                            P.add_action(ACTION_OPEN, level->id);
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    if (pending_reload) {
                        // delete the current level and reattach the linked list
                        if (prev_level) {
                            prev_level->next = level->next;
                            delete level;
                            level = prev_level->next;
                        } else {
                            // we're deleting the head
                            lvlfile *next_level = level->next;
                            delete level;
                            level_list = next_level;
                            level = next_level;
                        }
                    } else {
                        prev_level = level;
                        level = level->next;
                    }

                    any_level_found = true;

                    ImGui::PopID();
                }
                ImGui::EndTable();
                if (skipped_levels > 0)
                    ImGui::TextDisabled("%s", std::string("(" + std::to_string(skipped_levels) + " levels not shown - search to narrow down results)").c_str());
                else if (!any_level_found)
                    ImGui::TextDisabled("No levels found");

                ImGui::EndChild();
            }

            ImGui::EndPopup();
        }
    }
}
