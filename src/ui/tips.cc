#include "settings.hh"
#include "ui_imgui.hh"

namespace UiTips {
    static bool do_open = false;

    void open() {
        ctip = rand() % num_tips;
        do_open = true;
    }

    void layout() {
        //TODO: optimize for uiscale!
        handle_do_open(&do_open, "Tips and tricks");
        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(ImVec2(400, 200));
        if (ImGui::BeginPopupModal("Tips and tricks", REF_TRUE, MODAL_FLAGS)) {
            ImGuiStyle &style = ImGui::GetStyle();
            float font_size = ImGui::GetFontSize();
            ImVec2 frame_padding = style.FramePadding;
            ImVec2 content_region = ImGui::GetContentRegionMax();

            //TODO remove hardcoded size
            if (ImGui::BeginChild("###tips-content-ctx", ImVec2(0, 115), false, FRAME_FLAGS)) {
                ImGui::TextWrapped("%s", tips[ctip]);
            }
            ImGui::EndChild();

            //Align at the bottom of the window
            ImGui::SetCursorPosY(content_region.y - (font_size + (2. * frame_padding.y)));
            if (ImGui::Button("<###tips-prev")) {
                if (--ctip < 0) ctip = num_tips - 1;
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(.8, .8, .8, 1), "Tip %d/%d", ctip + 1, num_tips);
            ImGui::SameLine();
            if (ImGui::Button(">###tips-next")) {
                ctip = (ctip + 1) % num_tips;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Don't show again", (bool*) &settings["hide_tips"]->v.b)) {
                settings.save();
            }
            ImGui::SameLine();
            const char* close_text = "Close";
            const ImVec2 close_text_size = ImGui::CalcTextSize(close_text);
            ImGui::SetCursorPosX(content_region.x - (close_text_size.x + frame_padding.x * 2.));
            if (ImGui::Button(close_text)) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}
