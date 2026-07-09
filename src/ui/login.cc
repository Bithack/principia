#include "imgui.hh"
#include "ui.hh"

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
        if (signal == SIGNAL_LOGIN_SUCCESS)
            login_status = LoginStatus::ResultSuccess;
        else if (signal == SIGNAL_LOGIN_FAILED)
            login_status = LoginStatus::ResultFailure;
    }

    void do_login() {
        login_status = LoginStatus::LoggingIn;
        login_data *data = new login_data;
        strncpy(data->username, username.c_str(), 256);
        strncpy(data->password, password.c_str(), 256);
        P.add_action(ACTION_LOGIN, data);
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
        if (ImGui::BeginPopupModal("Log in", REF_TRUE, MODAL_FLAGS)) {
            if (login_status == LoginStatus::ResultSuccess) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }

            bool req_username_len = username.length() > 0;
            bool req_pass_len = password.length() > 0;

            if (ImGui::BeginTable("layout", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV)) {
                ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthFixed, 250.0f);
                ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                ImGui::BeginChild("left_panel", ImVec2(250, 125), false);

                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere();
                }
                bool activate = false;

                ImGui::SetNextItemWidth(-FLT_MIN);
                activate |= ImGui::InputTextWithHint("###username", "Username", &username,
                    ImGuiInputTextFlags_EnterReturnsTrue);

                ImGui::SetNextItemWidth(-FLT_MIN);
                activate |= ImGui::InputTextWithHint("###password", "Password", &password,
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_Password);

                ImGui::Dummy(ImVec2(0.0f, 5.0f));

                bool can_submit =
                    (login_status != LoginStatus::LoggingIn) &&
                    (login_status != LoginStatus::ResultSuccess) &&
                    (req_pass_len && req_username_len);

                ImGui::BeginDisabled(!can_submit);
                if (ImGui::Button("Log in", ImVec2(120, 0)) || (can_submit && activate))
                    do_login();

                ImGui::EndDisabled();

                ImGui::SameLine();
                if (login_status == LoginStatus::LoggingIn)
                    ImGui::TextUnformatted("Logging in...");
                else if (login_status == LoginStatus::ResultFailure)
                    ImGui::TextColored(ImVec4(1., 0., 0., 1.), "Login failed");

                ImGui::EndChild();
                ImGui::TableSetColumnIndex(1);
                ImGui::BeginChild("right_panel", ImVec2(200, 135), false);

                ImGui::BeginChild("register_text", ImVec2(200, 70), false);
                ImGui::TextWrapped("Don't have an account?\n\nRegister one and join the Principia community!");
                ImGui::EndChild();

                ImGui::Dummy(ImVec2(0.0f, 5.0f));

                float button_width = 120.0f;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - button_width) * 0.5f);
                if (ImGui::Button("Register", ImVec2(button_width, 0))) {
                    COMMUNITY_URL("register");
                    ui::open_url(url);
                }

                ImGui::EndChild();
                ImGui::EndTable();

            }

            ImGui::EndPopup();
        }
    }
}
