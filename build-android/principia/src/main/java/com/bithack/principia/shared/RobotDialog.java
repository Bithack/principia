package com.bithack.principia.shared;

import java.util.ArrayList;

import com.bithack.principia.shared.MultiSpinner.MultiSpinnerListener;

import org.libsdl.app.PrincipiaBackend;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import android.os.Bundle;
import android.app.Dialog;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.LinearLayout;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.CheckBox;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.ToggleButton;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.util.Log;

public class RobotDialog {
    public static final int NUM_TABS = 2;
    static Dialog _dialog;

    static ToggleButton tab_buttons[];
    static ScrollView tab_views[];

    static Button btn_save;
    static Button btn_cancel;

    static View view;
    static RadioGroup rg_default_state;
    static RadioGroup rg_faction;
    static CheckBox cb_roam;
    static RadioGroup rg_initial_dir;

    static ArrayList<String> he_array = new ArrayList<String>();
    static Spinner head_equipment;
    static LinearLayout ll_he;

    static ArrayList<String> h_array = new ArrayList<String>();
    static Spinner head;
    static LinearLayout ll_h;

    static ArrayList<String> be_array = new ArrayList<String>();
    static Spinner back_equipment;
    static LinearLayout ll_be;

    static ArrayList<String> fe_array = new ArrayList<String>();
    static Spinner front_equipment;
    static LinearLayout ll_fe;

    static ArrayList<String> f_array = new ArrayList<String>();
    static Spinner feet;
    static LinearLayout ll_f;

    static ArrayList<String> bs_array = new ArrayList<String>();
    static Spinner bolt_set;
    static LinearLayout ll_bs;

    static ArrayList<String> w_array = new ArrayList<String>();
    static ArrayList<Integer> w_array_ids = new ArrayList<Integer>();
    static MultiSpinner weapons;
    static LinearLayout ll_w;

    static ArrayList<String> t_array = new ArrayList<String>();
    static ArrayList<Integer> t_array_ids = new ArrayList<Integer>();
    static MultiSpinner tools;
    static LinearLayout ll_t;

