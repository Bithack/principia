package com.bithack.principia.shared;

import java.util.ArrayList;
import java.util.List;

import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import com.bithack.principia.R;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.util.Log;

public class PlayDialog
{
    /**
     * Source 0 = DB level
     * Source 1 = Test-playing level
     */

    static final String[] shared_items = new String[] {
        SDLActivity.mSingleton.getString(R.string.open_save),
        SDLActivity.mSingleton.getString(R.string.back_to_sandbox),
        SDLActivity.mSingleton.getString(R.string.back_to_main_menu),
        SDLActivity.mSingleton.getString(R.string.cancel)
    };

    static final String save_state = SDLActivity.mSingleton.getString(R.string.save_state);

    static final String[] community_items = new String[] {
        SDLActivity.mSingleton.getString(R.string.restart_level),
        SDLActivity.mSingleton.getString(R.string.back_to_community)
    };

    public static Dialog create_dialog()
    {
        int source = PrincipiaBackend.getLevelIdType();/*XXX*/

        if (source > 100) {
            source -= 100;
        }
        AlertDialog.Builder bld = new AlertDialog.Builder(SDLActivity.mSingleton);
        List<CharSequence> items = new ArrayList<CharSequence>();

        if (PrincipiaBackend.getLevelFlag(33)) {
            items.add(save_state);
        }

        items.add(SDLActivity.mSingleton.getString(R.string.open_save));

        if (PrincipiaBackend.isAdventure()) {
            items.add(SDLActivity.mSingleton.getString(R.string.selfdestruct));
        }

        if (source == 1) {
            items.add(SDLActivity.mSingleton.getString(R.string.restart_level));
            items.add(SDLActivity.mSingleton.getString(R.string.back_to_community));
        } else if (source == 0) {
            items.add(SDLActivity.mSingleton.getString(R.string.back_to_sandbox));
        }

        items.add(SDLActivity.mSingleton.getString(R.string.back_to_main_menu));
        items.add(SDLActivity.mSingleton.getString(R.string.cancel));

        final CharSequence[] real_items = items.toArray(new CharSequence[items.size()]);

        bld.setItems(real_items, new DialogInterface.OnClickListener(){
            public void onClick(DialogInterface dialog, int which) {
                String cool = real_items[which].toString();

                if (cool.equalsIgnoreCase("open save")) {
                    SDLActivity.mSingleton.runOnUiThread(new Runnable(){
                        public void run() {
                            try { SDLActivity.mSingleton.removeDialog(SDLActivity.DIALOG_OPEN); } catch(Exception e) {};
                            try { SDLActivity.mSingleton.removeDialog(SDLActivity.DIALOG_OPEN_STATE); } catch(Exception e) {};
                        }
                    });
                    SDLActivity.mSingleton.showDialog(SDLActivity.DIALOG_OPEN_STATE);
                } else if (cool.equalsIgnoreCase("back to sandbox")) {
                    PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_BACK, 0);
                } else if (cool.equalsIgnoreCase("back to community")) {
                    SDLActivity.wv.loadUrl(PrincipiaBackend.getCurrentCommunityUrl());
                    SDLActivity.wv_dialog.show();
                } else if (cool.equalsIgnoreCase("save state")) {
                    PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_SAVE_STATE, 0);
                } else if (cool.equalsIgnoreCase("back to main menu")) {
                    PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_GOTO_MAINMENU, 0);
                } else if (cool.equalsIgnoreCase("restart level")) {
                    PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_RESTART_LEVEL, 0);
                } else if (cool.equalsIgnoreCase("self-destruct")) {
                    PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_SELF_DESTRUCT, 0);
                } else {
                    Log.e("PRINCIPIA", "UNKNOWN THING: " + cool);
                }

                dialog.dismiss();
            }
        });

        return bld.create();
                  /*
        _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
            .setPositiveButton("Restart level", new OnClickListener(){
                public void onClick(DialogInterface dialog, int which)
                {
                }
            })
            .setNeutralButton("Back", new OnClickListener(){
                public void onClick(DialogInterface dialog, int which)
                {
                    PrincipiaBackend.addActionAsInt(SDLActivity.ACTION_BACK, 0);
                }
            })
            .setNegativeButton("Cancel", new OnClickListener(){
                public void onClick(DialogInterface dialog, int which)
                {
                }
            })
            .create();
            */
    }
}
