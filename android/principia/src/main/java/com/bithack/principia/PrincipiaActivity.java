package com.bithack.principia;

import com.bithack.principia.shared.ColorChooserDialog;
import com.bithack.principia.shared.CommunityDialog;
import com.bithack.principia.shared.ConfirmDialog;
import com.bithack.principia.shared.ConfirmDialog.OnOptionSelectedListener;
import com.bithack.principia.shared.ExportDialog;
import com.bithack.principia.shared.FactoryDialog;
import com.bithack.principia.shared.ImportDialog;
import com.bithack.principia.shared.JumperDialog;
import com.bithack.principia.shared.Level;
import com.bithack.principia.shared.LevelDialog;
import com.bithack.principia.shared.LoginDialog;
import com.bithack.principia.shared.OpenDialog;
import com.bithack.principia.shared.PromptDialog;
import com.bithack.principia.shared.PromptSettingsDialog;
import com.bithack.principia.shared.PublishDialog;
import com.bithack.principia.shared.PublishedDialog;
import com.bithack.principia.shared.RegisterDialog;
import com.bithack.principia.shared.RobotDialog;
import com.bithack.principia.shared.ScriptDialog;
import com.bithack.principia.shared.SettingsDialog;
import com.bithack.principia.shared.StickyDialog;

import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

import java.util.ArrayList;

import android.app.*;
import android.content.*;
import android.view.*;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.os.*;
import android.graphics.*;
import android.media.*;
import android.hardware.*;
import android.widget.ArrayAdapter;

import org.libsdl.app.SDLActivity;

import java.util.List;

public class PrincipiaActivity extends SDLActivity implements View.OnSystemUiVisibilityChangeListener, DialogInterface.OnDismissListener, DialogInterface.OnShowListener {
    private static final String TAG = "PrincipiaActivity";

    public static int num_dialogs = 0;

    static Toast last_toast = null;
    private static SettingsDialog settings_dialog;

    public static final int LEVEL_LOCAL   = 0;
    public static final int LEVEL_DB      = 1;
    public static final int LEVEL_MAIN    = 2;
    public static final int LEVEL_SYS     = 3;
    public static final int LEVEL_PARTIAL = 4;

    public static final int LEVEL_LOCAL_STATE = 100;
    public static final int LEVEL_DB_STATE    = 101;
    public static final int LEVEL_MAIN_STATE  = 102;

    public static final int ACTION_OPEN = 1;
    public static final int ACTION_STICKY = 6;
    public static final int ACTION_LOGIN =  7;
    public static final int ACTION_PUBLISH = 11;
    public static final int ACTION_UPGRADE_LEVEL = 16;
    public static final int ACTION_SET_STICKY_TEXT = 18;
    public static final int ACTION_IMPORT_OBJECT = 19;
    public static final int ACTION_EXPORT_OBJECT = 20;
    public static final int ACTION_HIGHLIGHT_SELECTED = 28;
    public static final int ACTION_GOTO_MAINMENU = 33;
    public static final int ACTION_DELETE_LEVEL = 34;
    public static final int ACTION_DELETE_PARTIAL = 35;
    public static final int ACTION_SET_LEVEL_TYPE = 36;
    public static final int ACTION_ENTITY_MODIFIED = 38;
    public static final int ACTION_AUTOSAVE = 49;

    public static final int DIALOG_BEAM_COLOR       = 101;
    public static final int DIALOG_OPEN             = 103;
    public static final int DIALOG_NEW_LEVEL        = 104;
    public static final int DIALOG_PIXEL_COLOR      = 106;
    public static final int DIALOG_STICKY           = 109;
    public static final int DIALOG_OPEN_OBJECT      = 113;
    public static final int DIALOG_EXPORT           = 114;
    public static final int DIALOG_ROBOT            = 116;
    public static final int DIALOG_MULTIEMITTER     = 117;
    public static final int DIALOG_SETTINGS         = 121;
    public static final int DIALOG_LEVEL_PROPERTIES = 123;
    public static final int DIALOG_COMMUNITY        = 128;
    public static final int DIALOG_PROMPT_SETTINGS  = 129;
    public static final int DIALOG_PROMPT           = 130;
    public static final int DIALOG_JUMPER           = 136;
    public static final int DIALOG_PUBLISHED        = 138;
    public static final int DIALOG_LUASCRIPT        = 141;
    public static final int DIALOG_FACTORY          = 149;
    public static final int DIALOG_OPEN_STATE       = 156;
    public static final int DIALOG_POLYGON_COLOR    = 157;
    public static final int DIALOG_PUBLISH          = 300;
    public static final int DIALOG_LOGIN            = 301;
    public static final int DIALOG_REGISTER         = 303;

