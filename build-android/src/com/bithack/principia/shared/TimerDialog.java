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

public class TimerDialog {
    static Dialog _dialog;

    static View view;
    static NumberPicker np_seconds;
    static NumberPicker np_milliseconds;
    static NumberPicker np_num_ticks;
    static CheckBox cb_use_system_time;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.timer, null);

            cb_use_system_time = (CheckBox)view.findViewById(R.id.timer_use_system_time);

            np_seconds = (NumberPicker)view.findViewById(R.id.timer_seconds);
            np_seconds.setRange(0, 360);
            np_milliseconds = (NumberPicker)view.findViewById(R.id.timer_milliseconds);
            np_milliseconds.setRange(0, 1000);
            np_milliseconds.setLongStep(50);

            np_num_ticks = (NumberPicker)view.findViewById(R.id.timer_num_ticks);
            np_num_ticks.setRange(0, 255);

            bld.setTitle("Timer");
            bld.setView(view);
            bld.setPositiveButton("OK", new OnClickListener(){
                public void onClick(DialogInterface dialog, int which) {
                    int x = (TimerDialog.np_seconds.getValue() * 1000) + TimerDialog.np_milliseconds.getValue();
                    if (x < 16) TimerDialog.np_milliseconds.setValue(16);

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
        long full_time = PrincipiaBackend.getPropertyInt(0);
        int num_ticks = PrincipiaBackend.getPropertyInt8(1);
        long use_system_time = PrincipiaBackend.getPropertyInt(2);

        int s = (int)(full_time / 1000);
        int ms = (int)(full_time % 1000);
        np_seconds.setValue(s);
        np_milliseconds.setValue(ms);
        np_num_ticks.setValue(num_ticks);
        cb_use_system_time.setChecked(use_system_time != 0);
    }

    public static void save()
    {
        PrincipiaBackend.setTimerData(np_seconds.getValue(), np_milliseconds.getValue(), np_num_ticks.getValue(), cb_use_system_time.isChecked());
    }
}
