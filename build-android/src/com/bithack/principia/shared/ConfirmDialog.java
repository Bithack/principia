package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import org.libsdl.app.SDLActivity;
import org.libsdl.app.PrincipiaBackend;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;

public class ConfirmDialog
{
    public static final int OPTION_NO = 0;
    public static final int OPTION_YES = 1;
    public static final int OPTION_3 = 2;

    private OnOptionSelectedListener mListener = null;

    public interface OnOptionSelectedListener {
        public void onOptionSelected(int option);
    }

    public ConfirmDialog set_listener(OnOptionSelectedListener l)
    {
        this.mListener = l;

        return this;
    }

    public ConfirmDialog run(String message)
    {
        return this.run(message, "Yes", "No");
    }

    public ConfirmDialog run(String message, String button1, String button2)
    {
        return this.run(message, button1, button2, "", false);
    }

    public ConfirmDialog run(String message, String button1, String button2, String button3, final boolean dna_sandbox_back)
    {
        final CheckBox cb;

        AlertDialog dialog = new AlertDialog.Builder(SDLActivity.getContext()).create();
        if (dna_sandbox_back) {
            View view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.confirm_sandbox, null);
            dialog.setView(view);
            cb = (CheckBox)view.findViewById(R.id.cb_confirm_sandbox_quit);
            cb.setChecked(PrincipiaBackend.getSettingBool("dna_sandbox_back"));
        } else {
            dialog.setMessage(message);
            cb = null;
        }
        dialog.setCancelable(true);
        dialog.setOnShowListener(SDLActivity.mSingleton);
        dialog.setOnDismissListener(SDLActivity.mSingleton);
        dialog.setButton(DialogInterface.BUTTON_POSITIVE, button1, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int buttonId) {
                if (mListener != null) {
                    mListener.onOptionSelected(OPTION_YES);
                }

                if (dna_sandbox_back && cb != null) {
                    PrincipiaBackend.setSetting("dna_sandbox_back", cb.isChecked());
                }
            }
        });
        dialog.setButton(DialogInterface.BUTTON_NEGATIVE, button2, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int buttonId) {
                if (mListener != null) {
                    mListener.onOptionSelected(OPTION_NO);
                }

                if (dna_sandbox_back && cb != null) {
                    PrincipiaBackend.setSetting("dna_sandbox_back", cb.isChecked());
                }
            }
        });
        if (button3.length() > 0) {
            dialog.setButton(DialogInterface.BUTTON_NEUTRAL, button3, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int buttonId) {
                    if (mListener != null) {
                        mListener.onOptionSelected(OPTION_3);
                    }

                    if (dna_sandbox_back && cb != null) {
                        PrincipiaBackend.setSetting("dna_sandbox_back", cb.isChecked());
                    }
                }
            });
        }

        dialog.setIcon(android.R.drawable.ic_dialog_alert);
        dialog.show();

        return this;
    }
}
