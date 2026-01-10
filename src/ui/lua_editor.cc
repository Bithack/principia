#include "ui_imgui.hh"

namespace UiLuaEditor {
    static bool do_open = false;
    static entity *entity_ptr;
    static bool has_unsaved_changes = false;

    static std::string codeText;

    void init() {}

    static void flash_controller() {
        tms_infof("Flashing controller");

        //get ptr to len and buf, freeing the old buf if present
        uint32_t *len = &entity_ptr->properties[0].v.s.len;
        char **buf = &entity_ptr->properties[0].v.s.buf;
        if (*buf) free(*buf);

        //get code ptr and len
        const char *src = codeText.c_str();
        *len = codeText.size();

        //trim trailing newlines
        while (*len && (src[*len - 1] == '\n')) --*len;

        //create a new buffer and copy the data
        //principia lua code is not zero terminated
        *buf = (char*) malloc(*len);
        memcpy(*buf, src, *len);

        has_unsaved_changes = false;
    }

    static void reload_code() {
        uint32_t len = entity_ptr->properties[0].v.s.len;
        char *buf = entity_ptr->properties[0].v.s.buf;
        // char *code = (char*) malloc(len + 1);
        // memcpy(code, buf, len);
        // code[len] = '\0';
        tms_infof("code len %d", len);
        std::string code = std::string(buf, len);
        codeText = code;
        tms_infof("buf load success");
        has_unsaved_changes = false;
    }

    void open(entity *entity /*= G->selection.e*/) {
        do_open = true;
        entity_ptr = entity;
        reload_code();
    }

    void layout() {
        //TODO better ui design

        ImGuiIO& io = ImGui::GetIO();
        handle_do_open(&do_open, "Code editor");
        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(ImVec2(800, 0.));
        if (ImGui::BeginPopupModal("Code editor", REF_TRUE, MODAL_FLAGS | ImGuiWindowFlags_UnsavedDocument)) {
            ImGui::PushFont(ui_font_mono.font);

            ImGui::InputTextMultiline("##source", &codeText, ImVec2(775., 500.), ImGuiInputTextFlags_AllowTabInput, nullptr);

            ImGui::PopFont();

            ImGui::Spacing();

            if (ImGui::Button("Save (Ctrl+S)") | (io.KeyCtrl && ImGui::IsKeyReleased(ImGuiKey_S))) {
                flash_controller();
                reload_code();
            }
            ImGui::SameLine();
            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }

            ImGui::EndPopup();
        }
    }
}
