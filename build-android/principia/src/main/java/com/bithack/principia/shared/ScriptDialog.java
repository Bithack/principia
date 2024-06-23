package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import com.bithack.principia.shared.ConfirmDialog.OnOptionSelectedListener;
import com.bithack.principia.shared.CustomLinearLayout.OnKeyboardStateChangedListener;

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
    static Dialog _dialog;

    static View view;
    static EditText code;
    static Button save;
    static Button cancel;
    static CustomLinearLayout ll_code;
    static LinearLayout ll_end;
    static int current_keyboard_state = CustomLinearLayout.KEYBOARD_HIDDEN;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.script, null);
            _dialog = new Dialog(PrincipiaActivity.getContext(), android.R.style.Theme_NoTitleBar_Fullscreen) {
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
            int ime_options = code.getImeOptions();
            save = (Button)view.findViewById(R.id.script_save);
            ime_options |= 0x80000000; // force ascii
            code.setImeOptions(ime_options);
            cancel = (Button)view.findViewById(R.id.script_cancel);

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
    }

    public static void save()
    {
        long included_libraries = 0;

        for (int x=0; x<3; ++x) {
            included_libraries |= 1 * (1 << x+2);
        }

        PrincipiaBackend.setPropertyString(0, code.getText().toString());
        PrincipiaBackend.setPropertyInt(1, included_libraries);

        PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_HIGHLIGHT_SELECTED, 0);
        PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_ENTITY_MODIFIED, 0);
        PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_AUTOSAVE, 0);
    }
}
