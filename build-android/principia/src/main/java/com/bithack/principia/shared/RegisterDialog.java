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
import android.widget.ProgressBar;

public class RegisterDialog
{
    static AlertDialog _dialog;

    static View view;
    static EditText et_username;
    static EditText et_email;
    static EditText et_password;
    static EditText et_password_confirm;
    public static ProgressBar progressbar;
    public static int num_tries = 0;
    public static final int MAX_TRIES = 3;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.register, null);

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                    .setView(view)
                    .setTitle("Register")
                    .setPositiveButton("Register", null)
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
                            final String username = et_username.getText().toString().trim();
                            final String password = et_password.getText().toString().trim();
                            final String password_confirm = et_password_confirm.getText().toString().trim();
                            final String email = et_email.getText().toString().trim();

                            if (password.length() < 6 || password.length() > 100) {
                                SDLActivity.message("Your password must be at least 3 and at most 100 characters.", 0);
                                return;
                            }

                            if (!password.equals(password_confirm)) {
                                SDLActivity.message("The two passwords you entered don't match.", 0);
                                return;
                            }

                            if (username.length() < 3 || username.length() > 20) {
                                SDLActivity.message("Your username must be at least 3 and at most 20 characters.", 0);
                                return;
                            }

                            if (!android.util.Patterns.EMAIL_ADDRESS.matcher(email).matches()) {
                                SDLActivity.message("You must enter a valid email address.", 0);
                                return;
                            }

                            progressbar.setVisibility(View.VISIBLE);
                            PrincipiaBackend.register(username, email, password);
                        }
                    });
                }
            });

            et_username = (EditText)view.findViewById(R.id.register_username);
            et_email = (EditText)view.findViewById(R.id.register_email);
            et_password = (EditText)view.findViewById(R.id.register_password);
            et_password_confirm = (EditText)view.findViewById(R.id.register_password_confirm);
            progressbar = (ProgressBar)view.findViewById(R.id.register_progress);
        }

        return _dialog;
    }
}
