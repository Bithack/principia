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
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

public class PromptSettingsDialog
{
    static AlertDialog _dialog;

    static View view;
    static EditText et_b1;
    static EditText et_b2;
    static EditText et_b3;
    static EditText et_message;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.prompt_settings, null);

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                    .setView(view)
                    .setTitle("Prompt settings")
                    .setPositiveButton("Save", null)
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
                            String b1_text = et_b1.getText().toString().trim();
                            String b2_text = et_b2.getText().toString().trim();
                            String b3_text = et_b3.getText().toString().trim();
                            String message = et_message.getText().toString().trim();
                            int b1_len = b1_text.length();
                            int b2_len = b2_text.length();
                            int b3_len = b3_text.length();
                            int message_len = message.length();

                            if (message_len <= 0) {
                                SDLActivity.message("You must enter a message for the prompt.", 0);
                                return;
                            }

                            if (b1_len <= 0 && b2_len <= 0 && b3_len <= 0) {
                                SDLActivity.message("You must use at least one button.", 0);
                                return;
                            }

                            PrincipiaBackend.setPromptPropertyString(0, b1_text);
                            PrincipiaBackend.setPromptPropertyString(1, b2_text);
                            PrincipiaBackend.setPromptPropertyString(2, b3_text);
                            PrincipiaBackend.setPromptPropertyString(3, message);

                            PrincipiaBackend.refreshPrompt();

                            Log.v("Principia", String.format("[%s]: %s/%s/%s", message, b1_text, b2_text, b3_text));

                            _dialog.dismiss();
                        }
                    });
                }
            });

            et_b1 = (EditText)view.findViewById(R.id.prompt_button1_text);
            et_b2 = (EditText)view.findViewById(R.id.prompt_button2_text);
            et_b3 = (EditText)view.findViewById(R.id.prompt_button3_text);
            et_message = (EditText)view.findViewById(R.id.prompt_message);
        }

        return _dialog;
    }

    public static void prepare(Dialog d)
    {
        et_b1.setText(PrincipiaBackend.getPromptPropertyString(0));
        et_b2.setText(PrincipiaBackend.getPromptPropertyString(1));
        et_b3.setText(PrincipiaBackend.getPromptPropertyString(2));
        et_message.setText(PrincipiaBackend.getPromptPropertyString(3));
    }
}
