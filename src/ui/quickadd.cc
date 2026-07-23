#include "decorations.hh"
#include "game.hh"
#include "imgui.hh"
#include "item.hh"
#include "main.hh"
#include "types.hh"
#include "ui.hh"

namespace UiQuickadd {
    static bool do_open = false;
    static std::string query{""};
    static std::string trimmed_query{""};

    enum class ItemCategory {
        MenuObject,
        MenuItem,
        MenuDecoration
    };
    struct SearchItem {
        ItemCategory cat;
        uint32_t id;
    };

    static bool is_haystack_inited = false;
    static std::vector<SearchItem> haystack;
    static std::vector<size_t> search_results; //referencing idx in haystack
    //last known best item, will be used in case there are no search results
    static size_t last_viable_solution;

    static std::string resolve_item_name(SearchItem item) {
        std::string name;
        switch (item.cat) {
            case ItemCategory::MenuObject: {
                const struct menu_obj &obj = menu_objects[item.id];
                name = obj.e->get_name();
                break;
            }
            case ItemCategory::MenuItem: {
                const struct item_option &itm = item_options[item.id];
                name = std::string(itm.name) + " (Item)";
                break;
            }
            case ItemCategory::MenuDecoration: {
                name = std::string(decorations[item.id].name) + " (Decoration)";
                break;
            }
        }
        return name;
    }

    static std::vector<uint32_t> low_confidence;
    static void search() {
        search_results.clear();
        low_confidence.clear();

        trimmed_query = query;

        // trim leading whitespace
        trimmed_query.erase(
            trimmed_query.begin(),
            std::find_if(trimmed_query.begin(), trimmed_query.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));

        // trim trailing whitespace
        trimmed_query.erase(
            std::find_if(trimmed_query.rbegin(),
            trimmed_query.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), trimmed_query.end());

        for (int i = 0; i < haystack.size(); i++) {
            SearchItem item = haystack[i];
            std::string name = resolve_item_name(item);

            if (lax_search(name, trimmed_query))
                search_results.push_back(i);
            else if (lax_search(trimmed_query, name))
                low_confidence.push_back(i);
        }

        //Low confidence results are pushed after regular ones
        //Low confidence = no query in name, but name is in query.
        //So while searching for "Thick plank"...
        // Thick plank is a regular match
        // Plank is a low confidence match
        for (int i = 0; i < low_confidence.size(); i++)
            search_results.push_back(low_confidence[i]);

        tms_debugf(
            "search \"%s\" %d/%d matched, (%d low confidence)",
            trimmed_query.c_str(),
            (int) search_results.size(),
            (int) haystack.size(),
            (int) low_confidence.size()
        );

        if (search_results.size() > 0)
            last_viable_solution = search_results[0];
    }

    // HAYSTACK IS LAZY-INITED!
    // WE CAN'T CALL THIS RIGHT AWAY
    // AS MENU OBJECTS ARE INITED *AFTER* UI!!!
    static void init_haystack() {
        if (is_haystack_inited)
            return;

        is_haystack_inited = true;

        //Setup haystack
        haystack.clear();
        haystack.reserve(menu_objects.size());
        for (int i = 0; i < menu_objects.size(); i++) {
            SearchItem itm = {ItemCategory::MenuObject, (uint32_t)i};
            haystack.push_back(itm);
        }
        for (int x = 0; x < NUM_ITEMS; x++) {
            SearchItem itm = {ItemCategory::MenuItem, (uint32_t)x};
            haystack.push_back(itm);
        }
        for (int x = 0; x < NUM_DECORATIONS; x++) {
            SearchItem itm = {ItemCategory::MenuDecoration, (uint32_t)x};
            haystack.push_back(itm);
        }
        tms_infof("init qs haystack with size %d", (int) haystack.size());
        tms_debugf("DEBUG: menu obj cnt %d", (int) menu_objects.size());

        //for opt. reasons
        search_results.reserve(haystack.size());
        low_confidence.reserve(haystack.size());
    }

    void open() {
        do_open = true;
        query = "";
        trimmed_query = "";
        search_results.clear();
        init_haystack();
        search();
    }

    static void activate_item(SearchItem item) {
        switch (item.cat) {
            case ItemCategory::MenuObject: {
                p_gid g_id = menu_objects[item.id].e->g_id;
                P.add_action(ACTION_CONSTRUCT_ENTITY, g_id);
                break;
            }
            case ItemCategory::MenuItem:
                return P.add_action(ACTION_CONSTRUCT_ITEM, item.id);
            case ItemCategory::MenuDecoration:
                return P.add_action(ACTION_CONSTRUCT_DECORATION, item.id);
        }
    }

    void layout() {
        handle_do_open(&do_open, "quickadd");
        if (ImGui::BeginPopup("quickadd", POPUP_FLAGS)) {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }

            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();

            if (ImGui::InputTextWithHint(
                "###qs-search",
                "Search for components",
                &query,
                ImGuiInputTextFlags_EnterReturnsTrue
            )) {
                if (search_results.size() > 0) {
                    activate_item(haystack[search_results[0]]);
                } else {
                    tms_debugf("falling back to last best solution, can't just refuse to do anything!");
                    activate_item(haystack[last_viable_solution]);
                }
                ImGui::CloseCurrentPopup();
            };
            if (ImGui::IsItemEdited())
                search();

            const float area_height = ImGui::GetTextLineHeightWithSpacing() * 7.25f + ImGui::GetStyle().FramePadding.y * 2.0f;
            if (ImGui::BeginChild(ImGui::GetID("qsbox"), ImVec2(-FLT_MIN, area_height), ImGuiChildFlags_FrameStyle)) {
                for (int i = 0; i < search_results.size(); i++) {
                    ImGui::PushID(i);
                    SearchItem item = haystack[search_results[i]];
                    if (ImGui::Selectable(resolve_item_name(item).c_str())) {
                        activate_item(item);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopID();
                }
                if (search_results.size() == 0) {
                    if (strcasecmp(trimmed_query.c_str(), "s" "nefler") == 0) {
                        if (ImGui::Selectable("S" "nefler")) {
                            ui::message("Coming soon!");
                            ImGui::CloseCurrentPopup();
                        }
                    } else {
                        SearchItem item = haystack[last_viable_solution];
                        if (ImGui::Selectable(resolve_item_name(item).c_str())) {
                            activate_item(item);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
                ImGui::EndChild();
            }

            ImGui::EndPopup();
        }
    }
}
