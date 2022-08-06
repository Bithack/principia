package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import com.bithack.principia.PrincipiaActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;

public class PuzzlePlayDialog
{
    static Dialog _dialog = null;

    public PuzzlePlayDialog()
    {
        AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);
        bld.setTitle("Play method");
        bld.setMessage("Do you want to test play the level, or just simulate it?");

        bld.setPositiveButton("Test play", new OnClickListener(){
            public void onClick(DialogInterface dialog, int which)
            {
                PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_PUZZLEPLAY, 0);
                SDLActivity.message("Test-playing level!", 0);
            }
        });

        bld.setNeutralButton("Simulate", new OnClickListener(){
            public void onClick(DialogInterface dialog, int which)
            {
                PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_PUZZLEPLAY, 1);
                SDLActivity.message("Simulating level!", 0);
            }
        });

        bld.setNegativeButton("Cancel", new OnClickListener(){
            public void onClick(DialogInterface dialog, int which)
            {
            }
        });

        _dialog = bld.create();
    }

    public Dialog get_dialog()
    {
        return _dialog;
    }
}
