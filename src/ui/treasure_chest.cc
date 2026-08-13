#include "treasure_chest.hh"
#include "game.hh"
#include "imgui.hh"
#include "item.hh"
#include "main.hh"
#include "resource.hh"
#include <sstream>
#include <string>

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

    void apply_properties() {
        entity* e = G->selection.e;
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

    void add_entity() {
        if (selected_entity < 0 || selected_entity >= menu_objects.size())
            return;

        entity* selected_entity_ptr = menu_objects[selected_entity].e;
        if (!selected_entity_ptr)
            return;

        // check if item already exists, and if so just add the count to that entry
        for (auto& item : chest_items) {
            if (item.g_id == selected_entity_ptr->g_id && item.sub_id == selected_sub_entity) {
                item.count += selected_count;
                return;
            }
        }

        chest_items.push_back({
            selected_entity_ptr->g_id,
            selected_sub_entity,
            selected_count
        });
    }

    void open() {
        do_open = true;
        entity* e = G->selection.e;
        chest_items.clear();
        selected_index = -1;

        if (e && e->g_id == O_TREASURE_CHEST) {
            char* str = strdup(e->properties[0].v.s.buf);
            std::vector<treasure_chest_item> parsed_items = treasure_chest::parse_items(str);
            free(str);
            for (const auto& item : parsed_items)
                chest_items.push_back({ item.g_id, item.sub_id, item.count });
        }
    }

    void layout() {
        handle_do_open(&do_open, "Treasure chest");

        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(UI(300., 0.));
        if (ImGui::BeginPopupModal("Treasure chest", REF_TRUE, MODAL_FLAGS)) {
            entity* e = G->selection.e;

            // Entities
            std::vector<std::string> entity_labels;
            std::vector<const char*> entity_label_ptrs;
            for (const auto& obj : menu_objects)
                if (obj.e != nullptr)
                    entity_labels.push_back(obj.e->get_name());

            for (auto& label : entity_labels)
                entity_label_ptrs.push_back(label.c_str());

            static int prev_selected_entity = -1;
            if (ImGui::Combo("##Entity", &selected_entity, entity_label_ptrs.data(), (int)entity_label_ptrs.size())) {
                selected_sub_entity = 0;
                prev_selected_entity = selected_entity;
            }

            // Sub-Entities
            std::vector<std::string> sub_entity_labels;
            std::vector<const char*> sub_entity_ptrs;
            if (selected_entity >= 0 && selected_entity < menu_objects.size() && menu_objects[selected_entity].e) {
                entity* selected_entity_ptr = menu_objects[selected_entity].e;
                int g_id = selected_entity_ptr->g_id;

                if (g_id == O_ITEM) {
                    for (int i = 0; i < NUM_ITEMS; ++i)
                        sub_entity_labels.emplace_back(item_options[i].name);
                } else if (g_id == O_RESOURCE) {
                    for (int i = 0; i < NUM_RESOURCES; ++i)
                        sub_entity_labels.emplace_back(resource_data[i].name);
                }

                for (auto& label : sub_entity_labels)
                    sub_entity_ptrs.push_back(label.c_str());

                if (!sub_entity_ptrs.empty()) {
                    ImGui::Combo("##SubEntity", &selected_sub_entity, sub_entity_ptrs.data(), (int)sub_entity_ptrs.size());
                } else {
                    ImGui::BeginDisabled();
                    int dummy = 0;
                    const char* no_sub_entity[] = {"- No sub-entities -"};
                    ImGui::Combo("##SubEntity", &dummy, no_sub_entity, 1);
                    ImGui::EndDisabled();
                }
            }

            // (consider adding amount limit)
            ImGui::InputInt("Amount", &selected_count);
            selected_count = tclampf(selected_count, 1, 9999);

            ImGui::Separator();
            if (ImGui::Button("Add entity"))
                add_entity();

            if (selected_index >= 0 && selected_index < chest_items.size()) {
                ImGui::SameLine();
                if (ImGui::Button("Remove selected")) {
                    chest_items.erase(chest_items.begin() + selected_index);
                    selected_index = -1;
                }
            }

            // Table
            ImGui::Separator();
            ImGui::BeginChild("###treasure_items", UI(0., 175.), false);
            if (ImGui::BeginTable("ChestItems", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Entity", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, UI(45.0f));
                ImGui::TableHeadersRow();

                for (int i = 0; i < chest_items.size(); ++i) {
                    auto& item = chest_items[i];
                    ImGui::TableNextRow();

                    std::string obj_name;
                    if (item.g_id == O_ITEM)
                        obj_name = std::string(item_options[item.sub_id].name) + " (Item)";
                    else if (item.g_id == O_RESOURCE)
                        obj_name = std::string(resource_data[item.sub_id].name) + " (Resource)";
                    else
                        obj_name = std::string(of::get_object_name_by_gid(item.g_id));

                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushID(i);
                    if (ImGui::RadioButton(obj_name.c_str(), (i == selected_index)))
                        selected_index = i;

                    ImGui::PopID();

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", item.count);
                }

                ImGui::EndTable();
            }
            ImGui::EndChild();

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
