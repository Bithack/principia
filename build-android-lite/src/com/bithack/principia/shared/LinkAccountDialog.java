package com.bithack.principia.shared;

import com.bithack.principialite.PrincipiaActivity;
import com.bithack.principialite.R;
import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnShowListener;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;

import com.google.android.vending.licensing.LicenseValidator;

public class LinkAccountDialog
{
	static AlertDialog _dialog;
	
	static View view;
	static EditText et_username;
	static EditText et_password;
	public static ProgressBar progressbar;
	public static int num_tries = 0;
	public static final int MAX_TRIES = 3;
	
	public static Dialog get_dialog()
	{
		if (_dialog == null) {
			view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.link_account, null);
			
			_dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
					.setView(view)
					.setTitle("Link account")
					.setPositiveButton("Link account", null)
					.setNegativeButton("Cancel", new OnClickListener(){public void onClick(DialogInterface dialog, int which){}})					
					.create();
			
			_dialog.setOnShowListener(new OnShowListener() {
				@Override
				public void onShow(DialogInterface dialog) {
					SDLActivity.on_show(dialog);
					
					Button b = _dialog.getButton(AlertDialog.BUTTON_POSITIVE);
					
					b.setOnClickListener(new View.OnClickListener() {
						
						@Override
						public void onClick(View v) {
							final String username = et_username.getText().toString().trim();
							final String password = et_password.getText().toString().trim();
							
							if (password.length() < 6 || password.length() > 100) {
								SDLActivity.message("You must enter a valid password.", 0);
								return;
							}
							
							if (username.length() < 3 || username.length() > 20) {
								SDLActivity.message("You must enter a valid username.", 0);
								return;
							}
							
							progressbar.setVisibility(View.VISIBLE);
							
							LicenseValidator.setListener(new LicenseValidator.Listener() {
								@Override
								public void onUpdate(String signature,
										String userdata) {
									PrincipiaBackend.linkAccount(username, password, signature, userdata);
								}

								@Override
								public void onFailure() {
									Log.v("Principia", "Failed!");
									SDLActivity.mSingleton.runOnUiThread(new Runnable(){
										public void run() {
											LinkAccountDialog.num_tries ++;
											if (LinkAccountDialog.num_tries < LinkAccountDialog.MAX_TRIES) {
												SDLActivity.mSingleton.do_check();
											} else {
												progressbar.setVisibility(View.GONE);
												SDLActivity.message("An unknown error has occured when attempting to link your account.", 0);
											}
										}
									});
								}
							});
							
							LinkAccountDialog.num_tries = 0;
							SDLActivity.mSingleton.do_check();
						}
					});
				}
			});

			et_username = (EditText)view.findViewById(R.id.link_account_username);
			et_password = (EditText)view.findViewById(R.id.link_account_password);
			progressbar = (ProgressBar)view.findViewById(R.id.link_account_progress);
		}
		
		return _dialog;
	}
}
