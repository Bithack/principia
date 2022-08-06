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

public class FrequencyRangeDialog {
    static Dialog _dialog;

    static View view;
    static EditText et_freqrange_start;
    static EditText et_freqrange_range;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.frequency_range, null);

            et_freqrange_start = (EditText)view.findViewById(R.id.et_freqrange_start);
            et_freqrange_range = (EditText)view.findViewById(R.id.et_freqrange_range);

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
        et_freqrange_start.setText(Long.toString(PrincipiaBackend.getPropertyInt(0)));
        et_freqrange_range.setText(Long.toString(PrincipiaBackend.getPropertyInt(1)));
    }

    public static void save()
    {
        long f_start = 0;
        long f_range = 0;
        try {
            f_start = Long.parseLong(et_freqrange_start.getText().toString());
        } catch (NumberFormatException e) { f_start = 0; }
        try {
            f_range = Long.parseLong(et_freqrange_range.getText().toString());
        } catch (NumberFormatException e) { f_range = 0; }
        PrincipiaBackend.setFrequencyRange(f_start, f_range);
    }
}
