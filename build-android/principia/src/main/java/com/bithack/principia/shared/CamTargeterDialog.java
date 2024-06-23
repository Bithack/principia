package com.bithack.principia.shared;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import android.os.Bundle;
import android.app.Dialog;
import android.content.DialogInterface;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.widget.EditText;

public class CamTargeterDialog implements OnSeekBarChangeListener, OnEditorActionListener, OnFocusChangeListener {
    static final float MULTIPLIER = 100.f;

    static Dialog _dialog;

    static View view;
    static Spinner s_follow_mode;
    static Spinner s_offset_mode;

    static CamTargeterDialog mSingleton;

    static Button btn_save;
    static Button btn_cancel;

    static SeekBar  sb_x_offset;
    static EditText et_x_offset;

    static SeekBar  sb_y_offset;
    static EditText et_y_offset;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            if (mSingleton == null) {
                mSingleton = new CamTargeterDialog();
            }

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.cam_targeter, null);

            s_follow_mode = (Spinner)view.findViewById(R.id.follow_mode);
            s_offset_mode = (Spinner)view.findViewById(R.id.offset_mode);

            sb_x_offset = (SeekBar) view.findViewById(R.id.level_x_offset_sb);
            et_x_offset = (EditText)view.findViewById(R.id.level_x_offset_et);

            sb_x_offset.setOnSeekBarChangeListener(mSingleton);
            et_x_offset.setOnEditorActionListener(mSingleton);
            et_x_offset.setOnFocusChangeListener(mSingleton);

            sb_y_offset = (SeekBar) view.findViewById(R.id.level_y_offset_sb);
            et_y_offset = (EditText)view.findViewById(R.id.level_y_offset_et);

            sb_y_offset.setOnSeekBarChangeListener(mSingleton);
            et_y_offset.setOnEditorActionListener(mSingleton);
            et_y_offset.setOnFocusChangeListener(mSingleton);

            btn_save = (Button)view.findViewById(R.id.cam_targeter_btn_save);
            btn_cancel = (Button)view.findViewById(R.id.cam_targeter_btn_cancel);

            btn_save.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    save();
                    _dialog.dismiss();
                }
            });

            btn_cancel.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    _dialog.dismiss();
                }
            });

            _dialog = new Dialog(PrincipiaActivity.mSingleton, android.R.style.Theme_NoTitleBar_Fullscreen) {
                @Override
                protected void onCreate(Bundle si)
                {
                    super.onCreate(si);
                    getWindow().setLayout(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
                }
            };

            _dialog.setContentView(view);
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        s_follow_mode.setSelection(PrincipiaBackend.getCamTargeterFollowMode());
        s_offset_mode.setSelection(PrincipiaBackend.getPropertyInt8(2));

        float x_offset = PrincipiaBackend.getPropertyFloat(3);
        float y_offset = PrincipiaBackend.getPropertyFloat(4);

        {
            float    v  =    x_offset;
            EditText et = et_x_offset;
            SeekBar  sb = sb_x_offset;

            et.setText(Float.toString(v));
            sb.setProgress((int)((v + (sb.getMax()/2/MULTIPLIER)) * MULTIPLIER));
        }

        {
            float    v  =    y_offset;
            EditText et = et_y_offset;
            SeekBar  sb = sb_y_offset;

            et.setText(Float.toString(v));
            sb.setProgress((int)((v + (sb.getMax()/2/MULTIPLIER)) * MULTIPLIER));
        }
    }

    public static void save()
    {
        PrincipiaBackend.setCamTargeterFollowMode(s_follow_mode.getSelectedItemPosition());
        PrincipiaBackend.setPropertyInt8(2, s_offset_mode.getSelectedItemPosition());
        float x_offset, y_offset;

        StringBuilder sb = new StringBuilder();

        try {
            x_offset = Float.parseFloat(et_x_offset.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid inupt given to X offset");
            x_offset = 0.f;
        }

        try {
            y_offset = Float.parseFloat(et_y_offset.getText().toString());
        } catch (NumberFormatException e) {
            sb.append("Invalid inupt given to Y offset");
            y_offset = 0.f;
        }

        PrincipiaBackend.setPropertyFloat(3, x_offset);
        PrincipiaBackend.setPropertyFloat(4, y_offset);
    }

    @Override
    public void onProgressChanged(SeekBar sb, int progress,
            boolean from_user) {
        float value = (sb.getProgress()-(sb.getMax()/2)) / MULTIPLIER;

        EditText et = null;

        if (sb == CamTargeterDialog.sb_x_offset) {
            et = CamTargeterDialog.et_x_offset;
        } else if (sb == CamTargeterDialog.sb_y_offset) {
            et = CamTargeterDialog.et_y_offset;
        }

        if (et != null) {
            et.setText(Float.toString(value));
        }
    }

    @Override
    public boolean onEditorAction(TextView tv, int actionId,
            KeyEvent event) {
        if (event != null) {
            return true;
        }

        this.cool(tv);

        return false;
    }

    public void cool(TextView v)
    {
        SeekBar sb = null;

        if (v == CamTargeterDialog.et_x_offset) {
            sb = CamTargeterDialog.sb_x_offset;
        } else if (v == CamTargeterDialog.et_y_offset) {
            sb = CamTargeterDialog.sb_y_offset;
        }

        if (sb != null) {
            float val;

            try {
                val = Float.parseFloat(v.getText().toString());
            } catch (NumberFormatException e) {
                val = (sb.getProgress()-(sb.getMax()/2)) / MULTIPLIER;
            }

            float max = (float)sb.getMax() / 100.f/2.f;
            float min = -max;

            if (val < min) {
                val = min;
            } else if (val > max) {
                val = max;
            }

            v.setText(Float.toString(val));
            sb.setProgress((int)((val + (sb.getMax()/2/MULTIPLIER)) * MULTIPLIER));
        }
    }

    @Override
    public void onFocusChange(View v, boolean hasFocus) {
        if (!hasFocus) {
            TextView tv = (TextView)v;

            this.cool(tv);
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) { }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) { }
}
