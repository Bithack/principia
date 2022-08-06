package com.bithack.principia.shared;

import org.libsdl.app.SDLActivity;

import android.app.AlertDialog;
import android.content.DialogInterface;

public class ConfirmDialog
{
	public static final int OPTION_NO = 0;
	public static final int OPTION_YES = 1;
	
	private OnOptionSelectedListener mListener = null;
	
	public interface OnOptionSelectedListener {
		public void onOptionSelected(int option);
	}
	
	public ConfirmDialog set_listener(OnOptionSelectedListener l)
	{
		this.mListener = l;
		
		return this;
	}

	public ConfirmDialog run(String message)
	{
		return this.run(message, "Yes", "No");
	}
	
	public ConfirmDialog run(String message, String button1, String button2)
	{
		AlertDialog dialog = new AlertDialog.Builder(SDLActivity.getContext()).create();
        //dialog.setTitle("Confirmation");
        dialog.setMessage(message);
        dialog.setCancelable(false);
        dialog.setOnShowListener(SDLActivity.mSingleton);
        dialog.setOnDismissListener(SDLActivity.mSingleton);
        dialog.setButton(DialogInterface.BUTTON_POSITIVE, button1, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int buttonId) {
            	if (mListener != null) {
            		mListener.onOptionSelected(OPTION_YES);
            	}
            }
        });
        dialog.setButton(DialogInterface.BUTTON_NEGATIVE, button2, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int buttonId) {
            	if (mListener != null) {
            		mListener.onOptionSelected(OPTION_NO);
            	}
            }
        });
        
        dialog.setIcon(android.R.drawable.ic_dialog_alert);
        dialog.show();
        
        return this;
	}
}