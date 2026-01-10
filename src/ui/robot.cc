#include "faction.hh"
#include "item.hh"
#include "robot_base.hh"
#include "ui_imgui.hh"

#include <sstream>

namespace UiRobot {
	static std::vector<uint32_t> equipment;
    static bool do_open = false;
    static bool needs_refresh = true;
    static entity* last_selected = nullptr;

    void open() {
        entity* e = G->selection.e;

        last_selected = e;
        needs_refresh = false;

        if (e->properties[ROBOT_PROPERTY_HEAD].v.i8 < 0 || e->properties[ROBOT_PROPERTY_HEAD].v.i8 >= NUM_HEAD_TYPES) {
            e->properties[ROBOT_PROPERTY_HEAD].v.i8 = 0;
        }
        if (e->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8 < 0 || e->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8 >= NUM_HEAD_EQUIPMENT_TYPES) {
            e->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8 = 0;
        }
        if (e->properties[ROBOT_PROPERTY_BACK].v.i8 < 0 || e->properties[ROBOT_PROPERTY_BACK].v.i8 >= NUM_BACK_EQUIPMENT_TYPES) {
            e->properties[ROBOT_PROPERTY_BACK].v.i8 = 0;
        }
        if (e->properties[ROBOT_PROPERTY_FRONT].v.i8 < 0 || e->properties[ROBOT_PROPERTY_FRONT].v.i8 >= NUM_FRONT_EQUIPMENT_TYPES) {
            e->properties[ROBOT_PROPERTY_FRONT].v.i8 = 0;
        }
        if (e->properties[ROBOT_PROPERTY_FEET].v.i8 < 0 || e->properties[ROBOT_PROPERTY_FEET].v.i8 >= NUM_FEET_TYPES) {
            e->properties[ROBOT_PROPERTY_FEET].v.i8 = 0;
        }
        if (e->properties[ROBOT_PROPERTY_BOLT_SET].v.i8 < 0 || e->properties[ROBOT_PROPERTY_BOLT_SET].v.i8 >= NUM_BOLT_SETS) {
            e->properties[ROBOT_PROPERTY_BOLT_SET].v.i8 = 0;
        }

        if (e->properties[ROBOT_PROPERTY_STATE].v.i8 < CREATURE_IDLE || e->properties[ROBOT_PROPERTY_STATE].v.i8 > CREATURE_DEAD) {
            e->properties[ROBOT_PROPERTY_STATE].v.i8 = CREATURE_IDLE;
        }
        if (e->properties[ROBOT_PROPERTY_ROAMING].v.i8 != 0 && e->properties[ROBOT_PROPERTY_ROAMING].v.i8 != 1) {
            e->properties[ROBOT_PROPERTY_ROAMING].v.i8 = 0;
        }
        if (e->properties[ROBOT_PROPERTY_DIR].v.i8 < 0 || e->properties[ROBOT_PROPERTY_DIR].v.i8 > 2) {
            e->properties[ROBOT_PROPERTY_DIR].v.i8 = 1;
        }
        if (e->properties[ROBOT_PROPERTY_FACTION].v.i8 < 0 || e->properties[ROBOT_PROPERTY_FACTION].v.i8 >= NUM_FACTIONS) {
            e->properties[ROBOT_PROPERTY_FACTION].v.i8 = FACTION_ENEMY;
        }

        equipment.clear();
        if (e->properties[ROBOT_PROPERTY_EQUIPMENT].v.s.buf) {
            std::vector<char*> eq_parts = p_split(e->properties[ROBOT_PROPERTY_EQUIPMENT].v.s.buf, e->properties[ROBOT_PROPERTY_EQUIPMENT].v.s.len, ";");
            for (char* part : eq_parts) {
                uint32_t item_id = atoi(part);
                if (item_id < NUM_ITEMS) {
                    equipment.push_back(item_id);
                }
            }
        }

        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "Robot Settings:");
        if (ImGui::BeginPopupModal("Robot Settings:", nullptr, MODAL_FLAGS)) {
            entity* e = G->selection.e;
            if (e != last_selected) {
                last_selected = e;
                needs_refresh = true;
            }
            creature* c = static_cast<creature*>(e);
            // Default State
            bool isAdventure = (e->id == G->state.adventure_id && W->is_adventure());
            if (isAdventure) {
                ImGui::BeginDisabled();
            }

            ImGui::Text("Default State");
            int state = e->properties[ROBOT_PROPERTY_STATE].v.i8;
            if (ImGui::RadioButton("Idle", &state, CREATURE_IDLE)) {
                e->properties[ROBOT_PROPERTY_STATE].v.i8 = static_cast<uint8_t>(state);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Walking", &state, CREATURE_WALK)) {
                e->properties[ROBOT_PROPERTY_STATE].v.i8 = static_cast<uint8_t>(state);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Dead", &state, CREATURE_DEAD)) {
                e->properties[ROBOT_PROPERTY_STATE].v.i8 = static_cast<uint8_t>(state);
            }
            ImGui::SeparatorText("");

            // Roaming
            bool roaming = e->properties[ROBOT_PROPERTY_ROAMING].v.i8 != 0;
            if (ImGui::Checkbox("Roaming", &roaming)) {
                e->properties[ROBOT_PROPERTY_ROAMING].v.i8 = roaming ? 1 : 0;
            }

            // Initial Direction
            ImGui::SeparatorText("Initial Direction");
            int direction = e->properties[ROBOT_PROPERTY_DIR].v.i8;
            if (ImGui::RadioButton("Left", &direction, 0)) {
                e->properties[ROBOT_PROPERTY_DIR].v.i8 = static_cast<uint8_t>(direction);
                ((robot_base*)e)->set_i_dir(DIR_LEFT);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Random", &direction, 1)) {
                e->properties[ROBOT_PROPERTY_DIR].v.i8 = static_cast<uint8_t>(direction);
                ((robot_base*)e)->set_i_dir(0.f);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Right", &direction, 2)) {
                e->properties[ROBOT_PROPERTY_DIR].v.i8 = static_cast<uint8_t>(direction);
                ((robot_base*)e)->set_i_dir(DIR_RIGHT);
            }
            ImGui::Separator();
            if (isAdventure) {
                ImGui::EndDisabled();
            }

            // Faction
            ImGui::SeparatorText("Faction");
            int faction = e->properties[ROBOT_PROPERTY_FACTION].v.i8;
            for (int x = 0; x < NUM_FACTIONS; ++x) {
                if (ImGui::RadioButton(factions[x].name, &faction, x)) {
                    e->properties[ROBOT_PROPERTY_FACTION].v.i8 = static_cast<uint8_t>(faction);
                    ((robot_base*)e)->set_faction(faction);
                }
            }
            if (faction >= NUM_FACTIONS) {
                e->properties[ROBOT_PROPERTY_FACTION].v.i8 = FACTION_ENEMY;
                ((robot_base*)e)->set_faction(FACTION_ENEMY);
            }

            // Equipment
            ImGui::SeparatorText("Equipment");
            auto item_cb_append = [&e](const char* labelPrefix, uint8_t* equipment, int numTypes, const int* itemArray, bool hasFeature = true) {
                if (!hasFeature) return;
                int globalItemId = (*equipment < numTypes) ? itemArray[*equipment] : 0;
                std::string currentLabel = item::get_ui_name(globalItemId);
                if (currentLabel.empty() || globalItemId == 0) {
                    currentLabel = "None";
                }
                if (ImGui::BeginCombo(labelPrefix, currentLabel.c_str())) {
                    if (ImGui::Selectable("None", *equipment == 0)) {
                        *equipment = 0;
                        needs_refresh = true;
                    }
                    for (int i = 0; i < numTypes; ++i) {
                        int itemId = itemArray[i];
                        if (itemId <= 0 || itemId >= NUM_ITEMS) {
                            continue;
                        }
                        const char* label = item::get_ui_name(itemId);
                        if (label == nullptr || strlen(label) == 0) {
                            label = "Unknown Item";
                        }
                        bool selected = (globalItemId == itemId);
                        if (ImGui::Selectable(label, selected)) {
                            *equipment = static_cast<uint8_t>(i);
                            needs_refresh = true;
                        }
                    }
                    ImGui::EndCombo();
                }
            };
            if (c->has_feature(CREATURE_FEATURE_HEAD)) {
                item_cb_append("Head", &e->properties[ROBOT_PROPERTY_HEAD].v.i8, NUM_HEAD_TYPES, _head_to_item);
                item_cb_append("Head Equipment", &e->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8, NUM_HEAD_EQUIPMENT_TYPES, _head_equipment_to_item);
            } else {
                ImGui::BeginDisabled();
                item_cb_append("Head", &e->properties[ROBOT_PROPERTY_HEAD].v.i8, NUM_HEAD_TYPES, _head_to_item, false);
                item_cb_append("Head Equipment", &e->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8, NUM_HEAD_EQUIPMENT_TYPES, _head_equipment_to_item, false);
                ImGui::EndDisabled();
            }
            if (c->has_feature(CREATURE_FEATURE_BACK_EQUIPMENT)) {
                item_cb_append("Back Equipment", &e->properties[ROBOT_PROPERTY_BACK].v.i8, NUM_BACK_EQUIPMENT_TYPES, _back_to_item);
            } else {
                ImGui::BeginDisabled();
                item_cb_append("Back Equipment", &e->properties[ROBOT_PROPERTY_BACK].v.i8, NUM_BACK_EQUIPMENT_TYPES, _back_to_item, false);
                ImGui::EndDisabled();
            }
            if (c->has_feature(CREATURE_FEATURE_FRONT_EQUIPMENT)) {
                item_cb_append("Front Equipment", &e->properties[ROBOT_PROPERTY_FRONT].v.i8, NUM_FRONT_EQUIPMENT_TYPES, _front_to_item);
            } else {
                ImGui::BeginDisabled();
                item_cb_append("Front Equipment", &e->properties[ROBOT_PROPERTY_FRONT].v.i8, NUM_FRONT_EQUIPMENT_TYPES, _front_to_item, false);
                ImGui::EndDisabled();
            }
            item_cb_append("Feet", &e->properties[ROBOT_PROPERTY_FEET].v.i8, NUM_FEET_TYPES, _feet_to_item);
            item_cb_append("Bolt Set", &e->properties[ROBOT_PROPERTY_BOLT_SET].v.i8, NUM_BOLT_SETS, _bolt_to_item);

            if (needs_refresh) {
                equipment.clear();
                if (e->properties[ROBOT_PROPERTY_EQUIPMENT].v.s.buf) {
                    std::vector<char*> eq_parts = p_split(e->properties[ROBOT_PROPERTY_EQUIPMENT].v.s.buf, e->properties[ROBOT_PROPERTY_EQUIPMENT].v.s.len, ";");
                    for (char* part : eq_parts) {
                        uint32_t item_id = atoi(part);
                        if (item_id < NUM_ITEMS && item_id != 0 && item_id != 1) {
                            equipment.push_back(item_id);
                        }
                    }
                }
                needs_refresh = false;
            }

            ImGui::SeparatorText("Equipped items");
            if (ImGui::BeginListBox("##EquipmentList", ImVec2(0, ImGui::GetTextLineHeight() * 10))) {
                for (int x = 0; x < NUM_ITEMS; ++x) {
                    struct item_option* io = &item_options[x];
                    if (io->category != ITEM_CATEGORY_WEAPON &&
                        io->category != ITEM_CATEGORY_TOOL &&
                        io->category != ITEM_CATEGORY_CIRCUIT) {
                        continue;
                    }
                    bool equipped = std::find(equipment.begin(), equipment.end(), x) != equipment.end();
                    std::string label = item::get_ui_name(x);
                    if (equipped) {
                        label += " (Equipped)";
                    }
                    if (ImGui::Selectable(label.c_str())) {
                        if (equipped) {
                            equipment.erase(std::remove(equipment.begin(), equipment.end(), x), equipment.end());
                        } else {
                            equipment.push_back(x);
                        }
                    }
                }
                ImGui::EndListBox();
            }

            // Buttons
            ImGui::SeparatorText("");
            if (ImGui::Button("Apply")) {
                std::vector<int> equippedItems;

                auto push_if_valid = [&](uint8_t typeSpecificId, const int* itemArray, int numTypes) {
                    if (typeSpecificId < numTypes && typeSpecificId > 0) {
                        int globalItemId = itemArray[typeSpecificId];
                        if (globalItemId > 0 && globalItemId < NUM_ITEMS) {
                            equippedItems.push_back(globalItemId);
                        }
                    }
                };

                push_if_valid(e->properties[ROBOT_PROPERTY_HEAD].v.i8, _head_to_item, NUM_HEAD_TYPES);
                push_if_valid(e->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8, _head_equipment_to_item, NUM_HEAD_EQUIPMENT_TYPES);
                push_if_valid(e->properties[ROBOT_PROPERTY_BACK].v.i8, _back_to_item, NUM_BACK_EQUIPMENT_TYPES);
                push_if_valid(e->properties[ROBOT_PROPERTY_FRONT].v.i8, _front_to_item, NUM_FRONT_EQUIPMENT_TYPES);
                push_if_valid(e->properties[ROBOT_PROPERTY_FEET].v.i8, _feet_to_item, NUM_FEET_TYPES);
                push_if_valid(e->properties[ROBOT_PROPERTY_BOLT_SET].v.i8, _bolt_to_item, NUM_BOLT_SETS);
                for (uint32_t itemId : equipment) {
                    if (itemId < NUM_ITEMS && itemId != 0 && itemId != 1) {
                        equippedItems.push_back(itemId);
                    }
                }

                std::stringstream ss;
                for (size_t i = 0; i < equippedItems.size(); ++i) {
                    if (i > 0) ss << ";";
                    ss << equippedItems[i];
                }
                e->set_property(ROBOT_PROPERTY_EQUIPMENT, ss.str().c_str());
                ui::message("Robot properties saved!");
                P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
                P.add_action(ACTION_RESELECT, 0);
                W->add_action(e->id, ACTION_CALL_ON_LOAD);
                ImGui::CloseCurrentPopup();
                last_selected = nullptr;
                needs_refresh = true;
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                last_selected = nullptr;
                needs_refresh = true;
            }
            ImGui::EndPopup();
        }
    }
}
