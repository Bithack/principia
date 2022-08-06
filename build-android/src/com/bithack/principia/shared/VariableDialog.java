package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;

public class VariableDialog {
    static Dialog _dialog;

    static View view;
    static LinearLayout layout;
    static EditText et_name;
    static Button btn_reset_this;
    static Button btn_reset_all;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.variable, null);

            et_name = (EditText)view.findViewById(R.id.variable_name);
            btn_reset_this = (Button)view.findViewById(R.id.variable_reset_this);
            btn_reset_all = (Button)view.findViewById(R.id.variable_reset_all);

            bld.setTitle("Variable");
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

            btn_reset_this.setOnClickListener(new Button.OnClickListener() {
                @Override
                public void onClick(View v) {
                    PrincipiaBackend.resetVariable(et_name.getText().toString().trim().replaceAll("[^a-zA_Z0-9_-]", ""));
                }
            });

            btn_reset_all.setOnClickListener(new Button.OnClickListener() {
                @Override
                public void onClick(View v) {
                    PrincipiaBackend.resetAllVariables();
                }
            });

            _dialog = bld.create();
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        String variable_name = PrincipiaBackend.getPropertyString(0);
        et_name.setText(variable_name);
    }

    public static void save()
    {
        String s = et_name.getText().toString().trim().replaceAll("[^a-zA_Z0-9_-]", "");
        if (s.length() > 0 && s.length() < 50) {
            PrincipiaBackend.setPropertyString(0, s);
            SDLActivity.message("Saved variable name.", 0);
        } else {
            SDLActivity.message("You must enter a valid variable name. a-zA-Z0-9_- allowed.", 0);
        }
    }
}
