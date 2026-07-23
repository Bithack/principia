#include "imgui.hh"
#include "main.hh"
#include "ui.hh"

namespace UiExport {
    static bool do_open = false;
    static char obj_name[256];

    void export_object() {
        P.add_action(ACTION_EXPORT_OBJECT, strdup(obj_name));
        ui::message("Object exported!");

        ImGui::CloseCurrentPopup();
    }

    void open() {
        do_open = true;
        obj_name[0] = '\0';
    }

    void layout() {
        handle_do_open(&do_open, "Export object");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Export object", REF_TRUE, MODAL_FLAGS)) {

            ImGui::Text("Enter a name for this object:");

            bool activate = ImGui::InputText(
                "##obj_name",
                obj_name,
                IM_ARRAYSIZE(obj_name),
                ImGuiInputTextFlags_EnterReturnsTrue);

            bool disabled = obj_name[0] == '\0';

            ImGui_ButtonBarPadding();

            if (disabled)
                ImGui::BeginDisabled();

            if (ImGui_SaveButton() || (!disabled && activate))
                export_object();

            if (disabled)
                ImGui::EndDisabled();

            ImGui::SameLine();

            if (ImGui_CancelButton())
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
    }
}
