#include "main.hh"
#include "simplebg.hh"
#include "imgui.hh"
#include "ui.hh"
#include "world.hh"

namespace UiLevelProperties {
    static bool do_open = false;

    static std::string name = "";
    static std::string description = "";
    static uint8_t type = 0;

    static int bg = 0;
    static float bg_color[4];
    static int border[4]; // Left, right, bottom, top
    static float gravity[2];

    static int position_iterations;
    static int velocity_iterations;
    static float prismatic_tolerance;
    static float pivot_tolerance;
    static float linear_damping;
    static float angular_damping;
    static float joint_friction;

    static int final_score;
    static bool pause_on_win;
    static bool display_score;
    static float creature_absorb_time;
    static float player_respawn_time;
    static uint64_t flags;

    static bool is_upgrading = false;

    void apply_properties() {
        W->level.name_len = name.length();
        memcpy(W->level.name, name.c_str(), W->level.name_len);

        if (description.length() > 0) {
            W->level.descr_len = description.length();
            W->level.descr = (char*)realloc(W->level.descr, description.length());

            memcpy(W->level.descr, description.c_str(), description.length());
        } else
            W->level.descr_len = 0;

        W->level.bg = bg;
        W->level.bg_color = pack_rgba(bg_color[0], bg_color[1], bg_color[2], bg_color[3]);

        float left = border[0];
        float right = border[1];
        float down = border[2];
        float up = border[3];

        float w = left + right;
        float h = down + up;

        bool resized = false;

        if (w < 5.f) {
            resized = true;
            left += 6-(uint16_t)w;
        }
        if (h < 5.f) {
            resized = true;
            down += 6-(uint16_t)w;
        }

        if (resized)
            ui::message("Your level size was increased to the minimum allowed.");

        W->level.size_x[0] = left;
        W->level.size_x[1] = right;
        W->level.size_y[0] = down;
        W->level.size_y[1] = up;

        W->level.gravity_x = gravity[0];
        W->level.gravity_y = gravity[1];

        W->level.position_iterations = (uint8_t)position_iterations;
        W->level.velocity_iterations = (uint8_t)velocity_iterations;
        W->level.prismatic_tolerance = prismatic_tolerance;
        W->level.pivot_tolerance = pivot_tolerance;
        W->level.linear_damping = linear_damping;
        W->level.angular_damping = angular_damping;
        W->level.joint_friction = joint_friction;

        W->level.final_score = final_score;
        W->level.pause_on_finish = pause_on_win;
        W->level.show_score = display_score;
        W->level.dead_enemy_absorb_time = creature_absorb_time;
        W->level.time_before_player_can_respawn = player_respawn_time;
        W->level.flags = flags;

        if (type == LCAT_ADVENTURE)
            P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_ADVENTURE);
        else if (type == LCAT_PUZZLE)
            P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_PUZZLE);
        else if (type == LCAT_CUSTOM)
            P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_CUSTOM);

        P.add_action(ACTION_RELOAD_LEVEL, 0);

        ImGui::CloseCurrentPopup();
    }

    void upgrade_level() {
        apply_properties();
        P.add_action(ACTION_UPGRADE_LEVEL, 0);

        ImGui::CloseCurrentPopup();
    }

    void reload_border_sizes() {
        border[0] = W->level.size_x[0];
        border[1] = W->level.size_x[1];
        border[2] = W->level.size_y[0];
        border[3] = W->level.size_y[1];
    }

    void open() {
        do_open = true;

        name = std::string(W->level.name, W->level.name_len);
        description = std::string(W->level.descr, W->level.descr_len);
        type = W->level.type;

        bg = W->level.bg;
        unpack_rgba(W->level.bg_color, &bg_color[0], &bg_color[1], &bg_color[2], &bg_color[3]);
        reload_border_sizes();
        gravity[0] = W->level.gravity_x;
        gravity[1] = W->level.gravity_y;

        position_iterations = W->level.position_iterations;
        velocity_iterations = W->level.velocity_iterations;
        prismatic_tolerance = W->level.prismatic_tolerance;
        pivot_tolerance = W->level.pivot_tolerance;
        linear_damping = W->level.linear_damping;
        angular_damping = W->level.angular_damping;
        joint_friction = W->level.joint_friction;

        final_score = W->level.final_score;
        pause_on_win = W->level.pause_on_finish;
        display_score = W->level.show_score;
        creature_absorb_time = W->level.dead_enemy_absorb_time;
        player_respawn_time = W->level.time_before_player_can_respawn;
        flags = W->level.flags;
    }

    void tab_information() {
        ImGui::TextUnformatted("Name");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##LevelName", &name);
        if (name.length() > 255)
            name = name.substr(0, 255);

        ImGui::TextUnformatted("Description");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextMultiline("###LevelDescr", &description);

        ImGui::SeparatorText("Type");
        if (ImGui::RadioButton("Adventure", type == LCAT_ADVENTURE))
            type = LCAT_ADVENTURE;

        if (ImGui::RadioButton("Puzzle", type == LCAT_PUZZLE))
            type = LCAT_PUZZLE;

        if (ImGui::RadioButton("Custom", type == LCAT_CUSTOM))
            type = LCAT_CUSTOM;
    }

    void tab_world() {
        ImGui::Combo("Background", &bg, available_bgs, num_bgs);
        if (bg == 6 || bg == 7) // SIX SEVEN
            ImGui::ColorEdit4("###bgc", &bg_color[0], ImGuiColorEditFlags_NoAlpha);

        ImGui::SeparatorText("Border size");
        ImGui::DragInt("Left", &border[0], .1, 0, UINT16_MAX, "%d", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragInt("Right", &border[1], .1, 0, UINT16_MAX, "%d", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragInt("Bottom", &border[2], .1, 0, UINT16_MAX, "%d", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragInt("Top", &border[3], .1, 0, UINT16_MAX, "%d", ImGuiSliderFlags_AlwaysClamp);

        if (ImGui::Button("Auto-fit borders", UI(150., 0.))) {
            P.add_action(ACTION_AUTOFIT_LEVEL_BORDERS, 0);
        }

        ImGui::SeparatorText("Gravity");
        ImGui::DragFloat("X###gravityx", &W->level.gravity_x, .1, -75., 75., "%.01f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("Y###gravityy", &W->level.gravity_y, .1, -75., 75., "%.01f", ImGuiSliderFlags_AlwaysClamp);
    }

    void tab_physics() {
        ImGui::TextUnformatted("Position Iterations");
        ImGui::SliderInt("###position_iterations", &position_iterations, 10, 255, nullptr, ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("The amount of position iterations primarily affects dynamic objects. Lower = better performance.");

        ImGui::TextUnformatted("Velocity Iterations");
        ImGui::SliderInt("###velocity_iterations", &velocity_iterations, 10, 255, nullptr, ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Primarily affects motors and connection. Lower = better performance.");

        ImGui::TextUnformatted("Prismatic Tolerance");
        ImGui::SliderFloat("###prismatic_tolerance", &prismatic_tolerance, 0.0f, 0.075f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::TextUnformatted("Pivot Tolerance");
        ImGui::SliderFloat("###pivot_tolerance", &pivot_tolerance, 0.0f, 0.075f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::TextUnformatted("Linear Damping");
        ImGui::SliderFloat("###linear_damping", &linear_damping, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::TextUnformatted("Angular Damping");
        ImGui::SliderFloat("###angular_damping", &angular_damping, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::TextUnformatted("Joint Friction");
        ImGui::SliderFloat("###joint_friction", &joint_friction, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    }

    void tab_gameplay() {
        ImGui::TextUnformatted("Final score");
        ImGui::InputInt("##final_score", &final_score);
        ImGui::SetItemTooltip("What score the player has to reach to win the level. (0 disables this win condition)");

        if (W->level.version == LEVEL_VERSION) {
            std::string button_label = "Level version: " + std::string(level_version_string(LEVEL_VERSION)) + " (latest)";
            ImGui::BeginDisabled();
            ImGui::Button(button_label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0));
            ImGui::EndDisabled();
        } else {
            std::string button_label = "Level version: " + std::string(level_version_string(W->level.version)) + " - upgrade to latest (" + std::string(level_version_string(LEVEL_VERSION)) + ")";
            if (ImGui::Button(button_label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                is_upgrading = true;
            }
        }

        if (is_upgrading) {
            ImGui::BeginChild("##upgrade_info", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);
            ImGui::TextWrapped("Are you sure you want to upgrade the version of this level?"
                "\n\n"
                "To get access to new features the version associated with this level "
                "must be upgraded. This action can not be undone. Please save a copy before "
                "upgrading your level."
                "\n\n"
                "By upgrading this level, some object properties such as density, "
                "restitution, friction and applied forces might differ from earlier versions and affect "
                "how your level is simulated.");

            ImGui::Spacing();

            if (ImGui::Button("Upgrade")) {
                upgrade_level();
                is_upgrading = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                is_upgrading = false;
            }

            ImGui::EndChild();
        }

        if (W->level.version < 7)
            ImGui::BeginDisabled();

        ImGui::Checkbox("Pause on win", &pause_on_win);
        ImGui::SetItemTooltip("Pause the simulation once the win condition has been reached.");

        ImGui::Checkbox("Display score", &display_score);
        ImGui::SetItemTooltip("Display the score in the top-right corner.");

        if (W->level.version < 7)
            ImGui::EndDisabled();

        if (!(flags & LVL_ABSORB_DEAD_ENEMIES))
            ImGui::BeginDisabled();

        ImGui::TextUnformatted("Creature absorb time");
        ImGui::SliderFloat("###creature_absorb_time", &creature_absorb_time, 0.0f, 60.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Time before dead creatures are absorbed");

        if (!(flags & LVL_ABSORB_DEAD_ENEMIES))
            ImGui::EndDisabled();

        ImGui::TextUnformatted("Player respawn time");
        ImGui::SliderFloat("###player_respawn_time", &player_respawn_time, 0.0f, 60.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Delay between a player's death and their ability to respawn");

        if (W->level.version < 9)
            return;

        auto lvl_flag_toggle = [](uint64_t flag, const char *label, const char *help, bool disabled = false) {
            bool x = (flags & flag) != 0;
            if (disabled)
                ImGui::BeginDisabled();

            if (ImGui::Checkbox(label, &x)) {
                if (x)
                    flags |= flag;
                else
                    flags &= ~flag;
            }

            if ((help != 0) && (*help != 0))
                ImGui::SetItemTooltip("%s", help);

            if (disabled)
                ImGui::EndDisabled();
        };

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
            "Splits up the level into chunks, leading to better performance for large levels."
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
            "Allow players to submit a high score even if they did not win the level."
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

    void layout() {
        auto begin_scrolling_tab = [](const char *name, const char *id) {
            if (!ImGui::BeginTabItem(name))
                return false;

            ImGui::BeginChild(id, ImGui::GetContentRegionAvail(), false);
            return true;
        };

        auto end_scrolling_tab = []() {
            ImGui::EndChild();
            ImGui::EndTabItem();
        };

        handle_do_open(&do_open, "Level properties");
        ImGui_CenterNextWindow();

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowSize(
            ImVec2(
                tclampf(viewport->WorkSize.x * 0.35f, UI(350.0f), UI(600.0f)),
                viewport->WorkSize.y * 0.90f),
            ImGuiCond_Always);

        if (ImGui::BeginPopupModal("Level properties", REF_TRUE, MODAL_FLAGS)) {

            const float footer_height =
                ImGui::GetFrameHeightWithSpacing() +
                ImGui::GetStyle().ItemSpacing.y +
                ImGui::GetStyle().WindowPadding.y * 1.5f;

            // Everything except the footer.
            ImGui::BeginChild("##properties", ImVec2(0, -footer_height), false);

            if (ImGui::BeginTabBar("###lvlproptabbar")) {
                if (begin_scrolling_tab("Information", "##information")) {
                    tab_information();
                    end_scrolling_tab();
                }
                if (begin_scrolling_tab("World", "##world")) {
                    tab_world();
                    end_scrolling_tab();
                }
                if (begin_scrolling_tab("Physics", "##physics")) {
                    tab_physics();
                    end_scrolling_tab();
                }
                if (begin_scrolling_tab("Gameplay", "##gameplay")) {
                    tab_gameplay();
                    end_scrolling_tab();
                }
                ImGui::EndTabBar();
            }

            ImGui::EndChild();

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
