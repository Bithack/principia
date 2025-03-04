package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.CheckBox;

public class SoundManDialog {
    static Dialog _dialog;

    static View view;

    static Spinner s_sounds;
    static CheckBox cb_catch_all;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            final String title = PrincipiaActivity.mSingleton.getString(R.string.sound_manager);
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.soundman, null);

            cb_catch_all = (CheckBox)view.findViewById(R.id.sm_catch_all);
            s_sounds = (Spinner)view.findViewById(R.id.sm_sounds);

            String[] array_data = PrincipiaBackend.getSounds().split(",.,");
            ArrayAdapter<String> spinnerArrayAdapter = new ArrayAdapter<String>(SDLActivity.mSingleton, android.R.layout.select_dialog_item, array_data);
            s_sounds.setAdapter(spinnerArrayAdapter);

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setTitle(title)
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
        s_sounds.setSelection((int)PrincipiaBackend.getPropertyInt(0));
        boolean catch_all_active = PrincipiaBackend.getPropertyInt8(1) != 0;

        cb_catch_all.setChecked(catch_all_active);
    }

    public static void save()
    {
        PrincipiaBackend.setPropertyInt(0, s_sounds.getSelectedItemPosition());
        PrincipiaBackend.setPropertyInt8(1, cb_catch_all.isChecked() ? 1 : 0);

        PrincipiaBackend.fixed();
    }
}
