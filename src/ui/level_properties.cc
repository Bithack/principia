#include "simplebg.hh"
#include "ui_imgui.hh"

namespace UiLevelProperties {
    static bool do_open = false;

    void open() {
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "Level properties");
        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSizeConstraints(ImVec2(450., 550.), ImVec2(FLT_MAX, FLT_MAX));
        if (ImGui::BeginPopupModal("Level properties", REF_TRUE, MODAL_FLAGS)) {
            if (ImGui::BeginTabBar("###lvlproptabbar")) {
                if (ImGui::BeginTabItem("Information")) {
                    ImGui::SeparatorText("Metadata");

                    std::string lvl_name(W->level.name, W->level.name_len);
                    bool over_soft_limit = lvl_name.length() >= LEVEL_NAME_LEN_SOFT_LIMIT + 1;
                    ImGui::BeginDisabled(over_soft_limit);
                    ImGui::TextUnformatted("Name");
                    if (ImGui::InputTextWithHint("##LevelName", LEVEL_NAME_PLACEHOLDER, &lvl_name)) {
                        size_t to_copy = (size_t)(std::min)((int) lvl_name.length(), LEVEL_NAME_LEN_HARD_LIMIT);
                        memcpy(&W->level.name, lvl_name.data(), to_copy);
                        W->level.name_len = lvl_name.length();
                    }
                    ImGui::EndDisabled();

                    std::string lvl_descr(W->level.descr, W->level.descr_len);
                    ImGui::TextUnformatted("Description");
                    if (ImGui::InputTextMultiline("###LevelDescr", &lvl_descr)) {
                        W->level.descr = strdup(lvl_descr.c_str());
                        W->level.descr_len = lvl_descr.length();
                    }

                    ImGui::SeparatorText("Type");
                    if (ImGui::RadioButton("Adventure", W->level.type == LCAT_ADVENTURE)) {
                        P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_ADVENTURE);
                    }
                    if (ImGui::RadioButton("Puzzle", W->level.type == LCAT_PUZZLE)) {
                        P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_PUZZLE);
                    }
                    if (ImGui::RadioButton("Custom", W->level.type == LCAT_CUSTOM)) {
                        P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_CUSTOM);
                    }

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("World")) {
                    //Background
                    {
                        static int current_item = 0;
                            const char* items[] = { "Item 1", "Item 2", "Item 3", "Item 4" };

                        if (ImGui::Combo("Background", &current_item, available_bgs, num_bgs)) {
                            W->level.bg = current_item;
                            P.add_action(ACTION_RELOAD_LEVEL, 0);
                        }
                        ImGuiStyle style = ImGui::GetStyle();

                        bool current_bg_colored = false;
                        for (const int *ptr = colored_bgs; ; ++ptr) {
                            if ((*ptr == -1) || (*ptr == W->level.bg)) {
                                current_bg_colored = *ptr != -1;
                                break;
                            }
                        }
                        if (current_bg_colored) {
                            float col[4];
                            unpack_rgba(W->level.bg_color, &col[0], &col[1], &col[2], &col[3]);
                            col[3] = 1.;
                            //ImGui::SameLine();
                            if (ImGui::ColorEdit4("###bgc", col, ImGuiColorEditFlags_NoAlpha)) {
                                W->level.bg_color = pack_rgba(col[0], col[1], col[2], 1.);
                            }
                            if (ImGui::IsItemDeactivatedAfterEdit()) {
                                P.add_action(ACTION_RELOAD_LEVEL, 0);
                            }
                        }
                    }

                    //Gravity
                    {
                        ImGui::SeparatorText("Gravity");
                        ImGui::SliderFloat("X###gravityx", &W->level.gravity_x, -40., 40., "%.01f");
                        ImGui::SliderFloat("Y###gravityy", &W->level.gravity_y, -40., 40., "%.01f");
                    }
                    //ImGui::SameLine();

                    //Border size
                    //TODO check for "impossible" border sizes (<2, causes glitchy rendering)
                    {
                        static const ImVec2 bs = ImVec2(128. + 24., 100.);

                        float ix = 48.;
                        float iy = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.;

                        bool do_reload = false;

                        auto slider = [ix, &do_reload](const char *id, uint16_t *x, float flip, int flags = 0) {
                            ImGui::PushItemWidth(ix);
                            int xint = *x;
                            if (ImGui::DragInt(id, &xint, flip * 0.1f, 0, 0, "%d", flags)) {
                                *x = (std::max)(0, xint);
                            }
                            ImGui::PopItemWidth();
                            do_reload |= ImGui::IsItemDeactivatedAfterEdit();
                        };

                        ImGui::SeparatorText("Border size");

                        ImVec2 c = ImGui::GetCursorPos();
                        ImVec2 p_min = ImGui::GetCursorScreenPos();
                        ImVec2 p_max = ImVec2(p_min.x + bs.x, p_min.y + bs.y);
                        ImGui::Dummy(bs);
                        ImVec2 after_dummy = ImGui::GetCursorPos();

                        ImGui::GetWindowDrawList()->AddRect(
                            ImVec2(p_min.x + ix / 2, p_min.y + iy / 2),
                            ImVec2(p_max.x - ix / 2, p_max.y - iy / 2),
                            ImColor(255, 255, 255, 64),
                            5., 0, 2.
                        );

                        ImGui::SetCursorPos(ImVec2(c.x, c.y + bs.y / 2 - iy / 2));
                        slider("###x0", &W->level.size_x[0], -1.);

                        ImGui::SetCursorPos(ImVec2(c.x + bs.x - ix, c.y + bs.y / 2 - iy / 2));
                        slider("###x1", &W->level.size_x[1], 1.);

                        ImGui::SetCursorPos(ImVec2(c.x + bs.x / 2 - ix / 2, c.y + bs.y - iy));
                        slider("###y0", &W->level.size_y[0], -1., ImGuiSliderFlags_Vertical);

                        ImGui::SetCursorPos(ImVec2(c.x + bs.x / 2 - ix / 2, c.y));
                        slider("###y1", &W->level.size_y[1], 1., ImGuiSliderFlags_Vertical);

                        //Button right in the center
                        // const char *btext = "FIT";
                        // float bix = ImGui::CalcTextSize(btext).y + ImGui::GetStyle().FramePadding.x * 2.;
                        // ImGui::SetCursorPos(ImVec2(c.x + bs.x / 2 - bix / 2, c.y + bs.y / 2 - iy / 2));
                        // ImGui::Button(btext);

                        if (do_reload) P.add_action(ACTION_RELOAD_LEVEL, 0);

                        ImGui::SetCursorPos(after_dummy);

                        if (ImGui::Button("Auto-fit borders", ImVec2(bs.x, 0.))) {
                            P.add_action(ACTION_AUTOFIT_LEVEL_BORDERS, 0);
                        }
                    }

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Physics")) {
                    ImGui::TextUnformatted("These settings can affect simulation performance.\nyada yada yada this is a placeholder text\nfor the physics tab :3");

                    auto reload_if_changed = [](){
                        if (ImGui::IsItemDeactivatedAfterEdit()) {
                            P.add_action(ACTION_RELOAD_LEVEL, 0);
                        }
                    };
                    auto slider_uint8t = [](uint8_t* x) {
                        int tmp = (int)*x;
                        //HACK: use pointer as unique id
                        ImGui::PushID((size_t)x);
                        if (ImGui::SliderInt("###slider", &tmp, 10, 255)) {
                            *x = tmp & 0xff;
                        }
                        ImGui::PopID();
                    };
                    auto slider_float = [](float* x, float rng) {
                        float tmp = *x;
                        //HACK: use pointer as unique id
                        ImGui::PushID((size_t)x);
                        if (ImGui::SliderFloat("###slider", &tmp, 0.0f, rng)) {
                            *x = tmp;
                        }
                        ImGui::PopID();
                    };

                    ImGui::SeparatorText("");
                    ImGui::TextUnformatted("Position Iterations");
                    slider_uint8t(&W->level.position_iterations);
                    reload_if_changed();
                    ImGui::TextUnformatted("Velocity Iterations");
                    slider_uint8t(&W->level.velocity_iterations);
                    reload_if_changed();

                    ImGui::SeparatorText("");
                    ImGui::TextUnformatted("Prismatic Tolerance");
                    slider_float(&W->level.prismatic_tolerance, 0.075f);
                    reload_if_changed();
                    ImGui::TextUnformatted("Pivot Tolerance");
                    slider_float(&W->level.pivot_tolerance, 0.075f);
                    reload_if_changed();

                    ImGui::SeparatorText("");
                    ImGui::TextUnformatted("Linear Damping");
                    slider_float(&W->level.linear_damping, 10.00f);
                    reload_if_changed();
                    ImGui::TextUnformatted("Angular Damping");
                    slider_float(&W->level.angular_damping, 10.00f);
                    reload_if_changed();

                    ImGui::SeparatorText("");
                    ImGui::TextUnformatted("Joint Friction");
                    slider_float(&W->level.joint_friction, 10.00f);
                    reload_if_changed();

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Gameplay")) {
                    auto lvl_flag_toggle = [](uint64_t flag, const char *label, const char *help, bool disabled = false) {
                        bool x = (W->level.flags & flag) != 0;
                        if (disabled) {
                            ImGui::BeginDisabled();
                        }
                        if (ImGui::Checkbox(label, &x)) {
                            if (x) {
                                W->level.flags |= flag;
                            } else {
                                W->level.flags &= ~flag;
                            }
                            P.add_action(ACTION_RELOAD_LEVEL, 0);
                        }
                        if (disabled) {
                            ImGui::EndDisabled();
                        }
                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) {
                            if (ImGui::BeginTooltip()) {
                                if ((help != 0) && (*help != 0)) {
                                    ImGui::TextUnformatted(help);
                                } else {
                                    ImGui::BeginDisabled();
                                    ImGui::TextUnformatted("<no help available>");
                                    ImGui::EndDisabled();
                                }
                                ImGui::EndTooltip();
                            }
                        }
                    };

                    if (ImGui::BeginChild("###gameplay-scroll", ImVec2(0, ImGui::GetContentRegionAvail().y))) {
                        ImGui::SeparatorText("Flags");
                        lvl_flag_toggle(
                            LVL_DISABLE_LAYER_SWITCH,
                            "Disable layer switch",
                                "In adventure mode, disable manual robot layer switching.\nIn puzzle mode, restrict layer switching for objects.",
                            !((W->level.type == LCAT_PUZZLE) || (W->level.type == LCAT_ADVENTURE))
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_INTERACTIVE,
                            "Disable interactive",
                            "Disable the ability to handle interactive objects."
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_FALL_DAMAGE,
                            "Disable fall damage",
                            "Disable the damage robots take when they fall."
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_CONNECTIONS,
                            "Disable connections",
                            "Disable the ability to create connections\n(Puzzle mode only)",
                            W->level.type != LCAT_PUZZLE
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_STATIC_CONNS,
                            "Disable static connections",
                            "Disable connections to static objects such as platforms\n(Puzzle mode only)",
                            W->level.type != LCAT_PUZZLE
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_JUMP,
                            "Disable jumping",
                            "Disable the robots ability to jump manually\n(Adventure mode only)",
                            W->level.type != LCAT_ADVENTURE
                        );
                        ///XXX: this applies to sandbox mode too, right?
                        lvl_flag_toggle(
                            LVL_DISABLE_ROBOT_HIT_SCORE,
                            "Disable robot hit score",
                            "Do not award points for shooting other robots"
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_ZOOM,
                            "Disable zoom",
                            "Disable the player's ability to zoom."
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_CAM_MOVEMENT,
                            "Disable cam movement",
                            "Disable the players ability to manually move the camera."
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_INITIAL_WAIT,
                            "Disable initial wait",
                            "Disable the waiting state when a level is started."
                        );
                        lvl_flag_toggle(
                            LVL_UNLIMITED_ENEMY_VISION,
                            "Unlimited enemy vision",
                            "If enabled, enemy robots will be able see their target from any distance and through obstacles, and will always try to find a path to it."
                        );
                        lvl_flag_toggle(
                            LVL_ENABLE_INTERACTIVE_DESTRUCTION,
                            "Interactive destruction",
                            "If enabled, interactive objects can be destroyed by shooting or blowing them up."
                        );
                        lvl_flag_toggle(
                            LVL_ABSORB_DEAD_ENEMIES,
                            "Absorb dead enemies",
                            "If enabled, enemy corpses will despawn after a short amount of time."
                        );
                        lvl_flag_toggle(
                            LVL_SNAP,
                            "Snap by default",
                            "When the player drags or rotates an object it will snap to a grid by default (good for easy beginner levels).\n(Puzzle mode only)",
                            W->level.type != LCAT_PUZZLE
                        );

                        lvl_flag_toggle(
                            LVL_NAIL_CONNS,
                            "Hide beam connections",
                            "Use less visible nail-shaped connections for planks and beams.\nExisting connections will not be changed if this flag is modified."
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_CONTINUE_BUTTON,
                            "Disable continue button",
                            "If initial wait is disabled, this option disables the Continue button in the lower right corner. Use pkgwarp to go to the next level instead."
                        );
                        lvl_flag_toggle(
                            LVL_SINGLE_LAYER_EXPLOSIONS,
                            "Single-layer explosions",
                            "Enable this flag to prevent explosions from reaching objects in other layers."
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_DAMAGE,
                            "Disable damage",
                            "Disable damage to any robot."
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_3RD_LAYER,
                            "Disable third layer",
                            "If enabled, prevents objects from being moved to the third layer."
                        );
                        lvl_flag_toggle(
                            LVL_PORTRAIT_MODE,
                            "Portrait mode",
                            "If enabled, the view will be set to portrait (vertical) mode during play."
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_RC_CAMERA_SNAP,
                            "Disable RC camera snap",
                            "If enabled, the camera won't move to any selected RC."
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_PHYSICS,
                            "Disable physics",
                            "If enabled, physics simulation in the level will be disabled."
                        );
                        lvl_flag_toggle(
                            LVL_DO_NOT_REQUIRE_DRAGFIELD,
                            "Do not require dragfield",
                            "If enabled, dragfields will not be required in order to move interactive objects."
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_ROBOT_SPECIAL_ACTION,
                            "Disable robot special action",
                            "If enabled, the adventure robot won't be able to perform it's special action."
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_ADVENTURE_MAX_ZOOM,
                            "Disable adventure max zoom",
                            "If enabled, the zoom will no longer be limited while following the adventure robot.\n(Adventure mode only)",
                            W->level.type != LCAT_ADVENTURE
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_ROAM_LAYER_SWITCH,
                            "Disable roam layer switch",
                            "Disable the roaming robot's ability to change layers."
                        );
                        lvl_flag_toggle(
                            LVL_CHUNKED_LEVEL_LOADING,
                            "Chunked level loading",
                            NULL
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_CAVEVIEW,
                            "Disable adventure caveview",
                            "Disable the caveview which appears when the adventure robot is in the second layer, with terrain in front of it in the third layer"
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_ROCKET_TRIGGER_EXPLOSIVES,
                            "Disable rocket triggering explosives",
                            "Prevent rockets from triggering any explosives when in contact"
                        );
                        lvl_flag_toggle(
                            LVL_STORE_SCORE_ON_GAME_OVER,
                            "Store high score on game over",
                            NULL
                        );
                        lvl_flag_toggle(
                            LVL_ALLOW_HIGH_SCORE_SUBMISSIONS,
                            "Allow high score submissions",
                            "Allow players to submit their high scores to be displayed on your levels community page."
                        );
                        lvl_flag_toggle(
                            LVL_LOWER_SCORE_IS_BETTER,
                            "Lower score is better",
                            "A lower score is considered better than a higher score."
                        );
                        lvl_flag_toggle(
                            LVL_AUTOMATICALLY_SUBMIT_SCORE,
                            "Automatically submit score on finish",
                            "Automatically submit score for the user when the level finishes."
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_ENDSCREENS,
                            "Disable end-screens",
                            "Disable any end-game sound or messages. Works well when \"Pause on win\" is disabled.\nNote that this also disables the score submission button.\nYou can still use the `game:submit_score()` lua function in order to submit highscores."
                        );
                        lvl_flag_toggle(
                            LVL_ALLOW_QUICKSAVING,
                            "Allow quicksaving",
                            "If enabled, the player can save their progress at any time."
                        );
                        lvl_flag_toggle(
                            LVL_ALLOW_RESPAWN_WITHOUT_CHECKPOINT,
                            "Allow respawn without checkpoint",
                            "If disabled, robots cannot respawn if they are not associated with any checkpoint."
                        );
                        lvl_flag_toggle(
                            LVL_DEAD_CREATURE_DESTRUCTION,
                            "Allow dead creature destruction",
                            "If enabled, creature corpses can be destroyed by shooting them."
                        );
                    }
                    ImGui::EndChild();

                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndPopup();
        }
    }
}
