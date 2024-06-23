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
import android.widget.EditText;

public class SaveAsDialog
{
    static Dialog _dialog;

    static View view;
    static EditText et_name;

    public static boolean refresh_name = false;
    public static boolean copy;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);
            bld.setTitle("Save a copy");

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.save_as, null);
            et_name = (EditText)view.findViewById(R.id.save_name);
            bld.setView(view);

            bld.setNeutralButton("Save", new OnClickListener(){
                public void onClick(DialogInterface dialog, int which)
                {
                    String name = et_name.getText().toString().trim();
                    PrincipiaBackend.setLevelName(name);
                    PrincipiaBackend.triggerSave(SaveAsDialog.copy);
                }
            });

            bld.setNegativeButton("Cancel", new OnClickListener(){
                public void onClick(DialogInterface dialog, int which)
                {
                }
            });

            _dialog = bld.create();
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        if (refresh_name) {
            String name = PrincipiaBackend.getLevelName();

            if (copy) {
                _dialog.setTitle("Save a copy");
                name = name + " (copy)";

            } else
                _dialog.setTitle("Save level");
            et_name.setText(name);
            refresh_name = false;
        }
    }
}
