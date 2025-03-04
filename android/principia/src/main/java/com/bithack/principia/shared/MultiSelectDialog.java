package com.bithack.principia.shared;

import android.app.Dialog;
import android.content.DialogInterface;
import android.graphics.Color;
import android.graphics.PorterDuff.Mode;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;

import org.libsdl.app.PrincipiaBackend;

import java.util.ArrayList;

public class MultiSelectDialog implements OnSeekBarChangeListener, OnCheckedChangeListener {
    static Dialog _dialog;

    static View view;
    static ToggleButton tab_buttons[];
    static ScrollView tab_views[];

    static Button btn_apply;
    static Button btn_close;

    static final int JOINT_STRENGTH         = 0;
    static final int PLASTIC_COLOR          = 1;
    static final int PLASTIC_DENSITY        = 2;
    static final int CONNECTION_RENDER_TYPE = 3;
    static final int MISC                   = 4;

    static MultiSelectDialog mSingleton;

    public static class Tab {
        int type;
        public String title;
        ToggleButton button;
        ScrollView view;
        int view_id;

        public Tab(int view_id, String title)
        {
            this.view_id = view_id;
            this.title = title;
        }
    }

    static ArrayList<Tab> tabs = new ArrayList<Tab>();

    /* JOINT STRENGTH */
    static SeekBar  sb_joint_strength;
    static TextView tv_joint_strength;

    /* PLASTIC DENSITY */
    static SeekBar  sb_plastic_density;
    static TextView tv_plastic_density;

    /* PLASTIC COLOR */
    static Button  color_btn;
    static SeekBar color_red;
    static SeekBar color_green;
    static SeekBar color_blue;

    /* CONNECTION RENDER TYPE */
    static RadioGroup render_type;

