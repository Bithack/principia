package com.bithack.principia.shared;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import android.app.Dialog;
import android.os.Bundle;
import android.content.DialogInterface;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;

public class SynthesizerDialog {
    private static Dialog _dialog;
    private static Button btn_save, btn_cancel;

    private static View view;
    private static LinearLayout synth_pulse_width_ll;
    private static NumberPicker np_base_freq;
    private static NumberPicker np_peak_freq;
    private static Spinner sp_waveform;

    public static SeekBar synth_pulse_width;
    public static SeekBar synth_bitcrushing;
    public static SeekBar synth_volume_vibrato_hz;
    public static SeekBar synth_volume_vibrato_extent;
    public static SeekBar synth_freq_vibrato_hz;
    public static SeekBar synth_freq_vibrato_extent;
    public static TextView synth_pulse_width_tv;
    public static TextView synth_bitcrushing_tv;
    public static TextView synth_volume_vibrato_hz_tv;
    public static TextView synth_volume_vibrato_extent_tv;
    public static TextView synth_freq_vibrato_hz_tv;
    public static TextView synth_freq_vibrato_extent_tv;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.synthesizer, null);

            _dialog = new Dialog(PrincipiaActivity.mSingleton, android.R.style.Theme_NoTitleBar_Fullscreen){
                @Override
                protected void onCreate(Bundle saved_instance) {
                    super.onCreate(saved_instance);
                    getWindow().setLayout(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
                }
            };
            _dialog.setContentView(view);

            btn_save   = (Button)view.findViewById(R.id.synth_save);
            btn_cancel = (Button)view.findViewById(R.id.synth_cancel);

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

            np_base_freq = (NumberPicker)view.findViewById(R.id.synth_base);
            np_base_freq.setRange(0, 440*8);
            np_base_freq.setLongStep(100);
            np_base_freq.setStep(10);
            np_peak_freq = (NumberPicker)view.findViewById(R.id.synth_peak);
            np_peak_freq.setRange(0, 440*8);
            np_peak_freq.setLongStep(100);
            np_peak_freq.setStep(10);

            synth_pulse_width_ll = (LinearLayout)view.findViewById(R.id.synth_pulse_width_ll);
            synth_pulse_width = (SeekBar)view.findViewById(R.id.synth_pulse_width);
            synth_pulse_width.setMax(100);
            synth_bitcrushing = (SeekBar)view.findViewById(R.id.synth_bitcrushing);
            synth_bitcrushing.setMax(64);
            synth_volume_vibrato_hz = (SeekBar)view.findViewById(R.id.synth_volume_vibrato_hz);
            synth_volume_vibrato_hz.setMax(32);
            synth_volume_vibrato_extent = (SeekBar)view.findViewById(R.id.synth_volume_vibrato_extent);
            synth_volume_vibrato_extent.setMax(100);
            synth_freq_vibrato_hz = (SeekBar)view.findViewById(R.id.synth_freq_vibrato_hz);
            synth_freq_vibrato_hz.setMax(32);
            synth_freq_vibrato_extent = (SeekBar)view.findViewById(R.id.synth_freq_vibrato_extent);
            synth_freq_vibrato_extent.setMax(100);

            synth_pulse_width_tv = (TextView)view.findViewById(R.id.synth_pulse_width_tv);
            synth_bitcrushing_tv = (TextView)view.findViewById(R.id.synth_bitcrushing_tv);
            synth_volume_vibrato_hz_tv = (TextView)view.findViewById(R.id.synth_volume_vibrato_hz_tv);
            synth_volume_vibrato_extent_tv = (TextView)view.findViewById(R.id.synth_volume_vibrato_extent_tv);
            synth_freq_vibrato_hz_tv = (TextView)view.findViewById(R.id.synth_freq_vibrato_hz_tv);
            synth_freq_vibrato_extent_tv = (TextView)view.findViewById(R.id.synth_freq_vibrato_extent_tv);

            synth_pulse_width.setOnSeekBarChangeListener(SDLActivity.mSingleton);
            synth_bitcrushing.setOnSeekBarChangeListener(SDLActivity.mSingleton);
            synth_volume_vibrato_hz.setOnSeekBarChangeListener(SDLActivity.mSingleton);
            synth_volume_vibrato_extent.setOnSeekBarChangeListener(SDLActivity.mSingleton);
            synth_freq_vibrato_hz.setOnSeekBarChangeListener(SDLActivity.mSingleton);
            synth_freq_vibrato_extent.setOnSeekBarChangeListener(SDLActivity.mSingleton);

            sp_waveform = (Spinner)view.findViewById(R.id.synth_waveform);
            sp_waveform.setOnItemSelectedListener(new OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> arg0, View v,
                        int position, long id) {
                    if (position == 2) { /* pulse */
                        synth_pulse_width_ll.setVisibility(View.VISIBLE);
                    } else {
                        synth_pulse_width_ll.setVisibility(View.GONE);
                    }
                }

                @Override
                public void onNothingSelected(AdapterView<?> arg0) {
                }
            });

            String[] waveforms = PrincipiaBackend.getSynthWaveforms().split(",");
            ArrayAdapter<String> spinner_aa = new ArrayAdapter<String>(SDLActivity.getContext(), android.R.layout.select_dialog_item, waveforms);
            sp_waveform.setAdapter(spinner_aa);
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        int g_id = PrincipiaBackend.getSelectionGid();

        if (g_id == 175) {
            Log.v("Principia", "Prepare");
            np_base_freq.setValue((int)PrincipiaBackend.getPropertyFloat(0));
            np_peak_freq.setValue((int)PrincipiaBackend.getPropertyFloat(1));
            sp_waveform.setSelection((int)PrincipiaBackend.getPropertyInt(2));
            synth_bitcrushing.setProgress((int)(PrincipiaBackend.getPropertyFloat(3)));
            synth_volume_vibrato_hz.setProgress((int)(PrincipiaBackend.getPropertyFloat(4)));
            synth_freq_vibrato_hz.setProgress((int)(PrincipiaBackend.getPropertyFloat(5)));
            synth_volume_vibrato_extent.setProgress((int)(PrincipiaBackend.getPropertyFloat(6) * 100.f));
            synth_freq_vibrato_extent.setProgress((int)(PrincipiaBackend.getPropertyFloat(7) * 100.f));
            synth_pulse_width.setProgress((int)(PrincipiaBackend.getPropertyFloat(8) * 100.f));
        }
    }

    public static void save()
    {
        int g_id = PrincipiaBackend.getSelectionGid();

        if (g_id == 175) {
            float low = np_base_freq.getValue();
            float high = np_peak_freq.getValue();
            int waveform = sp_waveform.getSelectedItemPosition();
            float pulse_width = (float)synth_pulse_width.getProgress() / 100.f;
            float bitcrushing = (float)synth_bitcrushing.getProgress();
            float vol_vib_hz = (float)synth_volume_vibrato_hz.getProgress();
            float freq_vib_hz = (float)synth_freq_vibrato_hz.getProgress();
            float vol_vib_ex = (float)synth_volume_vibrato_extent.getProgress() / 100.f;
            float freq_vib_ex = (float)synth_freq_vibrato_extent.getProgress() / 100.f;

            if (high < low) high = low;

            PrincipiaBackend.setPropertyFloat(0, low);
            PrincipiaBackend.setPropertyFloat(1, high);
            PrincipiaBackend.setPropertyInt(2, waveform);
            PrincipiaBackend.setPropertyFloat(3, bitcrushing);
            PrincipiaBackend.setPropertyFloat(4, vol_vib_hz);
            PrincipiaBackend.setPropertyFloat(5, freq_vib_hz);
            PrincipiaBackend.setPropertyFloat(6, vol_vib_ex);
            PrincipiaBackend.setPropertyFloat(7, freq_vib_ex);
            PrincipiaBackend.setPropertyFloat(8, pulse_width);
        }
    }
}
