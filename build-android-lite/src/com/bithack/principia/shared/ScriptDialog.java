package com.bithack.principia.shared;

import java.util.ArrayList;
import java.util.List;

import com.bithack.principialite.PrincipiaActivity;
import com.bithack.principialite.R;
import com.bithack.principia.shared.ConfirmDialog.OnOptionSelectedListener;
import com.bithack.principia.shared.CustomLinearLayout.OnKeyboardStateChangedListener;
import com.bithack.principia.shared.MultiSpinner.MultiSpinnerListener;

import org.libsdl.app.PrincipiaBackend;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;

public class ScriptDialog {
	public static final int NUM_LIBRARIES = 2;

	static Dialog _dialog;
	
	static View view;
	static EditText code;
	static Button save;
	static Button cancel;
	static CustomLinearLayout ll_code;
	static LinearLayout ll_end;
	static MultiSpinner sp_libraries;
	static int current_keyboard_state = CustomLinearLayout.KEYBOARD_HIDDEN;
	static List<String> items;
	
	public static Dialog get_dialog()
	{
		if (_dialog == null) {
			items = new ArrayList<String>();
			items.add("string");
			items.add("table");

			view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.script, null);
			_dialog = new Dialog(PrincipiaActivity.getContext(), R.style.CodeDialog) {
				@Override
				protected void onCreate(Bundle saved_instance) {
					super.onCreate(saved_instance);
					getWindow().setLayout(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
				}
			};
			_dialog.setContentView(view);
			
			ll_code = (CustomLinearLayout)view.findViewById(R.id.ll_code);
			ll_code.set_listener(new OnKeyboardStateChangedListener() {
				@Override
				public void onKeyboardStateChanged(int new_state) {
					//if (current_keyboard_state == new_state) return;
					if (new_state == CustomLinearLayout.KEYBOARD_HIDDEN) {
						ll_end.setVisibility(View.VISIBLE);
					} else if (new_state == CustomLinearLayout.KEYBOARD_SHOWN) {
						ll_end.setVisibility(View.GONE);
					}
				}
			});
			ll_end = (LinearLayout)view.findViewById(R.id.ll_end);
			code = (EditText)view.findViewById(R.id.script_code);
			code.setHorizontallyScrolling(true);
			save = (Button)view.findViewById(R.id.script_save);
			cancel = (Button)view.findViewById(R.id.script_cancel);

			sp_libraries = (MultiSpinner)view.findViewById(R.id.script_libraries);
			sp_libraries.setItems(items, PrincipiaActivity.mSingleton.getString(R.string.libraries), new MultiSpinnerListener() {
				@Override public void onItemsSelected(boolean[] selected) { }
			});

			save.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					save();
					_dialog.dismiss();
				}
			});
			cancel.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					new ConfirmDialog()
					.set_listener(new OnOptionSelectedListener() {
						@Override
						public void onOptionSelected(int option) {
							if (option == ConfirmDialog.OPTION_YES) {
								_dialog.dismiss();
							}
						}
					})
					.run(PrincipiaActivity.mSingleton.getString(R.string.confirm_cancel_script_dialog));
				}
			});
		}
		
		return _dialog;
	}

	public static void prepare(DialogInterface di)
	{
		code.setText(PrincipiaBackend.getPropertyString(0));
		long included_libraries = PrincipiaBackend.getPropertyInt(1);
		for (int x=0; x<ScriptDialog.NUM_LIBRARIES; ++x) {
			if ((included_libraries & (1 << x)) == 0) {
				sp_libraries.selected[x] = false;
			} else {
				sp_libraries.selected[x] = true;
			}
		}
	}
	
	public static void save()
	{
		long included_libraries = 0;
		for (int x=0; x<ScriptDialog.NUM_LIBRARIES; ++x) {
			included_libraries |= (sp_libraries.selected[x] ? 1 : 0) * (1 << x);
		}

		PrincipiaBackend.setPropertyString(0, code.getText().toString());
		PrincipiaBackend.setPropertyInt(1, included_libraries);

		PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_HIGHLIGHT_SELECTED, 0);
		PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_ENTITY_MODIFIED, 0);
	}
}
