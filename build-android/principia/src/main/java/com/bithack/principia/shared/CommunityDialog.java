package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

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
                        SDLActivity.wv.loadUrl(PrincipiaBackend.getCurrentCommunityUrl());
                        SDLActivity.wv_dialog.show();
                    }}
                )
                .setNegativeButton("Main menu", new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which){
                        PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_GOTO_MAINMENU, 0);
                    }}
                )
                .create();
    }

    public Dialog get_dialog()
    {
        return _dialog;
    }
}
