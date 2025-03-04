package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;

import org.libsdl.app.PrincipiaBackend;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

public class ColorChooserDialog {
    static AlertDialog _dialog;
    static EditText et_red;
    static EditText et_green;
    static EditText et_blue;
    static EditText et_alpha;
    static SeekBar sb_red;
    static SeekBar sb_green;
    static SeekBar sb_blue;
    static SeekBar sb_alpha;
    static ImageView sv_color;
    static Canvas color_canvas;
    static LinearLayout ll_alpha;
    private static View view;

    public static void update_canvas()
    {
        int r,g,b;
        r = sb_red.getProgress();
        g = sb_green.getProgress();
        b = sb_blue.getProgress();
        color_canvas.drawRGB(r, g, b);
        sv_color.draw(color_canvas);
        sv_color.refreshDrawableState();
        sv_color.postInvalidate();
    }

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            Bitmap bitmap = Bitmap.createBitmap(80, 50, Bitmap.Config.ARGB_8888);
            color_canvas = new android.graphics.Canvas(bitmap);
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.color_chooser, null);

            et_red = (EditText)view.findViewById(R.id.et_red);
            et_green = (EditText)view.findViewById(R.id.et_green);
            et_blue = (EditText)view.findViewById(R.id.et_blue);
            et_alpha = (EditText)view.findViewById(R.id.et_alpha);
            sb_red = (SeekBar)view.findViewById(R.id.sb_red);
            sb_green = (SeekBar)view.findViewById(R.id.sb_green);
            sb_blue = (SeekBar)view.findViewById(R.id.sb_blue);
            sb_alpha = (SeekBar)view.findViewById(R.id.sb_alpha);
            sv_color = (ImageView)view.findViewById(R.id.iv_color);
            ll_alpha = (LinearLayout)view.findViewById(R.id.ll_alpha);
            sv_color.setImageBitmap(bitmap);

            sb_red.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        et_red.setText(String.valueOf(progress));
                        update_canvas();
                    }
                }
                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {}
            });

            sb_green.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        et_green.setText(String.valueOf(progress));
                        update_canvas();
                    }
                }
                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {}
            });
            sb_blue.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        et_blue.setText(String.valueOf(progress));
                        update_canvas();
                    }
                }
                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {}
            });
            sb_alpha.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        et_alpha.setText(String.valueOf(progress));
                        update_canvas();
                    }
                }
                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                }
            });

            et_red.setOnFocusChangeListener(new OnFocusChangeListener() {
                @Override
                public void onFocusChange(View v, boolean hasFocus) {
                    int progress = 0;
                    try {
                        progress = Integer.parseInt(et_red.getText().toString());
                    } catch (NumberFormatException e) {
                        progress = 0;
                    }
                    sb_red.setProgress(progress);
                }
            });
            et_green.setOnFocusChangeListener(new OnFocusChangeListener() {
                @Override
                public void onFocusChange(View v, boolean hasFocus) {
                    int progress = 0;
                    try {
                        progress = Integer.parseInt(et_green.getText().toString());
                    } catch (NumberFormatException e) {
                        progress = 0;
                    }
                    sb_green.setProgress(progress);
                }
            });
            et_blue.setOnFocusChangeListener(new OnFocusChangeListener() {
                @Override
                public void onFocusChange(View v, boolean hasFocus) {
                    int progress = 0;
                    try {
                        progress = Integer.parseInt(et_blue.getText().toString());
                    } catch (NumberFormatException e) {
                        progress = 0;
                    }
                    sb_blue.setProgress(progress);
                }
            });
            et_alpha.setOnFocusChangeListener(new OnFocusChangeListener() {
                @Override
                public void onFocusChange(View v, boolean hasFocus) {
                    int progress = 0;
                    try {
                        progress = Integer.parseInt(et_alpha.getText().toString());
                    } catch (NumberFormatException e) {
                        progress = 0;
                    }
                    sb_alpha.setProgress(progress);
                }
            });

            //bld.setTitle("color picker");
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

            _dialog = bld.create();
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di, boolean has_alpha)
    {
        int color = PrincipiaBackend.getEntityColor();
        int r = Color.red(color);
        int g = Color.green(color);
        int b = Color.blue(color);
        int a = Color.alpha(color);

        et_red.setText(String.valueOf(r));
        et_green.setText(String.valueOf(g));
        et_blue.setText(String.valueOf(b));
        et_alpha.setText(String.valueOf(a));
        sb_red.setProgress(r);
        sb_green.setProgress(g);
        sb_blue.setProgress(b);
        sb_alpha.setProgress(a);

        if (has_alpha) {
            ll_alpha.setVisibility(View.VISIBLE);
        } else {
            ll_alpha.setVisibility(View.GONE);
            sb_alpha.setProgress(255);
        }

        update_canvas();
    }

    public static void save()
    {
        int color = 0;
        int r,g,b,a;

        try {
            r = Integer.parseInt(et_red.getText().toString());
        } catch (NumberFormatException e) {
            r = 0;
        }

        try {
            g = Integer.parseInt(et_green.getText().toString());
        } catch (NumberFormatException e) {
            g = 0;
        }

        try {
            b = Integer.parseInt(et_blue.getText().toString());
        } catch (NumberFormatException e) {
            b = 0;
        }

        try {
            a = Integer.parseInt(et_alpha.getText().toString());
        } catch (NumberFormatException e) {
            a = 0;
        }

        color = Color.argb(a, r, g, b);

        Log.v("Principia", String.format("%d - %d/%d,%d,%d,", color, a, r, g, b));
        PrincipiaBackend.setEntityColor(color);
    }
}
