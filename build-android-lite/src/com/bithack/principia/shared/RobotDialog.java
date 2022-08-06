package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principialite.PrincipiaActivity;
import com.bithack.principialite.R;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.RadioButton;
import android.widget.RadioGroup;

public class RobotDialog {
	static Dialog _dialog;
	
	static View view;
	static RadioGroup rg_default_state;
	static CheckBox cb_roam;
	static RadioGroup rg_initial_dir;
	
	public static Dialog get_dialog()
	{
		if (_dialog == null) {
			AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

			view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.robot, null);

			rg_default_state = (RadioGroup)view.findViewById(R.id.robot_default_state);
			cb_roam = (CheckBox)view.findViewById(R.id.robot_roam);
			rg_initial_dir = (RadioGroup)view.findViewById(R.id.robot_initial_dir);
			
			bld.setTitle("Robot");
			bld.setView(view);
			bld.setPositiveButton("OK", new OnClickListener(){
				public void onClick(DialogInterface dialog, int which) {
					save();
					dialog.dismiss();
				}
				
			});
			bld.setNegativeButton("Cancel", new OnClickListener(){
				public void onClick(DialogInterface dialog, int which) {
					dialog.dismiss();
				}
				
			});
			
			RobotDialog._dialog = bld.create();
		}
		
		return _dialog;
	}

	public static void prepare(DialogInterface di)
	{
		boolean roam = PrincipiaBackend.getRobotRoam();
		int state = PrincipiaBackend.getRobotState();
		int dir = PrincipiaBackend.getRobotDir();
		
		cb_roam.setChecked(roam);
		
		switch (state) {
			case 0: ((RadioButton)view.findViewById(R.id.robot_state_idle)).setChecked(true); break;
			case 1: ((RadioButton)view.findViewById(R.id.robot_state_walk)).setChecked(true); break;
			case 3: ((RadioButton)view.findViewById(R.id.robot_state_dead)).setChecked(true); break;
		}

		switch (dir) {
			case 1: ((RadioButton)view.findViewById(R.id.robot_dir_left)).setChecked(true); break;
			case 0: ((RadioButton)view.findViewById(R.id.robot_dir_random)).setChecked(true); break;
			case 2: ((RadioButton)view.findViewById(R.id.robot_dir_right)).setChecked(true); break;
		}
	}
	
	public static void save()
	{
		int state = 0;
		int dir = 0;
		
		switch (rg_default_state.getCheckedRadioButtonId()) {
			case R.id.robot_state_idle: state = 0; break;
			case R.id.robot_state_walk: state = 1; break;
			case R.id.robot_state_dead: state = 3; break;
		}

		switch (rg_initial_dir.getCheckedRadioButtonId()) {
			case R.id.robot_dir_left: dir = 1; break;
			case R.id.robot_dir_random: dir = 0; break;
			case R.id.robot_dir_right: dir = 2; break;
		}

		PrincipiaBackend.setRobotStuff(state, cb_roam.isChecked(), dir);
	}
}
