#include "settings.hh"
#include "main.hh"
#include "soundmanager.hh"
#include "imgui.hh"
#include <string>
#include <thread>
#include <unordered_map>

namespace UiSettings {
    static bool do_open = false;

    enum class IfDone {
        Nothing,
        Exit,
        Reload,
    };

    static IfDone if_done = IfDone::Nothing;
    static bool is_saving = false;

    static std::unordered_map<const char*, setting*> local_settings;

    static const char* copy_settings[] = {
        //GRAPHICS
        "enable_shadows",
        "shadow_quality",
        "shadow_map_resx",
        "shadow_map_resy",
        "enable_ao",
        "ao_map_res",
        "postprocess",
        "enable_bloom",
        "vsync",
        "gamma_correct",
        //VOLUME
        "volume",
        "muted",
        //CONTROLS
        "control_type",
        "touch_controls",
        "cam_speed_modifier",
        "smooth_cam",
        "zoom_speed",
        "smooth_zoom",
        "jail_cursor",
        "rc_lock_cursor",
        "widget_control_sensitivity",
        //INTERFACE
        "uiscale",
        "window_fullscreen",
        "window_resizable",
        "autosave_screensize",
        "score_automatically_submit",
        "score_ask_before_submitting",
        "display_fps",
        //SANDBOX
        "hide_tips",
        "display_object_id",
        "display_grapher_value",
        "display_wireless_frequency",
        "dna_sandbox_back",
        "menu_speed",
        "smooth_menu",
        NULL
    };

    static void save_thread() {

        while (!P.can_set_settings) {
            tms_debugf("Waiting for can_set_settings...");
            SDL_Delay(1);
        }

        tms_debugf("Ok, ready, saving...");

        for (size_t i = 0; copy_settings[i] != NULL; i++) {
            tms_infof("writing setting %s", copy_settings[i]);
            memcpy(settings[copy_settings[i]], local_settings[copy_settings[i]], sizeof(setting));
        }

        if (!settings.save()) {
            tms_errorf("Unable to save settings.");
        } else {
            tms_infof("Successfully saved settings to file.");
        }

        tms_infof("Successfully saved settings, returning...");

        sm::load_settings();
        _tms.touch_controls = settings["touch_controls"]->v.b;
        P.can_reload_graphics = true;
        is_saving = false;
    }

    static void save_settings() {
        tms_infof("Saving settings...");
        is_saving = true;
        P.can_reload_graphics = false;
        P.can_set_settings = false;
        P.add_action(ACTION_RELOAD_GRAPHICS, 0);
        std::thread thread(save_thread);
        thread.detach();
    }

    static void read_settings() {
        tms_infof("Reading settings...");
        for (auto& it: local_settings) {
            tms_debugf("free %s", it.first);
            delete local_settings[it.first];
        }

        local_settings.clear();
        for (size_t i = 0; copy_settings[i] != NULL; i++) {
            tms_debugf("reading setting %s", copy_settings[i]);
            setting *heap_setting = new setting;
            memcpy(heap_setting, settings[copy_settings[i]], sizeof(setting));
            local_settings[copy_settings[i]] = heap_setting;
        }
    }

    void open() {
        do_open = true;
        is_saving = false;
        if_done = IfDone::Nothing;
        read_settings();
    }

    static void im_resolution_picker(
        std::string friendly_name,
        const char *setting_x,
        const char *setting_y,
        const char* items[],
        int32_t items_x[],
        int32_t items_y[]
    ) {
        int item_count = 0;
        while (items[item_count] != NULL) { item_count++; }
        item_count++; //to overwrite the terminator

        std::string cust = string_format("%dx%d", local_settings[setting_x]->v.i, local_settings[setting_y]->v.i);
        items_x[item_count - 1] = local_settings[setting_x]->v.i;
        items_y[item_count - 1] = local_settings[setting_y]->v.i;
        items[item_count - 1] = cust.c_str();

        int item_current = item_count - 1;
        for (int i = 0; i < item_count; i++) {
            if (
                (items_x[i] == local_settings[setting_x]->v.i) &&
                (items_y[i] == local_settings[setting_y]->v.i)
            ) {
                item_current = i;
                break;
            }
        }

        ImGui::PushID(friendly_name.c_str());
        ImGui::TextUnformatted(friendly_name.c_str());
        ImGui::Combo("###combo", &item_current, items, (std::max)(item_count - 1, item_current + 1));
        ImGui::PopID();

        local_settings[setting_x]->v.i = items_x[item_current];
        local_settings[setting_y]->v.i = items_y[item_current];
    }

