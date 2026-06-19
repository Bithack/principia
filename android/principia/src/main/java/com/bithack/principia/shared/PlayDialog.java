package com.bithack.principia.shared;

import java.util.ArrayList;
import java.util.List;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.PrincipiaBackend;
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
        PrincipiaActivity.mSingleton.getString(R.string.open_save),
        PrincipiaActivity.mSingleton.getString(R.string.back_to_sandbox),
        PrincipiaActivity.mSingleton.getString(R.string.back_to_main_menu),
        PrincipiaActivity.mSingleton.getString(R.string.cancel)
    };

    static final String save_state = PrincipiaActivity.mSingleton.getString(R.string.save_state);

    static final String[] community_items = new String[] {
        PrincipiaActivity.mSingleton.getString(R.string.restart_level),
        PrincipiaActivity.mSingleton.getString(R.string.back_to_community)
    };

    public static Dialog create_dialog()
    {
        int source = PrincipiaBackend.getLevelIdType();/*XXX*/

        if (source > 100) {
            source -= 100;
        }
        AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);
        List<CharSequence> items = new ArrayList<CharSequence>();

        if (PrincipiaBackend.getLevelFlag(33)) {
            items.add(save_state);
        }

        items.add(PrincipiaActivity.mSingleton.getString(R.string.open_save));

        if (PrincipiaBackend.isAdventure()) {
            items.add(PrincipiaActivity.mSingleton.getString(R.string.selfdestruct));
        }

        if (source == 1) {
            items.add(PrincipiaActivity.mSingleton.getString(R.string.restart_level));
            items.add(PrincipiaActivity.mSingleton.getString(R.string.back_to_community));
        } else if (source == 0) {
            items.add(PrincipiaActivity.mSingleton.getString(R.string.back_to_sandbox));
        }

        items.add(PrincipiaActivity.mSingleton.getString(R.string.back_to_main_menu));
        items.add(PrincipiaActivity.mSingleton.getString(R.string.cancel));

        final CharSequence[] real_items = items.toArray(new CharSequence[items.size()]);

        bld.setItems(real_items, new DialogInterface.OnClickListener(){
            public void onClick(DialogInterface dialog, int which) {
                String cool = real_items[which].toString();

                if (cool.equalsIgnoreCase("open save")) {
                    PrincipiaActivity.mSingleton.runOnUiThread(new Runnable(){
                        public void run() {
                            try { PrincipiaActivity.mSingleton.removeDialog(PrincipiaActivity.DIALOG_OPEN); } catch(Exception e) {};
                            try { PrincipiaActivity.mSingleton.removeDialog(PrincipiaActivity.DIALOG_OPEN_STATE); } catch(Exception e) {};
                        }
                    });
                    PrincipiaActivity.mSingleton.showDialog(PrincipiaActivity.DIALOG_OPEN_STATE);
                } else if (cool.equalsIgnoreCase("back to sandbox")) {
                    PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_BACK, 0);
                } else if (cool.equalsIgnoreCase("back to community")) {
                    PrincipiaActivity.wv.loadUrl(PrincipiaBackend.getCurrentCommunityUrl());
                    PrincipiaActivity.wv_dialog.show();
                } else if (cool.equalsIgnoreCase("save state")) {
                    PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_SAVE_STATE, 0);
                } else if (cool.equalsIgnoreCase("back to main menu")) {
                    PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_GOTO_MAINMENU, 0);
                } else if (cool.equalsIgnoreCase("restart level")) {
                    PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_RESTART_LEVEL, 0);
                } else if (cool.equalsIgnoreCase("self-destruct")) {
                    PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_SELF_DESTRUCT, 0);
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
                    PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_BACK, 0);
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
