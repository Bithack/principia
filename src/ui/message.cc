#include "ui_imgui.hh"

namespace UiMessage {
    static bool do_open = false;
    static std::string message {""};
    static MessageType msg_type = MessageType::Error;

    void open(const char* msg, MessageType typ /*=MessageType::Message*/) {
        do_open = true;
        msg_type = typ;
        message.assign(msg);
    }

    void layout() {
        handle_do_open(&do_open, "###info-popup");
        ImGui_CenterNextWindow();
        const char* typ;
        switch (msg_type) {
            case MessageType::Message:
                typ = "Message###info-popup";
                break;

            case MessageType::Error:
                typ = "Error###info-popup";
                break;

            case MessageType::LevelInfo:
                typ = "Level description###info-popup";
                break;
        }
        ImGui::SetNextWindowSize(ImVec2(400., 0.));
        if (ImGui::BeginPopupModal(typ, NULL, MODAL_FLAGS)) {
            ImGui::TextWrapped("%s", message.c_str());

            ImGui::Dummy(ImVec2(0.0f, 40.0f));

            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}
