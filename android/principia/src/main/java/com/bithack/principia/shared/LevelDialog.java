package com.bithack.principia.shared;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import com.bithack.principia.shared.ConfirmDialog.OnOptionSelectedListener;

import android.app.Dialog;
import android.content.DialogInterface;
import android.graphics.Color;
import android.graphics.PorterDuff.Mode;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.Spinner;
import android.widget.TableRow;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.widget.ToggleButton;

public class LevelDialog implements OnSeekBarChangeListener, OnEditorActionListener, OnFocusChangeListener {
    public static final int NUM_TABS = 3;
    static Dialog _dialog;

    static View view;
    static ToggleButton tab_buttons[];
    static ScrollView tab_views[];
    static EditText et_name;
    static EditText et_descr;
    static Spinner s_bg;
    static EditText et_border_left;
    static EditText et_border_right;
    static EditText et_border_bottom;
    static EditText et_border_top;
    static EditText et_gravity_x;
    static EditText et_gravity_y;
    static EditText et_final_score;
    static NumberPicker np_position_iterations;
    static NumberPicker np_velocity_iterations;
    static CheckBox cb_pause_on_win;
    static CheckBox cb_display_score;
    static List<LevelProperty> level_properties;
    static RadioGroup level_type;
    static RadioButton level_adventure;
    static RadioButton level_custom;
    static Button level_upgrade_version;

    static SeekBar  sb_prismatic_tolerance;
    static EditText et_prismatic_tolerance;

    static SeekBar  sb_pivot_tolerance;
    static EditText et_pivot_tolerance;

    static SeekBar  sb_linear_damping;
    static EditText et_linear_damping;

    static SeekBar  sb_angular_damping;
    static EditText et_angular_damping;

    static SeekBar  sb_joint_friction;
    static EditText et_joint_friction;

    static SeekBar  sb_creature_absorb_time;
    static EditText et_creature_absorb_time;

    static SeekBar  sb_player_respawn_time;
    static EditText et_player_respawn_time;

    static Button btn_save;
    static Button btn_cancel;
    static Button btn_bg_color;
    static TableRow row_bg_color;
    static TableRow row_bg_color_red;
    static TableRow row_bg_color_green;
    static TableRow row_bg_color_blue;
    static SeekBar sb_bg_color_red;
    static SeekBar sb_bg_color_green;
    static SeekBar sb_bg_color_blue;

    static int border_left = 10, border_right = 10, border_top = 10, border_bottom = 10, final_score = 10;
    static float gravity_x = 0.f, gravity_y = -20.f;

    static LevelDialog mSingleton;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            if (mSingleton == null) {
                mSingleton = new LevelDialog();
            }

