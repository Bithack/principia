package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.text.Html;

public class InfoDialog
{
    Dialog _dialog;

    public static String description;

    public InfoDialog()
    {
        AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);
        bld.setTitle("Level description");
        bld.setMessage(description);

        bld.setNeutralButton("Close", new OnClickListener(){
            public void onClick(DialogInterface dialog, int which) { }
        });

        this._dialog = bld.create();
    }

    public Dialog get_dialog()
    {
        return this._dialog;
    }
}
