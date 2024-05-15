package com.bithack.principia.shared;

import java.util.ArrayList;
import java.util.List;
import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import org.libsdl.app.PrincipiaBackend;
import org.libsdl.app.SDLActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

public class DigitalDisplayDialog {
    static Dialog _dialog;

    static View view;
    static CheckBox cb_wrap_around;
    static LinearLayout ll_dd;
    static LinearLayout ll_wrap;
    static NumberPicker np_initial_position;
    static EditText et_symbols;
    static TextView tv_dd_symbol_id;
    static TextView tv_dd_num_symbols;
    static Button btn_dd_next;
    static Button btn_dd_previous;
    static Button btn_dd_insert;
    static Button btn_dd_append;
    static Button btn_dd_remove;
    static int cur_symbol;
    static int num_symbols;

    static final int MIN_INITIAL_POS = 1;
    static final int NUM_CHARS_IN_SYMBOL = 35;
    static final String EMPTY_SYMBOL = "00000000000000000000000000000000000";

    static CheckBox[] cb_symbols;

    static List<String> symbols;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            AlertDialog.Builder bld = new AlertDialog.Builder(PrincipiaActivity.mSingleton);
            cur_symbol = 0;
            symbols = new ArrayList<String>();

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.digital_display, null);

            cb_wrap_around = (CheckBox)view.findViewById(R.id.cb_wrap_around);
            ll_dd = (LinearLayout)view.findViewById(R.id.ll_dd);
            ll_wrap = (LinearLayout)view.findViewById(R.id.display_ll_wrap);

            np_initial_position = new com.bithack.principia.shared.NumberPicker(SDLActivity.getContext());
            np_initial_position.setRange(MIN_INITIAL_POS, 40);

            ll_dd.addView((View)np_initial_position);

            tv_dd_symbol_id = (TextView)view.findViewById(R.id.tv_dd_symbol_id);
            tv_dd_num_symbols = (TextView)view.findViewById(R.id.tv_dd_num_symbols);

            cb_symbols = new CheckBox[35];
            cb_symbols[0] = (CheckBox)view.findViewById(R.id.cb_dd_1);
            cb_symbols[1] = (CheckBox)view.findViewById(R.id.cb_dd_2);
            cb_symbols[2] = (CheckBox)view.findViewById(R.id.cb_dd_3);
            cb_symbols[3] = (CheckBox)view.findViewById(R.id.cb_dd_4);
            cb_symbols[4] = (CheckBox)view.findViewById(R.id.cb_dd_5);
            cb_symbols[5] = (CheckBox)view.findViewById(R.id.cb_dd_6);
            cb_symbols[6] = (CheckBox)view.findViewById(R.id.cb_dd_7);
            cb_symbols[7] = (CheckBox)view.findViewById(R.id.cb_dd_8);
            cb_symbols[8] = (CheckBox)view.findViewById(R.id.cb_dd_9);
            cb_symbols[9] = (CheckBox)view.findViewById(R.id.cb_dd_10);
            cb_symbols[10] = (CheckBox)view.findViewById(R.id.cb_dd_11);
            cb_symbols[11] = (CheckBox)view.findViewById(R.id.cb_dd_12);
            cb_symbols[12] = (CheckBox)view.findViewById(R.id.cb_dd_13);
            cb_symbols[13] = (CheckBox)view.findViewById(R.id.cb_dd_14);
            cb_symbols[14] = (CheckBox)view.findViewById(R.id.cb_dd_15);
            cb_symbols[15] = (CheckBox)view.findViewById(R.id.cb_dd_16);
            cb_symbols[16] = (CheckBox)view.findViewById(R.id.cb_dd_17);
            cb_symbols[17] = (CheckBox)view.findViewById(R.id.cb_dd_18);
            cb_symbols[18] = (CheckBox)view.findViewById(R.id.cb_dd_19);
            cb_symbols[19] = (CheckBox)view.findViewById(R.id.cb_dd_20);
            cb_symbols[20] = (CheckBox)view.findViewById(R.id.cb_dd_21);
            cb_symbols[21] = (CheckBox)view.findViewById(R.id.cb_dd_22);
            cb_symbols[22] = (CheckBox)view.findViewById(R.id.cb_dd_23);
            cb_symbols[23] = (CheckBox)view.findViewById(R.id.cb_dd_24);
            cb_symbols[24] = (CheckBox)view.findViewById(R.id.cb_dd_25);
            cb_symbols[25] = (CheckBox)view.findViewById(R.id.cb_dd_26);
            cb_symbols[26] = (CheckBox)view.findViewById(R.id.cb_dd_27);
            cb_symbols[27] = (CheckBox)view.findViewById(R.id.cb_dd_28);
            cb_symbols[28] = (CheckBox)view.findViewById(R.id.cb_dd_29);
            cb_symbols[29] = (CheckBox)view.findViewById(R.id.cb_dd_30);
            cb_symbols[30] = (CheckBox)view.findViewById(R.id.cb_dd_31);
            cb_symbols[31] = (CheckBox)view.findViewById(R.id.cb_dd_32);
            cb_symbols[32] = (CheckBox)view.findViewById(R.id.cb_dd_33);
            cb_symbols[33] = (CheckBox)view.findViewById(R.id.cb_dd_34);
            cb_symbols[34] = (CheckBox)view.findViewById(R.id.cb_dd_35);

            for (int x=0; x<35; ++x) {
                final int y = x;
                cb_symbols[x].setOnCheckedChangeListener(new OnCheckedChangeListener() {

                    @Override
                    public void onCheckedChanged(CompoundButton buttonView,
                            boolean isChecked) {
                        String cur_symbol_str = null;
                        if (symbols.size() < cur_symbol) {
                            Log.e("Principia", "cur_symbol out of bounds");
                            cur_symbol_str = EMPTY_SYMBOL;
                        } else {
                            cur_symbol_str = symbols.get(cur_symbol);
                            if (cur_symbol_str.length() != NUM_CHARS_IN_SYMBOL) {
                                Log.e("Principia", "Invalid number of chars in symbol.");
                                cur_symbol_str = EMPTY_SYMBOL;
                            }
                        }

                        StringBuilder new_str = new StringBuilder(cur_symbol_str);
                        try {
                            new_str.setCharAt(y, (isChecked?'1':'0'));
                            symbols.set(cur_symbol, new_str.toString());
                        } catch (StringIndexOutOfBoundsException e) {
                            Log.e("Principia", "An unknown error occured: " + e.getMessage());
                        }
                    }
                });
            }

            btn_dd_next = (Button)view.findViewById(R.id.btn_dd_next);
            btn_dd_previous = (Button)view.findViewById(R.id.btn_dd_previous);

            btn_dd_insert = (Button)view.findViewById(R.id.btn_dd_insert);
            btn_dd_append = (Button)view.findViewById(R.id.btn_dd_append);
            btn_dd_remove = (Button)view.findViewById(R.id.btn_dd_remove);

            btn_dd_next.setOnTouchListener(new OnTouchListener() {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    if (event.getAction() == MotionEvent.ACTION_DOWN) {
                        cur_symbol ++;
                        if (cur_symbol >= num_symbols) cur_symbol = num_symbols-1;

                        reload_symbol();

                        return true;
                    }
                    return false;
                }
               });

            btn_dd_previous.setOnTouchListener(new OnTouchListener() {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    if (event.getAction() == MotionEvent.ACTION_DOWN) {
                        cur_symbol --;
                        if (cur_symbol < 0) cur_symbol = 0;

                        reload_symbol();

                        return true;
                    }
                    return false;
                }
               });

            btn_dd_insert.setOnTouchListener(new OnTouchListener() {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    if (event.getAction() == MotionEvent.ACTION_DOWN) {
                        if (num_symbols == 40) {
                            SDLActivity.message("Maximum number of symbols reached.", 0);
                            return false;
                        }

                        symbols.add(cur_symbol, "00000000000000000000000000000000000");

                        reload_symbol();

                        return true;
                    }
                    return false;
                }
               });

            btn_dd_append.setOnTouchListener(new OnTouchListener() {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    if (event.getAction() == MotionEvent.ACTION_DOWN) {
                        if (num_symbols == 40) {
                            SDLActivity.message("Maximum number of symbols reached.", 0);
                            return false;
                        }

                        cur_symbol ++;
                        symbols.add(cur_symbol, "00000000000000000000000000000000000");

                        reload_symbol();

                        return true;
                    }
                    return false;
                }
               });

            btn_dd_remove.setOnTouchListener(new OnTouchListener() {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    if (event.getAction() == MotionEvent.ACTION_DOWN) {
                        if (symbols.size() == 1) {
                            /* We must have at least one symbol */
                            return true;
                        }

                        symbols.remove(cur_symbol);

                        if (cur_symbol >= symbols.size()) cur_symbol = symbols.size() - 1;

                        reload_symbol();

                        return true;
                    }
                    return false;
                }
               });


            bld.setTitle("Digital Display");
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

    public static void reload_symbol()
    {
        String symbol = symbols.get(cur_symbol);
        int len = symbol.length();

        for (int x=0; x<35; ++x) {
            if (x >= len) {
                cb_symbols[x].setChecked(false);
            } else {
                cb_symbols[x].setChecked(symbol.charAt(x) == '1'?true:false);
            }
        }

        Log.v("Principia", "Symbol fetched:    "+symbol);

        refresh_counters();
    }

    public static void refresh_counters()
    {
        num_symbols = symbols.size();

        tv_dd_symbol_id.setText(Integer.toString(cur_symbol+1));
        tv_dd_num_symbols.setText(Integer.toString(num_symbols));

        np_initial_position.setRange(MIN_INITIAL_POS, num_symbols);
    }

    public static void prepare(DialogInterface di)
    {
        symbols.clear();

        int wrap_around = PrincipiaBackend.getPropertyInt8(0);
        int initial_position = PrincipiaBackend.getPropertyInt8(1) + 1;
        String symbols_str = PrincipiaBackend.getPropertyString(2);
        String[] symbols_arr = symbols_str.split("\n\n");

        for (int i=0; i<symbols_arr.length; ++i) {
            StringBuilder sb = new StringBuilder();
            for (int x=0; x<symbols_arr[i].length(); ++x) {
                if (symbols_arr[i].charAt(x) != '\n') {
                    sb.append(symbols_arr[i].charAt(x));
                }
            }

            if (sb.length() != NUM_CHARS_IN_SYMBOL) {
                sb.delete(0, sb.length());
                Log.e("Principia", "had to manufacture a new string.");
                for (int x=0; x<NUM_CHARS_IN_SYMBOL;++x) {
                    sb.append('0');
                }
            }
            symbols.add(sb.toString());
            Log.v("Principia", "Got a cool symbol: "+ sb);
        }

        cur_symbol = initial_position - 1;

        reload_symbol();

        cb_wrap_around.setChecked(wrap_around == 1?true:false);
        np_initial_position.setValue(initial_position);

        int g_id = PrincipiaBackend.getSelectionGid();

        if (g_id == 193) { // active display
            ll_wrap.setVisibility(View.GONE);
        } else {
            ll_wrap.setVisibility(View.VISIBLE);
        }
    }

    public static void save()
    {
        StringBuilder full_symbols = new StringBuilder();
        for (int i=0; i<symbols.size(); ++i) {
            if (i != 0) {
                full_symbols.append("\n\n");
            }
            full_symbols.append(symbols.get(i));
        }
        int ip = np_initial_position.getValue() - 1;

        if (ip >= symbols.size()) ip = symbols.size() - 1;

        /* TODO: On save, it should make sure np_initial_position is less than the symbol count */
        PrincipiaBackend.setDigitalDisplayStuff(cb_wrap_around.isChecked(), ip, full_symbols.toString());
    }
}
