package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import com.bithack.principialite.PrincipiaActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.net.Uri;

public class CommunityDialog
{
	static Dialog _dialog = null;
	
	public CommunityDialog()
	{
		_dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
				.setMessage("Do you want to return to the community site or to the main menu?")
				.setPositiveButton("Community", new OnClickListener(){
					public void onClick(DialogInterface dialog, int which){
						//SDLActivity.wv_dialog.show();
						/*
						PackageManager pm = SDLActivity.mSingleton.getPackageManager();
						Intent queryIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("http://bithack.com"));
						ActivityInfo af = queryIntent.resolveActivityInfo(pm, 0);
						Intent launchIntent = new Intent(Intent.ACTION_MAIN);
						launchIntent.setClassName(af.packageName, af.name);
						SDLActivity.mSingleton.startActivity(launchIntent);
						*/
						SDLActivity.wv.loadUrl(PrincipiaBackend.getCurrentCommunityUrl());
						SDLActivity.wv_dialog.show();
					}}
				)
				.setNegativeButton("Main menu", new OnClickListener(){
					public void onClick(DialogInterface dialog, int which){
						PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_GOTO_MAINMENU, 0);
					}}
				)
				.create();
	}
	
	public Dialog get_dialog()
	{
		return _dialog;
	}
}