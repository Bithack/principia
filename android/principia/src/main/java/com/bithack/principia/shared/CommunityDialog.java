package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaBackend;
import com.bithack.principia.PrincipiaActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;

public class CommunityDialog
{
    static Dialog _dialog = null;

    public CommunityDialog()
    {
        _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setMessage("Do you want to return to the community site or to the main menu?")
                .setPositiveButton("Community", new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which){
                        PrincipiaActivity.wv.loadUrl(PrincipiaBackend.getCurrentCommunityUrl());
                        PrincipiaActivity.wv_dialog.show();
                    }}
                )
                .setNegativeButton("Main menu", new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which){
                        PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_GOTO_MAINMENU, 0);
                    }}
                )
                .create();
    }

    public Dialog get_dialog()
    {
        return _dialog;
    }
}
