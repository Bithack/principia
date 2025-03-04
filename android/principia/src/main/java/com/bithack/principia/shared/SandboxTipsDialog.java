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
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.TextView;

public class SandboxTipsDialog
{
    AlertDialog _dialog;

    final View view;
    final TextView tv_tip;
    final CheckBox cb_dont_show_again;

    public SandboxTipsDialog()
    {
        view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.sandbox_tips, null);
        _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setView(view)
                .setTitle("Tips & tricks")
                .setPositiveButton("OK", new OnClickListener(){public void onClick(DialogInterface dialog, int which) {}})
                .setNeutralButton("Next", null)
                .setNegativeButton("More tips & tricks", new OnClickListener(){public void onClick(DialogInterface dialog, int which){SDLActivity.open_url("https://principia-web.se/wiki/Getting_Started");}})
                .create();

        _dialog.setOnShowListener(new OnShowListener() {
            @Override
            public void onShow(DialogInterface dialog) {
                SDLActivity.on_show(dialog);

                Button b = _dialog.getButton(AlertDialog.BUTTON_NEUTRAL);

                b.setOnClickListener(new View.OnClickListener() {

                    @Override
                    public void onClick(View v) {
                        new_tip();
                    }
                });
            }
        });

        tv_tip = (TextView)view.findViewById(R.id.sandbox_tips_text);
        cb_dont_show_again = (CheckBox)view.findViewById(R.id.sandbox_tips_dont_show_again);

        cb_dont_show_again.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView,
                    boolean isChecked) {
                PrincipiaBackend.setSetting("hide_tips", isChecked);
            }

        });

        this.new_tip();
    }

    public void new_tip()
    {
        tv_tip.setText(PrincipiaBackend.getSandboxTip());
    }

    public Dialog get_dialog()
    {
        return this._dialog;
    }
}
