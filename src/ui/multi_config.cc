#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiMultiConfig {
    static bool do_open = false;

    static float joint_strength = 1.f;
    static ImVec4 plastic_color = ImVec4(1.f, 1.f, 1.f, 1.f);
    static float plastic_density = 1.f;
    static int connection_render_type = CONN_RENDER_DEFAULT;

    enum {
        TAB_JOINT_STRENGTH,
        TAB_PLASTIC_COLOR,
        TAB_PLASTIC_DENSITY,
        TAB_CONNECTION_RENDER_TYPE,
        TAB_MISCELLANEOUS,
    };
    static bool enabled_tabs[5] = { true, false, false, true, true };
    static int current_tab = TAB_JOINT_STRENGTH;
    static bool any_entity_locked = false;

    static void apply() {
        switch (current_tab) {
            case TAB_JOINT_STRENGTH:
                P.add_action(ACTION_MULTI_JOINT_STRENGTH, INT_TO_VOID((int)(joint_strength * 100.f)));
                break;

            case TAB_PLASTIC_COLOR: {
                tvec4 *vec = (tvec4*)malloc(sizeof(tvec4));
                vec->r = plastic_color.x;
                vec->g = plastic_color.y;
                vec->b = plastic_color.z;
                vec->a = 1.f;

                P.add_action(ACTION_MULTI_PLASTIC_COLOR, vec);
                break;
            }

            case TAB_PLASTIC_DENSITY:
                P.add_action(ACTION_MULTI_PLASTIC_DENSITY, INT_TO_VOID((int)(plastic_density * 100.f)));
                break;

            case TAB_CONNECTION_RENDER_TYPE:
                P.add_action(ACTION_MULTI_CHANGE_CONNECTION_RENDER_TYPE, UINT_TO_VOID((uint8_t)connection_render_type));
                break;
        }

        ImGui::CloseCurrentPopup();
    }

    static int tab_joint_strength() {
        ImGui::SliderFloat("Strength", &joint_strength, 0.f, 1.f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Spacing();

        ImGui::TextWrapped("Applying a new joint strength might make your selection change its position/state slightly.\n\n"
            "Make sure you save your level before pressing Apply.");

        return TAB_JOINT_STRENGTH;
    }

    static int tab_plastic_color() {
        ImGui::ColorEdit3("##plasticcolor", (float *)&plastic_color);

        ImGui::Spacing();

        ImGui::TextWrapped("This will change the color of all plastic objects in your current selection.");

        return TAB_PLASTIC_COLOR;
    }

    static int tab_plastic_density() {
        ImGui::SliderFloat("Density", &plastic_density, 0.f, 1.f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Spacing();

        ImGui::TextWrapped("This will change the density of all plastic objects in your current selection.");

        return TAB_PLASTIC_DENSITY;
    }

    static int tab_connection_render_type() {
        ImGui::RadioButton("Default", &connection_render_type, CONN_RENDER_DEFAULT);
        ImGui::RadioButton("Small", &connection_render_type, CONN_RENDER_SMALL);
        ImGui::RadioButton("Hide", &connection_render_type, CONN_RENDER_HIDE);

        ImGui::Spacing();

        ImGui::TextWrapped("This will change the render type of all connections in your current selection.");

        return TAB_CONNECTION_RENDER_TYPE;
    }

    static int tab_miscellaneous() {
        ImGui::TextWrapped("Unlock any previously locked objects.");

        if (!any_entity_locked)
            ImGui::BeginDisabled();

        if (ImGui::Button("Unlock all")) {
            P.add_action(ACTION_MULTI_UNLOCK_ALL, 0);
            ImGui::CloseCurrentPopup();
        }

        if (!any_entity_locked) {
            ImGui::SetItemTooltip("No locked objects in selection.");
            ImGui::EndDisabled();
        }

        ImGui::Separator();

        ImGui::TextWrapped("Disconnect all connection from selected objects.");

        if (ImGui::Button("Disconnect all")) {
            P.add_action(ACTION_MULTI_DISCONNECT_ALL, 0);
            ImGui::CloseCurrentPopup();
        }

        ImGui::Spacing();

        return TAB_MISCELLANEOUS;
    }

    void open() {
        do_open = true;

        if (G->state.sandbox && W->is_paused() && !G->state.test_playing) {
            if (G->get_mode() == GAME_MODE_MULTISEL && G->selection.m) {
                for (std::set<entity*>::iterator i = G->selection.m->begin();
                        i != G->selection.m->end(); i++) {
                    entity *e = *i;

                    if (e->flag_active(ENTITY_IS_PLASTIC)) {
                        enabled_tabs[TAB_PLASTIC_COLOR] = true;
                        enabled_tabs[TAB_PLASTIC_DENSITY] = true;
                    }

                    if (e->flag_active(ENTITY_IS_LOCKED)) {
                        any_entity_locked = true;
                    }
                }
            }
        }
    }

    void layout() {
        handle_do_open(&do_open, "Multi config");
        ImGui_CenterNextWindow();

        auto begin_scrolling_tab = [](const char *name, const char *id) {
            if (!ImGui::BeginTabItem(name))
                return false;

            ImGui::BeginChild(id, ImGui::GetContentRegionAvail(), false);
            return true;
        };

        auto end_scrolling_tab = []() {
            ImGui::EndChild();
            ImGui::EndTabItem();
        };

        float footer_height =
            ImGui::GetStyle().SeparatorSize +
            ImGui::GetFrameHeightWithSpacing() +
            ImGui::GetStyle().WindowPadding.y;

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowSize(
            ImVec2(
                0,
                std::min(viewport->WorkSize.y * 0.98f, UI(250) + footer_height)),
            ImGuiCond_Always);
        if (ImGui::BeginPopupModal("Multi config", REF_TRUE, MODAL_FLAGS)) {
            ImGui::BeginChild("##content", ImVec2(0, -footer_height), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX);

            if (ImGui::BeginTabBar("##tabs")) {
                if (begin_scrolling_tab("Joint strength", "##joint")) {
                    current_tab = tab_joint_strength();
                    end_scrolling_tab();
                }

                if (enabled_tabs[TAB_PLASTIC_COLOR] && begin_scrolling_tab("Plastic color", "##plastic_color")) {
                    current_tab = tab_plastic_color();
                    end_scrolling_tab();
                }

                if (enabled_tabs[TAB_PLASTIC_DENSITY] && begin_scrolling_tab("Plastic density", "##plastic_density")) {
                    current_tab = tab_plastic_density();
                    end_scrolling_tab();
                }

                if (begin_scrolling_tab("Connection render type", "##render_type")) {
                    current_tab = tab_connection_render_type();
                    end_scrolling_tab();
                }

                if (begin_scrolling_tab("Miscellaneous", "##misc")) {
                    current_tab = tab_miscellaneous();
                    end_scrolling_tab();
                }

                ImGui::EndTabBar();
            }

            ImGui::EndChild();

            ImGui::Separator();

            ImGui::BeginDisabled(current_tab == TAB_MISCELLANEOUS);

            if (ImGui::Button("Apply", UI(70., 0.)))
                apply();

            ImGui::EndDisabled();

            ImGui::SameLine();

            if (ImGui::Button("Cancel", UI(70., 0.)))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
    }
}
