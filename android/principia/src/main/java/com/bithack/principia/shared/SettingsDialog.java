package com.bithack.principia.shared;

import java.util.Locale;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;

import android.app.Dialog;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

public class SettingsDialog implements OnSeekBarChangeListener, OnClickListener {
    Dialog _dialog;
    CharSequence[] filelist = null;
    String[] absolute_filelist = null;
    final View view;

    final SeekBar sb_shadowresolution;
    final SeekBar sb_shadowsoftness;
    final SeekBar sb_uiscale;
    final SeekBar settings_cam_speed;
    final SeekBar settings_zoom_speed;
    final CheckBox cb_enableshadows;
    final RadioGroup rg_ao;
    final RadioGroup rg_fps;

    final CheckBox settings_smooth_cam;
    final CheckBox settings_smooth_zoom;
    final CheckBox settings_border_scroll_enabled;
    final CheckBox settings_display_object_ids;
    final CheckBox settings_display_grapher_value;
    final CheckBox settings_display_wireless_frequency;
    final CheckBox settings_hide_tips;
    final CheckBox settings_sandbox_back_dna;

    final SeekBar settings_volume;
    final TextView curr_volume;
    final CheckBox settings_muted;

    final SeekBar settings_border_scroll_speed;

    final Button btn_preset_low, btn_preset_medium, btn_preset_high;
    final Button btn_save, btn_cancel;

    final TextView text_curr_res;
    final TextView text_curr_softness;
    final TextView text_curr_uiscale;
    final TextView settings_cam_speed_tv;
    final TextView settings_zoom_speed_tv;
    final TextView settings_border_scroll_speed_tv;

    final private String shadow_resolutions[] = {
        "256x256",
        "512x256",
        "512x512",
        "1024x512",
        "1024x1024",
        "2048x2048"
    };

