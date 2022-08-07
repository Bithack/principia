package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import com.bithack.principia.PrincipiaActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnShowListener;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;

public class ImportDialog
{
    private final AlertDialog _dialog;
    private final String[] level_names;

    public static ListView lv;
    public static ArrayAdapter<Level> list_adapter = new ArrayAdapter<Level>(SDLActivity.mSingleton,
            android.R.layout.select_dialog_item);

    public ImportDialog(final boolean is_multiemitter)
    {
        ImportDialog.list_adapter.clear();

        AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

        String level_list = PrincipiaBackend.getLevels(SDLActivity.LEVEL_PARTIAL);
        String[] levels = level_list.split("\n");

        level_names = new String[levels.length];

        bld.setTitle("Open object");

        if (level_list.length() > 0) {
            for (int x=0; x<levels.length; x++) {
                String[] data = levels[x].split("\\,", 4);
                if (data.length != 4) {
                    level_names[x] = "";
                    continue;
                }
                try {
                    int id = Integer.parseInt(data[0], 10);
                    int save_id = Integer.parseInt(data[1], 10);
                    int level_type = Integer.parseInt(data[2], 10);
                    String name = data[3];

                    Level l = new Level(id, name);
                    l.set_save_id(save_id);
                    l.set_level_type(level_type);

                    ImportDialog.list_adapter.add(l);

                    level_names[x] = name;
                } catch (NumberFormatException e) { }

                bld.setItems(new String[0], null);
            }
            bld.setItems(level_names, new OnClickListener(){
                public void onClick(DialogInterface dialog, int which)
                {
                }
            });
        } else {
            bld.setMessage("No saved objects found.");
        }

        bld.setNegativeButton("Close", new OnClickListener(){
            public void onClick(DialogInterface dialog, int which)
            {
            }
        });

        this._dialog = bld.create();
        this._dialog.setOnShowListener(new OnShowListener() {
            @Override
            public void onShow(DialogInterface dialog) {
                SDLActivity.on_show(dialog);
                ListView lv = _dialog.getListView();
                ImportDialog.lv = lv;
                if (lv != null) {
                    lv.setOnItemClickListener(new OnItemClickListener() {
                        @Override
                        public void onItemClick(AdapterView<?> parent, View view,
                                int position, long id) {
                            Level level = (Level)parent.getAdapter().getItem(position);

                            Log.v("Principia", String.format("%s: %d", level.get_name(), level.get_id()));

                            if (is_multiemitter) {
                                PrincipiaBackend.setMultiemitterObject(level.get_id());
                            } else {
                                PrincipiaBackend.setImportObject(level.get_id());
                            }

                            _dialog.dismiss();
                        }
                    });
                    lv.setAdapter(ImportDialog.list_adapter);
                    SDLActivity.mSingleton.registerForContextMenu(lv);
                } else {
                    Log.v("Principia", "listview = null");
                }
            }
        });
    }

    public Dialog get_dialog()
    {
        return this._dialog;
    }
}
