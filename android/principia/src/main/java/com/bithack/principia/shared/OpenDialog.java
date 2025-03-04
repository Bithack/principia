package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import com.bithack.principia.PrincipiaActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.util.Log;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnShowListener;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ListView;

public class OpenDialog
{
    final AlertDialog _dialog;
    final String[] level_names;

    public static ListView lv;

    public OpenDialog(final boolean is_state)
    {
        SDLActivity.open_adapter.clear();

        AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

        String level_list = PrincipiaBackend.getLevels(is_state ? SDLActivity.LEVEL_LOCAL_STATE : SDLActivity.LEVEL_LOCAL);
        Log.v("Principia", "Level list: " + level_list);
        String[] levels = level_list.split("\n");

        level_names = new String[levels.length];

        if (is_state) {
            bld.setTitle("Open save");
        } else {
            bld.setTitle("Open level");
        }

        if (level_list.length() > 0) {
            for (int x=0; x<levels.length; x++) {
                String[] data = levels[x].split("\\,", 4);
                if (data.length != 4) {
                    level_names[x] = "";
                    continue;
                }

                int id = Integer.parseInt(data[0], 10);
                int save_id = Integer.parseInt(data[1], 10);
                int level_type = Integer.parseInt(data[2], 10);
                String name = data[3];

                Level l = new Level(id, name);
                l.set_save_id(save_id);
                l.set_level_type(level_type);

                Log.v("Principia", "Adding "+name);

                SDLActivity.open_adapter.add(l);

                level_names[x] = name;
            }

            bld.setItems(new String[0], null);
        } else {
            bld.setMessage("No saved levels found.");
        }

        bld.setNegativeButton("Close", new OnClickListener(){
            public void onClick(DialogInterface dialog, int which)
            {
            }
        });

        this._dialog = bld.create();
        this._dialog.setOnShowListener(new OnShowListener()
        {
            @Override
        public void onShow(DialogInterface dialog)
        {
            SDLActivity.on_show(dialog);
            ListView lv = _dialog.getListView();
            OpenDialog.lv = lv;
            if (lv != null) {
                lv.setOnItemClickListener(new OnItemClickListener() {
                    @Override
                    public void onItemClick(AdapterView<?> parent, View view,
                            int position, long id) {
                        Level level = (Level)parent.getAdapter().getItem(position);
                        if (is_state) {
                            PrincipiaBackend.openState(level.get_level_type(), level.get_id(), level.get_save_id(), SDLActivity.is_cool); /* XXX */
                        } else {
                            PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_OPEN, level.get_id());
                        }
                        _dialog.dismiss();
                    }

                });
                lv.setAdapter(SDLActivity.open_adapter);
                SDLActivity.mSingleton.registerForContextMenu(lv);
            }
        }
        });
    }

    public Dialog get_dialog()
    {
        return this._dialog;
    }
}
