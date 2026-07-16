#include "entity.hh"
#include "escript.hh"
#include "game.hh"
#include "imgui.hh"
#include "ui.hh"

namespace UiLuaEditor {
    static bool do_open = false;
    static entity *e;
    static bool has_unsaved_changes = false;
    static bool use_external_editor = false;
    static bool prev_external_editor_state = false;
    static bool confirm_exit = false;

    static std::string saved_text;
    static std::string codeText;
    static char external_path[ESCRIPT_EXTERNAL_PATH_LEN];

    static void reload_code() {
        uint32_t len = e->properties[0].v.s.len;
        char *buf = e->properties[0].v.s.buf;

        codeText = std::string(buf, len);
        saved_text = std::string(buf, len);
        has_unsaved_changes = false;
    }

    static void apply_properties() {

        //get ptr to len and buf, freeing the old buf if present
        uint32_t *len = &e->properties[0].v.s.len;
        char **buf = &e->properties[0].v.s.buf;
        if (*buf)
            free(*buf);

        //get code ptr and len
        const char *src = codeText.c_str();
        *len = codeText.size();

        //trim trailing newlines
        while (*len && (src[*len - 1] == '\n'))
            --*len;

        //create a new buffer and copy the data
        //principia lua code is not zero terminated
        *buf = (char*) malloc(*len);
        memcpy(*buf, src, *len);
    }

    static void save_external_file() {
        FILE *fh = fopen(external_path, "w");

        if (!fh) {
            tms_errorf("Could not open external file for writing: %s", external_path);
            return;
        }

        fwrite(codeText.c_str(), sizeof(char), codeText.size(), fh);
        tms_infof("Write to %s", external_path);
        fclose(fh);

        apply_properties();
        reload_code();
    }

    static void load_external_file() {
        FILE *fh = fopen(external_path, "r");

        if (!fh) {
            tms_errorf("Could not open external file for reading: %s", external_path);
            return;
        }

        fseek(fh, 0L, SEEK_END);
        size_t sz = ftell(fh);
        rewind(fh);

        char *data = (char*)calloc(sizeof(char), sz + 1);

        size_t result = fread(data, sizeof(char), sz, fh);
        if (result == sz) {
            tms_debugf("Successfully read source from file! Reloading...");
            codeText = std::string(data, sz);
            apply_properties();
            reload_code();
        }

        free(data);
        fclose(fh);
    }

    void open(entity *entity) {
        do_open = true;
        e = G->selection.e;

        use_external_editor = e->properties[1].v.i & ESCRIPT_USE_EXTERNAL_EDITOR;
        ((escript*)e)->generate_external_path(external_path);
        reload_code();
    }

    void layout() {
        ImGuiIO& io = ImGui::GetIO();
        handle_do_open(&do_open, "Lua Script");
        ImGui_CenterNextWindow();

        if (ImGui::BeginPopupModal("Lua Script", has_unsaved_changes ? REF_TRUE : NULL,
                MODAL_FLAGS | (has_unsaved_changes ? ImGuiWindowFlags_UnsavedDocument : 0))) {

            has_unsaved_changes = (codeText != saved_text);

            if (prev_external_editor_state != use_external_editor) {
                if (use_external_editor) {
                    // If we are switching to external editor, save the current code to the external file
                    save_external_file();
                } else {
                    // If we are switching back to internal editor, load the code from the file
                    load_external_file();
                }

                prev_external_editor_state = use_external_editor;

                e->properties[1].v.i = ESCRIPT_INCLUDE_STRING | ESCRIPT_INCLUDE_TABLE | ESCRIPT_LISTEN_ON_INPUT
                        | ((int)use_external_editor * ESCRIPT_USE_EXTERNAL_EDITOR);
            }

            ImGui::BeginChild("left_panel", UI(550., 350.25), false);
            ImGui::PushFont(ui_font_mono.font, 0.0f);

            if (use_external_editor)
                ImGui::BeginDisabled();

            ImGui::InputTextMultiline("##source", &codeText, UI(550., use_external_editor ? 150. : 350.),
                ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_WordWrap, nullptr);

            if (use_external_editor)
                ImGui::EndDisabled();

            ImGui::PopFont();

            if (use_external_editor) {
                ImGui::Spacing();

                char file_path[ESCRIPT_EXTERNAL_PATH_LEN];
                ((escript*)e)->generate_external_path(file_path);

                ImGui::TextWrapped("External path: %s", file_path);

                ImGui::TextWrapped("Open the file path above with your favourite code editor and edit the code there.");
                ImGui::TextWrapped("Before you press play in Principia, remember to save the external file!");

                if (ImGui::Button("Open external file", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    char url[2048];
                    snprintf(url, 2047, "file://%s", pkgman::get_cache_path(W->level_id_type));
                    ui::open_url(url);
                }
            }

            ImGui::EndChild();

            ImGui::Checkbox("Use external editor", &use_external_editor);
            ImGui::SetItemTooltip("Check this file if you want to edit the Lua from an external editor. \n(Toggling will save the current code)");

            if (!use_external_editor) {
                if (ImGui::Button("Save (Ctrl+S)", UI(100., 0.)) || (io.KeyCtrl && ImGui::IsKeyReleased(ImGuiKey_S))) {
                    apply_properties();
                    reload_code();
                }

                ImGui::SameLine();

                if (ImGui::Button("Save & Exit", UI(100., 0.))) {
                    apply_properties();
                    reload_code();

                    P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
                    P.add_action(ACTION_ENTITY_MODIFIED, 0);
                    P.add_action(ACTION_AUTOSAVE, 0);

                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();
            }

            if (ImGui::Button(confirm_exit ? "Confirm?" : "Close", UI(100., 0.))) {
                if (!has_unsaved_changes || confirm_exit) {
                    confirm_exit = false;
                    ImGui::CloseCurrentPopup();
                } else {
                    confirm_exit = true;
                }
            }

            ImGui::EndPopup();
        }
    }
}
