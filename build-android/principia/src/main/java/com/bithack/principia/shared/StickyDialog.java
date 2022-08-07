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
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.RadioButton;

public class StickyDialog {
    static Dialog _dialog;

    static View view;
    static EditText et_text;
    static RadioGroup rg_textsize;
    static CheckBox cb_horiz;
    static CheckBox cb_vert;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.sticky, null);

            et_text = (EditText)view.findViewById(R.id.sticky_text);
            rg_textsize = (RadioGroup)view.findViewById(R.id.sticky_textsize);
            cb_horiz = (CheckBox)view.findViewById(R.id.sticky_horiz);
            cb_vert = (CheckBox)view.findViewById(R.id.sticky_vert);

            bld.setTitle("Sticky Note");
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

            _dialog = bld.create();
        }

        return _dialog;
    }

    public static void save()
    {
        int size = 1;

        switch (rg_textsize.getCheckedRadioButtonId()) {
            case R.id.stickysize0: size = 0; break;
            case R.id.stickysize1: size = 1; break;
            case R.id.stickysize2: size = 2; break;
            case R.id.stickysize3: size = 3; break;
        }
        PrincipiaBackend.setStickyStuff(et_text.getText().toString(), cb_horiz.isChecked(), cb_vert.isChecked(), size);
    }

    public static void prepare(DialogInterface di)
    {
        int size = PrincipiaBackend.getStickySize();

        et_text.setText(PrincipiaBackend.getStickyText());
        cb_horiz.setChecked(PrincipiaBackend.getStickyCenterHoriz());
        cb_vert.setChecked(PrincipiaBackend.getStickyCenterVert());

        switch (size) {
            case 0: ((RadioButton)view.findViewById(R.id.stickysize0)).setChecked(true); break;
            case 1: ((RadioButton)view.findViewById(R.id.stickysize1)).setChecked(true); break;
            case 2: ((RadioButton)view.findViewById(R.id.stickysize2)).setChecked(true); break;
            case 3: ((RadioButton)view.findViewById(R.id.stickysize3)).setChecked(true); break;
        }
    }
}
