package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import com.bithack.principialite.PrincipiaActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;

public class PlayDialog
{
	static Dialog _dialog = null;
	
	public PlayDialog()
	{
		_dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
			.setPositiveButton("Restart level", new OnClickListener(){
				public void onClick(DialogInterface dialog, int which)
				{
					PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_RESTART_LEVEL, 0);
				}
			})
			.setNeutralButton("Back", new OnClickListener(){
				public void onClick(DialogInterface dialog, int which)
				{
					PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_BACK, 0);
				}
			})
			.setNegativeButton("Cancel", new OnClickListener(){
				public void onClick(DialogInterface dialog, int which)
				{
				}
			})
			.create();
	}
	
	public Dialog get_dialog()
	{
		return _dialog;
	}
}