    /* MISC */

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            if (mSingleton == null) {
                mSingleton = new MultiSelectDialog();
            }

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.multiselect, null);

            _dialog = new Dialog(PrincipiaActivity.mSingleton, android.R.style.Theme_NoTitleBar_Fullscreen){
                @Override
                protected void onCreate(Bundle saved_instance) {
                    super.onCreate(saved_instance);
                    getWindow().setLayout(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
                }
            };
            _dialog.setContentView(view);

            tabs.add(new Tab(R.id.sv_joint_strength, PrincipiaActivity.mSingleton.getString(R.string.joint_strength)));
            tabs.add(new Tab(R.id.sv_plastic_color, PrincipiaActivity.mSingleton.getString(R.string.plastic_color)));
            tabs.add(new Tab(R.id.sv_plastic_density, PrincipiaActivity.mSingleton.getString(R.string.plastic_density)));
            tabs.add(new Tab(R.id.sv_connection_render_type, PrincipiaActivity.mSingleton.getString(R.string.connection_render_type)));
            tabs.add(new Tab(R.id.sv_misc, PrincipiaActivity.mSingleton.getString(R.string.misc)));

            LinearLayout ll_tabs = (LinearLayout)view.findViewById(R.id.ll_tabs);

            tab_views = new ScrollView[tabs.size()];
            tab_buttons = new ToggleButton[tabs.size()];

            for (int x=0; x<tabs.size(); ++x) {
                ToggleButton tb = new ToggleButton(PrincipiaActivity.mSingleton.getContext());

                Tab tab = tabs.get(x);

                tab_views[x] = (ScrollView)view.findViewById(tab.view_id);

                tab.type = x;

                tb.setText(tab.title);
                tb.setTextOn(tab.title);
                tb.setTextOff(tab.title);

                tb.setTag(x);
                tb.setOnCheckedChangeListener(mSingleton);
                tb.setVisibility(LinearLayout.GONE);

                tb.setLayoutParams(new LinearLayout.LayoutParams(
                            LayoutParams.WRAP_CONTENT,
                            LayoutParams.WRAP_CONTENT,
                            1.f));

                tab.button = tb;
                tab.view = tab_views[x];

                ll_tabs.addView(tb);
            }

            btn_apply = view.findViewById(R.id.btn_apply);
            btn_close = view.findViewById(R.id.btn_close);

            btn_apply.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    apply_current_tab();
                }
            });

            btn_close.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    _dialog.dismiss();
                }
            });

            OnSeekBarChangeListener sb_color_listener = new OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress,
                        boolean fromUser) {
                    if (fromUser) {
                        int red = color_red.getProgress();
                        int green = color_green.getProgress();
                        int blue = color_blue.getProgress();
                        int c = Color.rgb(red, green, blue);
                        Drawable d = color_btn.getBackground();
                        d.setColorFilter(c, Mode.LIGHTEN);
                        color_btn.setTextColor(Color.rgb(255-red, 255-green, 255-blue));
                    }
                }
                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                }
            };

            /* JOINT STRENGTH */
            sb_joint_strength = (SeekBar) view.findViewById(R.id.sb_joint_strength);
            tv_joint_strength = (TextView)view.findViewById(R.id.tv_joint_strength);
            sb_joint_strength.setOnSeekBarChangeListener(mSingleton);

            /* PLASTIC COLOR */
            color_btn = (Button)view.findViewById(R.id.btn_color);

            color_red = (SeekBar)view.findViewById(R.id.color_red);
            color_green = (SeekBar)view.findViewById(R.id.color_green);
            color_blue = (SeekBar)view.findViewById(R.id.color_blue);

            color_red.setOnSeekBarChangeListener(sb_color_listener);
            color_green.setOnSeekBarChangeListener(sb_color_listener);
            color_blue.setOnSeekBarChangeListener(sb_color_listener);

            /* PLASTIC DENSITY */
            sb_plastic_density = (SeekBar) view.findViewById(R.id.sb_plastic_density);
            tv_plastic_density = (TextView)view.findViewById(R.id.tv_plastic_density);
            sb_plastic_density.setOnSeekBarChangeListener(mSingleton);

            /* CONNECTION RENDER TYPE */
            render_type = (RadioGroup)view.findViewById(R.id.rg_render_type);

            /* MISC */
            {
                Button btn = (Button)view.findViewById(R.id.btn_unlock_all);

                btn.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_MULTI_UNLOCK_ALL, 0);
                        PrincipiaActivity.message("Unlocked all entities in multiselection.", 0);
                    }
                });
            }
            {
                Button btn = (Button)view.findViewById(R.id.btn_disconnect_all);

                btn.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_MULTI_DISCONNECT_ALL, 0);
                        PrincipiaActivity.message("Disconnected all entities in multiselection.", 0);
                    }
                });
            }

            tabs.get(JOINT_STRENGTH).button.setChecked(true);
            tabs.get(JOINT_STRENGTH).view.setVisibility(LinearLayout.VISIBLE);
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        for (int x=0; x<tabs.size(); ++x) {
            Tab tab = tabs.get(x);

            //tab.button.setVisibility(LinearLayout.GONE);
            tab.button.setVisibility(LinearLayout.VISIBLE);

            Log.v("PRINCIPIA", "asd " + tab.title + " is now gone.");
        }

        tabs.get(JOINT_STRENGTH).button.setVisibility(LinearLayout.VISIBLE);
        tabs.get(CONNECTION_RENDER_TYPE).button.setVisibility(LinearLayout.VISIBLE);
        tabs.get(MISC).button.setVisibility(LinearLayout.VISIBLE);

        // connection render type RESET
        render_type.check(R.id.rb_normal);
    }

    public static void apply_current_tab()
    {
        String message = "Nothing to apply here";

        for (int x=0; x<tabs.size(); ++x) {
            Tab tab = tabs.get(x);

            if (tab.view.getVisibility() == LinearLayout.VISIBLE) {
                switch (x) {
                    case JOINT_STRENGTH:
                        PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_MULTI_JOINT_STRENGTH, sb_joint_strength.getProgress()/10);
                        message = "Modified the strength of all joints.";
                        break;

                    case PLASTIC_COLOR:
                        PrincipiaBackend.addActionAsVec4(PrincipiaActivity.ACTION_MULTI_PLASTIC_COLOR,
                                color_red.getProgress() / 255.f,
                                color_green.getProgress() / 255.f,
                                color_blue.getProgress() / 255.f,
                                1.f
                                );
                        message = "Changed the color of all plastic entities.";
                        break;

                    case PLASTIC_DENSITY:
                        PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_MULTI_PLASTIC_DENSITY, sb_plastic_density.getProgress()/10);
                        message = "Changed the density of all plastic entities.";
                        break;

                    case CONNECTION_RENDER_TYPE:
                        {
                            int t; // CONN_RENDER_DEFAULT
                            switch (render_type.getCheckedRadioButtonId()) {
                                case R.id.rb_small:
                                    t = 1; // CONN_RENDER_SMALL
                                    break;

                                case R.id.rb_hidden:
                                    t = 3; // CONN_RENDER_HIDE
                                    break;

                                case R.id.rb_normal:
                                default:
                                    t = 0; // CONN_RENDER_DEFAULT
                                    break;
                            }

                        PrincipiaBackend.addActionAsInt(PrincipiaActivity.ACTION_MULTI_CHANGE_CONNECTION_RENDER_TYPE, t);
                        }
                        message = "Changed the render type of all connections.";
                        break;

                    case MISC:
                        break;

                    default:
                        break;
                }

                break;
            }
        }

        PrincipiaActivity.message(message, 0);
    }

    @Override
    public void onProgressChanged(SeekBar sb, int progress,
            boolean from_user) {
        float value = sb.getProgress() / 1000.f;

        TextView tv = null;

        if (sb == sb_plastic_density) {
            tv = tv_plastic_density;
        } else if (sb == sb_joint_strength) {
            if (value >= 1.f) {
                tv_joint_strength.setText("Unbreakable");
            } else {
                tv = tv_joint_strength;
            }
        }

        if (tv != null) {
            tv.setText(Float.toString(value));
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) { }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) { }

    @Override
    public void onCheckedChanged(CompoundButton buttonView,
            boolean isChecked) {
        if (isChecked) {
            for (int x=0; x<tabs.size(); ++x) {
                if (x == ((Integer)buttonView.getTag())) {
                    tab_views[x].setVisibility(ScrollView.VISIBLE);
                    tabs.get(x).button.setClickable(false);
                } else {
                    tab_views[x].setVisibility(ScrollView.GONE);
                    tabs.get(x).button.setChecked(false);
                    tabs.get(x).button.setClickable(true);
                }
            }
        }
    }
}
