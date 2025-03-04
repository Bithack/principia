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

public class DecorationDialog {
    static Dialog _dialog;

    static View view;
    static Spinner s_deco;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.decoration, null);

            s_deco = (Spinner)view.findViewById(R.id.s_deco);
            String[] consumables = PrincipiaBackend.getDecorations().split(",.,");

            ArrayAdapter<String> spinnerArrayAdapter = new ArrayAdapter<String>(SDLActivity.mSingleton, android.R.layout.select_dialog_item, consumables);
            s_deco.setAdapter(spinnerArrayAdapter);

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setTitle(PrincipiaActivity.mSingleton.getString(R.string.decoration))
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
        s_deco.setSelection((int)PrincipiaBackend.getPropertyInt(0));
    }

    public static void save()
    {
        PrincipiaBackend.setPropertyInt(0, s_deco.getSelectedItemPosition());

        PrincipiaBackend.fixed();
    }
}
