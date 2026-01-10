#include "settings.hh"
#include "soundmanager.hh"
#include "ui_imgui.hh"
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
        "touch_controls",
        "jail_cursor",
        "cam_speed_modifier",
        "smooth_cam",
        "zoom_speed",
        "smooth_zoom",
        //"smooth_menu",
        //INTERFACE
        "hide_tips",
        "display_grapher_value",
        "display_object_id",
        "display_fps",
        "uiscale",
        "first_adventure", "tutorial",
        "menu_speed",
        "smooth_menu",
        "emulate_touch",
        "rc_lock_cursor",
#ifdef DEBUG
        "debug",
#endif
        NULL
    };

    static void on_before_apply() {
        tms_infof("Preparing to reload stuff later...");
    }

    static void on_after_apply() {
        tms_infof("Now, reloading some stuff (as promised!)...");

        //Reload sound manager settings to apply new volume
        sm::load_settings();
    }

    static void save_thread() {
        tms_debugf("inside save_thread()");
        tms_infof("Waiting for can_set_settings...");
        while (!P.can_set_settings) {
            tms_debugf("Waiting for can_set_settings...");
            SDL_Delay(1);
        }
        tms_debugf("Ok, ready, saving...");
        on_before_apply();
        for (size_t i = 0; copy_settings[i] != NULL; i++) {
            tms_infof("writing setting %s", copy_settings[i]);
            memcpy(settings[copy_settings[i]], local_settings[copy_settings[i]], sizeof(setting));
        }
        tms_assertf(settings.save(), "Unable to save settings.");
        on_after_apply();
        tms_infof("Successfully saved settings, returning...");
        P.can_reload_graphics = true;
        is_saving = false;
        tms_debugf("save_thread() completed");
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
            free((void*) local_settings[it.first]);
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

    void layout() {
        handle_do_open(&do_open, "Settings");
        ImGui_CenterNextWindow();
        //TODO unsaved changes indicator
        if (ImGui::BeginPopupModal("Settings", is_saving ? NULL : REF_TRUE, MODAL_FLAGS)) {
            if ((if_done == IfDone::Exit) && !is_saving) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            } else if ((if_done == IfDone::Reload) && !is_saving) {
                if_done = IfDone::Nothing;
                read_settings();
            }

            if (ImGui::BeginTabBar("###settings-tabbbar")) {
                bool graphics_tab = ImGui::BeginTabItem("Graphics");
                ImGui::SetItemTooltip("Configure graphics and display settings");
                if (graphics_tab) {
                    // ImGui::BeginTable("###graphics-settings", 2);
                    // ImGui::TableNextColumn();

                    ImGui::SeparatorText("Shadows");
                    ImGui::Checkbox("Enable shadows", (bool*) &local_settings["enable_shadows"]->v.b);
                    ImGui::BeginDisabled(!local_settings["enable_shadows"]->v.b);
                    ImGui::Checkbox("Smooth shadows", (bool*) &local_settings["shadow_quality"]->v.u8);
                    {
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

                    ImGui::SeparatorText("Post-processing");

                    // ImGui::Checkbox("Enable post-processing", (bool*) &local_settings["postprocess"]->v.b);
                    // ImGui::BeginDisabled(!local_settings["postprocess"]->v.b);
                    // ImGui::Checkbox("Enable bloom", local_settings["postprocess"]->v.b ? ((bool*) &local_settings["enable_bloom"]->v.b) : REF_FALSE);
                    // ImGui::SetItemTooltip("Adds a subtle glow effect to bright objects");
                    // ImGui::EndDisabled();ImGui::Checkbox("Enable post-processing", (bool*) &local_settings["postprocess"]->v.b);

                    //XXX: Post-processing always enables bloom, so these two settings basically do the same thing
                    bool is_bloom_enabled = local_settings["enable_bloom"]->v.b && local_settings["postprocess"]->v.b;
                    if (ImGui::Checkbox("Enable bloom", &is_bloom_enabled)) {
                        local_settings["postprocess"]->v.b = is_bloom_enabled;
                        local_settings["enable_bloom"]->v.b = is_bloom_enabled;
                    }
                    ImGui::SetItemTooltip("Adds a subtle glow effect to bright objects");

                    ImGui::Checkbox("Gamma correction", (bool*) &local_settings["gamma_correct"]->v.b);
                    ImGui::SetItemTooltip("Adjusts the brightness and contrast to ensure accurate color representation");

                    ImGui::SeparatorText("Display");

                    //VSync option has no effect on Android
                    #ifdef TMS_BACKEND_PC
                    ImGui::Checkbox("Enable V-Sync", (bool*) &local_settings["vsync"]->v.b);
                    ImGui::SetItemTooltip("Helps eliminate screen tearing by limiting the refresh rate.\nMay introduce a slight input delay.");
                    #endif

                    ImGui::EndTabItem();
                }

                bool sound_tab = ImGui::BeginTabItem("Sound");
                ImGui::SetItemTooltip("Change volume and other sound settings");
                if (sound_tab) {
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

                    ImGui::EndTabItem();
                }

                bool controls_tab = ImGui::BeginTabItem("Controls");
                ImGui::SetItemTooltip("Mouse, keyboard and touchscreen settings");
                if (controls_tab) {
                    ImGui::EndTabItem();

                    ImGui::SeparatorText("Camera");

                    ImGui::TextUnformatted("Camera speed");
                    ImGui::SliderFloat("###Camera-speed", (float*) &local_settings["cam_speed_modifier"]->v.f, 0.1, 15.);

                    ImGui::Checkbox("Smooth camera", (bool*) &local_settings["smooth_cam"]->v.b);

                    ImGui::TextUnformatted("Zoom speed");
                    ImGui::SliderFloat("###Camera-zoom-speed", (float*) &local_settings["zoom_speed"]->v.f, 0.1, 3.);

                    ImGui::Checkbox("Smooth zoom", (bool*) &local_settings["smooth_zoom"]->v.b);

                    ImGui::SeparatorText("Menu");

                    ImGui::TextUnformatted("Menu scroll speed");
                    ImGui::SliderFloat("###Menu-speed", (float*) &local_settings["menu_speed"]->v.f, 1., 15.);

                    ImGui::Checkbox("Smooth menu scrolling", (bool*) &local_settings["smooth_menu"]->v.b);

                    ImGui::SeparatorText("Mouse");

                    ImGui::Checkbox("Enable cursor jail", (bool*) &local_settings["jail_cursor"]->v.b);
                    ImGui::SetItemTooltip("Lock the cursor inside the the game window while playing a level");

                    ImGui::Checkbox("Enable RC cursor lock", (bool*) &local_settings["rc_lock_cursor"]->v.b);
                    ImGui::SetItemTooltip("Lock the cursor while controlling RC widgets");

                    ImGui::SeparatorText("Touchscreen");

                    ImGui::Checkbox("Enable on-screen controls", (bool*) &local_settings["touch_controls"]->v.b);
                    ImGui::SetItemTooltip("Enable touch-friendly on-screen controls");

                    ImGui::Checkbox("Emulate touch", (bool*) &local_settings["emulate_touch"]->v.b);
                    ImGui::SetItemTooltip("Enable this if you use an external device other than a mouse to control Principia, such as a Wacom pad.");
                }

                bool interface_tab = ImGui::BeginTabItem("Interface");
                ImGui::SetItemTooltip("Change UI scaling, visibility options and other interface settings");
                if (interface_tab) {
                    ImGui::SeparatorText("Interface");

                    ImGui::TextUnformatted("UI Scale (requires restart)");
                    std::string display_value = string_format("%.01f", local_settings["uiscale"]->v.f);
                    ImGui::SliderFloat("###uiScale", &local_settings["uiscale"]->v.f, 0.2, 2., display_value.c_str());
                    local_settings["uiscale"]->v.f = (int)(local_settings["uiscale"]->v.f * 10) * 0.1f;

                    ImGui::SeparatorText("Help & Tips");

                    ImGui::Checkbox("Do not show tips", (bool*) &local_settings["hide_tips"]->v.b);

                    ImGui::SeparatorText("Advanced");

                    ImGui::Checkbox("Display grapher values", (bool*) &local_settings["display_grapher_value"]->v.b);

                    ImGui::Checkbox("Display object IDs", (bool*) &local_settings["display_object_id"]->v.b);

                    ImGui::TextUnformatted("Display FPS");
                    ImGui::Combo("###displayFPS", (int*) &local_settings["display_fps"]->v.u8, "Off\0On\0Graph\0Graph (Raw)\0", 4);

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();

                //This assumes separator height == 1. which results in actual height of 0
                float button_area_height =
                    ImGui::GetStyle().ItemSpacing.y + //Separator spacing
                    (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.); // Buttons
                if (ImGui::GetContentRegionAvail().y > button_area_height) {
                    ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y - button_area_height);
                }
                ImGui::Separator();
                ImGui::BeginDisabled(is_saving);
                bool do_save = false;
                if (ImGui::Button("Apply")) {
                    if_done = IfDone::Reload;
                    save_settings();
                }
                ImGui::SameLine();
                if (ImGui::Button("Save")) {
                    if_done = IfDone::Exit;
                    save_settings();
                }
                ImGui::EndDisabled();
            }
            ImGui::EndPopup();
        }
    }
}
