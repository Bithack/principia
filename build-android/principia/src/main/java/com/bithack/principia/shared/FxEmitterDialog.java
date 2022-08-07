package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.Spinner;
import android.widget.TextView;

public class FxEmitterDialog {
    static Dialog _dialog;

    static View view;
    static Spinner s_effect_1;
    static Spinner s_effect_2;
    static Spinner s_effect_3;
    static Spinner s_effect_4;
    static SeekBar seekbar_radius;
    static SeekBar seekbar_count;
    static SeekBar seekbar_interval;
    static TextView tv_radius;
    static TextView tv_count;
    static TextView tv_interval;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.fx_emitter, null);

            s_effect_1 = (Spinner)view.findViewById(R.id.fx_1);
            s_effect_2 = (Spinner)view.findViewById(R.id.fx_2);
            s_effect_3 = (Spinner)view.findViewById(R.id.fx_3);
            s_effect_4 = (Spinner)view.findViewById(R.id.fx_4);

            seekbar_radius = (SeekBar)view.findViewById(R.id.seekbar_radius);
            seekbar_count = (SeekBar)view.findViewById(R.id.seekbar_count);
            seekbar_interval = (SeekBar)view.findViewById(R.id.seekbar_interval);
            tv_radius = (TextView)view.findViewById(R.id.tv_radius);
            tv_count = (TextView)view.findViewById(R.id.tv_count);
            tv_interval = (TextView)view.findViewById(R.id.tv_interval);

            seekbar_radius.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

                @Override
                public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    double value = (((double)(progress) / 40.0) * 5.0) + 0.125;
                    tv_radius.setText(String.format("%.3f", value));
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {
                    // TODO Auto-generated method stub

                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                    // TODO Auto-generated method stub

                }
            });

            seekbar_count.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

                @Override
                public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    int value = progress + 1;
                    tv_count.setText(String.format("%d", value));
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {
                    // TODO Auto-generated method stub

                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                    // TODO Auto-generated method stub

                }
            });

            seekbar_interval.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

                @Override
                public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    double value = ((double)(progress) / 20.0f) + 0.05;
                    tv_interval.setText(String.format("%.2f", value));
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {
                    // TODO Auto-generated method stub

                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                    // TODO Auto-generated method stub

                }

            });

            bld.setTitle("FX Emitter");
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

            FxEmitterDialog._dialog = bld.create();
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        String effects = PrincipiaBackend.getFxEmitterEffects();

        String data[] = effects.split(",");

        if (data.length == 4) {
            long effect_1 = Long.parseLong(data[0]);
            long effect_2 = Long.parseLong(data[1]);
            long effect_3 = Long.parseLong(data[2]);
            long effect_4 = Long.parseLong(data[3]);

            if (effect_1 == 3735928559L) {
                s_effect_1.setSelection(0);
            } else {
                s_effect_1.setSelection((int)effect_1 + 1);
            }

            if (effect_2 == 3735928559L) {
                s_effect_2.setSelection(0);
            } else {
                s_effect_2.setSelection((int)effect_2 + 1);
            }
            if (effect_3 == 3735928559L) {
                s_effect_3.setSelection(0);
            } else {
                s_effect_3.setSelection((int)effect_3 + 1);
            }
            if (effect_4 == 3735928559L) {
                s_effect_4.setSelection(0);
            } else {
                s_effect_4.setSelection((int)effect_4 + 1);
            }

            float radius = PrincipiaBackend.getPropertyFloat(0);
            long count = PrincipiaBackend.getPropertyInt(1);
            float interval = PrincipiaBackend.getPropertyFloat(2);

            int progress = 0;

            progress = (int) ((radius - 0.125)/0.125);
            seekbar_radius.setProgress(progress);
            progress = (int) (count - 1);
            seekbar_count.setProgress(progress);
            progress = (int) ((interval - 0.05)/0.05);
            seekbar_interval.setProgress(progress);
        } else {
            Log.v("Principia", "invalid number of fx emitter data fields");
        }
    }

    public static void save()
    {
        float radius    = (float) (((double)(seekbar_radius.getProgress())   / 40.0) * 5.0) + 0.125f;
        long count      = (long)  (seekbar_count.getProgress() + 1);
        float interval  = (float) ((double)(seekbar_interval.getProgress()) / 20.0) + 0.05f;
        PrincipiaBackend.setPropertyFloat(0, radius);
        PrincipiaBackend.setPropertyInt(1, count);
        PrincipiaBackend.setPropertyFloat(2, interval);
        PrincipiaBackend.setFxEmitterEffects(s_effect_1.getSelectedItemPosition(),
                                              s_effect_2.getSelectedItemPosition(),
                                              s_effect_3.getSelectedItemPosition(),
                                              s_effect_4.getSelectedItemPosition());
    }
}
