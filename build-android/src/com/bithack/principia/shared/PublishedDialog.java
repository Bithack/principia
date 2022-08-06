package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import org.libsdl.app.PrincipiaBackend;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;

public class PublishedDialog
{
    private final AlertDialog _dialog;

    public PublishedDialog()
    {
        _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setView(LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.published, null))
                .setTitle(R.string.published_successfully)
                .setPositiveButton(R.string.go_to_level_page, new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which){
                        PrincipiaActivity.open_url(PrincipiaBackend.getLevelPage());
                    }
                })
                .setNegativeButton(R.string.cancel, new OnClickListener(){public void onClick(DialogInterface dialog, int which){}})
                .create();
    }

    public Dialog get_dialog()
    {
        return this._dialog;
    }
}
