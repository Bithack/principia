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

public class ExportDialog
{
    static Dialog _dialog;

    static View view;
    static EditText et_name;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);
            bld.setTitle("Export object");

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.export, null);
            et_name = (EditText)view.findViewById(R.id.et_object_name);
            bld.setView(view);

            bld.setNeutralButton("Save", new OnClickListener(){
                public void onClick(DialogInterface dialog, int which)
                {
                    String name = et_name.getText().toString().trim();
                    PrincipiaBackend.saveObject(name);
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
        et_name.setText("");
    }
}
