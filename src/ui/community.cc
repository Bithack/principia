#include "imgui.hh"
#include "main.hh"
#include "ui.hh"
#include "world.hh"

namespace UiCommunity {
    static bool do_open = false;

    void open() {
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "###community-exit");

#ifdef SDL_PLATFORM_ANDROID
        static bool android_dialog = true;
#else
        static bool android_dialog = false;
#endif

        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(UI(320, .0));

        const char *title = android_dialog ? "Leave level?###community-exit" : "Back to main menu?###community-exit";
        if (ImGui::BeginPopupModal(title, REF_TRUE, MODAL_FLAGS)) {
            ImGui::TextWrapped(android_dialog
                ? "Do you want to return to the community site or to the main menu?"
                : "Do you want to return to the main menu?");

            ImGui::Dummy(UI(0.0f, 25.0f));

            if (android_dialog) {
                if (ImGui::Button("Community")) {
                    COMMUNITY_URL("level/%d", W->level.community_id);
                    ui::open_url(url);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
            }

            if (ImGui::Button(android_dialog ? "Main menu" : "Yes", android_dialog ? UI(0., 0.) : UI(70., 0)) || ImGui_EnterAction()) {
                P.add_action(ACTION_GOTO_MAINMENU, 0);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button(android_dialog ? "Cancel" : "No", UI(70., 0.)) || (!android_dialog && ImGui_EscAction())) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}