    public SettingsDialog()
    {
        view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.settings, null);
        this._dialog = new Dialog(PrincipiaActivity.mSingleton, android.R.style.Theme_NoTitleBar_Fullscreen){
            @Override
            protected void onCreate(Bundle saved_instance) {
                super.onCreate(saved_instance);
                getWindow().setLayout(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
            }
        };
        this._dialog.setContentView(view);

        sb_shadowresolution = (SeekBar)view.findViewById(R.id.seekbar_shadowresolution);
        sb_shadowsoftness = (SeekBar)view.findViewById(R.id.seekbar_shadowsoftness);
        sb_uiscale = (SeekBar)view.findViewById(R.id.settings_uiscale);
        rg_ao = (RadioGroup)view.findViewById(R.id.radiogroup_ao);
        rg_fps = (RadioGroup)view.findViewById(R.id.rg_fps);
        cb_enableshadows = (CheckBox)view.findViewById(R.id.checkbox_enableshadows);

        btn_preset_low = (Button)view.findViewById(R.id.btn_preset_low);
        btn_preset_medium = (Button)view.findViewById(R.id.btn_preset_medium);
        btn_preset_high = (Button)view.findViewById(R.id.btn_preset_high);

        text_curr_res = (TextView)view.findViewById(R.id.curr_res);
        text_curr_softness = (TextView)view.findViewById(R.id.curr_softness);
        text_curr_uiscale = (TextView)view.findViewById(R.id.tv_uiscale);

        settings_volume = (SeekBar)view.findViewById(R.id.seekbar_volume);
        curr_volume = (TextView)view.findViewById(R.id.curr_volume);
        settings_muted = (CheckBox)view.findViewById(R.id.checkbox_muted);

        settings_cam_speed = (SeekBar)view.findViewById(R.id.settings_cam_speed);
        settings_cam_speed_tv = (TextView)view.findViewById(R.id.settings_cam_speed_tv);

        settings_zoom_speed = (SeekBar)view.findViewById(R.id.settings_zoom_speed);
        settings_zoom_speed_tv = (TextView)view.findViewById(R.id.settings_zoom_speed_tv);

        settings_smooth_cam = (CheckBox)view.findViewById(R.id.settings_smooth_cam);
        settings_smooth_zoom = (CheckBox)view.findViewById(R.id.settings_smooth_zoom);

        settings_border_scroll_enabled = (CheckBox)view.findViewById(R.id.settings_border_scroll);
        settings_border_scroll_speed = (SeekBar)view.findViewById(R.id.settings_border_scroll_speed);
        settings_border_scroll_speed_tv = (TextView)view.findViewById(R.id.settings_border_scroll_speed_tv);

        settings_display_object_ids = (CheckBox)view.findViewById(R.id.settings_display_object_ids);
        settings_display_grapher_value = (CheckBox)view.findViewById(R.id.settings_display_grapher_value);
        settings_display_wireless_frequency = (CheckBox)view.findViewById(R.id.settings_display_wireless_frequency);
        settings_hide_tips = (CheckBox)view.findViewById(R.id.settings_hide_tips);
        settings_sandbox_back_dna = (CheckBox)view.findViewById(R.id.settings_sandbox_back_dna);

        btn_save   = (Button)view.findViewById(R.id.settings_save);
        btn_cancel = (Button)view.findViewById(R.id.settings_cancel);

        settings_cam_speed.setOnSeekBarChangeListener(this);
        settings_zoom_speed.setOnSeekBarChangeListener(this);
        settings_border_scroll_speed.setOnSeekBarChangeListener(this);
        sb_shadowresolution.setOnSeekBarChangeListener(this);
        sb_shadowsoftness.setOnSeekBarChangeListener(this);
        sb_uiscale.setOnSeekBarChangeListener(this);
        settings_volume.setOnSeekBarChangeListener(this);

        btn_preset_low.setOnClickListener(this);
        btn_preset_medium.setOnClickListener(this);
        btn_preset_high.setOnClickListener(this);

        btn_save.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                save();
                _dialog.dismiss();
            }
        });

        btn_cancel.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                _dialog.dismiss();
            }
        });
    }

    public void save()
    {
        boolean enable_shadows = cb_enableshadows.isChecked();
        boolean enable_ao = true;
        int shadow_quality;
        int shadow_map_resx;
        int shadow_map_resy;
        int ao_map_res = 256;
        float uiscale_base = (float)sb_uiscale.getProgress() / 10.f;
        float uiscale = uiscale_base + 0.5f;
        float cam_speed_base = (float)settings_cam_speed.getProgress() / 10.f;
        float cam_speed = cam_speed_base + 0.3f;
        float zoom_speed_base = (float)settings_zoom_speed.getProgress() / 10.f;
        float zoom_speed = zoom_speed_base + 0.3f;
        boolean smooth_cam = this.settings_smooth_cam.isChecked();
        boolean smooth_zoom = this.settings_smooth_zoom.isChecked();
        boolean display_object_ids = this.settings_display_object_ids.isChecked();
        boolean display_grapher_value = this.settings_display_grapher_value.isChecked();
        boolean display_wireless_frequency = this.settings_display_wireless_frequency.isChecked();
        boolean hide_tips = this.settings_hide_tips.isChecked();
        boolean sandbox_back_dna = this.settings_sandbox_back_dna.isChecked();
        float volume = this.settings_volume.getProgress() / 100.f;
        boolean muted = this.settings_muted.isChecked();

        boolean border_scroll_enabled = this.settings_border_scroll_enabled.isChecked();
        float border_scroll_speed_base = (float)this.settings_border_scroll_speed.getProgress() / 10.f;
        float border_scroll_speed = border_scroll_speed_base + 0.5f;

        switch (rg_ao.getCheckedRadioButtonId()) {
            case R.id.rg_values_off: enable_ao = false; break;
            case R.id.rg_values_low: ao_map_res = 128; break;
            case R.id.rg_values_medium: ao_map_res = 256; break;
            case R.id.rg_values_high: ao_map_res = 512; break;
        }

        switch (sb_shadowresolution.getProgress()) {
            case 0: shadow_map_resx = 256; shadow_map_resy = 256; break;
            case 1: shadow_map_resx = 512; shadow_map_resy = 256; break;
            case 2: shadow_map_resx = 512; shadow_map_resy = 512; break;
            case 3: shadow_map_resx = 1024; shadow_map_resy = 512; break;
            default: case 4: shadow_map_resx = 1024; shadow_map_resy = 1024; break;
            case 5: shadow_map_resx = 2048; shadow_map_resy = 2048; break;
        }

        int display_fps = 0;

        switch (rg_fps.getCheckedRadioButtonId()) {
            case R.id.rg_fps_off:       display_fps = 0; break;
            case R.id.rg_fps_on:        display_fps = 1; break;
            case R.id.rg_fps_graph:     display_fps = 3; break;
        }

        shadow_quality = sb_shadowsoftness.getProgress();

        PrincipiaBackend.setSettings(enable_shadows, enable_ao,
                                     shadow_quality, shadow_map_resx, shadow_map_resy,
                                     ao_map_res, uiscale,
                                     cam_speed, zoom_speed,
                                     smooth_cam, smooth_zoom,
                                     border_scroll_enabled, border_scroll_speed,
                                     display_object_ids, display_grapher_value, display_wireless_frequency,
                                     volume, muted,
                                     hide_tips, sandbox_back_dna,
                                     display_fps);
    }

    public void load()
    {
        Log.v("Principia", "Loading settings.");
        Settings s = PrincipiaBackend.getSettings();
        int sr = -1;
        String sr_str = String.format(Locale.US, "%dx%d", s.shadow_map_resx, s.shadow_map_resy);
        for (int x=0; x<shadow_resolutions.length; ++x) {
            if (sr_str.equals(shadow_resolutions[x])) {
                sr = x;
                break;
            }
        }

        if (sr == -1) {
            sb_shadowresolution.setProgress(0);
        } else {
            sb_shadowresolution.setProgress(sr);
        }

        text_curr_res.setText(sr_str);

        cb_enableshadows.setChecked(s.enable_shadows);
        sb_shadowsoftness.setProgress(s.shadow_quality);

        if (s.enable_ao) {
            if (s.ao_map_res == 512) {
                rg_ao.check(R.id.rg_values_high);
            } else if (s.ao_map_res == 256) {
                rg_ao.check(R.id.rg_values_medium);
            } else {
                rg_ao.check(R.id.rg_values_low);
            }
        } else {
            rg_ao.check(R.id.rg_values_off);
        }

        if (s.display_fps == 1) {
            rg_fps.check(R.id.rg_fps_on);
        } else if (s.display_fps == 2 || s.display_fps == 3) {
            rg_fps.check(R.id.rg_fps_graph);
        } else {
            rg_fps.check(R.id.rg_fps_off);
        }

        int uiscale_step = (int)Math.round((((double)s.uiscale - 0.5) * 10.0));

        if (uiscale_step == 0) {
            text_curr_uiscale.setText("0.5");
        }
        sb_uiscale.setProgress(uiscale_step);

        int cam_speed_step = (int)Math.round((((double)s.cam_speed - 0.3) * 10.0));
        if (cam_speed_step == 0) this.settings_cam_speed_tv.setText("0.5");
        this.settings_cam_speed.setProgress(cam_speed_step);

        int zoom_speed_step = (int)Math.round((((double)s.zoom_speed - 0.3) * 10.0));
        if (zoom_speed_step == 0) this.settings_zoom_speed_tv.setText("0.5");
        this.settings_zoom_speed.setProgress(zoom_speed_step);

        this.settings_smooth_cam.setChecked(s.smooth_cam);
        this.settings_smooth_zoom.setChecked(s.smooth_zoom);

        this.settings_border_scroll_enabled.setChecked(s.border_scroll_enabled);
        int border_scroll_speed_step = (int)Math.round((((double)s.border_scroll_speed - 0.5) * 10.0));
        if (border_scroll_speed_step == 0) this.settings_cam_speed_tv.setText("0.5");
        this.settings_border_scroll_speed.setProgress(border_scroll_speed_step);
        this.settings_display_object_ids.setChecked(s.display_object_ids);
        this.settings_display_grapher_value.setChecked(s.display_grapher_value);
        this.settings_display_wireless_frequency.setChecked(s.display_wireless_frequency);
        this.settings_hide_tips.setChecked(s.hide_tips);
        this.settings_sandbox_back_dna.setChecked(s.sandbox_back_dna);

        int vol = (int)Math.round(s.volume * 100.f);
        this.settings_volume.setProgress(vol);
        this.curr_volume.setText(String.format(Locale.US, "%d%%", vol));
        this.settings_muted.setChecked(s.muted);
    }

    public Dialog get_dialog()
    {
        return this._dialog;
    }

    @Override
    public void onProgressChanged(SeekBar sb, int progress,
            boolean from_user) {
        if (sb == this.settings_cam_speed) {
            this.settings_cam_speed_tv.setText(String.format(Locale.US, "%.2f", ((float)progress / 10.f)+.3f));
        } else if (sb == this.sb_shadowresolution) {
            this.text_curr_res.setText(this.shadow_resolutions[progress]);
        } else if (sb == this.sb_shadowsoftness) {
            if (progress == 0) {
                text_curr_softness.setText(R.string.sharp);
            } else {
                text_curr_softness.setText(R.string.soft);
            }
        } else if (sb == this.sb_uiscale) {
            double base = (double)progress / 10.f;
            double uiscale = base + 0.5f;
            text_curr_uiscale.setText(String.format(Locale.US, "%.1f", uiscale));
        } else if (sb == this.settings_zoom_speed) {
            this.settings_zoom_speed_tv.setText(String.format(Locale.US, "%.2f", ((float)progress / 10.f)+.3f));
        } else if (sb == this.settings_border_scroll_speed) {
            this.settings_border_scroll_speed_tv.setText(String.format(Locale.US, "%.2f", ((float)progress / 10.f)+.5f));
        } else if (sb == this.settings_volume) {
            this.curr_volume.setText(String.format(Locale.US, "%d%%", progress));
        }
    }

    @Override
    public void onClick(View v) {
        if (v == this.btn_preset_low) {
            sb_shadowresolution.setProgress(0);
            sb_shadowsoftness.setProgress(0);
            rg_ao.check(R.id.rg_values_off);
            cb_enableshadows.setChecked(false);
        } else if (v == this.btn_preset_medium) {
            sb_shadowresolution.setProgress(3);
            sb_shadowsoftness.setProgress(0);
            rg_ao.check(R.id.rg_values_low);
            cb_enableshadows.setChecked(true);
        } else if (v == this.btn_preset_high) {
            sb_shadowresolution.setProgress(5);
            sb_shadowsoftness.setProgress(1);
            rg_ao.check(R.id.rg_values_medium);
            cb_enableshadows.setChecked(true);
        }
    }


    @Override
    public void onStartTrackingTouch(SeekBar seekBar) { }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) { }
}
