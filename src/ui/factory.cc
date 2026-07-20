#include "const.hh"
#include "entity.hh"
#include "factory.hh"
#include "faction.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"
#include <sstream>

namespace UiFactory {
    static bool do_open = false;

    static uint32_t oil;
    static uint32_t faction;
    static uint32_t resources[NUM_RESOURCES];

    static std::vector<factory_object> factory_objects;
    static std::vector<bool> factory_enabled;
    static std::vector<uint32_t> factory_recipes;

    static void calculate_indices() {
        int index = 1;

        for (size_t i = 0; i < factory_enabled.size(); ++i) {
            if (factory_enabled[i])
                ++index;
        }
    }

    static int get_factory_index(size_t row) {
        if (!factory_enabled[row])
            return -1;

        int index = 1;
        for (size_t i = 0; i < row; ++i) {
            if (factory_enabled[i])
                ++index;
        }

        return index;
    }

    void apply_properties() {
        entity *e = G->selection.e;
        if (!e || !IS_FACTORY(e->g_id))
            return;

        factory *f = static_cast<factory*>(e);

        f->properties[1].v.i = oil;
        f->properties[2].v.i = faction;

        for (int i = 0; i < NUM_RESOURCES; ++i)
            f->properties[FACTORY_NUM_EXTRA_PROPERTIES + i].v.i = resources[i];

        std::stringstream ss;
        bool first = true;

        for (size_t i = 0; i < factory_enabled.size(); ++i) {
            if (!factory_enabled[i])
                continue;

            if (!first)
                ss << ';';

            first = false;
            ss << i;
        }

        f->set_property(0, ss.str().c_str());

        tms_debugf("Recipe string: %s", f->properties[0].v.s.buf);

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);

        ImGui::CloseCurrentPopup();
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (!e || !IS_FACTORY(e->g_id))
            return;

        oil = e->properties[1].v.i;
        faction = e->properties[2].v.i;

        for (int x=0; x<NUM_RESOURCES; ++x)
            resources[x] = e->properties[FACTORY_NUM_EXTRA_PROPERTIES+x].v.i;

        factory_objects.clear();
        factory_enabled.clear();
        factory_recipes.clear();

        factory *fa = static_cast<factory*>(e);

        factory_objects = fa->objects();

        factory::generate_recipes(
            &factory_recipes,
            fa->properties[0].v.s.buf
        );

        for (size_t i = 0; i < factory_objects.size(); ++i) {
            bool enabled = false;

            for (uint32_t recipe : factory_recipes) {
                if (recipe == i) {
                    enabled = true;
                    break;
                }
            }

            factory_enabled.push_back(enabled);
        }
    }

    void column_one() {
        // oil
        int value = oil;
        ImGui::InputInt("Oil", &value, 1, 10);
        oil = tclampf(value, 0, UINT16_MAX);

        // resources
        for (int x=0; x<NUM_RESOURCES; ++x) {
            value = resources[x];
            ImGui::InputInt(resource_data[x].name, &value, 1, 10);
            resources[x] = tclampf(value, 0, UINT16_MAX);
        }

        // faction
        entity *e = G->selection.e;
        if (e->g_id == O_ROBOT_FACTORY &&
            ImGui::BeginCombo("Faction", factions[faction].name)) {
            for (int i = 0; i < sizeof(factions) / sizeof(factions[0]); ++i) {
                bool is_selected = (faction == i);
                if (ImGui::Selectable(factions[i].name, is_selected))
                    faction = i;

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui_ButtonBar(apply_properties);
    }

    void column_two() {
        ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable("factory_objects", 3, flags, ImVec2(0, UI(400)))) {

            ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed, UI(17.f));
            ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, UI(17.f));
            ImGui::TableSetupColumn("Recipe", ImGuiTableColumnFlags_WidthStretch);

            entity *e = G->selection.e;
            factory *fa = (e && IS_FACTORY(e->g_id)) ? static_cast<factory*>(e) : nullptr;

            for (size_t i = 0; i < factory_objects.size(); ++i) {
                const factory_object &fo = factory_objects[i];

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);

                bool enabled = factory_enabled[i];
                if (ImGui::Checkbox(("##enabled" + std::to_string(i)).c_str(), &enabled)) {
                    factory_enabled[i] = enabled;
                    calculate_indices();
                }

                ImGui::TableSetColumnIndex(1);

                int index = get_factory_index(i);
                if (index >= 0)
                    ImGui::Text("%d", index);

                ImGui::TableSetColumnIndex(2);

                const char *name;
                if (fa && (fa->factory_type == FACTORY_ARMORY || fa->factory_type == FACTORY_OIL_MIXER))
                    name = item_options[fo.gid].name;
                else
                    name = of::get_object_name_by_gid(fo.gid);

                ImGui::TextUnformatted(name);
            }

            ImGui::EndTable();
        }
    }

    void layout() {
        handle_do_open(&do_open, "Factory");
        ImGui_CenterNextWindow();

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowSize(
            ImVec2(
                0,
                SDL_min(viewport->WorkSize.y * 0.98f, UI(460))),
            ImGuiCond_Always);

        if (ImGui::BeginPopupModal("Factory", REF_TRUE, MODAL_FLAGS)) {
            entity* e = G->selection.e;

            if (ImGui::BeginTable("layout", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV)) {
                ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthFixed, UI(200.f));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                ImGui::BeginChild("left_panel", UI(0, 412), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX);
                column_one();
                ImGui::EndChild();

                ImGui::TableSetColumnIndex(1);

                ImGui::BeginChild("right_panel", UI(0, 412), false);
                column_two();
                ImGui::EndChild();

                ImGui::EndTable();
            }

            ImGui::EndPopup();
        }
    }
}
