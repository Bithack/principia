package com.bithack.principia.shared;

import java.util.ArrayList;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Spinner;
import android.widget.ArrayAdapter;

public class KeyListenerDialog {
    static Dialog _dialog;

    static View view;
    static Spinner s_keys;

    static TwoWayHashmap<Integer, Integer> keys = new TwoWayHashmap<Integer, Integer>();

    public static Dialog get_dialog()
    {
        if (_dialog == null) {

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.key_listener, null);

            ArrayList<String> array_spinner = new ArrayList<String>();

            s_keys = (Spinner)view.findViewById(R.id.kl_keys);

            String keys_str[] = PrincipiaBackend.getKeys().split(",.,");

            int n = 0;

            for (String key_data : keys_str) {
                String data[] = key_data.split("=_=");

                if (data.length == 2) {
                    array_spinner.add(data[1]);
                    keys.add(n, Integer.parseInt(data[0]));
                    ++ n;
                }
            }

            ArrayAdapter<String> adapter = new ArrayAdapter<String>(PrincipiaActivity.mSingleton,
                    android.R.layout.simple_spinner_item, array_spinner);
            s_keys.setAdapter(adapter);

            _dialog = new AlertDialog.Builder(PrincipiaActivity.mSingleton)
                .setTitle(PrincipiaActivity.mSingleton.getString(R.string.key_listener))
                .setView(view)
                .setPositiveButton(PrincipiaActivity.mSingleton.getString(R.string.ok), new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which) {
                        save();
                        dialog.dismiss();
                    }
                })
                .setNegativeButton(PrincipiaActivity.mSingleton.getString(R.string.cancel), new OnClickListener(){
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                })
                .create();
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        s_keys.setSelection(keys.getBackward((int)PrincipiaBackend.getPropertyInt(0)));
    }

    public static void save()
    {
        PrincipiaBackend.setPropertyInt(0, keys.getForward(s_keys.getSelectedItemPosition()));

        PrincipiaBackend.fixed();
    }
}
