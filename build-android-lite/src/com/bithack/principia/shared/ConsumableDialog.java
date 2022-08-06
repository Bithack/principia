package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import com.bithack.principialite.PrincipiaActivity;
import com.bithack.principialite.R;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Spinner;

public class ConsumableDialog {
	static Dialog _dialog;
	
	static View view;
	static Spinner s_consumable;
	
	public static Dialog get_dialog()
	{
		if (_dialog == null) {
			AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

			view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.consumable, null);
			
			s_consumable = (Spinner)view.findViewById(R.id.s_consumable);
			String[] consumables = PrincipiaBackend.getConsumables().split(",");
			
			ArrayAdapter<String> spinnerArrayAdapter = new ArrayAdapter<String>(SDLActivity.mSingleton, android.R.layout.select_dialog_item, consumables);
			s_consumable.setAdapter(spinnerArrayAdapter);
			
			bld.setTitle("Consumable");
			bld.setView(view);
			bld.setPositiveButton("OK", new OnClickListener(){
				public void onClick(DialogInterface dialog, int which) {
					save();
					dialog.dismiss();
				}
				
			});
			bld.setNegativeButton("Cancel", new OnClickListener(){
				public void onClick(DialogInterface dialog, int which) {
					dialog.dismiss();
				}
				
			});
			
			ConsumableDialog._dialog = bld.create();
		}
		
		return _dialog;
	}

	public static void prepare(DialogInterface di)
	{
		s_consumable.setSelection(PrincipiaBackend.getConsumableType());
	}
	
	public static void save()
	{
		PrincipiaBackend.setConsumableType(s_consumable.getSelectedItemPosition());
	}
}
