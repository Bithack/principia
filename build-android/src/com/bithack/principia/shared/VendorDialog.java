package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;

public class VendorDialog {
    static Dialog _dialog;

    static View view;

    static NumberPicker np_amount;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.vendor, null);

            np_amount = (NumberPicker)view.findViewById(R.id.np_amount);
            np_amount.setRange(1, 63335);
            np_amount.setLongStep(5);

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setTitle(PrincipiaActivity.mSingleton.getString(R.string.vendor))
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
        long amount = PrincipiaBackend.getPropertyInt(2);
    }

    public static void save()
    {
        long val = np_amount.getValue();
        if (val < 1) {
            val = 1;
        } else if (val > 65535) {
            val = 65535;
        }

        PrincipiaBackend.setPropertyInt(2, val);

        PrincipiaBackend.fixed();
    }
}
