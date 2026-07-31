#include "adventure.hh"
#include "game.hh"
#include "imgui.hh"
#include "prompt.hh"
#include "world.hh"

namespace UiPrompt {
    static bool do_open = false;

    static char *prompt_text;
    static std::string response_text[3];

    void open() {
        do_open = true;

        if (W->is_adventure() && adventure::player) {
            adventure::player->stop_moving(DIR_LEFT);
            adventure::player->stop_moving(DIR_RIGHT);
        }

        base_prompt *bp = G->current_prompt->get_base_prompt();
        if (G->current_prompt && G->current_prompt->is_prompt_compatible() && bp) {
            prompt_text = *bp->message;
            for (int i = 0; i < 3; ++i) {
                const struct base_prompt::prompt_button &btn = bp->buttons[i];
                if (*btn.len > 0 && btn.buf) {
                    response_text[i] = std::string(*btn.buf, *btn.len);
                } else {
                    response_text[i] = "";
                }
            }
        }
    }

    void layout() {
        handle_do_open(&do_open, "Prompt");

        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSizeConstraints(ImVec2(UI(300), 0), ImVec2(FLT_MAX, FLT_MAX));
        if (ImGui::BeginPopupModal("Prompt", NULL, MODAL_FLAGS)) {
            ImGui::TextWrapped("%s", prompt_text);

            ImGui::Dummy(UI(0.0f, 25.0f));

            for (int i = 0; i < 3; ++i) {
                if (response_text[i].empty())
                    continue;

                std::string button_label = response_text[i] + "##prompt_button_" + std::to_string(i);
                if (ImGui::Button(button_label.c_str())) {
                    base_prompt *bp = G->current_prompt->get_base_prompt();

                    if (bp) {
                        tms_debugf("setting prompt response from here");
                        bp->set_response(i + 1);
                    }
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();
            }

            ImGui::EndPopup();
        }
    }

}