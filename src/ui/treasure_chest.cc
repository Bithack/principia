#include "treasure_chest.hh"
#include "item.hh"
#include "ui_imgui.hh"

#include <string>
#include <sstream>

namespace UiTreasureChest {
	static bool do_open = false;
    static int selected_index = -1;
    static int selected_entity = 0;
    static int selected_sub_entity = 0;
    static int selected_count = 1;

    struct ChestItem {
        int g_id;
        int sub_id;
        int count;
    };

    static std::vector<ChestItem> chest_items = {};

    void open() {
        do_open = true;
        entity* e = G->selection.e;
        chest_items.clear();
        selected_index = -1;

        if (e && e->g_id == O_TREASURE_CHEST) {
            char* str = strdup(e->properties[0].v.s.buf);
            std::vector<treasure_chest_item> parsed_items = treasure_chest::parse_items(str);
            free(str);
            for (const auto& item : parsed_items) {
                chest_items.push_back({ item.g_id, item.sub_id, item.count });
            }
        }
    }

    void layout() {
        handle_do_open(&do_open, "Treasure chest");
        if (ImGui::BeginPopupModal("Treasure chest", REF_TRUE, MODAL_FLAGS)) {
            entity* e = G->selection.e;

            // Entities
            std::vector<std::string> entity_labels;
            std::vector<const char*> entity_label_ptrs;
            for (const auto& obj : menu_objects) {
                if (obj.e != nullptr) {
                    entity_labels.push_back(obj.e->get_name());
                }
            }
            for (auto& label : entity_labels) {
                entity_label_ptrs.push_back(label.c_str());
            }
            if (!entity_label_ptrs.empty()) {
                static int prev_selected_entity = -1;
                if (ImGui::Combo("##Entity", &selected_entity, entity_label_ptrs.data(), (int)entity_label_ptrs.size())) {
                    selected_sub_entity = 0;
                    prev_selected_entity = selected_entity;
                }
            }
            else {
                ImGui::TextDisabled("No entities");
            }

            // Sub-Entities
            std::vector<std::string> sub_entity_labels;
            std::vector<const char*> sub_entity_ptrs;
            if (selected_entity >= 0 && selected_entity < menu_objects.size()) {
                entity* selected_entity_ptr = menu_objects[selected_entity].e;
                if (selected_entity_ptr) {
                    int g_id = selected_entity_ptr->g_id;

                    if (g_id == O_ITEM) {
                        for (int i = 0; i < NUM_ITEMS; ++i) {
                            sub_entity_labels.emplace_back(item_options[i].name);
                        }
                    } else if (g_id == O_RESOURCE) {
                        for (int i = 0; i < NUM_RESOURCES; ++i) {
                            sub_entity_labels.emplace_back(resource_data[i].name);
                        }
                    }

                    for (auto& label : sub_entity_labels) {
                        sub_entity_ptrs.push_back(label.c_str());
                    }

                    if (!sub_entity_ptrs.empty()) {
                        ImGui::Combo("##SubEntity", &selected_sub_entity, sub_entity_ptrs.data(), (int)sub_entity_ptrs.size());
                    } else {
                        ImGui::TextDisabled("No sub-entities");
                    }
                }
            }

            // (consider adding amount limit)
            ImGui::InputInt("Amount", &selected_count);
            if (selected_count < 1){
                selected_count = 1;
            }

            static std::vector<treasure_chest_item> parsed_items;
            parsed_items.clear();
            if (e && e->g_id == O_TREASURE_CHEST) {
                char* str = strdup(e->properties[0].v.s.buf);
                parsed_items = treasure_chest::parse_items(str);
                free(str);
            }

            ImGui::Separator();
            if (ImGui::Button("Add entity")) {
                if (selected_entity >= 0 && selected_entity < menu_objects.size()) {
                    entity* selected_entity_ptr = menu_objects[selected_entity].e;
                    if (selected_entity_ptr) {
                        chest_items.push_back({ selected_entity_ptr->g_id, selected_sub_entity, selected_count });
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove selected")) {
                if (selected_index >= 0 && selected_index < chest_items.size()) {
                    chest_items.erase(chest_items.begin() + selected_index);
                    selected_index = -1;
                }
            }

            // Table
            ImGui::Separator();
            if (ImGui::BeginTable("ChestItems", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Entity");
                ImGui::TableSetupColumn("Sub-Entity");
                ImGui::TableSetupColumn("Count");
                ImGui::TableHeadersRow();

                for (int i = 0; i < chest_items.size(); ++i) {
                    auto& item = chest_items[i];
                    ImGui::TableNextRow();

                    const char* g_name = "Unknown";
                    const char* sub_name = "-";

                    switch (item.g_id) {
                        case O_ITEM:
                            g_name = "Item";
                            if (item.sub_id >= 0 && item.sub_id < NUM_ITEMS) {
                                sub_name = item_options[item.sub_id].name;
                            }
                            break;

                        case O_RESOURCE:
                            g_name = "Resource";
                            if (item.sub_id >= 0 && item.sub_id < NUM_RESOURCES) {
                                sub_name = resource_data[item.sub_id].name;
                            }
                            break;

                        default: {
                            // (no sub-entity)
                            entity* e = of::create(item.g_id);
                            if (e) {
                                g_name = e->get_name();
                                delete e;
                            }
                            break;
                        }
                    }

                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushID(i);
                    bool is_selected = (i == selected_index);
                    if (ImGui::RadioButton(g_name, is_selected)) {
                        selected_index = i;
                    }
                    ImGui::PopID();
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(sub_name);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%d", item.count);
                }

                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::SeparatorText("");
            if (ImGui::Button("Apply")) {
                if (e && e->g_id == O_TREASURE_CHEST) {
                    treasure_chest* tc = static_cast<treasure_chest*>(e);
                    std::stringstream ss;
                    for (size_t i = 0; i < chest_items.size(); ++i) {
                        if (i > 0) ss << ";";
                        ss << chest_items[i].g_id << ":" << chest_items[i].sub_id << ":" << chest_items[i].count;
                    }
                    tc->set_property(0, ss.str().c_str());
                    P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
                    P.add_action(ACTION_RESELECT, 0);
                }
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}
