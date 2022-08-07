package com.bithack.principia.shared;

import java.util.Locale;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import com.bithack.principia.shared.RangeSeekBar.OnRangeSeekBarChangeListener;

import org.libsdl.app.PrincipiaBackend;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

public class TouchFieldDialog {
    static Dialog _dialog;

    static View view;
    static LinearLayout layout;
    static TextView tv_lower_x;
    static TextView tv_upper_x;
    static TextView tv_lower_y;
    static TextView tv_upper_y;

    static RangeSeekBar<Float> rsb_x;
    static RangeSeekBar<Float> rsb_y;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.cursor_field, null);

            layout = (LinearLayout)view.findViewById(R.id.cursor_field_layout);
            tv_lower_x = (TextView)layout.findViewById(R.id.cursor_field_lower_x);
            tv_upper_x = (TextView)layout.findViewById(R.id.cursor_field_upper_x);
            tv_lower_y = (TextView)layout.findViewById(R.id.cursor_field_lower_y);
            tv_upper_y = (TextView)layout.findViewById(R.id.cursor_field_upper_y);

            rsb_x = new RangeSeekBar<Float>(-3.f, 3.f, PrincipiaActivity.getContext());
            rsb_x.setOnRangeSeekBarChangeListener(new OnRangeSeekBarChangeListener<Float>() {
                @Override
                public void onRangeSeekBarValuesChanged(
                        RangeSeekBar<?> bar, Float minValue, Float maxValue) {
                    tv_lower_x.setText(String.format(Locale.US, "%.2f", minValue));
                    tv_upper_x.setText(String.format(Locale.US, "%.2f", maxValue));
                }
            });

            layout.addView(rsb_x, 2);

            rsb_y = new RangeSeekBar<Float>(-3.f, 3.f, PrincipiaActivity.getContext());
            rsb_y.setOnRangeSeekBarChangeListener(new OnRangeSeekBarChangeListener<Float>() {
                @Override
                public void onRangeSeekBarValuesChanged(
                        RangeSeekBar<?> bar, Float minValue, Float maxValue) {
                    tv_lower_y.setText(String.format(Locale.US, "%.2f", minValue));
                    tv_upper_y.setText(String.format(Locale.US, "%.2f", maxValue));
                }
            });
            layout.addView(rsb_y);

            bld.setTitle("Cursor Field");
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
        float right, up, left, down;
        right = PrincipiaBackend.getPropertyFloat(0);
        up = PrincipiaBackend.getPropertyFloat(1);
        left = PrincipiaBackend.getPropertyFloat(2);
        down = PrincipiaBackend.getPropertyFloat(3);
        rsb_x.setSelectedMinValue(left);
        rsb_x.setSelectedMaxValue(right);
        rsb_y.setSelectedMinValue(down);
        rsb_y.setSelectedMaxValue(up);

        tv_lower_x.setText(String.format(Locale.US, "%.2f", left));
        tv_upper_x.setText(String.format(Locale.US, "%.2f", right));
        tv_lower_y.setText(String.format(Locale.US, "%.2f", down));
        tv_upper_y.setText(String.format(Locale.US, "%.2f", up));
    }

    public static void save()
    {
        float right, up, left, down;
        right = rsb_x.getSelectedMaxValue();
        up = rsb_y.getSelectedMaxValue();
        left = rsb_x.getSelectedMinValue();
        down = rsb_y.getSelectedMinValue();

        if (right < left+0.5f) right = left+0.5f;
        if (right > 3.f) {
            right = 3.f;
            left = right-0.5f;
        }
        if (up < down+0.5f) up = down+0.5f;
        if (up > 3.f) {
            up = 3.f;
            down = up-0.5f;
        }
        PrincipiaBackend.setPropertyFloat(0, right);
        PrincipiaBackend.setPropertyFloat(1, up);
        PrincipiaBackend.setPropertyFloat(2, left);
        PrincipiaBackend.setPropertyFloat(3, down);
    }
}