    void graphics_tab() {
        ImGui::SeparatorText("Shadows");
        ImGui::Checkbox("Enable shadows", (bool*) &local_settings["enable_shadows"]->v.b);
        ImGui::BeginDisabled(!local_settings["enable_shadows"]->v.b);
        ImGui::Checkbox("Smooth shadows", (bool*) &local_settings["shadow_quality"]->v.u8);
        {
            // XXX: add back "(native)"?
            const char* resolutions[] = { "4096x4096", "4096x2048", "2048x2048", "2048x1024", "1024x1024", "1024x512", "512x512", "512x256", NULL };
            int32_t values_x[] = { 4096, 4096, 2048, 2048, 1024, 1024, 512, 512, -1 };
            int32_t values_y[] = { 4096, 2048, 2048, 1024, 1024, 512,  512, 256, -1 };
            im_resolution_picker(
                "Shadow resolution",
                "shadow_map_resx",
                "shadow_map_resy",
                resolutions,
                values_x,
                values_y
            );
        }
        ImGui::EndDisabled();

        ImGui::SeparatorText("Ambient Occlusion");
        ImGui::Checkbox("Enable AO", (bool*) &local_settings["enable_ao"]->v.b);
        ImGui::SetItemTooltip("Adds subtle shading behind objects");

        ImGui::BeginDisabled(!local_settings["enable_ao"]->v.b);
        {
            const char* resolutions[] = { "512x512", "256x256", "128x128", NULL };
            int32_t values[] = { 512, 256, 128, -1 };
            im_resolution_picker(
                "AO resolution",
                "ao_map_res",
                "ao_map_res",
                resolutions,
                values,
                values
            );
        }
        ImGui::EndDisabled();

        ImGui::SeparatorText("Miscellaneous");

        //XXX: Post-processing always enables bloom, so these two settings basically do the same thing
        bool is_bloom_enabled = local_settings["enable_bloom"]->v.b && local_settings["postprocess"]->v.b;
        if (ImGui::Checkbox("Enable bloom", &is_bloom_enabled)) {
            local_settings["postprocess"]->v.b = is_bloom_enabled;
            local_settings["enable_bloom"]->v.b = is_bloom_enabled;
        }
        ImGui::SetItemTooltip("Adds a subtle glow effect to bright objects");

        ImGui::Checkbox("Gamma correction", (bool*) &local_settings["gamma_correct"]->v.b);
        ImGui::SetItemTooltip("Adjusts the brightness and contrast to ensure accurate color representation");

        ImGui::Checkbox("Enable V-Sync", (bool*) &local_settings["vsync"]->v.b);
        ImGui::SetItemTooltip("Helps eliminate screen tearing by limiting the refresh rate.\nMay introduce a slight input delay.");
    }

    void sound_tab() {
        ImGui::SeparatorText("Volume");

        ImGui::BeginDisabled(local_settings["muted"]->v.b);
        ImGui::SliderFloat(
            "###volume-slider",
            local_settings["muted"]->v.b ? REF_FZERO : ((float*) &local_settings["volume"]->v.f),
            0.f, 1.f
        );
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            float volume = sm::volume;
            sm::volume = local_settings["volume"]->v.f;
            sm::play(&sm::click, sm::position.x, sm::position.y, rand(), 1., false, 0, true);
            sm::volume = volume;
        }
        ImGui::EndDisabled();

