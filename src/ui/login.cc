#include "ui.hh"
#include "ui_imgui.hh"

namespace UiLogin {
    enum class LoginStatus {
        No,
        LoggingIn,
        ResultSuccess,
        ResultFailure
    };

    static bool do_open = false;
    static std::string username{""};
    static std::string password{""};
    static LoginStatus login_status = LoginStatus::No;

    void complete_login(int signal) {
        switch (signal) {
            case SIGNAL_LOGIN_SUCCESS:
                login_status = LoginStatus::ResultSuccess;
                break;
            case SIGNAL_LOGIN_FAILED:
                login_status = LoginStatus::ResultFailure;
                break;
        }
    }

    void open() {
        do_open = true;
        username = "";
        password = "";
        login_status = LoginStatus::No;
    }

    void layout() {
        handle_do_open(&do_open, "Log in");
        ImGui_CenterNextWindow();
        //Only allow closing the window if a login attempt is not in progress
        bool *allow_closing = (login_status != LoginStatus::LoggingIn) ? REF_TRUE : NULL;
        if (ImGui::BeginPopupModal("Log in", allow_closing, MODAL_FLAGS)) {
            if (login_status == LoginStatus::ResultSuccess) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }

            bool req_username_len = username.length() > 0;
            bool req_pass_len = password.length() > 0;

            ImGui::BeginDisabled(
                (login_status == LoginStatus::LoggingIn) ||
                (login_status == LoginStatus::ResultSuccess)
            );

            if (ImGui::IsWindowAppearing()) {
                ImGui::SetKeyboardFocusHere();
            }
            bool activate = false;
            activate |= ImGui::InputTextWithHint("###username", "Username", &username, ImGuiInputTextFlags_EnterReturnsTrue);
            activate |= ImGui::InputTextWithHint("###password", "Password", &password, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_Password);

            ImGui::EndDisabled();

            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 3.0f));

            bool can_submit =
                (login_status != LoginStatus::LoggingIn) &&
                (login_status != LoginStatus::ResultSuccess) &&
                (req_pass_len && req_username_len);
            ImGui::BeginDisabled(!can_submit);
            if (ImGui::Button("  Log in  ") || (can_submit && activate)) {
                login_status = LoginStatus::LoggingIn;
                login_data *data = new login_data;
                strncpy(data->username, username.c_str(), 256);
                strncpy(data->password, password.c_str(), 256);
                P.add_action(ACTION_LOGIN, data);
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("  Register  ") || (can_submit && activate)) {
                COMMUNITY_URL("register");
                ui::open_url(url);
            }

            ImGui::SameLine();

            switch (login_status) {
                case LoginStatus::LoggingIn:
                    ImGui::TextUnformatted("Logging in...");
                    break;
                case LoginStatus::ResultFailure:
                    ImGui::TextColored(ImVec4(1., 0., 0., 1.), "Login failed"); // Login attempt failed
                    break;
                default:
                    break;
            }

            ImGui::EndPopup();
        }
    }
}
