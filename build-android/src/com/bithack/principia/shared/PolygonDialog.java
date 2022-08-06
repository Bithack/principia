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
import android.widget.CheckBox;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

public class PolygonDialog {
    static Dialog _dialog;

    static View view;

    static SeekBar sb_sublayer_depth;

    static CheckBox cb_front_align;

    static TextView tv_sublayer_depth;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.polygon, null);

            sb_sublayer_depth = (SeekBar)view.findViewById(R.id.polygon_sublayer_depth);
            tv_sublayer_depth = (TextView)view.findViewById(R.id.polygon_sublayer_depth_tv);

            cb_front_align = (CheckBox)view.findViewById(R.id.polygon_front_align);

            sb_sublayer_depth.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        tv_sublayer_depth.setText(String.format(Locale.US, "%d", progress+1));
                    }
                }
                @Override public void onStartTrackingTouch(SeekBar seekBar) { }
                @Override public void onStopTrackingTouch(SeekBar seekBar) { }
            });

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setTitle(PrincipiaActivity.mSingleton.getString(R.string.plastic_polygon))
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
        int sublayer_depth = PrincipiaBackend.getPropertyInt8(0);
        int front_align = PrincipiaBackend.getPropertyInt8(1);

        sb_sublayer_depth.setProgress(sublayer_depth);

        tv_sublayer_depth.setText(String.format(Locale.US, "%d", sublayer_depth+1));

        cb_front_align.setChecked(front_align == 1);
    }

    public static void save()
    {
        int sublayer_depth = sb_sublayer_depth.getProgress();
        boolean front_align = cb_front_align.isChecked();

        PrincipiaBackend.setPropertyInt8(0, sublayer_depth);
        PrincipiaBackend.setPropertyInt8(1, front_align ? 1 : 0);

        PrincipiaBackend.fixed();
    }
}