        ImGui::Checkbox("Mute", (bool*) &local_settings["muted"]->v.b);
    }

    void controls_tab() {
        ImGui::Checkbox("Enable touch controls", (bool*) &local_settings["touch_controls"]->v.b);
        ImGui::SetItemTooltip("Enable touchscreen controls and other touch-related behaviour.");

        bool alternate_controls = local_settings["control_type"]->v.i == 0;
        ImGui::Checkbox("Alternate keyboard-only adventure controls", &alternate_controls);
        local_settings["control_type"]->v.i = alternate_controls ? 0 : 1;
        ImGui::SetItemTooltip("Alternate keyboard-only adventure controls. Arrow keys to move and aim, Ctrl to attack, PgUp/PgDown to change layers.");

        ImGui::SeparatorText("Camera");

        ImGui::TextUnformatted("Camera speed");
        ImGui::SliderFloat("###Camera-speed", (float*) &local_settings["cam_speed_modifier"]->v.f, 0.1, 15., "%.1f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Checkbox("Smooth camera", (bool*) &local_settings["smooth_cam"]->v.b);

        ImGui::TextUnformatted("Zoom speed");
        ImGui::SliderFloat("###Camera-zoom-speed", (float*) &local_settings["zoom_speed"]->v.f, 0.1, 3., "%.1f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Checkbox("Smooth zoom", (bool*) &local_settings["smooth_zoom"]->v.b);

        ImGui::SeparatorText("Mouse");

        ImGui::Checkbox("Enable cursor jail", (bool*) &local_settings["jail_cursor"]->v.b);
        ImGui::SetItemTooltip("Lock the cursor inside the the game window while playing a level");

        ImGui::Checkbox("Enable RC cursor lock", (bool*) &local_settings["rc_lock_cursor"]->v.b);
        ImGui::SetItemTooltip("Lock the cursor while controlling RC widgets");

        ImGui::TextUnformatted("Widget sensitivity");
        ImGui::SliderFloat("###Widget-sensitivity", (float*) &local_settings["widget_control_sensitivity"]->v.f, 0.1, 8., "%.1f");
        ImGui::SetItemTooltip("Controls the mouse-movement-sensitivity used to control sliders, radials and fields using the hotkey mode.");
    }

    void interface_tab() {
        ImGui::TextUnformatted("UI Scale (requires restart)");
        ImGui::SliderFloat("###uiScale", &local_settings["uiscale"]->v.f, 0.5, 2., "%.1f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::SeparatorText("Windowing");

        ImGui::Checkbox("Fullscreen mode", (bool*) &local_settings["window_fullscreen"]->v.b);

        ImGui::Checkbox("Resizable window (BETA)", (bool*) &local_settings["window_resizable"]->v.b);
        ImGui::SetItemTooltip("Allow the window to be resized.\n\nNOTE: Principia does not support resizing while in-game.\nThings will break.");

        ImGui::Checkbox("Autosave screen size", (bool*) &local_settings["autosave_screensize"]->v.b);
        ImGui::SetItemTooltip("Automatically save the current screen size when the window is resized, for next time the game is launched.");

        ImGui::SeparatorText("Highscores");

        ImGui::Checkbox("Automatically submit highscores", (bool*) &local_settings["score_automatically_submit"]->v.b);

        ImGui::Checkbox("Ask before submitting highscores", (bool*) &local_settings["score_ask_before_submitting"]->v.b);

        ImGui::SeparatorText("Advanced");

        ImGui::TextUnformatted("Display FPS");
        ImGui::Combo("###displayFPS", (int*) &local_settings["display_fps"]->v.u8, "Off\0On\0Graph\0Graph (Raw)\0", 4);
    }

    void sandbox_tab() {
        ImGui::Checkbox("Hide tips & tricks", (bool*) &local_settings["hide_tips"]->v.b);
        ImGui::SetItemTooltip("Hide the tips & tricks dialog that appears when opening the sandbox.");

        ImGui::Checkbox("Display object IDs", (bool*) &local_settings["display_object_id"]->v.b);

        ImGui::Checkbox("Display grapher values", (bool*) &local_settings["display_grapher_value"]->v.b);
        ImGui::SetItemTooltip("Display the current value that passes through a grapher when simulating a level.");

        ImGui::Checkbox("Display wireless frequencies", (bool*) &local_settings["display_wireless_frequency"]->v.b);
        ImGui::SetItemTooltip("Display the frequency of the Receiver or the Mini transmitter when paused and zoomed in (sandbox only).");

        ImGui::Checkbox("Do not confirm quitting sandbox adventure", (bool*) &local_settings["dna_sandbox_back"]->v.b);
        ImGui::SetItemTooltip("Do not show the \"Are you sure you want to quit?\" dialog when exiting a sandbox adventure level.");

        ImGui::SeparatorText("Menu");

        ImGui::TextUnformatted("Menu scroll speed");
        ImGui::SliderFloat("###Menu-speed", (float*)&local_settings["menu_speed"]->v.f, 1., 15., "%.1f",
            ImGuiSliderFlags_AlwaysClamp);

        ImGui::Checkbox("Smooth menu scrolling", (bool*) &local_settings["smooth_menu"]->v.b);
    }

    void layout() {
        handle_do_open(&do_open, "Settings");
        ImGui_CenterNextWindow();

        auto begin_scrolling_tab = [](const char *name, const char *id, const char *tooltip) {
            bool active = ImGui::BeginTabItem(name);
            ImGui::SetItemTooltip("%s", tooltip);
            if (!active)
                return false;

            ImGui::BeginChild(id, ImGui::GetContentRegionAvail(), false);
            return true;
        };

        auto end_scrolling_tab = []() {
            ImGui::EndChild();
            ImGui::EndTabItem();
        };

        const float footer_height =
            ImGui::GetStyle().SeparatorSize +
            ImGui::GetFrameHeightWithSpacing() +
            ImGui::GetStyle().ItemSpacing.y;

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowSize(
            ImVec2(
                0,
                std::min(viewport->WorkSize.y * 0.98f, UI(510) + footer_height)),
            ImGuiCond_Always);
        if (ImGui::BeginPopupModal("Settings", is_saving ? NULL : REF_TRUE, MODAL_FLAGS)) {
            if ((if_done == IfDone::Exit) && !is_saving) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            } else if ((if_done == IfDone::Reload) && !is_saving) {
                if_done = IfDone::Nothing;
                read_settings();
            }

            ImGui::BeginChild("##settings", ImVec2(0, -footer_height), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX);

            if (ImGui::BeginTabBar("###settings-tabbbar")) {
                if (begin_scrolling_tab("Graphics", "###graphics-tab", "Configure graphics and display settings")) {
                    graphics_tab();
                    end_scrolling_tab();
                }

                if (begin_scrolling_tab("Sound", "###sound-tab", "Configure sound and volume settings")) {
                    sound_tab();
                    end_scrolling_tab();
                }

                if (begin_scrolling_tab("Controls", "###controls-tab", "Mouse, keyboard and touchscreen settings")) {
                    controls_tab();
                    end_scrolling_tab();
                }

                if (begin_scrolling_tab("Interface", "###interface-tab", "UI scaling, visibility and other interface settings")) {
                    interface_tab();
                    end_scrolling_tab();
                }

                if (begin_scrolling_tab("Sandbox", "###sandbox-tab", "Configure sandbox settings")) {
                    sandbox_tab();
                    end_scrolling_tab();
                }

                ImGui::EndTabBar();
            }

            ImGui::EndChild();

            if (ImGui::GetContentRegionAvail().y > footer_height) {
                ImGui::SetCursorPosY((ImGui::GetContentRegionAvail().y + ImGui::GetCursorScreenPos().y - ImGui::GetWindowPos().y) - footer_height);
            }
            ImGui::Separator();
            ImGui::BeginDisabled(is_saving);
            bool do_save = false;
            if (ImGui::Button("Apply", UI(70., 0.))) {
                if_done = IfDone::Reload;
                save_settings();
            }
            ImGui::SameLine();
            if (ImGui::Button("Save", UI(70., 0.))) {
                if_done = IfDone::Exit;
                save_settings();
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", UI(70., 0.))) {
                if_done = IfDone::Exit;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}
