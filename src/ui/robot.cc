#include "faction.hh"
#include "game.hh"
#include "imgui.hh"
#include "item.hh"
#include "main.hh"
#include "robot_base.hh"
#include "ui.hh"
#include <sstream>

namespace UiRobot {
    static bool do_open = false;

    static int default_state = 0;
    static bool roaming = false;
    static int initial_dir = 0;
    static int faction = 0;

    static uint8_t head;
    static uint8_t head_equipment;
    static uint8_t back_equipment;
    static uint8_t front_equipment;
    static uint8_t feet;
    static uint8_t bolt_set;

    static std::vector<uint32_t> equipment;

    void apply_properties() {
        entity* e = G->selection.e;

        if (!e || !e->flag_active(ENTITY_IS_ROBOT))
            return;

        creature* c = static_cast<creature*>(e);

        default_state = tclampf(default_state, CREATURE_IDLE, CREATURE_DEAD);
        e->properties[ROBOT_PROPERTY_STATE].v.i8 = default_state;

        e->properties[ROBOT_PROPERTY_ROAMING].v.i8 = roaming ? 1 : 0;

        initial_dir = tclampf(initial_dir, 0, 2);
        e->properties[ROBOT_PROPERTY_DIR].v.i8 = initial_dir;

        if (initial_dir == 1)
            ((robot_base*)e)->set_i_dir(DIR_LEFT);
        else if (initial_dir == 0)
            ((robot_base*)e)->set_i_dir(0.f);
        else if (initial_dir == 2)
            ((robot_base*)e)->set_i_dir(DIR_RIGHT);

        faction = tclampf(faction, 0, NUM_FACTIONS - 1);
        e->properties[ROBOT_PROPERTY_FACTION].v.i8 = faction;
        ((robot_base*)e)->set_faction(faction);

        if (c->has_feature(CREATURE_FEATURE_HEAD)) {
            e->properties[ROBOT_PROPERTY_HEAD].v.i8 = tclampf(head, 0, NUM_HEAD_TYPES - 1);
            e->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8 = tclampf(head_equipment, 0, NUM_HEAD_EQUIPMENT_TYPES - 1);
        }

        if (c->has_feature(CREATURE_FEATURE_BACK_EQUIPMENT))
            e->properties[ROBOT_PROPERTY_BACK].v.i8 = tclampf(back_equipment, 0, NUM_BACK_EQUIPMENT_TYPES - 1);

        if (c->has_feature(CREATURE_FEATURE_FRONT_EQUIPMENT))
            e->properties[ROBOT_PROPERTY_FRONT].v.i8 = tclampf(front_equipment, 0, NUM_FRONT_EQUIPMENT_TYPES - 1);

        e->properties[ROBOT_PROPERTY_FEET].v.i8 = tclampf(feet, 0, NUM_FEET_TYPES - 1);
        e->properties[ROBOT_PROPERTY_BOLT_SET].v.i8 = tclampf(bolt_set, 0, NUM_BOLT_SETS - 1);

        std::stringstream ss;
        for (size_t i = 0; i < equipment.size(); ++i) {
            if (i > 0) ss << ";";
            ss << equipment[i];
        }
        e->set_property(ROBOT_PROPERTY_EQUIPMENT, ss.str().c_str());

        ui::message("Robot properties saved!");

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);

        W->add_action(e->id, ACTION_CALL_ON_LOAD);

