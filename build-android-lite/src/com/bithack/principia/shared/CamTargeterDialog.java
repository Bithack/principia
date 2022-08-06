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
import android.widget.Spinner;

public class CamTargeterDialog {
	static Dialog _dialog;
	
	static View view;
	static Spinner s_follow_mode;
	
	public static Dialog get_dialog()
	{
		if (_dialog == null) {
			AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

			view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.cam_targeter, null);
			
			s_follow_mode = (Spinner)view.findViewById(R.id.follow_mode);
			
			bld.setTitle("Cam Targeter");
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
			
			_dialog = bld.create();
		}
		
		return _dialog;
	}

	public static void prepare(DialogInterface di)
	{
		s_follow_mode.setSelection(PrincipiaBackend.getCamTargeterFollowMode());
	}
	
	public static void save()
	{
		PrincipiaBackend.setCamTargeterFollowMode(s_follow_mode.getSelectedItemPosition());
	}
}
