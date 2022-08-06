package com.bithack.principia.shared;

import java.util.ArrayList;
import java.util.List;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;

import org.libsdl.app.PrincipiaBackend;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;

public class PromptDialog
{
    Dialog _dialog;

    public PromptDialog()
    {
        AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);
        bld.setMessage(PrincipiaBackend.getPromptPropertyString(3));
        bld.setCancelable(false);

        String b1 = PrincipiaBackend.getPromptPropertyString(0);
        String b2 = PrincipiaBackend.getPromptPropertyString(1);
        String b3 = PrincipiaBackend.getPromptPropertyString(2);

        if (PrincipiaBackend.getLevelVersion() >= 21) {
            if (b1.length() > 0) {
                bld.setPositiveButton(b1, new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which) {
                        PrincipiaBackend.setPromptResponse(PrincipiaActivity.PROMPT_RESPONSE_A);
                    }
                });
            }
            if (b2.length() > 0) {
                bld.setNeutralButton(b2, new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which) {
                        PrincipiaBackend.setPromptResponse(PrincipiaActivity.PROMPT_RESPONSE_B);
                    }
                });
            }
            if (b3.length() > 0) {
                bld.setNegativeButton(b3, new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which) {
                        PrincipiaBackend.setPromptResponse(PrincipiaActivity.PROMPT_RESPONSE_C);
                    }
                });
            }
        } else {
            List<String> button_texts = new ArrayList<String>();
            if (b1.length() > 0) button_texts.add(b1);
            if (b2.length() > 0) button_texts.add(b2);
            if (b3.length() > 0) button_texts.add(b3);

            int x = 0;
            for (String text : button_texts) {
                if (x == 0) {
                    bld.setPositiveButton(text, new OnClickListener(){
                        public void onClick(DialogInterface dialog, int which) {
                            PrincipiaBackend.setPromptResponse(PrincipiaActivity.PROMPT_RESPONSE_A);
                        }
                    });
                } else if (x == 1) {
                    bld.setNeutralButton(text, new OnClickListener(){
                        public void onClick(DialogInterface dialog, int which) {
                            PrincipiaBackend.setPromptResponse(PrincipiaActivity.PROMPT_RESPONSE_B);
                        }
                    });
                } else if (x == 2) {
                    bld.setNegativeButton(text, new OnClickListener(){
                        public void onClick(DialogInterface dialog, int which) {
                            PrincipiaBackend.setPromptResponse(PrincipiaActivity.PROMPT_RESPONSE_C);
                        }
                    });
                }
                x ++;
            }
        }

        this._dialog = bld.create();
        this._dialog.setCanceledOnTouchOutside(false);
        this._dialog.getWindow().setWindowAnimations(R.style.DialogNoAnimation);
    }

    public Dialog get_dialog()
    {
        return this._dialog;

    }
}
