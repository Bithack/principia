#include "ui_imgui.hh"

namespace UiLevelManager {
    struct lvlinfo_ext {
        lvlinfo info;
        uint32_t id;
        int type;
    };

    static bool do_open = false;
    static std::string search_query{""};

    static lvlfile *level_list = nullptr;
    static int level_list_type = LEVEL_LOCAL;

    static lvlinfo_ext *level_metadata = nullptr;
    static tms_texture *level_icon;

    static void upload_level_icon() {
        tms_texture_load_mem(level_icon, (const char*) &level_metadata->info.icon, 128, 128, 1);
        tms_texture_upload(level_icon);
    }

    static int update_level_info(int id_type, uint32_t id) {
        if (level_metadata) {
            //Check if data needs to be reloaded
            if ((level_metadata->id == id) && (level_metadata->type == id_type)) return 0;

            //Dealloc current data
            level_metadata->info.~lvlinfo();
            free(level_metadata);
        }

        level_metadata = new lvlinfo_ext;

        //Update meta
        level_metadata->id = id;
        level_metadata->type = id_type;

        //Read level info
        lvledit lvl;
        if (lvl.open(id_type, id)) {
            level_metadata->info = lvl.lvl;
            if (level_metadata->info.descr_len && level_metadata->info.descr) {
                level_metadata->info.descr = strdup(level_metadata->info.descr);
            }
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
        //Get a new list of levels
        level_list = pkgman::get_levels(level_list_type);
    }

    void init() {
        level_icon = tms_texture_alloc();
    }

    void open() {
        do_open = true;
        search_query = "";
        level_list_type = LEVEL_LOCAL;
        reload_level_list();
    }

    void layout() {
        ImGuiIO& io = ImGui::GetIO();
        handle_do_open(&do_open, "Level Manager");
        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(ImVec2(800., 0.));
        if (ImGui::BeginPopupModal("Level Manager", REF_TRUE, MODAL_FLAGS)) {
            bool any_level_found = false;

            //Top action bar
            {
                //Align stuff to the right
                //lvlname width + padding
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 200.);

                //Actual level name field
                ImGui::PushItemWidth(200.);
                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere();
                }
                ImGui::InputTextWithHint("##LvlmanLevelName", "Search levels", &search_query);
                ImGui::PopItemWidth();
            }

            ImGui::Separator();

            //Actual level list
            ImGui::BeginChild("save_list_child", ImVec2(0., 500.), ImGuiChildFlags_NavFlattened, FRAME_FLAGS);
            if (ImGui::BeginTable("save_list", 5, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
                //Setup table columns
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Last modified", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                lvlfile *level = level_list;
                while (level) {
                    //Search (lax_search is used to ignore case)
                    if ((search_query.length() > 0) && !(
                        lax_search(level->name, search_query) ||
                        (std::to_string(level->id).find(search_query) != std::string::npos)
                    )) {
                        //Just skip levels we don't like
                        level = level->next;
                        continue;
                    }

                    //This is required to prevent ID conflicts
                    ImGui::PushID(level->id);

                    //Start laying out the table row...
                    ImGui::TableNextRow();

                    //ID
                    if (ImGui::TableNextColumn()) {
                        ImGui::Text("%d", level->id);
                    }

                    //Name
                    if (ImGui::TableNextColumn()) {
                        ImGui::SetNextItemWidth(999.);
                        ImGui::LabelText("##levelname", "%s", level->name);

                        //Display description if hovered
                        if (ImGui::BeginItemTooltip()) {
                            update_level_info(level->id_type, level->id);

                            if (!level_metadata) {
                                ImGui::TextColored(ImVec4(1.,.3,.3,1.), "Failed to load level metadata");
                            } else if (level_metadata->info.descr_len && level_metadata->info.descr) {
                                ImGui::PushTextWrapPos(400);
                                ImGui::TextWrapped("%s", level_metadata->info.descr);
                                ImGui::PopTextWrapPos();
                            } else {
                                ImGui::TextColored(ImVec4(.6,.6,.6,1.), "<no description>");
                            }
                            ImGui::EndTooltip();
                        }
                    }

                    //Modified date
                    if (ImGui::TableNextColumn()) {
                        ImGui::TextUnformatted(level->modified_date);
                    }

                    //Version
                    if (ImGui::TableNextColumn()) {
                        const char* version_str = level_version_string(level->version);
                        ImGui::Text("%s", version_str);
                    }

                    //Actions
                    if (ImGui::TableNextColumn()) {
                        // Delete level ---
                        // To prevent accidental level deletion,
                        // Shift must be held while clicking the button
                        bool allow_delete = io.KeyShift;
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, allow_delete ? 1. : .6);
                        if (ImGui::Button("Delete##delete-sandbox-level")) {
                            if (allow_delete && G->delete_level(level->id_type, level->id, level->save_id)) {
                                //If deleting current local level, remove it's local_id
                                //This disables the "save" option
                                if ((level->id_type == LEVEL_LOCAL) && (level->id == W->level.local_id))
                                    W->level.local_id = 0;

                                //Reload the list of levels
                                reload_level_list();
                            }
                        }
                        ImGui::PopStyleVar();
                        if (!allow_delete) ImGui::SetItemTooltip("Hold Shift to unlock");

                        // Open level ---
                        ImGui::SameLine();
                        if (ImGui::Button("Open level")) {
                            P.add_action(ACTION_OPEN, level->id);
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    level = level->next;
                    any_level_found = true;

                    ImGui::PopID();
                }
                ImGui::EndTable();
                if (!any_level_found) {
                    ImGui::TextUnformatted("No levels found");
                }
                ImGui::EndChild();
            }
            ImGui::EndPopup();
        }
    }
}
