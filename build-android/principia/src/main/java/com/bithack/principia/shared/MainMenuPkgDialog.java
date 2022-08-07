package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.widget.Toast;

public class MainMenuPkgDialog
{
    static Dialog _dialog = null;

    public MainMenuPkgDialog()
    {
        _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
            .setMessage("Select package")
            .setPositiveButton("Main Puzzles", new OnClickListener(){
                public void onClick(DialogInterface dialog, int which)
                {
                    PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_MAIN_MENU_PKG, 0);
                }
            })
            .setNeutralButton("Adventure Introduction", new OnClickListener(){
                public void onClick(DialogInterface dialog, int which)
                {
                    PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_MAIN_MENU_PKG, 1);
                }
            })
            .create();
    }

    public Dialog get_dialog()
    {
        return _dialog;
    }
}