    static ArrayList<String> c_array = new ArrayList<String>();
    static ArrayList<Integer> c_array_ids = new ArrayList<Integer>();
    static MultiSpinner circuits;
    static LinearLayout ll_c;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.robot, null);

            tab_buttons = new ToggleButton[NUM_TABS];
            tab_views = new ScrollView[NUM_TABS];

            tab_buttons[0] = (ToggleButton)view.findViewById(R.id.robot_tab_state);
            tab_buttons[1] = (ToggleButton)view.findViewById(R.id.robot_tab_equipment);

            tab_views[0] = (ScrollView)view.findViewById(R.id.robot_sv_state);
            tab_views[1] = (ScrollView)view.findViewById(R.id.robot_sv_equipment);

            btn_save = (Button)view.findViewById(R.id.robot_btn_save);
            btn_cancel = (Button)view.findViewById(R.id.robot_btn_cancel);

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

            OnCheckedChangeListener l = new OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton bv, boolean is_checked) {
                    if (is_checked) {
                        for (int x=0; x<NUM_TABS; ++x) {
                            if (x == ((Integer)bv.getTag())) {
                                tab_views[x].setVisibility(ScrollView.VISIBLE);
                            } else {
                                tab_views[x].setVisibility(ScrollView.GONE);
                                tab_buttons[x].setChecked(false);
                            }
                        }
                    }
                }
            };

            for (int x=0; x<NUM_TABS; ++x) {
                tab_buttons[x].setTag(x);
                tab_buttons[x].setOnCheckedChangeListener(l);
            }

            head_equipment = (Spinner)view.findViewById(R.id.robot_head_equipment);
            head = (Spinner)view.findViewById(R.id.robot_head);
            back_equipment = (Spinner)view.findViewById(R.id.robot_back_equipment);
            front_equipment = (Spinner)view.findViewById(R.id.robot_front_equipment);
            feet = (Spinner)view.findViewById(R.id.robot_feet);
            bolt_set = (Spinner)view.findViewById(R.id.robot_bolt_set);
            weapons = (MultiSpinner)view.findViewById(R.id.robot_weapons);
            tools = (MultiSpinner)view.findViewById(R.id.robot_tools);
            circuits = (MultiSpinner)view.findViewById(R.id.robot_circuits);

            ll_he = (LinearLayout)view.findViewById(R.id.ll_robot_head_equipment);
            ll_h = (LinearLayout)view.findViewById(R.id.ll_robot_head);
            ll_be = (LinearLayout)view.findViewById(R.id.ll_robot_back_equipment);
            ll_fe = (LinearLayout)view.findViewById(R.id.ll_robot_front_equipment);
            ll_f = (LinearLayout)view.findViewById(R.id.ll_robot_feet);
            ll_bs = (LinearLayout)view.findViewById(R.id.ll_robot_bolt_set);
            ll_w = (LinearLayout)view.findViewById(R.id.ll_robot_weapons);
            ll_t = (LinearLayout)view.findViewById(R.id.ll_robot_tools);
            ll_c = (LinearLayout)view.findViewById(R.id.ll_robot_circuits);

            for (String s : PrincipiaBackend.getEquipmentsHeadEquipment().split(",")) {
                he_array.add(s);
            }

            for (String s : PrincipiaBackend.getEquipmentsHead().split(",")) {
                h_array.add(s);
            }

            for (String s : PrincipiaBackend.getEquipmentsBackEquipment().split(",")) {
                be_array.add(s);
            }

            for (String s : PrincipiaBackend.getEquipmentsFrontEquipment().split(",")) {
                fe_array.add(s);
            }

            for (String s : PrincipiaBackend.getEquipmentsFeet().split(",")) {
                f_array.add(s);
            }

            for (String s : PrincipiaBackend.getEquipmentsBoltSet().split(",")) {
                bs_array.add(s);
            }

            for (String s : PrincipiaBackend.getEquipmentsWeapons().split(",")) {
                String[] parts = s.split("=");
                w_array_ids.add(Integer.parseInt(parts[0]));
                w_array.add(parts[1]);
            }

            for (String s : PrincipiaBackend.getEquipmentsTools().split(",")) {
                String[] parts = s.split("=");
                t_array_ids.add(Integer.parseInt(parts[0]));
                t_array.add(parts[1]);
            }

            ArrayAdapter<String> he_aa = new ArrayAdapter<String>(PrincipiaActivity.mSingleton, android.R.layout.simple_spinner_item, he_array);
            he_aa.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            head_equipment.setAdapter(he_aa);

            ArrayAdapter<String> h_aa = new ArrayAdapter<String>(PrincipiaActivity.mSingleton, android.R.layout.simple_spinner_item, h_array);
            h_aa.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            head.setAdapter(h_aa);

            ArrayAdapter<String> be_aa = new ArrayAdapter<String>(PrincipiaActivity.mSingleton, android.R.layout.simple_spinner_item, be_array);
            be_aa.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            back_equipment.setAdapter(be_aa);

            ArrayAdapter<String> fe_aa = new ArrayAdapter<String>(PrincipiaActivity.mSingleton, android.R.layout.simple_spinner_item, fe_array);
            fe_aa.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            front_equipment.setAdapter(fe_aa);

            ArrayAdapter<String> f_aa = new ArrayAdapter<String>(PrincipiaActivity.mSingleton, android.R.layout.simple_spinner_item, f_array);
            f_aa.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            feet.setAdapter(f_aa);

            ArrayAdapter<String> bs_aa = new ArrayAdapter<String>(PrincipiaActivity.mSingleton, android.R.layout.simple_spinner_item, bs_array);
            bs_aa.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            bolt_set.setAdapter(bs_aa);

            weapons.setItems(w_array, PrincipiaActivity.mSingleton.getString(R.string.weapons), new MultiSpinnerListener() {
                @Override public void onItemsSelected(boolean[] selected) { }
            });

            tools.setItems(t_array, PrincipiaActivity.mSingleton.getString(R.string.tools), new MultiSpinnerListener() {
                @Override public void onItemsSelected(boolean[] selected) { }
            });

            rg_default_state = (RadioGroup)view.findViewById(R.id.robot_default_state);
            rg_faction = (RadioGroup)view.findViewById(R.id.robot_faction);
            cb_roam = (CheckBox)view.findViewById(R.id.robot_roam);
            rg_initial_dir = (RadioGroup)view.findViewById(R.id.robot_initial_dir);

            _dialog = new Dialog(PrincipiaActivity.mSingleton, android.R.style.Theme_NoTitleBar_Fullscreen) {
                @Override
                protected void onCreate(Bundle si) {
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
        c_array.clear();
        c_array_ids.clear();

        for (String s : PrincipiaBackend.getCompatibleCircuits().split(",")) {
            String[] parts = s.split("=");
            c_array_ids.add(Integer.parseInt(parts[0]));
            c_array.add(parts[1]);
        }

        circuits.setItems(c_array, PrincipiaActivity.mSingleton.getString(R.string.circuits), new MultiSpinnerListener() {
            @Override public void onItemsSelected(boolean[] selected) { }
        });

        head_equipment.setSelection(0);
        head.setSelection(0);
        back_equipment.setSelection(0);
        front_equipment.setSelection(0);
        feet.setSelection(0);
        bolt_set.setSelection(0);

        String robot_data = PrincipiaBackend.getRobotData();

        String equipments[] = PrincipiaBackend.getRobotEquipment().split(";");

        for (int x=0; x<w_array_ids.size(); ++x) {
            weapons.selected[x] = false;
        }

        for (int x=0; x<t_array_ids.size(); ++x) {
            tools.selected[x] = false;
        }

        for (int x=0; x<c_array_ids.size(); ++x) {
            circuits.selected[x] = false;
        }

        for (String eq : equipments) {
            int item_id = 0;
            try {
                item_id = Integer.parseInt(eq);
            } catch (NumberFormatException e) {
                Log.e("Principia", "Bad equipment: " + eq);
            }

            for (int x=0; x<w_array_ids.size(); ++x) {
                if (item_id == w_array_ids.get(x)) {
                    weapons.selected[x] = true;
                    break;
                }
            }
            for (int x=0; x<t_array_ids.size(); ++x) {
                if (item_id == t_array_ids.get(x)) {
                    tools.selected[x] = true;
                    break;
                }
            }
            for (int x=0; x<c_array_ids.size(); ++x) {
                if (item_id == c_array_ids.get(x)) {
                    circuits.selected[x] = true;
                    break;
                }
            }
        }

        String data[] = robot_data.split(",");
        Log.v("Principia", "Robot data length: " + data.length);
        if (data.length != 17) {
            return;
        }

        int x = 0;
        boolean is_player = "1".equals(data[x++]);
        boolean has_feature_head = "1".equals(data[x++]);
        boolean has_feature_back_equipment = "1".equals(data[x++]);
        boolean has_feature_front_equipment = "1".equals(data[x++]);
        boolean has_feature_weapons = "1".equals(data[x++]);
        boolean has_feature_tools = "1".equals(data[x++]);
        int state = 0;
        boolean roam = false;
        int dir = 0;
        int faction = 0;
        long circuits_compat = 0;
        try {
            head_equipment.setSelection(Integer.parseInt(data[x++]));
            head.setSelection(Integer.parseInt(data[x++]));
            back_equipment.setSelection(Integer.parseInt(data[x++]));
            front_equipment.setSelection(Integer.parseInt(data[x++]));
            feet.setSelection(Integer.parseInt(data[x++]));
            bolt_set.setSelection(Integer.parseInt(data[x++]));
            state = Integer.parseInt(data[x++]);
            roam = "1".equals(data[x++]);
            dir = Integer.parseInt(data[x++]);
            faction = Integer.parseInt(data[x++]);
            circuits_compat = Long.parseLong(data[x++]);
        } catch (NumberFormatException e) {
            Log.e("Principia", "Exception caught: " + e.getMessage());
        }

        ll_he.setVisibility(has_feature_head            ? View.VISIBLE : View.GONE);
        ll_h.setVisibility(has_feature_head             ? View.VISIBLE : View.GONE);
        ll_be.setVisibility(has_feature_back_equipment  ? View.VISIBLE : View.GONE);
        ll_fe.setVisibility(has_feature_front_equipment ? View.VISIBLE : View.GONE);
        ll_w.setVisibility(has_feature_weapons          ? View.VISIBLE : View.GONE);
        ll_t.setVisibility(has_feature_tools            ? View.VISIBLE : View.GONE);

        cb_roam.setChecked(roam);

        switch (faction) {
            case 0: ((RadioButton)view.findViewById(R.id.robot_enemy)).setChecked(true); break;
            case 1: ((RadioButton)view.findViewById(R.id.robot_friendly)).setChecked(true); break;
            case 2: ((RadioButton)view.findViewById(R.id.robot_neutral)).setChecked(true); break;
            case 3: ((RadioButton)view.findViewById(R.id.robot_chaotic)).setChecked(true); break;
        }

        switch (state) {
            case 0: ((RadioButton)view.findViewById(R.id.robot_state_idle)).setChecked(true); break;
            case 1: ((RadioButton)view.findViewById(R.id.robot_state_walk)).setChecked(true); break;
            case 3: ((RadioButton)view.findViewById(R.id.robot_state_dead)).setChecked(true); break;
        }

        switch (dir) {
            case 1: ((RadioButton)view.findViewById(R.id.robot_dir_left)).setChecked(true); break;
            case 0: ((RadioButton)view.findViewById(R.id.robot_dir_random)).setChecked(true); break;
            case 2: ((RadioButton)view.findViewById(R.id.robot_dir_right)).setChecked(true); break;
        }
    }

    public static void save()
    {
        int state = 0;
        int faction = 0;
        int dir = 0;

        switch (rg_default_state.getCheckedRadioButtonId()) {
            case R.id.robot_state_idle: state = 0; break;
            case R.id.robot_state_walk: state = 1; break;
            case R.id.robot_state_dead: state = 3; break;
        }

        switch (rg_faction.getCheckedRadioButtonId()) {
            case R.id.robot_friendly: faction = 1; break;
            case R.id.robot_enemy: faction = 0; break;
            case R.id.robot_neutral: faction = 2; break;
            case R.id.robot_chaotic: faction = 3; break;
        }

        switch (rg_initial_dir.getCheckedRadioButtonId()) {
            case R.id.robot_dir_left: dir = 1; break;
            case R.id.robot_dir_random: dir = 0; break;
            case R.id.robot_dir_right: dir = 2; break;
        }

        PrincipiaBackend.setRobotStuff(state, faction, cb_roam.isChecked(), dir);

        StringBuilder sb = new StringBuilder();

        int n = 0;

        for (int x=0; x<w_array_ids.size(); ++x) {
            if (weapons.selected[x]) {
                if (n++ != 0) { sb.append(";"); }
                sb.append(w_array_ids.get(x));
            }
        }
        for (int x=0; x<t_array_ids.size(); ++x) {
            if (tools.selected[x]) {
                if (n++ != 0) { sb.append(";"); }
                sb.append(t_array_ids.get(x));
            }
        }
        for (int x=0; x<c_array_ids.size(); ++x) {
            if (circuits.selected[x]) {
                if (n++ != 0) { sb.append(";"); }
                sb.append(c_array_ids.get(x));
            }
        }

        PrincipiaBackend.setPropertyString(7, sb.toString());

        PrincipiaBackend.setPropertyInt8(8,  (int)feet.getSelectedItemId());
        PrincipiaBackend.setPropertyInt8(9,  (int)head.getSelectedItemId());
        PrincipiaBackend.setPropertyInt8(10, (int)back_equipment.getSelectedItemId());
        PrincipiaBackend.setPropertyInt8(11, (int)head_equipment.getSelectedItemId());
        PrincipiaBackend.setPropertyInt8(12, (int)front_equipment.getSelectedItemId());
        PrincipiaBackend.setPropertyInt8(13, (int)bolt_set.getSelectedItemId());

        PrincipiaBackend.fixed();
    }
}
