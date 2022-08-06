package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import org.libsdl.app.PrincipiaBackend;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

public class JumperDialog {
    static Dialog _dialog;

    static View view;
    static SeekBar value_sb;
    static EditText value_et;
    static float old_value;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.jumper, null);

            value_sb = (SeekBar)view.findViewById(R.id.jumper_value);
            value_et = (EditText)view.findViewById(R.id.jumper_value_et);

            value_sb.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

                @Override
                public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        float value = progress / 100000.f;
                        value_et.setText(Float.toString(value));
                    }
                }
                @Override public void onStartTrackingTouch(SeekBar seekBar) { }
                @Override public void onStopTrackingTouch(SeekBar seekBar) { }
            });

            bld.setTitle("Jumper");
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
        float value = PrincipiaBackend.getPropertyFloat(0);
        old_value = value;
        value_et.setText(Float.toString(value));
        int progress = (int)(value * 100000.f);
        value_sb.setProgress(progress);
    }

    public static void save()
    {
        float new_value;

        try {
            new_value = Float.parseFloat(value_et.getText().toString());
        } catch (NumberFormatException e) {
            Log.v("Principia", "Using old value.");
            new_value = old_value;
        }

        Log.v("Principia", String.format("New value: %f", new_value));

        PrincipiaBackend.updateJumper(new_value);
    }
}