            tab_buttons = new ToggleButton[LevelDialog.NUM_TABS];
            tab_views = new ScrollView[LevelDialog.NUM_TABS];

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.level, null);

            _dialog = new Dialog(PrincipiaActivity.mSingleton, android.R.style.Theme_NoTitleBar_Fullscreen){
                @Override
                protected void onCreate(Bundle saved_instance) {
                    super.onCreate(saved_instance);
                    getWindow().setLayout(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
                }
            };
            _dialog.setContentView(view);

            btn_save = (Button)view.findViewById(R.id.level_btn_save);
            btn_cancel = (Button)view.findViewById(R.id.level_btn_cancel);

            btn_save.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    save();
                    _dialog.dismiss();
                }
            });

            btn_cancel.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    _dialog.dismiss();
                }
            });

            row_bg_color_red = (TableRow)view.findViewById(R.id.level_bg_color_red_row);
            row_bg_color = (TableRow)view.findViewById(R.id.level_bg_color_row);
            row_bg_color_green = (TableRow)view.findViewById(R.id.level_bg_color_green_row);
            row_bg_color_blue = (TableRow)view.findViewById(R.id.level_bg_color_blue_row);

            OnSeekBarChangeListener sb_color_listener = new OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        int red = sb_bg_color_red.getProgress();
                        int green = sb_bg_color_green.getProgress();
                        int blue = sb_bg_color_blue.getProgress();
                        int c = Color.rgb(red, green, blue);
                        Drawable d = btn_bg_color.getBackground();
                        d.setColorFilter(c, Mode.LIGHTEN);
                        btn_bg_color.setTextColor(Color.rgb(255-red, 255-green, 255-blue));
                    }
                }
                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                }
            };
            sb_bg_color_red = (SeekBar)view.findViewById(R.id.level_bg_color_red);
            sb_bg_color_green = (SeekBar)view.findViewById(R.id.level_bg_color_green);
            sb_bg_color_blue = (SeekBar)view.findViewById(R.id.level_bg_color_blue);

            sb_bg_color_red.setOnSeekBarChangeListener(sb_color_listener);
            sb_bg_color_green.setOnSeekBarChangeListener(sb_color_listener);
            sb_bg_color_blue.setOnSeekBarChangeListener(sb_color_listener);

            tab_buttons[0] = (ToggleButton)view.findViewById(R.id.tb_info);
            tab_buttons[1] = (ToggleButton)view.findViewById(R.id.tb_world);
            tab_buttons[2] = (ToggleButton)view.findViewById(R.id.tb_gameplay);

            tab_views[0]   = (ScrollView)view.findViewById(R.id.sv_info);
            tab_views[1]   = (ScrollView)view.findViewById(R.id.sv_world);
            tab_views[2]   = (ScrollView)view.findViewById(R.id.sv_gameplay);

            OnCheckedChangeListener l = new OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView,
                        boolean isChecked) {
                    if (isChecked) {
                        for (int x=0; x<LevelDialog.NUM_TABS; ++x) {
                            if (x == ((Integer)buttonView.getTag())) {
                                tab_views[x].setVisibility(ScrollView.VISIBLE);
                            } else {
                                tab_views[x].setVisibility(ScrollView.GONE);
                                tab_buttons[x].setChecked(false);
                            }
                        }
                    }
                }
            };

            for (int x=0; x<LevelDialog.NUM_TABS; ++x) {
                tab_buttons[x].setTag(x);
                tab_buttons[x].setOnCheckedChangeListener(l);
            }

            level_type = (RadioGroup)view.findViewById(R.id.level_type);
            level_adventure = (RadioButton)view.findViewById(R.id.level_type_adventure);
            level_custom = (RadioButton)view.findViewById(R.id.level_type_custom);

            /* Info */
            et_name = (EditText)view.findViewById(R.id.level_name);
            et_descr = (EditText)view.findViewById(R.id.level_descr);
            level_upgrade_version = (Button)view.findViewById(R.id.level_upgrade_version);

            /* World */
            s_bg = (Spinner)view.findViewById(R.id.level_bg);

            ArrayList<String> bg_array = Material.get_bgs();
            ArrayAdapter<String> bg_adapter = new ArrayAdapter<String>(PrincipiaActivity.mSingleton, android.R.layout.simple_spinner_dropdown_item, bg_array);
            s_bg.setAdapter(bg_adapter);
            s_bg.setOnItemSelectedListener(new OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> arg0, View arg1,
                        int arg2, long arg3) {
                    if (s_bg.getSelectedItemPosition() == 6 || s_bg.getSelectedItemPosition() == 7) {
                        row_bg_color.setVisibility(View.VISIBLE);
                    } else {
                        row_bg_color.setVisibility(View.GONE);
                        row_bg_color_red.setVisibility(View.GONE);
                        row_bg_color_green.setVisibility(View.GONE);
                        row_bg_color_blue.setVisibility(View.GONE);
                    }
                }
                @Override
                public void onNothingSelected(AdapterView<?> arg0) {
                }
            });
            et_border_left = (EditText)view.findViewById(R.id.et_border_left);
            et_border_right = (EditText)view.findViewById(R.id.et_border_right);
            et_border_bottom = (EditText)view.findViewById(R.id.et_border_bottom);
            et_border_top = (EditText)view.findViewById(R.id.et_border_top);
            et_gravity_x = (EditText)view.findViewById(R.id.level_gravity_x);
            et_gravity_y = (EditText)view.findViewById(R.id.level_gravity_y);
            et_final_score = (EditText)view.findViewById(R.id.level_final_score);

            sb_prismatic_tolerance = (SeekBar) view.findViewById(R.id.level_prismatic_tolerance_sb);
            et_prismatic_tolerance = (EditText)view.findViewById(R.id.level_prismatic_tolerance_et);

            sb_pivot_tolerance = (SeekBar) view.findViewById(R.id.level_pivot_tolerance_sb);
            et_pivot_tolerance = (EditText)view.findViewById(R.id.level_pivot_tolerance_et);

            sb_linear_damping = (SeekBar) view.findViewById(R.id.level_linear_damping_sb);
            et_linear_damping = (EditText)view.findViewById(R.id.level_linear_damping_et);

            sb_angular_damping = (SeekBar) view.findViewById(R.id.level_angular_damping_sb);
            et_angular_damping = (EditText)view.findViewById(R.id.level_angular_damping_et);

            sb_joint_friction = (SeekBar) view.findViewById(R.id.level_joint_friction_sb);
            et_joint_friction = (EditText)view.findViewById(R.id.level_joint_friction_et);

            sb_creature_absorb_time = (SeekBar) view.findViewById(R.id.level_creature_absorb_time_sb);
            et_creature_absorb_time = (EditText)view.findViewById(R.id.level_creature_absorb_time_et);

            sb_player_respawn_time = (SeekBar) view.findViewById(R.id.level_player_respawn_time_sb);
            et_player_respawn_time = (EditText)view.findViewById(R.id.level_player_respawn_time_et);

            np_position_iterations = (NumberPicker)view.findViewById(R.id.level_position_iterations);
            np_position_iterations.setRange(10, 255);
            np_position_iterations.setStep(5);
            np_velocity_iterations = (NumberPicker)view.findViewById(R.id.level_velocity_iterations);
            np_velocity_iterations.setRange(10, 255);
            np_velocity_iterations.setStep(5);

            sb_prismatic_tolerance.setOnSeekBarChangeListener(mSingleton);
            et_prismatic_tolerance.setOnEditorActionListener(mSingleton);
            et_prismatic_tolerance.setOnFocusChangeListener(mSingleton);

            sb_pivot_tolerance.setOnSeekBarChangeListener(mSingleton);
            et_pivot_tolerance.setOnEditorActionListener(mSingleton);
            et_pivot_tolerance.setOnFocusChangeListener(mSingleton);

            sb_linear_damping.setOnSeekBarChangeListener(mSingleton);
            et_linear_damping.setOnEditorActionListener(mSingleton);
            et_linear_damping.setOnFocusChangeListener(mSingleton);

            sb_angular_damping.setOnSeekBarChangeListener(mSingleton);
            et_angular_damping.setOnEditorActionListener(mSingleton);
            et_angular_damping.setOnFocusChangeListener(mSingleton);

            sb_joint_friction.setOnSeekBarChangeListener(mSingleton);
            et_joint_friction.setOnEditorActionListener(mSingleton);
            et_joint_friction.setOnFocusChangeListener(mSingleton);

            sb_creature_absorb_time.setOnSeekBarChangeListener(mSingleton);
            et_creature_absorb_time.setOnEditorActionListener(mSingleton);
            et_creature_absorb_time.setOnFocusChangeListener(mSingleton);

            sb_player_respawn_time.setOnSeekBarChangeListener(mSingleton);
            et_player_respawn_time.setOnEditorActionListener(mSingleton);
            et_player_respawn_time.setOnFocusChangeListener(mSingleton);

            cb_pause_on_win = (CheckBox)view.findViewById(R.id.pause_on_win);
            cb_display_score = (CheckBox)view.findViewById(R.id.display_score);

            level_properties = new ArrayList<LevelProperty>();
            CheckBox cb;

            cb = (CheckBox)view.findViewById(R.id.disable_layer_switch);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_interactive);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_static_connections);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_jumping);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_fall_damage);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_connections);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_cam_movement);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_initial_wait);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_robot_hit_score);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_zoom);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.absorb_dead_enemies);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.snap_by_default);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.unlimited_enemy_vision);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.interactive_destruction);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_damage);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.single_layer_explosions);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_continue_button);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.hide_beam_connections);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_3rd_layer);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.portrait_mode);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_camera_rc_snap);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_physics);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.do_not_require_dragfield);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_robot_special_action);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_adventure_max_zoom);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_roam_layer_switch);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.chunked_level_loading);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_caveview);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_rocket_trigger_explosives);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.store_score_on_game_over);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.allow_high_score_submissions);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.lower_score_is_better);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.disable_endscreens);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.allow_quicksaving);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.allow_respawn_without_checkpoint);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));
            cb = (CheckBox)view.findViewById(R.id.dead_creature_destruction);
            level_properties.add(new LevelProperty(cb, Long.parseLong((String)cb.getTag())));

            level_upgrade_version.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    new ConfirmDialog()
                    .set_listener(new OnOptionSelectedListener() {
                        @Override
                        public void onOptionSelected(int option) {
                            if (option == ConfirmDialog.OPTION_YES) {
                                PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_UPGRADE_LEVEL, 0);
                            }
                        }
                    })
                    .run(PrincipiaActivity.mSingleton.getString(R.string.confirm_level_upgrade));
                }
            });

            btn_bg_color = (Button)view.findViewById(R.id.level_bg_color_btn);
            btn_bg_color.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View arg0) {
                    if (row_bg_color_red.getVisibility() == View.GONE) {
                        row_bg_color_red.setVisibility(View.VISIBLE);
                        row_bg_color_green.setVisibility(View.VISIBLE);
                        row_bg_color_blue.setVisibility(View.VISIBLE);
                    } else {
                        row_bg_color_red.setVisibility(View.GONE);
                        row_bg_color_green.setVisibility(View.GONE);
                        row_bg_color_blue.setVisibility(View.GONE);
                    }
                }
            });
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        String level_name = PrincipiaBackend.getLevelName();
        String level_descr = PrincipiaBackend.getLevelDescription();
        String level_info = PrincipiaBackend.getLevelInfo();

        int bg_color = 0;

        String data[] = level_info.split(",");
        /**
         * int bg,
         * int left_border, int right_border, int bottom_border, int top_border,
         * float gravity_x, float gravity_y,
         * int position_iterations, int velocity_iterations,
         * int final_score,
         * boolean pause_on_win,
         * boolean display_score,
         * float prismatic_tolerance, float pivot_tolerance,
         * int color,
         * float linear_damping,
         * float angular_damping,
         * float joint_friction,
         * float dead_enemy_absorb_time,
         * float time_Before_player_can_respawn
         **/
        if (data.length == 20) {
            try {
                int i = Integer.parseInt(data[0]);
                s_bg.setSelection(i);
            } catch (NumberFormatException e) {
                s_bg.setSelection(0);
            }
            et_border_left.setText(data[1]);
            et_border_right.setText(data[2]);
            et_border_bottom.setText(data[3]);
            et_border_top.setText(data[4]);
            et_gravity_x.setText(data[5]);
            et_gravity_y.setText(data[6]);
            try {
                np_position_iterations.setValue(Integer.parseInt(data[7]));
                np_velocity_iterations.setValue(Integer.parseInt(data[8]));
            } catch (NumberFormatException e) {
                np_position_iterations.setValue(10);
                np_velocity_iterations.setValue(10);
            }
            et_final_score.setText(data[9]);
            cb_pause_on_win.setChecked(Boolean.parseBoolean(data[10]));
            cb_display_score.setChecked(Boolean.parseBoolean(data[11]));

            try {
                border_left = Integer.parseInt(data[1]);
                border_right = Integer.parseInt(data[2]);
                border_bottom = Integer.parseInt(data[3]);
                border_top = Integer.parseInt(data[4]);
                gravity_x = Float.parseFloat(data[5]);
                gravity_y = Float.parseFloat(data[6]);
                final_score = Integer.parseInt(data[9]);
            } catch (NumberFormatException e) { }

            float prismatic_tolerance, pivot_tolerance;

            try {
                prismatic_tolerance = Float.parseFloat(data[12]);
                pivot_tolerance = Float.parseFloat(data[13]);
            } catch (NumberFormatException e) {
                prismatic_tolerance = 0.f;
                pivot_tolerance = 0.f;
            }

            try {
                bg_color = Integer.parseInt(data[14]);
            } catch (NumberFormatException e) {
                bg_color = 0;
            }

            float linear_damping, angular_damping, joint_friction, creature_absorb_time, player_respawn_time;

            try {
                linear_damping = Float.parseFloat(data[15]);
                angular_damping = Float.parseFloat(data[16]);
                joint_friction = Float.parseFloat(data[17]);
                creature_absorb_time = Float.parseFloat(data[18]);
                player_respawn_time = Float.parseFloat(data[19]);
            } catch (NumberFormatException e) {
                linear_damping = 0.f;
                angular_damping = 0.f;
                joint_friction = 0.f;
                creature_absorb_time = 0.f;
                player_respawn_time = 0.f;
            }

            {
                float    v  =    prismatic_tolerance;
                EditText et = et_prismatic_tolerance;
                SeekBar  sb = sb_prismatic_tolerance;

                et.setText(Float.toString(v));
                sb.setProgress((int)(v * 1000.f));
            }
            {
                float    v  =    pivot_tolerance;
                EditText et = et_pivot_tolerance;
                SeekBar  sb = sb_pivot_tolerance;

                et.setText(Float.toString(v));
                sb.setProgress((int)(v * 1000.f));
            }
            {
                float    v  =    linear_damping;
                EditText et = et_linear_damping;
                SeekBar  sb = sb_linear_damping;

                et.setText(Float.toString(v));
                sb.setProgress((int)(v * 1000.f));
            }
            {
                float    v  =    angular_damping;
                EditText et = et_angular_damping;
                SeekBar  sb = sb_angular_damping;

                et.setText(Float.toString(v));
                sb.setProgress((int)(v * 1000.f));
            }
            {
                float    v  =    joint_friction;
                EditText et = et_joint_friction;
                SeekBar  sb = sb_joint_friction;

                et.setText(Float.toString(v));
                sb.setProgress((int)(v * 1000.f));
            }
            {
                float    v  =    creature_absorb_time;
                EditText et = et_creature_absorb_time;
                SeekBar  sb = sb_creature_absorb_time;

                et.setText(Float.toString(v));
                sb.setProgress((int)(v * 1000.f));
            }
            {
                float    v  =    player_respawn_time;
                EditText et = et_player_respawn_time;
                SeekBar  sb = sb_player_respawn_time;

                et.setText(Float.toString(v));
                sb.setProgress((int)(v * 1000.f));
            }
        } else {
            Log.e("Principia", "invalid number of level info data fields: " + data.length);
        }

        int red = Color.alpha(bg_color);
        int green = Color.red(bg_color);
        int blue = Color.green(bg_color);

        sb_bg_color_red.setProgress(red);
        sb_bg_color_green.setProgress(green);
        sb_bg_color_blue.setProgress(blue);

        Drawable d = btn_bg_color.getBackground();
        d.setColorFilter(Color.rgb(red, green, blue), Mode.LIGHTEN);
        btn_bg_color.setTextColor(Color.rgb(255-red, 255-green, 255-blue));

        if (s_bg.getSelectedItemPosition() == 6 || s_bg.getSelectedItemPosition() == 7) {
            row_bg_color.setVisibility(View.VISIBLE);
        } else {
            row_bg_color.setVisibility(View.GONE);
        }

        et_name.setText(level_name);
        et_descr.setText(level_descr);

        for (int x=0; x<level_properties.size(); ++x) {
            LevelProperty p = level_properties.get(x);
            p.cb.setChecked(PrincipiaBackend.getLevelFlag(p.bs_value));
        }

        int lt = PrincipiaBackend.getLevelType();
        Log.v("Principia", String.format("Level type: %d", lt));

        if (lt == 1) {
            level_type.check(R.id.level_type_adventure);
        } else {
            level_type.check(R.id.level_type_custom);
        }

        int level_version = PrincipiaBackend.getLevelVersion();
        int max_level_version = PrincipiaBackend.getMaxLevelVersion();

        if (level_version == max_level_version) {
            level_upgrade_version.setText(String.format(Locale.US, "%d (latest)", level_version));
            level_upgrade_version.setEnabled(false);
        } else {
            level_upgrade_version.setText(String.format(Locale.US, "%d (Upgrade to %d)", level_version, max_level_version));
            level_upgrade_version.setEnabled(true);
        }

        row_bg_color_red.setVisibility(View.GONE);
        row_bg_color_green.setVisibility(View.GONE);
        row_bg_color_blue.setVisibility(View.GONE);
    }

    public static void save()
    {
        PrincipiaBackend.setLevelName(et_name.getText().toString().trim());
        PrincipiaBackend.setLevelDescription(et_descr.getText().toString().trim());

        PrincipiaBackend.resetLevelFlags();
        for (int x=0; x<level_properties.size(); ++x) {
            LevelProperty p = level_properties.get(x);
            if (p.cb.isChecked()) {
                Log.v("Principia", String.format("Setting level flag: %d", p.bs_value));
                PrincipiaBackend.setLevelFlag(p.bs_value);
            }
        }

        /**
          * int bg,
          * int left_border, int right_border, int bottom_border, int top_border,
          * float gravity_x, float gravity_y,
          * int position_iterations, int velocity_iterations,
          * int final_score,
          * boolean pause_on_win,
          * boolean display_score,
          * float prismatic_tolerance, float pivot_tolerance
          **/

        int _border_left = border_left;
        int _border_right = border_right;
        int _border_bottom = border_bottom;
        int _border_top = border_top;
        int _final_score = final_score;
        float _gravity_x = gravity_x;
        float _gravity_y = gravity_y;
        StringBuilder sb = new StringBuilder();
        try {
            _border_left = Integer.parseInt(et_border_left.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Left border");
        }
        try {
            _border_right = Integer.parseInt(et_border_right.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Right border");
        }
        try {
            _border_bottom = Integer.parseInt(et_border_bottom.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Bottom border");
        }
        try {
            _border_top = Integer.parseInt(et_border_top.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Top border");
        }
        try {
            _final_score = Integer.parseInt(et_final_score.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Final score.");
        }
        try {
            _gravity_x = Float.parseFloat(et_gravity_x.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Gravity X.");
        }
        try {
            _gravity_y = Float.parseFloat(et_gravity_y.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Gravity y.");
        }

        float prismatic_tolerance, pivot_tolerance;

        try {
            prismatic_tolerance = Float.parseFloat(et_prismatic_tolerance.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Prismatic tolerance");
            prismatic_tolerance = 0.f;
        }

        try {
            pivot_tolerance = Float.parseFloat(et_pivot_tolerance.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Pivot tolerance");
            pivot_tolerance = 0.f;
        }

        float linear_damping, angular_damping, joint_friction, creature_absorb_time, player_respawn_time;

        try {
            linear_damping = Float.parseFloat(et_linear_damping.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Linear damping");
            linear_damping = 0.f;
        }

        try {
            angular_damping = Float.parseFloat(et_angular_damping.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Angular damping");
            angular_damping = 0.f;
        }

        try {
            joint_friction = Float.parseFloat(et_joint_friction.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Joint friction");
            joint_friction = 0.f;
        }

        try {
            creature_absorb_time = Float.parseFloat(et_creature_absorb_time.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Creature absorb time");
            creature_absorb_time = 0.f;
        }

        try {
            player_respawn_time = Float.parseFloat(et_player_respawn_time.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid input given to Player respawn time");
            player_respawn_time = 0.f;
        }

        if (sb.length() > 0) {
            PrincipiaActivity.message(sb.toString(), 0);
        }

        PrincipiaBackend.setLevelInfo(s_bg.getSelectedItemPosition(),
                _border_left, _border_right,
                _border_bottom, _border_top,
                _gravity_x, _gravity_y,
                np_position_iterations.getValue(), np_velocity_iterations.getValue(),
                _final_score,
                cb_pause_on_win.isChecked(),
                cb_display_score.isChecked(),
                prismatic_tolerance, pivot_tolerance,
                Color.argb(sb_bg_color_red.getProgress(), sb_bg_color_green.getProgress(), sb_bg_color_blue.getProgress(), 255),
                linear_damping,
                angular_damping,
                joint_friction,
                creature_absorb_time,
                player_respawn_time
                );

        int checked_id = LevelDialog.level_type.getCheckedRadioButtonId();
        int lt = 0; // fallback to puzzle in case any old levels remain
        if (checked_id == R.id.level_type_adventure) {
            lt = 1;
        } else if (checked_id == R.id.level_type_custom) {
            lt = 2;
        }
        Log.v("Principia", String.format("new lt: %d. checked_id: %d", lt, checked_id));

        PrincipiaBackend.setLevelType(lt);
    }

    @Override
    public void onProgressChanged(SeekBar sb, int progress,
            boolean from_user) {
        float value = sb.getProgress() / 1000.f;

        EditText et = null;

        if (sb == LevelDialog.sb_prismatic_tolerance) {
            et = LevelDialog.et_prismatic_tolerance;
        } else if (sb == LevelDialog.sb_pivot_tolerance) {
            et = LevelDialog.et_pivot_tolerance;
        } else if (sb == LevelDialog.sb_linear_damping) {
            et = LevelDialog.et_linear_damping;
        } else if (sb == LevelDialog.sb_angular_damping) {
            et = LevelDialog.et_angular_damping;
        } else if (sb == LevelDialog.sb_joint_friction) {
            et = LevelDialog.et_joint_friction;
        } else if (sb == LevelDialog.sb_creature_absorb_time) {
            et = LevelDialog.et_creature_absorb_time;
        } else if (sb == LevelDialog.sb_player_respawn_time) {
            et = LevelDialog.et_player_respawn_time;
        }

        if (et != null) {
            et.setText(Float.toString(value));
        }
        Log.v("PRINCIPIA", "Wazzup");
    }

    @Override
    public boolean onEditorAction(TextView tv, int actionId,
            KeyEvent event) {
        if (event != null) {
            return true;
        }

        this.cool(tv);

        return false;
    }

    public void cool(TextView v)
    {
        SeekBar sb = null;

        if (v == LevelDialog.et_prismatic_tolerance) {
            sb = LevelDialog.sb_prismatic_tolerance;
        } else if (v == LevelDialog.et_pivot_tolerance) {
            sb = LevelDialog.sb_pivot_tolerance;
        } else if (v == LevelDialog.et_linear_damping) {
            sb = LevelDialog.sb_linear_damping;
        } else if (v == LevelDialog.et_angular_damping) {
            sb = LevelDialog.sb_angular_damping;
        } else if (v == LevelDialog.et_joint_friction) {
            sb = LevelDialog.sb_joint_friction;
        } else if (v == LevelDialog.et_creature_absorb_time) {
            sb = LevelDialog.sb_creature_absorb_time;
        } else if (v == LevelDialog.et_player_respawn_time) {
            sb = LevelDialog.sb_player_respawn_time;
        }

        if (sb != null) {
            float val;

            try {
                val = Float.parseFloat(v.getText().toString());
            } catch (NumberFormatException e) {
                int progress = sb.getProgress();
                val = progress / 1000.f;
            }

            float max = (float)sb.getMax() / 100.f;

            if (val < 0.f) {
                val = 0.f;
            } else if (val > max) {
                val = max;
            }

            v.setText(Float.toString(val));
            int p = (int)(val * 1000.f);
            sb.setProgress(p);
        }
    }

    @Override
    public void onFocusChange(View v, boolean hasFocus) {
        if (!hasFocus) {
            TextView tv = (TextView)v;

            this.cool(tv);
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) { }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) { }
}
