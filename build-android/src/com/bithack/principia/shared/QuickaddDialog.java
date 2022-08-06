package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnShowListener;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

public class QuickaddDialog
{
    static Dialog _dialog;

    static View view;
    static SurfaceView sv_man;
    static AutoCompleteTextView et_name;

    public static ArrayAdapter<String> object_adapter;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.quickadd, null);
            //AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);
            _dialog = new Dialog(PrincipiaActivity.mSingleton, R.style.TinyDialog);
            _dialog.setContentView(view);

            sv_man = (SurfaceView)view.findViewById(R.id.quickadd_sv);
            et_name = (AutoCompleteTextView)view.findViewById(R.id.quickadd_text);
            et_name.setAdapter(object_adapter);
            et_name.setOnEditorActionListener(new OnEditorActionListener() {
                @Override
                public boolean onEditorAction(TextView v, int actionId,
                        KeyEvent event) {
                    if (event != null) {
                        return true;
                    }

                    String str = String.valueOf(((AutoCompleteTextView) v).getText());
                    if ("".equals(str)) {
                    } else {
                        PrincipiaBackend.createObject(str);
                    }

                    _dialog.dismiss();
                    return false;
                }
            });
            sv_man.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    _dialog.dismiss();
                }
            });
            et_name.setOnFocusChangeListener(new View.OnFocusChangeListener() {
                @Override
                public void onFocusChange(View v, boolean hasFocus) {
                    if (hasFocus) {
                        _dialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
                    }
                }
            });
            //bld.setView(view);

            //_dialog = bld.create();

            _dialog.setOnShowListener(new OnShowListener() {

                @Override
                public void onShow(DialogInterface dialog) {
                    PrincipiaActivity.on_show(dialog);

                    sv_man.requestFocus();
                    et_name.requestFocus();
                    InputMethodManager imm = (InputMethodManager)PrincipiaActivity.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                    //imm.showSoftInput(et_name, InputMethodManager.SHOW_FORCED);
                    imm.showSoftInput(et_name, 0);
                }
            });
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
    }
}
