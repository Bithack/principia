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

public class FrequencyDialog {
    static Dialog _dialog;

    static View view;
    static EditText et_frequency;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.frequency, null);

            et_frequency = (EditText)view.findViewById(R.id.et_frequency);

            bld.setTitle("Frequency");
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

    public static void prepare(DialogInterface di)
    {
        et_frequency.setText(Long.toString(PrincipiaBackend.getPropertyInt(0)));
    }

    public static void save()
    {
        long freq = 0;
        try {
            freq = Long.parseLong(et_frequency.getText().toString());
        } catch (NumberFormatException e) {
            freq = 0;
        }
        PrincipiaBackend.setFrequency(freq);
    }
}

