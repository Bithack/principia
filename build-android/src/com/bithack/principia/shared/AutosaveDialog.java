package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

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
        /*
        Log.v("Principia", String.format("Board: %s\nBrand:%s\nDevice:%s\nDisplay:%s\nModel:%s",
                android.os.Build.BOARD,
                android.os.Build.BRAND,
                android.os.Build.DEVICE,
                android.os.Build.DISPLAY,
                android.os.Build.MODEL
                ));
                */
        _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setMessage("Autosave file detected. Open or remove?")
                .setPositiveButton("Open", new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which){
                        PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_OPEN_AUTOSAVE, 0);
                    }}
                )
                .setNegativeButton("Remove", new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which){
                        PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_REMOVE_AUTOSAVE, 0);
                    }}
                )
                .create();
    }

    public Dialog get_dialog()
    {
        return _dialog;
    }
}
