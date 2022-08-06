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
import android.widget.Spinner;

public class ResourceDialog {
    static Dialog _dialog;

    static View view;
    static Spinner s_type;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.resource, null);

            s_type = (Spinner)view.findViewById(R.id.s_resource_type);

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setTitle(PrincipiaActivity.mSingleton.getString(R.string.resource))
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
        int resource_type = PrincipiaBackend.getPropertyInt8(0);
        s_type.setSelection(resource_type);
    }

    public static void save()
    {
        PrincipiaBackend.setResourceType(s_type.getSelectedItemPosition());
    }
}
