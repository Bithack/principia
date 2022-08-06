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

public class SequencerDialog {
    static Dialog _dialog;

    static View view;
    static EditText sequence;
    static NumberPicker np_seconds;
    static NumberPicker np_milliseconds;
    static CheckBox wrap_around;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.sequencer, null);

            np_seconds = (NumberPicker)view.findViewById(R.id.sequencer_seconds);
            np_seconds.setRange(0, 360);
            np_milliseconds = (NumberPicker)view.findViewById(R.id.sequencer_milliseconds);
            np_milliseconds.setRange(0, 1000);
            np_milliseconds.setLongStep(50);

            sequence = (EditText)view.findViewById(R.id.sequencer_sequence);
            wrap_around = (CheckBox)view.findViewById(R.id.sequencer_wrap_around);

            bld.setTitle("Sequencer");
            bld.setView(view);
            bld.setPositiveButton("OK", new OnClickListener(){
                public void onClick(DialogInterface dialog, int which) {
                    int x = (SequencerDialog.np_seconds.getValue() * 1000) + SequencerDialog.np_milliseconds.getValue();
                    if (x < 16) SequencerDialog.np_milliseconds.setValue(16);

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
        String _sequence = PrincipiaBackend.getPropertyString(0);
        long full_time = PrincipiaBackend.getPropertyInt(1);
        boolean wrap = (PrincipiaBackend.getPropertyInt8(2) == 1);

        int s = (int)(full_time / 1000);
        int ms = (int)(full_time % 1000);
        np_seconds.setValue(s);
        np_milliseconds.setValue(ms);
        wrap_around.setChecked(wrap);

        sequence.setText(_sequence);
    }

    public static void save()
    {
        PrincipiaBackend.setSequencerData(sequence.getText().toString().trim(), np_seconds.getValue(), np_milliseconds.getValue(), wrap_around.isChecked());
    }
}
