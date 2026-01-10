#include "adventure.hh"
#include "faction.hh"
#include "robot_base.hh"
#include "ui_imgui.hh"

namespace UiSandboxMenu {
    static bool do_open = false;
    static b2Vec2 sb_position = b2Vec2_zero;
    static std::vector<uint32_t> bookmarks = {};

    void open() {
        do_open = true;
        sb_position = G->get_last_cursor_pos(0);
    }

    void layout() {
        handle_do_open(&do_open, "sandbox_menu");
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(225., 0.),
            ImVec2(FLT_MAX, FLT_MAX)
        );
        if (ImGui::BeginPopup("sandbox_menu", POPUP_FLAGS)) {
            //TODO keyboard shortcuts

            //True if current level can be saved as a copy
            //Saves can only be created if current level state is sandbox
            bool is_sandbox = G->state.sandbox;

            //True if already saved and the save can be updated
            //Saves can only be updated if:
            // - Current level state is sandbox
            // - Level is local (and not an auto-save)
            // - Level is already saved
            bool can_update_save =
                    G->state.sandbox &&
                    (W->level_id_type == LEVEL_LOCAL) &&
                    (W->level.local_id != 0); //&& W->level.name_len;

            //Info panel

            //Cursor:
            ImGui::Text("Cursor: (%.2f, %.2f)", sb_position.x, sb_position.y);

            //Go to menu
            if (ImGui::BeginMenu("Go to...")) {
                float z = G->cam->_position.z;

                auto goto_entity = [z](entity *ment) {
                    b2Vec2 xy = ment->get_position();
                    G->cam->set_position(xy.x, xy.y, z);
                    sb_position = xy;
                };
                auto goto_position = [z](b2Vec2 xy) {
                    G->cam->set_position(xy.x, xy.y, z);
                    sb_position = xy;
                };

                if (ImGui::MenuItem("0, 0")) {
                    goto_position(b2Vec2_zero);
                }

                if (W->is_adventure()) {
                    if (ImGui::MenuItem("Player")) {
                        if (adventure::player) {
                            goto_entity(adventure::player);
                        }
                    }
                }

                // Unimplemented: "Last camera position"
                // (functions as an undo from the last position that was gone to)

                if (ImGui::MenuItem("Last created entity")) {
                    if (!W->all_entities.empty()) {
                        goto_entity(W->all_entities.rbegin()->second);
                    }
                }

                ImGui::Separator();
                if (bookmarks.size() > 0) {
                    for (uint32_t eid : bookmarks) {
                        //TODO: remove bookmark by right clicking
                        //XXX: maybe auto remove if id is no longer valid???
                        ImGui::PushID(eid);
                        entity* ment = W->get_entity_by_id(eid);
                        if (!ment) continue;
                        //ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
                        std::string item_name = string_format("%s (id: %d)", ment->get_name(), eid);
                        bool activated = ImGui::MenuItem(item_name.c_str());
                        ImGui::SetItemTooltip(
                            "Position: (%.02f, %.02f)\n(Layer %d)",
                            // ment->get_name(),
                            // ment->g_id,
                            // eid,
                            ment->get_position().x,
                            ment->get_position().y,
                            ment->get_layer() + 1
                        );
                        if (activated) {
                            goto_entity(ment);
                        }
                        // Context menu for deleting right-clicked bookmark
                        if (ImGui::BeginPopupContextItem("BookmarkContext")) {
                            if (ImGui::MenuItem("Delete Bookmark")) {
                                bookmarks.erase(std::find(bookmarks.begin(), bookmarks.end(), eid));
                            }
                            ImGui::EndPopup();
                        }
                        //ImGui::PopItemFlag();
                        ImGui::PopID();
                    }
                } else {
                    ImGui::BeginDisabled();
                    ImGui::TextUnformatted("<no bookmarks>");
                    ImGui::EndDisabled();
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            //Selected object info:
            if (G->selection.e) {
                //If an object is selected, display it's info...
                //XXX: some of this stuff does the same things as principia ui items...
                //---- consider removal?
                entity* sent = G->selection.e;
                b2Vec2 sent_pos = sent->get_position();

                //ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
                bool is_bookmarked = std::find(bookmarks.begin(), bookmarks.end(), sent->id) != bookmarks.end();
                if (ImGui::MenuItem("Bookmark entity", NULL, &is_bookmarked)) {
                    ///XXX: this is UB if called multiple times with the same is_bookmarked value (which should never happen)
                    if (is_bookmarked) {
                        bookmarks.push_back(sent->id);
                    } else {
                        auto x = std::remove(bookmarks.begin(), bookmarks.end(), sent->id);
                        bookmarks.erase(x, bookmarks.end());
                    }
                }
                //ImGui::PopItemFlag();
                ImGui::SetItemTooltip("Save the entity to the 'Go to...' menu");

                bool already_at_cursor = sent_pos.x == sb_position.x && sent_pos.y == sb_position.y;
                ImGui::BeginDisabled(already_at_cursor);
                ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
                if (ImGui::MenuItem("Move to cursor" /*, NULL, already_at_cursor*/)) {
                    G->selection.e->set_position(sb_position);
                };
                ImGui::PopItemFlag();
                ImGui::EndDisabled();

                if (sent->is_creature() && W->is_adventure()) {
                    int adventure_id = W->level.get_adventure_id();
                    ImGui::BeginDisabled(sent->id == adventure_id);
                    if (ImGui::MenuItem("Set as player", NULL, sent->id == adventure_id)) {
                        creature *player = static_cast<class creature*>(G->selection.e);
                        if (player->is_robot()) {
                            robot_base *r = static_cast<class robot_base*>(player);
                            r->set_faction(FACTION_FRIENDLY);
                        }
                        W->level.set_adventure_id(player->id);
                        G->state.adventure_id = player->id;
                        adventure::player = player;
                    }
                    ImGui::EndDisabled();
                }

                ImGui::Separator();
            }

            //"Level properties"
            if (ImGui::MenuItem("Level properties")) {
                UiLevelProperties::open();
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("New level")) {
                UiNewLevel::open();
                ImGui::CloseCurrentPopup();
            }

            //"Save": update current save
            if (can_update_save && ImGui::MenuItem("Save")) {
                P.add_action(ACTION_SAVE, 0);
                ImGui::CloseCurrentPopup();
            }

            //"Save as...": create a new save
            if (is_sandbox && ImGui::MenuItem("Save copy")) {
                //TODO
                UiSave::open();
                ImGui::CloseCurrentPopup();
            }

            // Open the Level Manager
            if (ImGui::MenuItem("Open")) {
                UiLevelManager::open();
                ImGui::CloseCurrentPopup();
            }

            //"Publish online"
            if (is_sandbox) {
                ImGui::BeginDisabled(!P.user_id);
                ImGui::MenuItem("Publish online");
                ImGui::EndDisabled();
            }

            if (P.user_id && P.username) {
                // blah
            } else {
                if (ImGui::MenuItem("Log in")) {
                    UiLogin::open();
                };
            }

            if (ImGui::MenuItem("Settings")) {
                UiSettings::open();
            }

            if (ImGui::MenuItem("Back to menu")) {
                P.add_action(ACTION_GOTO_MAINMENU, 0);
            }

            if (ImGui::MenuItem("Help: Principia Wiki")) {
                ui::open_url("https://principia-web.se/wiki/");
            }

            if (ImGui::MenuItem("Help: Getting Started")) {
                ui::open_url("https://principia-web.se/wiki/Getting_Started");
            }

            ImGui::EndMenu();
        }
    }
}
