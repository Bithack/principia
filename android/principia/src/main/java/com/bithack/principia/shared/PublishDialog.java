package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnShowListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;

public class PublishDialog
{
    static AlertDialog _dialog;

    static View view;
    static EditText et_name;
    static EditText et_descr;

    static CheckBox cb_locked;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.publish, null);

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                    .setView(view)
                    .setTitle("Publish")
                    .setPositiveButton("Publish", null)
                    .setNegativeButton("Cancel", new OnClickListener(){public void onClick(DialogInterface dialog, int which){}})
                    .create();

            _dialog.setOnShowListener(new OnShowListener() {
                @Override
                public void onShow(DialogInterface dialog) {
                    SDLActivity.on_show(dialog);

                    Button b = _dialog.getButton(AlertDialog.BUTTON_POSITIVE);

                    b.setOnClickListener(new View.OnClickListener() {

                        @Override
                        public void onClick(View v) {
                            String name = et_name.getText().toString().trim();
                            String descr = et_descr.getText().toString().trim();

                            if (name.length() <= 0) {
                                SDLActivity.message("You must enter a name for your level!", 0);
                                return;
                            }

                            PrincipiaBackend.setLevelName(name);
                            PrincipiaBackend.setLevelDescription(descr);
                            PrincipiaBackend.setLevelLocked(cb_locked.isChecked());

                            PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_PUBLISH, 0);

                            _dialog.dismiss();
                        }
                    });
                }
            });

            et_name = (EditText)view.findViewById(R.id.publish_name);
            et_descr = (EditText)view.findViewById(R.id.publish_descr);
            cb_locked = (CheckBox)view.findViewById(R.id.publish_locked);
        }

        return _dialog;
    }

    public static void prepare(Dialog d)
    {
        et_name.setText(PrincipiaBackend.getLevelName());
        et_descr.setText(PrincipiaBackend.getLevelDescription());
    }
}
