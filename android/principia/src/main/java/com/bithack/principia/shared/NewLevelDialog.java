package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.widget.Toast;

public class NewLevelDialog
{
    static Dialog _dialog = null;
    public static String filename = "";

    public NewLevelDialog()
    {
        AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);
        bld.setTitle("Create new level");

        bld.setMessage("Please select the level type:\nAdventure - Control a robot\nCustom - Create whatever you want here!");

        /*bld.setPositiveButton("Puzzle", new OnClickListener(){
            public void onClick(DialogInterface dialog, int which)
            {
                PrincipiaBackend.triggerCreateLevel(0);
                Toast.makeText(PrincipiaActivity.mSingleton, "New Puzzle level created!", Toast.LENGTH_SHORT).show();
            }
        });*/

        bld.setNeutralButton("Adventure", new OnClickListener(){
            public void onClick(DialogInterface dialog, int which)
            {
                PrincipiaBackend.triggerCreateLevel(1);
                Toast.makeText(PrincipiaActivity.mSingleton, "New Adventure level created!", Toast.LENGTH_SHORT).show();
            }
        });

        bld.setNegativeButton("Custom", new OnClickListener(){
            public void onClick(DialogInterface dialog, int which)
            {
                PrincipiaBackend.triggerCreateLevel(2);
                Toast.makeText(PrincipiaActivity.mSingleton, "New Custom level created!", Toast.LENGTH_SHORT).show();
            }
        });

        _dialog = bld.create();
    }

    public Dialog get_dialog()
    {
        return _dialog;
    }
}