    public static final int CLOSE_ALL_DIALOGS            = 200;
    public static final int CLOSE_ABSOLUTELY_ALL_DIALOGS = 201;
    public static final int CLOSE_REGISTER_DIALOG        = 202;
    public static final int DISABLE_REGISTER_LOADER      = 203;

    public static final int PROMPT_RESPONSE_NONE = 0;
    public static final int PROMPT_RESPONSE_A    = 1;
    public static final int PROMPT_RESPONSE_B    = 2;
    public static final int PROMPT_RESPONSE_C    = 3;

    public WebViewManager webman;

    @Override
    protected String[] getLibraries() {
        return new String[] {
            "principia"
        };
    }

    public static PrincipiaActivity mSingleton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        mSingleton = this;

        super.onCreate(savedInstanceState);

        try {
            webman = new WebViewManager();
        } catch (Exception e) {
            Log.e(TAG, "Failed to init webview: "+e);
        }

        this.handle_intent(this.getIntent());

        open_adapter = new ArrayAdapter<Level>(SDLActivity.mSingleton, android.R.layout.select_dialog_item);
    }

    public static void message(final String s, final int longd) {
        SDLActivity.mSingleton.runOnUiThread(() -> {
            if (last_toast != null)
                last_toast.setText(s);
            else
                last_toast = Toast.makeText(SDLActivity.mSingleton, s, longd==1?Toast.LENGTH_LONG:Toast.LENGTH_SHORT);

            last_toast.show();
        });
    }

    public static void open_url(final String url) {
        mSingleton.webman.open_url(url);
    }

    public static boolean is_cool = false;

    public static void open_dialog(final int num) {
        open_dialog(num, false);
    }

    public static void open_dialog(final int num, final boolean is_cool) {
        PrincipiaActivity.is_cool = is_cool;

        SDLActivity.mSingleton.runOnUiThread(() -> {
            if (num == DIALOG_PROMPT) {
                try {SDLActivity.mSingleton.removeDialog(num);} catch(Exception e){};
            }
            try {SDLActivity.mSingleton.removeDialog(DIALOG_OPEN);} catch(Exception ignored){}
            try {SDLActivity.mSingleton.removeDialog(DIALOG_OPEN_STATE);} catch(Exception ignored){}
            try {SDLActivity.mSingleton.removeDialog(DIALOG_MULTIEMITTER);} catch(Exception ignored){}
            try {SDLActivity.mSingleton.removeDialog(DIALOG_OPEN_OBJECT);} catch(Exception ignored){}
            SDLActivity.mSingleton.showDialog(num);
        });
    }

    private List<Dialog> open_dialogs = new ArrayList<Dialog>();

    public static void confirm(final String text, final String button1, final int action1, final long action1_data, final String button2, final int action2, final long action2_data, final String button3, final int action3, final long action3_data, final boolean dna_sandbox) {
        SDLActivity.mSingleton.runOnUiThread(() -> new ConfirmDialog()
            .set_listener(option -> {
                if (option == ConfirmDialog.OPTION_YES)
                    PrincipiaBackend.addActionAsInt(action1, action1_data);
                else if (option == ConfirmDialog.OPTION_NO)
                    PrincipiaBackend.addActionAsInt(action2, action2_data);
                else if (option == ConfirmDialog.OPTION_3)
                    PrincipiaBackend.addActionAsInt(action3, action3_data);
            })
            .run(text, button1, button2, button3, dna_sandbox));
    }

    @Override
    protected Dialog onCreateDialog(int num) {
        Dialog d = null;

        switch (num) {

        case DIALOG_SETTINGS:
            if (settings_dialog == null)
                d = (settings_dialog = new SettingsDialog()).get_dialog();
            break;

        case DIALOG_OPEN:               d = (new OpenDialog(false)).get_dialog(); break;
        case DIALOG_LEVEL_PROPERTIES:   d = LevelDialog.get_dialog(); break;
        case DIALOG_STICKY:             d = StickyDialog.get_dialog(); break;
        case DIALOG_ROBOT:              d = RobotDialog.get_dialog(); break;
        case DIALOG_PIXEL_COLOR:        d = ColorChooserDialog.get_dialog(); break;
        case DIALOG_BEAM_COLOR:         d = ColorChooserDialog.get_dialog(); break;
        case DIALOG_POLYGON_COLOR:      d = ColorChooserDialog.get_dialog(); break;
        case DIALOG_EXPORT:             d = ExportDialog.get_dialog(); break;
        case DIALOG_MULTIEMITTER:       d = (new ImportDialog(true)).get_dialog(); break;
        case DIALOG_OPEN_OBJECT:        d = (new ImportDialog(false)).get_dialog(); break;
        case DIALOG_COMMUNITY:          d = (new CommunityDialog()).get_dialog(); break;
        case DIALOG_PROMPT_SETTINGS:    d = PromptSettingsDialog.get_dialog(); break;
        case DIALOG_PROMPT:             d = (new PromptDialog()).get_dialog(); break;
        case DIALOG_JUMPER:             d = JumperDialog.get_dialog(); break;
        case DIALOG_LUASCRIPT:          d = ScriptDialog.get_dialog(); break;
        case DIALOG_FACTORY:            d = FactoryDialog.get_dialog(); break;
        case DIALOG_OPEN_STATE:         d = (new OpenDialog(true)).get_dialog(); break;

        case DIALOG_PUBLISH:            d = PublishDialog.get_dialog(); break;
        case DIALOG_PUBLISHED:          d = (new PublishedDialog()).get_dialog(); break;
        case DIALOG_LOGIN:              d = LoginDialog.get_dialog(); break;
        case DIALOG_REGISTER:           d = RegisterDialog.get_dialog(); break;

        case CLOSE_ALL_DIALOGS:         break; /* do nothing */
        case CLOSE_ABSOLUTELY_ALL_DIALOGS:
            SDLActivity.mSingleton.runOnUiThread(() -> {
                Log.v("Principia", "Closing all dialogs.");

                for (Dialog open_dialog : open_dialogs) {
                    Log.v("Principia", "Closing a dialog["+open_dialog.toString()+"]");
                    //open_dialog.dismiss();
                }
                open_dialogs.clear();
            });
            break;

        case CLOSE_REGISTER_DIALOG:
            SDLActivity.mSingleton.runOnUiThread(() -> RegisterDialog.get_dialog().dismiss());
            break;

        case DISABLE_REGISTER_LOADER:
            SDLActivity.mSingleton.runOnUiThread(() -> RegisterDialog.progressbar.setVisibility(View.GONE));
            break;

        default: Log.e("Principia", "Unhandled UI Dialog: "+num); break;
        }

        if (d != null) {
            Log.v("Principia", "Adding dialog: "+ d);
            //this.open_dialogs.add(d);
            if (d.getWindow() != null) {
                d.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,WindowManager.LayoutParams.FLAG_FULLSCREEN);
            }
        }

        return d;
    }

    public static ArrayAdapter<Level> open_adapter;

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v,
              ContextMenuInfo menuInfo) {
        super.onCreateContextMenu(menu, v, menuInfo);
        if (v == OpenDialog.lv) {
            AdapterContextMenuInfo aInfo = (AdapterContextMenuInfo) menuInfo;

            final Level level = open_adapter.getItem(aInfo.position);

            menu.setHeaderTitle("Options for " + level.get_name());
            menu.add(1, 1, 1, "Delete")
                .setOnMenuItemClickListener(item -> {
                    new ConfirmDialog()
                    .set_listener(option -> {
                        if (option == ConfirmDialog.OPTION_YES) {
                            PrincipiaBackend.addActionAsTriple(ACTION_DELETE_LEVEL, level.get_level_type(), level.get_id(), level.get_save_id());
                            open_adapter.remove(level);
                        }
                    })
                    .run("Are you sure you want to delete this level?");

                    return false;
                });
        } else if (v == ImportDialog.lv) {
            AdapterContextMenuInfo aInfo = (AdapterContextMenuInfo) menuInfo;

            final Level level = ImportDialog.list_adapter.getItem(aInfo.position);

            menu.setHeaderTitle("Options for " + level.get_name());
            menu.add(1, 1, 1, "Delete")
                .setOnMenuItemClickListener(item -> {
                    new ConfirmDialog()
                    .set_listener(new OnOptionSelectedListener() {
                        @Override
                        public void onOptionSelected(int option) {
                            if (option == ConfirmDialog.OPTION_YES) {
                                PrincipiaBackend.addActionAsInt(ACTION_DELETE_PARTIAL, level.get_id());
                                ImportDialog.list_adapter.remove(level);
                            }
                        }
                    })
                    .run("Are you sure you want to delete this object?");

                    return false;
                });
        }

        /* For the details option:
         * Level ID
         * Level name
         * Date modified
         */
    }

    @Override
    public void onPrepareDialog(int d, Dialog dialog, Bundle bundle)
    {
        switch (d) {
            case DIALOG_SETTINGS:
                if (settings_dialog != null)
                    settings_dialog.load();
                break;

            case DIALOG_LEVEL_PROPERTIES:   LevelDialog.prepare(dialog); break;
            case DIALOG_ROBOT:              RobotDialog.prepare(dialog); break;
            case DIALOG_PIXEL_COLOR:        ColorChooserDialog.prepare(dialog, true); break;
            case DIALOG_BEAM_COLOR:         ColorChooserDialog.prepare(dialog, false); break;
            case DIALOG_POLYGON_COLOR:      ColorChooserDialog.prepare(dialog, false); break;
            case DIALOG_EXPORT:             ExportDialog.prepare(dialog); break;
            case DIALOG_STICKY:             StickyDialog.prepare(dialog); break;
            case DIALOG_PROMPT_SETTINGS:    PromptSettingsDialog.prepare(dialog); break;
            case DIALOG_JUMPER:             JumperDialog.prepare(dialog); break;
            case DIALOG_LUASCRIPT:          ScriptDialog.prepare(dialog); break;
            case DIALOG_FACTORY:            FactoryDialog.prepare(dialog); break;
            case DIALOG_PUBLISH:            PublishDialog.prepare(dialog); break;
            case DIALOG_LOGIN:              LoginDialog.prepare(dialog); break;
        }

        /* Dialogs that need a separate onShowListener */
        switch (d) {
            case DIALOG_PUBLISH:
            case DIALOG_LOGIN:
            case DIALOG_REGISTER:
            case DIALOG_PROMPT_SETTINGS:
            case DIALOG_OPEN:
            case DIALOG_OPEN_STATE:
            case DIALOG_OPEN_OBJECT:
            case DIALOG_MULTIEMITTER:
                break;

            default: dialog.setOnShowListener(this); break;
        }

        /* Dialogs that need to dismiss HARD */
        switch (d) {
            default: dialog.setOnDismissListener(this); break;
        }

        //dialog.setOnCancelListener(this);
    }

    public void onDismiss(DialogInterface dialog)
    {
        Log.v("Principia", "dialog onDismiss called");
        open_dialogs.remove(dialog);
        num_dialogs --;
        if (num_dialogs <= 0){
            num_dialogs = 0;
            PrincipiaBackend.focusGL(true);
        }
    }

    public void onShow(DialogInterface dialog) {
        Log.v("Principia", "dialog onShow called");
        this.open_dialogs.add((Dialog) dialog);
        num_dialogs  ++;
        if (num_dialogs == 1) {
            PrincipiaBackend.focusGL(false);
        }
    }

    public static void on_show(DialogInterface dialog) {
        Log.v("Principia", "dialog onShow called");
        num_dialogs ++;
        if (num_dialogs == 1) {
            PrincipiaBackend.focusGL(false);
        }
    }

    private void handle_intent(Intent i) {
        if (i != null && i.getScheme() != null && i.getScheme().equals("principia"))
            PrincipiaBackend.setarg(i.getDataString());
    }

    @Override
    public void onNewIntent(Intent i) {
        super.onNewIntent(i);
        handle_intent(i);
    }
}
