package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import com.bithack.principialite.PrincipiaActivity;

import android.app.AlertDialog;
import android.app.Dialog;
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
	
	public OpenDialog()
	{
		SDLActivity.open_adapter.clear();
		
		AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);
		
        String level_list = PrincipiaBackend.getLevels(0);
        String[] levels = level_list.split("\n");

        level_names = new String[levels.length];
		
		bld.setTitle("Open Level");
		
		if (level_list.length() > 0) {
			for (int x=0; x<levels.length; x++) {
                String[] data = levels[x].split("\\,", 2);
                if (data.length != 2) {
                	level_names[x] = "";
                	continue;
                }
                
                int id = Integer.parseInt(data[0], 10);
                String name = data[1];
                SDLActivity.open_adapter.add(new Level(id, name));
                
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
						PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_OPEN, level.get_id());
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
