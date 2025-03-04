package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import org.libsdl.app.PrincipiaBackend;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

public class RubberDialog {
    static Dialog _dialog;

    static View view;
    static SeekBar sb_restitution;
    static SeekBar sb_friction;
    static EditText et_restitution;
    static EditText et_friction;

    static float old_restitution;
    static float old_friction;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.rubber, null);

            sb_restitution = (SeekBar)view.findViewById(R.id.rubber_restitution);
            et_restitution = (EditText)view.findViewById(R.id.rubber_restitution_et);

            sb_friction = (SeekBar)view.findViewById(R.id.rubber_friction);
            et_friction = (EditText)view.findViewById(R.id.rubber_friction_et);

            sb_restitution.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        float value = progress / 100000.f;
                        et_restitution.setText(Float.toString(value));
                    }
                }
                @Override public void onStartTrackingTouch(SeekBar seekBar) { }
                @Override public void onStopTrackingTouch(SeekBar seekBar) { }
            });

            sb_friction.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        float value = (progress / 10000.f) + 1.f;
                        et_friction.setText(Float.toString(value));
                    }
                }
                @Override public void onStartTrackingTouch(SeekBar seekBar) { }
                @Override public void onStopTrackingTouch(SeekBar seekBar) { }
            });

            et_restitution.setOnEditorActionListener(new OnEditorActionListener() {
                @Override
                public boolean onEditorAction(TextView v, int actionId,
                        KeyEvent event) {
                    if (event != null) {
                        return true;
                    }

                    float val;
                    try {
                        val = Float.parseFloat(v.getText().toString());
                    } catch (NumberFormatException e) {
                        val = RubberDialog.old_restitution;
                    }

                    if (val < 0.f) val = 0.f;
                    else if (val > 1.f) val = 1.f;

                    v.setText(Float.toString(val));
                    int p = (int)(val * 100000.f);
                    sb_restitution.setProgress(p);
                    return false;
                }
            });

            et_friction.setOnEditorActionListener(new OnEditorActionListener() {
                @Override
                public boolean onEditorAction(TextView v, int actionId,
                        KeyEvent event) {
                    if (event != null) {
                        return true;
                    }

                    float val;
                    try {
                        val = Float.parseFloat(v.getText().toString());
                    } catch (NumberFormatException e) {
                        val = RubberDialog.old_friction;
                    }

                    if (val < 1.f) val = 1.f;
                    else if (val > 10.f) val = 10.f;

                    v.setText(Float.toString(val));
                    int p = (int)((val-1) * 10000.f);
                    sb_friction.setProgress(p);
                    return false;
                }
            });

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setTitle("Rubber properties")
                .setView(view)
                .setPositiveButton("OK", new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which) {
                        save();
                        dialog.dismiss();
                    }
                })
                .setNegativeButton("Cancel", new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                })
                .create();
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        float v_restitution = PrincipiaBackend.getPropertyFloat(1);
        float v_friction = PrincipiaBackend.getPropertyFloat(2);

        RubberDialog.old_restitution = v_restitution;
        RubberDialog.old_friction = v_friction;

        et_restitution.setText(Float.toString(v_restitution));
        et_friction.setText(Float.toString(v_friction));

        int p = (int)(v_restitution * 100000.f);
        sb_restitution.setProgress(p);

        p = (int)((v_friction-1) * 10000.f);
        sb_friction.setProgress(p);
    }

    public static void save()
    {
        float v_restitution = RubberDialog.old_restitution;
        float v_friction = RubberDialog.old_friction;

        try {
            v_restitution = Float.parseFloat(et_restitution.getText().toString());
        } catch (NumberFormatException e) {
            Log.v("Principia", "Using old restitution value.");
            v_restitution = RubberDialog.old_restitution;
        }

        try {
            v_friction = Float.parseFloat(et_friction.getText().toString());
        } catch (NumberFormatException e) {
            Log.v("Principia", "Using old friction value.");
            v_friction = RubberDialog.old_friction;
        }

        Log.v("Principia", String.format("New values: %.2f/%.2f", v_restitution, v_friction));

        PrincipiaBackend.updateRubberEntity(v_restitution, v_friction);
    }
}
