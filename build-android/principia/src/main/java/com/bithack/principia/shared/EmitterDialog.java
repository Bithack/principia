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

public class EmitterDialog {
    static Dialog _dialog;

    static View view;

    static SeekBar sb_absorb;

    static TextView tv_absorb;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.emitter, null);

            sb_absorb = (SeekBar)view.findViewById(R.id.emitter_sb_absorb);
            tv_absorb = (TextView)view.findViewById(R.id.emitter_tv_absorb);

            sb_absorb.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {

                        if (progress < 10) {
                            tv_absorb.setText(PrincipiaActivity.mSingleton.getString(R.string.off));
                        } else {
                            tv_absorb.setText(String.format(Locale.US, "after %.2f seconds.", progress / 10.f));
                        }
                    }
                }
                @Override public void onStartTrackingTouch(SeekBar seekBar) { }
                @Override public void onStopTrackingTouch(SeekBar seekBar) { }
            });

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setTitle(PrincipiaActivity.mSingleton.getString(R.string.emitter))
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
        int progress = (int)(PrincipiaBackend.getPropertyFloat(6) * 10.f);
        sb_absorb.setProgress(progress);

        if (progress < 10) {
            tv_absorb.setText(PrincipiaActivity.mSingleton.getString(R.string.off));
        } else {
            tv_absorb.setText(String.format(Locale.US, "after %.2f seconds.", progress / 10.f));
        }
    }

    public static void save()
    {
        PrincipiaBackend.setPropertyFloat(6, (float)sb_absorb.getProgress()/10.f);

        PrincipiaBackend.fixed();
    }
}
