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
import android.widget.EditText;

public class LoginDialog
{
    static AlertDialog _dialog;

    static View view;
    static EditText et_username;
    static EditText et_password;

    static Button btn_register_account;

    /* TODO */
    //static CheckBox cb_remember_username;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.login, null);

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                    .setView(view)
                    .setTitle("Log in")
                    .setPositiveButton("Log in", null)
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
                            String username = et_username.getText().toString().trim();
                            String password = et_password.getText().toString().trim();

                            if (username.length() <= 0 || password.length() <= 0) {
                                SDLActivity.message("You must enter a valid username and password.", 0);
                                return;
                            }

                            PrincipiaBackend.login(username, password);

                            _dialog.dismiss();
                        }
                    });
                }
            });

            et_username = (EditText)view.findViewById(R.id.login_username);
            et_password = (EditText)view.findViewById(R.id.login_password);
            btn_register_account = (Button)view.findViewById(R.id.btn_register_account);

            btn_register_account.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    SDLActivity.open_dialog(SDLActivity.DIALOG_REGISTER);
                    _dialog.dismiss();
                }
            });
        }

        return _dialog;
    }

    public static void prepare(Dialog d)
    {
        //TODO
        //et_username.setText(PrincipiaActivity.getSavedUsername());
    }
}
