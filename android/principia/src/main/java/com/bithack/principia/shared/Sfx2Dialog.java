package com.bithack.principia.shared;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.Spinner;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;

import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

public class Sfx2Dialog {
    static Dialog _dialog;

    static View view;
    static Spinner s_sfx;
    static CheckBox sfx_global;
    static CheckBox sfx_loop;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.sfx2_dialog, null);

            sfx_global = (CheckBox)view.findViewById(R.id.sfx_global);
            sfx_loop = (CheckBox)view.findViewById(R.id.sfx_loop);
            s_sfx = (Spinner)view.findViewById(R.id.s_sfx);
            String[] sound_effects = PrincipiaBackend.getSounds().split(",.,");

            ArrayAdapter<String> spinnerArrayAdapter = new ArrayAdapter<String>(SDLActivity.mSingleton, android.R.layout.select_dialog_item, sound_effects);
            s_sfx.setAdapter(spinnerArrayAdapter);

            bld.setTitle("SFX Emitter");
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

            Sfx2Dialog._dialog = bld.create();
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        s_sfx.setSelection((int) PrincipiaBackend.getPropertyInt(0));
        sfx_global.setChecked(PrincipiaBackend.getPropertyInt8(1) == 1);
        sfx_loop.setChecked(PrincipiaBackend.getPropertyInt8(3) == 1);
    }

    public static void save()
    {
        PrincipiaBackend.setPropertyInt(0, s_sfx.getSelectedItemId());
        PrincipiaBackend.setPropertyInt8(1, sfx_global.isChecked() ? 1 : 0);
        //TODO: property_index 2 (sound chunk)
        PrincipiaBackend.setPropertyInt8(3, sfx_loop.isChecked() ? 1 : 0);
        SDLActivity.message("Saved properties for SFX Emitter.", 0);
    }
}
