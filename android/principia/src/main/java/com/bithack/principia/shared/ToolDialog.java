package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;

public class ToolDialog
{
    static Dialog _dialog = null;

    public ToolDialog()
    {
        AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

        bld.setMessage("Please select which tool you want to use");

        bld.setPositiveButton("Connection Edit", new OnClickListener(){
            public void onClick(DialogInterface dialog, int which)
            {
                PrincipiaBackend.setGameMode(16);
            }
        });

        bld.setNeutralButton("Multi-Select", new OnClickListener(){
            public void onClick(DialogInterface dialog, int which)
            {
                PrincipiaBackend.setGameMode(6);
            }
        });

        bld.setNegativeButton("Terrain Paint", new OnClickListener(){
            public void onClick(DialogInterface dialog, int which)
            {
                PrincipiaBackend.setGameMode(13);
            }
        });

        _dialog = bld.create();
    }

    public Dialog get_dialog()
    {
        return _dialog;
    }
}
