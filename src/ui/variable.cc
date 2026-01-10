#include "ui_imgui.hh"

namespace UiVariable {
static bool do_open = false;
    static char variable_name[128] = "";

    void open() {
        do_open = true;

        entity *e = G->selection.e;

        if (e && (e->g_id == O_VAR_GETTER || e->g_id == O_VAR_SETTER)) {
            strcpy(variable_name, e->properties[0].v.s.buf);
        }
    }

    void layout() {
        handle_do_open(&do_open, "Variable chooser");

        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(ImVec2(400., 0.));
        if (ImGui::BeginPopupModal("Variable chooser", REF_TRUE, MODAL_FLAGS)) {

            ImGui::Text("Variable name:");
            ImGui::InputText("##VariableName", variable_name, IM_ARRAYSIZE(variable_name));

            ImGui::Dummy(ImVec2(0.0f, 10.0f));

            if (ImGui::Button("Reset Variable")) {
                std::map<std::string, float>::size_type num_deleted = W->level_variables.erase(variable_name);
                if (num_deleted != 0) {
                    W->save_cache(W->level_id_type, W->level.local_id);
                    ui::messagef("Successfully deleted data for variable '%s'", variable_name);
                } else
                    ui::messagef("No data found for variable '%s'", variable_name);
            }

            ImGui::SameLine();

            if (ImGui::Button("Reset All Variables")) {
                W->level_variables.clear();
                if (W->save_cache(W->level_id_type, W->level.local_id))
                    ui::message("All level-specific variables cleared.");
                else
                    ui::message("Unable to delete level-specific variables.");
            }

            ImGui::Dummy(ImVec2(0.0f, 60.0f));

            ImGui::Separator();

            if (ImGui::Button("Save")) {
                entity *e = G->selection.e;

                if (e && (e->g_id == O_VAR_SETTER || e->g_id == O_VAR_GETTER)) {
                    if (strlen(variable_name) && strlen(variable_name) <= 50) {
                        char var_name[51];

                        int i = 0;
                        for (int x=0; x<strlen(variable_name); ++x) {
                            if (isalnum(variable_name[x]) || variable_name[x] == '_' || variable_name[x] == '-') {
                                var_name[i++] = variable_name[x];
                            }
                        }
                        var_name[i] = '\0';

                        if (strlen(var_name)) {
                            e->set_property(0, var_name);
                            ui::messagef("Variable name '%s' saved.", var_name);
                            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
                            P.add_action(ACTION_RESELECT, 0);
                            ImGui::CloseCurrentPopup();
                        } else
                            ui::message("The variable name must contain at least one 'a-z0-9-_'-character.");
                    } else
                        ui::message("Variable name too long.");
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}