        ImGui::CloseCurrentPopup();
    }

    void open() {
        entity* e = G->selection.e;

        default_state = e->properties[ROBOT_PROPERTY_STATE].v.i8;
        roaming = e->properties[ROBOT_PROPERTY_ROAMING].v.i8 != 0;
        initial_dir = e->properties[ROBOT_PROPERTY_DIR].v.i8;
        faction = e->properties[ROBOT_PROPERTY_FACTION].v.i8;

        head = e->properties[ROBOT_PROPERTY_HEAD].v.i8;
        head_equipment = e->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8;
        back_equipment = e->properties[ROBOT_PROPERTY_BACK].v.i8;
        front_equipment = e->properties[ROBOT_PROPERTY_FRONT].v.i8;
        feet = e->properties[ROBOT_PROPERTY_FEET].v.i8;
        bolt_set = e->properties[ROBOT_PROPERTY_BOLT_SET].v.i8;

        equipment.clear();
        if (e->properties[ROBOT_PROPERTY_EQUIPMENT].v.s.buf) {
            std::vector<char*> eq_parts = p_split(e->properties[ROBOT_PROPERTY_EQUIPMENT].v.s.buf, e->properties[ROBOT_PROPERTY_EQUIPMENT].v.s.len, ";");
            for (char* part : eq_parts) {
                uint32_t item_id = atoi(part);
                if (item_id < NUM_ITEMS)
                    equipment.push_back(item_id);
            }
        }

        do_open = true;
    }

    void column_one() {
        entity *e = G->selection.e;

        if (!(e->id == G->state.adventure_id && W->is_adventure())) {
            ImGui::Text("Default State");
            ImGui::RadioButton("Idle", &default_state, CREATURE_IDLE);
            ImGui::SameLine();
            ImGui::RadioButton("Walking", &default_state, CREATURE_WALK);
            ImGui::SameLine();
            ImGui::RadioButton("Dead", &default_state, CREATURE_DEAD);

            ImGui::Checkbox("Roaming", &roaming);

            // Initial Direction
            ImGui::SeparatorText("Initial Direction");
            ImGui::RadioButton("Left", &initial_dir, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Random", &initial_dir, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Right", &initial_dir, 2);

            ImGui::SeparatorText("Faction");
        } else {
            ImGui::Text("Faction");
        }

        // Faction
        if (ImGui::BeginCombo("##faction", factions[faction].name)) {
            for (int i = 0; i < sizeof(factions) / sizeof(factions[0]); ++i) {
                bool is_selected = (faction == i);
                if (ImGui::Selectable(factions[i].name, is_selected))
                    faction = i;

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Equipment
        ImGui::SeparatorText("Equipment");
        auto item_cb_append = [&e](const char* labelPrefix, uint8_t* equipment, int numTypes, const int* itemArray) {
            int globalItemId = (*equipment < numTypes) ? itemArray[*equipment] : 0;
            std::string currentLabel = item::get_ui_name(globalItemId);
            if (currentLabel.empty() || globalItemId == 0)
                currentLabel = "None";

            if (ImGui::BeginCombo(labelPrefix, currentLabel.c_str())) {
                if (ImGui::Selectable("None", *equipment == 0))
                    *equipment = 0;

                for (int i = 0; i < numTypes; ++i) {
                    int itemId = itemArray[i];
                    if (itemId <= 0 || itemId >= NUM_ITEMS)
                        continue;

                    const char* label = item::get_ui_name(itemId);

                    bool selected = (globalItemId == itemId);
                    if (ImGui::Selectable(label, selected))
                        *equipment = static_cast<uint8_t>(i);
                }

                ImGui::EndCombo();
            }
        };

        creature* c = static_cast<creature*>(e);
        if (c->has_feature(CREATURE_FEATURE_HEAD)) {
            item_cb_append("Head", &head, NUM_HEAD_TYPES, _head_to_item);
            item_cb_append("Head Equipment", &head_equipment, NUM_HEAD_EQUIPMENT_TYPES, _head_equipment_to_item);
        }
        if (c->has_feature(CREATURE_FEATURE_BACK_EQUIPMENT))
            item_cb_append("Back Equipment", &back_equipment, NUM_BACK_EQUIPMENT_TYPES, _back_to_item);

        if (c->has_feature(CREATURE_FEATURE_FRONT_EQUIPMENT))
            item_cb_append("Front Equipment", &front_equipment, NUM_FRONT_EQUIPMENT_TYPES, _front_to_item);

        item_cb_append("Feet", &feet, NUM_FEET_TYPES, _feet_to_item);
        item_cb_append("Bolt Set", &bolt_set, NUM_BOLT_SETS, _bolt_to_item);

        ImGui_ButtonBar(apply_properties);
    }

    void column_two() {
        if (ImGui::BeginTable("##EquipmentTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, UI(17.f));
            ImGui::TableSetupColumn("");

            for (int x = 0; x < NUM_ITEMS; ++x) {
                item_option *io = &item_options[x];

                if (io->category != ITEM_CATEGORY_WEAPON &&
                    io->category != ITEM_CATEGORY_TOOL &&
                    io->category != ITEM_CATEGORY_CIRCUIT)
                    continue;

                bool equipped = std::find(equipment.begin(), equipment.end(), x) != equipment.end();

                ImGui::TableNextRow();

                // Checkbox column
                ImGui::TableSetColumnIndex(0);
                bool checked = equipped;
                std::string checkbox_id = "##equip_" + std::to_string(x);
                if (ImGui::Checkbox(checkbox_id.c_str(), &checked)) {
                    if (checked)
                        equipment.push_back(x);
                    else
                        equipment.erase(std::remove(equipment.begin(), equipment.end(), x), equipment.end());
                }

                // Name column
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(item::get_ui_name(x));
            }

            ImGui::EndTable();
        }
    }

    void layout() {
        handle_do_open(&do_open, "Robot settings");

        ImGui_CenterNextWindow();

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowSize(
            ImVec2(
                0.0f,
                SDL_min(viewport->WorkSize.y * 0.98f, UI(520))),
            ImGuiCond_Always);

        if (ImGui::BeginPopupModal("Robot settings", REF_TRUE, MODAL_FLAGS)) {
            entity* e = G->selection.e;

            if (ImGui::BeginTable("layout", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV)) {
                ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthFixed, UI(175.f));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                ImGui::BeginChild("left_panel", UI(350, 470), false);
                column_one();
                ImGui::EndChild();

                ImGui::TableSetColumnIndex(1);

                ImGui::BeginChild("right_panel", UI(175, 470), false);
                column_two();
                ImGui::EndChild();

                ImGui::EndTable();
            }

            ImGui::EndPopup();
        }
    }
}
