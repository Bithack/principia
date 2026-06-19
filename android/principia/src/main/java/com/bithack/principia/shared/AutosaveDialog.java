package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;

public class AutosaveDialog
{
    static Dialog _dialog = null;

    public AutosaveDialog()
    {
        _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setMessage("Autosave file detected. Open or remove?")
                .setPositiveButton("Open", new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which){
                        PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_OPEN_AUTOSAVE, 0);
                    }}
                )
                .setNegativeButton("Remove", new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which){
                        PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_REMOVE_AUTOSAVE, 0);
                    }}
                )
                .create();
    }

    public Dialog get_dialog()
    {
        return _dialog;
    }
}
