package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Spinner;

public class CommandPadDialog {
    static Dialog _dialog;

    static View view;
    static Spinner s_command;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.command_pad, null);

            s_command = (Spinner)view.findViewById(R.id.command);

            bld.setTitle("Command pad");
            bld.setView(view);
            bld.setPositiveButton("OK", new OnClickListener(){
                public void onClick(DialogInterface dialog, int which) {
                    save();
                    dialog.dismiss();
                }

            });
            bld.setNegativeButton("Cancel", new OnClickListener(){
                public void onClick(DialogInterface dialog, int which) {
                    dialog.dismiss();
                }

            });

            CommandPadDialog._dialog = bld.create();
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        //((Spinner)view.findViewById(R.id.command)).setSelection(PrincipiaActivity.getCommandPadCommand());
        s_command.setSelection(PrincipiaBackend.getCommandPadCommand());
    }

    public static void save()
    {
        PrincipiaBackend.setCommandPadCommand(s_command.getSelectedItemPosition());
    }
}
