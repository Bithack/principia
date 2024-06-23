package com.bithack.principia.shared;

import java.util.ArrayList;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;

import org.libsdl.app.PrincipiaBackend;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.widget.ToggleButton;

public class FactoryDialog {
    public static final int NUM_TABS = 2;
    public static int NUM_EXTRA_PROPERTIES = 0;
    static Dialog _dialog;

    static View view;

    static ToggleButton tab_buttons[];
    static ScrollView tab_views[];

    static LinearLayout ll_resources;
    static LinearLayout ll_recipes;

    static Button btn_save;
    static Button btn_cancel;

    static TableLayout tl_recipes;

    static ArrayList<NumberPicker> np_resources = new ArrayList<NumberPicker>();
    static ArrayList<Recipe> recipes = new ArrayList<Recipe>();

    static OnCheckedChangeListener recipe_cc;

    public static Dialog get_dialog()
    {
        if (_dialog == null) {
            NUM_EXTRA_PROPERTIES = PrincipiaBackend.getFactoryNumExtraProperties();
            recipe_cc = new OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton arg0, boolean arg1) {
                    recalculate_indices();
                }
            };
            tab_buttons = new ToggleButton[NUM_TABS];
            tab_views = new ScrollView[NUM_TABS];

            view = LayoutInflater.from(PrincipiaActivity.mSingleton).inflate(R.layout.factory, null);

            tab_buttons[0] = (ToggleButton)view.findViewById(R.id.factory_tab_resources);
            tab_buttons[1] = (ToggleButton)view.findViewById(R.id.factory_tab_recipes);

            tab_views[0] = (ScrollView)view.findViewById(R.id.factory_sv_resources);
            tab_views[1] = (ScrollView)view.findViewById(R.id.factory_sv_recipes);

            OnCheckedChangeListener l = new OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView,
                        boolean isChecked) {
                    if (isChecked) {
                        for (int x=0; x<FactoryDialog.NUM_TABS; ++x) {
                            if (x == ((Integer)buttonView.getTag())) {
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

            ll_resources = (LinearLayout)view.findViewById(R.id.factory_ll_resources);
            ll_recipes = (LinearLayout)view.findViewById(R.id.factory_ll_recipes);

            tl_recipes = (TableLayout)view.findViewById(R.id.factory_recipes);

            // we can generate the list of resources here
            String[] resources_list = PrincipiaBackend.getResources().split(",");

            for (String resource : resources_list) {
                LinearLayout ll = new LinearLayout(PrincipiaActivity.mSingleton);
                TextView tv = new TextView(PrincipiaActivity.mSingleton);
                NumberPicker np = new NumberPicker(PrincipiaActivity.mSingleton);

                ll.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));

                np.setCurrent(0);
                np.setRange(0, 65535);

                np.setLayoutParams(new TableLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT, 1.f));
                np.setGravity(Gravity.RIGHT);

                tv.setLayoutParams(new TableLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.MATCH_PARENT, 1.f));
                tv.setText(resource);

                tv.setGravity(Gravity.CENTER_VERTICAL | Gravity.LEFT);

                ll.addView(tv);
                ll.addView(np);

                ll_resources.addView(ll);

                np_resources.add(np);
            }

            btn_save = (Button)view.findViewById(R.id.factory_btn_save);
            btn_cancel = (Button)view.findViewById(R.id.factory_btn_cancel);

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

            _dialog = new Dialog(PrincipiaActivity.mSingleton, android.R.style.Theme_NoTitleBar_Fullscreen){
                @Override
                protected void onCreate(Bundle saved_instance) {
                    super.onCreate(saved_instance);
                    getWindow().setLayout(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
                }
            };
            _dialog.setContentView(view);
        }

        return _dialog;
    }

    public static void prepare(DialogInterface di)
    {
        // Clear all previous recipes
        tl_recipes.removeAllViews();
        recipes.clear();

        String[] recipe_list = PrincipiaBackend.getRecipes().split(",");

        for (String recipe_parts : recipe_list) {
            String[] data = recipe_parts.split(";");

            Log.v("Principia", "Recipe cool man: " + recipe_parts);

            if (data.length == 3) {
                String recipe_name = data[0];
                boolean enabled = false;
                int id = 0;

                Log.v("Principia", "Recipe name: " + recipe_name);

                try {
                    enabled = Integer.parseInt(data[1]) != 0;
                    id = Integer.parseInt(data[2]);
                } catch (NumberFormatException e) {
                    continue;
                }

                TableRow row = new TableRow(PrincipiaActivity.mSingleton);
                TextView tv_name = new TextView(PrincipiaActivity.mSingleton);
                TextView tv_index = new TextView(PrincipiaActivity.mSingleton);
                CheckBox cb_enabled = new CheckBox(PrincipiaActivity.mSingleton);

                cb_enabled.setChecked(enabled);
                cb_enabled.setGravity(Gravity.RIGHT);
                cb_enabled.setOnCheckedChangeListener(recipe_cc);

                tv_name.setText(recipe_name);

                tv_index.setText("-1"); // this will be calculated after everything has been added

                row.addView(tv_name);
                row.addView(tv_index);
                row.addView(cb_enabled);

                Recipe r = new Recipe();
                r.name = tv_name;
                r.index = tv_index;
                r.enabled = cb_enabled;
                r.id = id;

                tl_recipes.addView(row);

                recipes.add(r);
            }
        }

        String[] resource_list = PrincipiaBackend.getFactoryResources().split(",");

        for (int x=0; x<resource_list.length; ++x) {
            try {
                np_resources.get(x).setValue(Integer.parseInt(resource_list[x]));
            } catch (NumberFormatException ignored) {
            }
        }

        recalculate_indices();
    }

    public static void recalculate_indices()
    {
        int index = 0;

        for (Recipe r : recipes) {
            if (r.enabled.isChecked()) {
                r.index.setText(Integer.toString(++index));
            } else {
                r.index.setText("-1");
            }
        }
    }

    public static void save()
    {
        StringBuilder sb = new StringBuilder();

        boolean first = true;
        for (Recipe r : recipes) {
            if (r.enabled.isChecked()) {
                if (!first) {
                    sb.append(";");
                }

                sb.append(r.id);

                first = false;
            }
        }

        for (int x=0; x<np_resources.size(); ++x) {
            NumberPicker np = np_resources.get(x);

            if (x == 0) {
                // Special case for Oil
                PrincipiaBackend.setPropertyInt(1, np.getValue());
            } else {
                PrincipiaBackend.setPropertyInt(NUM_EXTRA_PROPERTIES + x - 1, np.getValue());
            }
        }

        Log.v("Principia", "ZZZ: '" + sb + "'");

        PrincipiaBackend.setPropertyString(0, sb.toString());
    }
}
