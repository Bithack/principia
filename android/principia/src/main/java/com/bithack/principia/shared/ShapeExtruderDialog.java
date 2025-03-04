package com.bithack.principia.shared;

import java.util.Locale;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import org.libsdl.app.PrincipiaBackend;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

public class ShapeExtruderDialog {
    static Dialog _dialog;

    static View view;
    static SeekBar sb_right;
    static SeekBar sb_up;
    static SeekBar sb_left;
    static SeekBar sb_down;
    static TextView tv_right;
    static TextView tv_up;
    static TextView tv_left;
    static TextView tv_down;

    static float old_right;
    static float old_up;
    static float old_left;
    static float old_down;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.shape_extruder, null);

            sb_right = (SeekBar)view.findViewById(R.id.shape_right_sb);
            sb_up    = (SeekBar)view.findViewById(R.id.shape_up_sb);
            sb_left  = (SeekBar)view.findViewById(R.id.shape_left_sb);
            sb_down  = (SeekBar)view.findViewById(R.id.shape_down_sb);

            tv_right = (TextView)view.findViewById(R.id.shape_right_tv);
            tv_up    = (TextView)view.findViewById(R.id.shape_up_tv);
            tv_left  = (TextView)view.findViewById(R.id.shape_left_tv);
            tv_down  = (TextView)view.findViewById(R.id.shape_down_tv);

            sb_right.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        float value = progress / 100.f;
                        tv_right.setText(String.format(Locale.US, "%.2f", value));
                    }
                }
                @Override public void onStartTrackingTouch(SeekBar seekBar) { }
                @Override public void onStopTrackingTouch(SeekBar seekBar) { }
            });

            sb_up.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        float value = progress / 100.f;
                        tv_up.setText(String.format(Locale.US, "%.2f", value));
                    }
                }
                @Override public void onStartTrackingTouch(SeekBar seekBar) { }
                @Override public void onStopTrackingTouch(SeekBar seekBar) { }
            });

            sb_left.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        float value = progress / 100.f;
                        tv_left.setText(String.format(Locale.US, "%.2f", value));
                    }
                }
                @Override public void onStartTrackingTouch(SeekBar seekBar) { }
                @Override public void onStopTrackingTouch(SeekBar seekBar) { }
            });

            sb_down.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        float value = progress / 100.f;
                        tv_down.setText(String.format(Locale.US, "%.2f", value));
                    }
                }
                @Override public void onStartTrackingTouch(SeekBar seekBar) { }
                @Override public void onStopTrackingTouch(SeekBar seekBar) { }
            });

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setTitle(PrincipiaActivity.mSingleton.getString(R.string.shape_extruder))
                .setView(view)
                .setPositiveButton(PrincipiaActivity.mSingleton.getString(R.string.ok), new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which) {
                        save();
                        dialog.dismiss();
                    }
                })
                .setNegativeButton(PrincipiaActivity.mSingleton.getString(R.string.cancel), new OnClickListener(){
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
        float v_right = PrincipiaBackend.getPropertyFloat(0);
        float v_up    = PrincipiaBackend.getPropertyFloat(1);
        float v_left  = PrincipiaBackend.getPropertyFloat(2);
        float v_down  = PrincipiaBackend.getPropertyFloat(3);
        int p;

        ShapeExtruderDialog.old_right = v_right;
        ShapeExtruderDialog.old_up    = v_up;
        ShapeExtruderDialog.old_left  = v_left;
        ShapeExtruderDialog.old_down  = v_down;

        tv_right.setText(String.format(Locale.US, "%.2f", v_right));
        tv_up.setText(String.format(Locale.US, "%.2f", v_up));
        tv_left.setText(String.format(Locale.US, "%.2f", v_left));
        tv_down.setText(String.format(Locale.US, "%.2f", v_down));

        p = (int)(v_right * 100.f);
        sb_right.setProgress(p);
        p = (int)(v_up * 100.f);
        sb_up.setProgress(p);
        p = (int)(v_left * 100.f);
        sb_left.setProgress(p);
        p = (int)(v_down * 100.f);
        sb_down.setProgress(p);
    }

    public static void save()
    {
        float v_right = ShapeExtruderDialog.old_right;
        float v_up    = ShapeExtruderDialog.old_up;
        float v_left  = ShapeExtruderDialog.old_left;
        float v_down  = ShapeExtruderDialog.old_down;

        try {
            v_right = Float.parseFloat(tv_right.getText().toString());
        } catch (NumberFormatException e) {
            v_right = ShapeExtruderDialog.old_right;
        }
        try {
            v_up = Float.parseFloat(tv_up.getText().toString());
        } catch (NumberFormatException e) {
            v_up = ShapeExtruderDialog.old_up;
        }
        try {
            v_left = Float.parseFloat(tv_left.getText().toString());
        } catch (NumberFormatException e) {
            v_left = ShapeExtruderDialog.old_left;
        }
        try {
            v_down = Float.parseFloat(tv_down.getText().toString());
        } catch (NumberFormatException e) {
            v_down = ShapeExtruderDialog.old_down;
        }

        PrincipiaBackend.updateShapeExtruder(v_right, v_up, v_left, v_down);
    }
}
