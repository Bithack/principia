package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;

public class PkgLevelDialog {
    static Dialog _dialog;

    static View view;
    static LinearLayout layout;
    static NumberPicker np_level_id;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.pkg_level_id, null);

            np_level_id = new com.bithack.principia.shared.NumberPicker(SDLActivity.getContext());
            np_level_id.setRange(0, 255);
            np_level_id.setValue(1);

            layout = (LinearLayout)view.findViewById(R.id.ll_pkg_level_id);

            layout.addView(np_level_id);

            bld.setTitle("Package level ID");
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

            PkgLevelDialog._dialog = bld.create();
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        np_level_id.setValue(PrincipiaBackend.getPkgItemLevelId());
    }

    public static void save()
    {
        PrincipiaBackend.setPkgItemLevelId(np_level_id.getValue());
    }
}
