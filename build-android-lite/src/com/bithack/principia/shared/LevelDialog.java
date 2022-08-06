package com.bithack.principia.shared;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principialite.PrincipiaActivity;
import com.bithack.principialite.R;
import com.bithack.principia.shared.ConfirmDialog.OnOptionSelectedListener;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup.LayoutParams;
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
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.widget.ToggleButton;

public class LevelDialog {
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
	static RadioButton level_puzzle;
	static RadioButton level_adventure;
	static RadioButton level_custom;
	static Button level_upgrade_version;
	static SeekBar sb_prismatic_tolerance;
	static SeekBar sb_pivot_tolerance;
	static EditText et_prismatic_tolerance;
	static EditText et_pivot_tolerance;
	static Button btn_save;
	static Button btn_cancel;
	
	static int border_left = 10, border_right = 10, border_top = 10, border_bottom = 10, final_score = 10;
	static float gravity_x = 0.f, gravity_y = -20.f;
	
	public static Dialog get_dialog()
	{
		if (_dialog == null) {
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
			level_puzzle = (RadioButton)view.findViewById(R.id.level_type_puzzle);
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
			et_border_left = (EditText)view.findViewById(R.id.et_border_left);
			et_border_right = (EditText)view.findViewById(R.id.et_border_right);
			et_border_bottom = (EditText)view.findViewById(R.id.et_border_bottom);
			et_border_top = (EditText)view.findViewById(R.id.et_border_top);
			et_gravity_x = (EditText)view.findViewById(R.id.level_gravity_x);
			et_gravity_y = (EditText)view.findViewById(R.id.level_gravity_y);
			et_final_score = (EditText)view.findViewById(R.id.level_final_score);
			sb_prismatic_tolerance = (SeekBar)view.findViewById(R.id.level_prismatic_tolerance_sb);
			et_prismatic_tolerance = (EditText)view.findViewById(R.id.level_prismatic_tolerance_et);
			sb_pivot_tolerance = (SeekBar)view.findViewById(R.id.level_pivot_tolerance_sb);
			et_pivot_tolerance = (EditText)view.findViewById(R.id.level_pivot_tolerance_et);
			np_position_iterations = (NumberPicker)view.findViewById(R.id.level_position_iterations);
			np_position_iterations.setRange(10, 255);
			np_position_iterations.setStep(5);
			np_velocity_iterations = (NumberPicker)view.findViewById(R.id.level_velocity_iterations);
			np_velocity_iterations.setRange(10, 255);
			np_velocity_iterations.setStep(5);
			
			sb_prismatic_tolerance.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
				@Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) { }
				@Override public void onStartTrackingTouch(SeekBar seekBar) { }
				@Override public void onStopTrackingTouch(SeekBar sb) {
					float value = sb.getProgress() / 1000.f;
					et_prismatic_tolerance.setText(Float.toString(value));
				}
			});
			et_prismatic_tolerance.setOnEditorActionListener(new OnEditorActionListener() {
				@Override
				public boolean onEditorAction(TextView v, int actionId,
						KeyEvent event) {
					if (event != null) {
						return true;
					}

					float val;
					try {
						val = Float.parseFloat(v.getText().toString());
					} catch (NumberFormatException e) {
						int progress = sb_prismatic_tolerance.getProgress();
						val = progress / 1000.f;
					}
					
					if (val < 0.f) val = 0.f;
					else if (val > 0.075f) val = 0.075f;

					v.setText(Float.toString(val));
					int p = (int)(val * 1000.f);
					sb_prismatic_tolerance.setProgress(p);
					return false;
				}
			});
			et_prismatic_tolerance.setOnFocusChangeListener(new OnFocusChangeListener() {
				@Override
				public void onFocusChange(View v, boolean hasFocus) {
					if (!hasFocus) {
						TextView tv = (TextView)v;
						float val;
						try {
							val = Float.parseFloat(tv.getText().toString());
						} catch (NumberFormatException e) {
							int progress = sb_prismatic_tolerance.getProgress();
							val = progress / 1000.f;
						}

						if (val < 0.f) val = 0.f;
						else if (val > 0.075f) val = 0.075f;

						tv.setText(Float.toString(val));
						int p = (int)(val * 1000.f);
						sb_prismatic_tolerance.setProgress(p);
					}
				}
			});

			sb_pivot_tolerance.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
				@Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) { }
				@Override public void onStartTrackingTouch(SeekBar seekBar) { }
				@Override public void onStopTrackingTouch(SeekBar sb) {
					float value = sb.getProgress() / 1000.f;
					et_pivot_tolerance.setText(Float.toString(value));
				}
			});
			et_pivot_tolerance.setOnEditorActionListener(new OnEditorActionListener() {
				@Override
				public boolean onEditorAction(TextView v, int actionId,
						KeyEvent event) {
					if (event != null) {
						return true;
					}

					float val;
					try {
						val = Float.parseFloat(v.getText().toString());
					} catch (NumberFormatException e) {
						int progress = sb_pivot_tolerance.getProgress();
						val = progress / 1000.f;
					}
					
					if (val < 0.f) val = 0.f;
					else if (val > 0.075f) val = 0.075f;

					v.setText(Float.toString(val));
					int p = (int)(val * 1000.f);
					sb_pivot_tolerance.setProgress(p);
					return false;
				}
			});
			et_pivot_tolerance.setOnFocusChangeListener(new OnFocusChangeListener() {
				@Override
				public void onFocusChange(View v, boolean hasFocus) {
					if (!hasFocus) {
						TextView tv = (TextView)v;
						float val;
						try {
							val = Float.parseFloat(tv.getText().toString());
						} catch (NumberFormatException e) {
							int progress = sb_pivot_tolerance.getProgress();
							val = progress / 1000.f;
						}

						if (val < 0.f) val = 0.f;
						else if (val > 0.075f) val = 0.075f;

						tv.setText(Float.toString(val));
						int p = (int)(val * 1000.f);
						sb_pivot_tolerance.setProgress(p);
					}
				}
			});
	
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
		}
		
		return _dialog;
	}
	
	public static void prepare(DialogInterface di)
	{
		String level_name = PrincipiaBackend.getLevelName();
		String level_descr = PrincipiaBackend.getLevelDescription();
		String level_info = PrincipiaBackend.getLevelInfo();
		
		String data[] = level_info.split(",");
		/**
         * int bg,
         * int left_border, int right_border, int bottom_border, int top_border,
         * float gravity_x, float gravity_y,
         * int position_iterations, int velocity_iterations,
         * int final_score,
         * boolean pause_on_win,
         * boolean display_score
         * float prismatic_tolerance, float pivot_tolerance
         **/
		if (data.length == 14) {
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
			
			int p;
			
			et_prismatic_tolerance.setText(Float.toString(prismatic_tolerance));
			et_pivot_tolerance.setText(Float.toString(pivot_tolerance));
			
			p = (int)(prismatic_tolerance * 1000.f);
			sb_prismatic_tolerance.setProgress(p);

			p = (int)(pivot_tolerance * 1000.f);
			sb_pivot_tolerance.setProgress(p);
		} else {
			Log.v("Principia", "invalid number of level info data fields");
		}
		
		et_name.setText(level_name);
		et_descr.setText(level_descr);
		
		for (int x=0; x<level_properties.size(); ++x) {
			LevelProperty p = level_properties.get(x);
			p.cb.setChecked(PrincipiaBackend.getLevelFlag(p.bs_value));
		}
		
		int lt = PrincipiaBackend.getLevelType();
		Log.v("Principia", String.format("Level type: %d", lt));
		
		if (lt == 0) {
			level_type.check(R.id.level_type_puzzle);
		} else if (lt == 1) {
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
	}
	
	public static void save()
	{
		PrincipiaBackend.setLevelName(et_name.getText().toString().trim());
		PrincipiaBackend.setLevelDescription(et_descr.getText().toString().trim());

		PrincipiaBackend.resetLevelFlags();
		for (int x=0; x<level_properties.size(); ++x) {
			LevelProperty p = level_properties.get(x);
			if (p.cb.isChecked()) {
				/* TODO: flags are not set properly */
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
				prismatic_tolerance, pivot_tolerance
				);
		
		int checked_id = LevelDialog.level_type.getCheckedRadioButtonId();
		int lt = 2;
		if (checked_id == R.id.level_type_puzzle) {
			lt = 0;
		} else if (checked_id == R.id.level_type_adventure) {
			lt = 1;
		} else if (checked_id == R.id.level_type_custom) {
			lt = 2;
		}
		Log.v("Principia", String.format("new lt: %d. checked_id: %d", lt ,checked_id));
		
		PrincipiaBackend.setLevelType(lt);
	}
}
