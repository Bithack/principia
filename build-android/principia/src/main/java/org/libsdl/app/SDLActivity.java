package org.libsdl.app;

import javax.microedition.khronos.egl.*;

import com.bithack.principia.PrincipiaActivity;
import com.bithack.principia.R;
import com.bithack.principia.shared.AutosaveDialog;
import com.bithack.principia.shared.CamTargeterDialog;
import com.bithack.principia.shared.ColorChooserDialog;
import com.bithack.principia.shared.CommandPadDialog;
import com.bithack.principia.shared.CommunityDialog;
import com.bithack.principia.shared.ConfirmDialog;
import com.bithack.principia.shared.ConfirmDialog.OnOptionSelectedListener;
import com.bithack.principia.shared.ConsumableDialog;
import com.bithack.principia.shared.DigitalDisplayDialog;
import com.bithack.principia.shared.EventListenerDialog;
import com.bithack.principia.shared.ExportDialog;
import com.bithack.principia.shared.FactoryDialog;
import com.bithack.principia.shared.FrequencyDialog;
import com.bithack.principia.shared.FrequencyRangeDialog;
import com.bithack.principia.shared.FxEmitterDialog;
import com.bithack.principia.shared.HelpDialog;
import com.bithack.principia.shared.ImportDialog;
import com.bithack.principia.shared.JumperDialog;
import com.bithack.principia.shared.Level;
import com.bithack.principia.shared.LevelDialog;
import com.bithack.principia.shared.LoginDialog;
import com.bithack.principia.shared.NewLevelDialog;
import com.bithack.principia.shared.OpenDialog;
import com.bithack.principia.shared.PkgLevelDialog;
import com.bithack.principia.shared.PlayDialog;
import com.bithack.principia.shared.PromptDialog;
import com.bithack.principia.shared.PromptSettingsDialog;
import com.bithack.principia.shared.PublishDialog;
import com.bithack.principia.shared.PublishedDialog;
import com.bithack.principia.shared.PuzzlePlayDialog;
import com.bithack.principia.shared.QuickaddDialog;
import com.bithack.principia.shared.RegisterDialog;
import com.bithack.principia.shared.ResourceDialog;
import com.bithack.principia.shared.RobotDialog;
import com.bithack.principia.shared.RubberDialog;
import com.bithack.principia.shared.SandboxTipsDialog;
import com.bithack.principia.shared.SaveAsDialog;
import com.bithack.principia.shared.ScriptDialog;
import com.bithack.principia.shared.SequencerDialog;
import com.bithack.principia.shared.SettingsDialog;
import com.bithack.principia.shared.Sfx2Dialog;
import com.bithack.principia.shared.SfxDialog;
import com.bithack.principia.shared.ShapeExtruderDialog;
import com.bithack.principia.shared.PolygonDialog;
import com.bithack.principia.shared.KeyListenerDialog;
import com.bithack.principia.shared.EmitterDialog;
import com.bithack.principia.shared.DecorationDialog;
import com.bithack.principia.shared.AnimalDialog;
import com.bithack.principia.shared.StickyDialog;
import com.bithack.principia.shared.SynthesizerDialog;
import com.bithack.principia.shared.TimerDialog;
import com.bithack.principia.shared.ToolDialog;
import com.bithack.principia.shared.TouchFieldDialog;
import com.bithack.principia.shared.VariableDialog;
import com.bithack.principia.shared.SoundManDialog;
import com.bithack.principia.shared.MultiSelectDialog;
import com.bithack.principia.shared.VendorDialog;

import android.app.*;
import android.content.*;
import android.content.DialogInterface.OnKeyListener;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.view.*;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.webkit.CookieManager;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.Toast;
import android.net.Uri;
import android.os.*;
import android.provider.Settings.Secure;
import android.util.DisplayMetrics;
import android.util.Log;
import android.graphics.*;
import android.media.*;
import android.hardware.*;
import android.widget.ArrayAdapter;

import androidx.core.view.WindowInsetsControllerCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
    SDL Activity
*/
public class SDLActivity extends Activity implements DialogInterface.OnDismissListener/*, DialogInterface.OnCancelListener*/, DialogInterface.OnShowListener, OnSeekBarChangeListener
{
    public static Dialog wv_dialog;
    public static WebView wv;
    public static CookieManager wv_cm;

    // Keep track of the paused state
    public static boolean mIsPaused;

    // Main components
    public static SDLActivity mSingleton;
    private static SDLSurface mSurface;

    private static SettingsDialog settings_dialog;

    static Toast last_toast = null;

    // This is what SDL runs in. It invokes SDL_main(), eventually
    private static Thread mSDLThread;

    // Audio
    private static Thread mAudioThread;
    private static AudioTrack mAudioTrack;

    // EGL private objects
    private static EGLContext  mEGLContext;
    private static EGLSurface  mEGLSurface;
    private static EGLDisplay  mEGLDisplay;
    private static EGLConfig   mEGLConfig;
    private static int mGLMajor, mGLMinor;

    // Load the .so
    static {
        //System.loadLibrary("SDL2");
        //System.loadLibrary("SDL2_image");
        //System.loadLibrary("SDL2_mixer");
        //System.loadLibrary("SDL2_ttf");
        System.loadLibrary("main");
    }

    public SurfaceHolder mHolder;

    private boolean really_quit = false;

    public static boolean mPauseState = false;

    // Setup
    protected void onCreate(Bundle savedInstanceState) {
        Log.v("SDL", "onCreate()");

        if (savedInstanceState == null) {
            Log.v("SDL", "this is a fresh create");
            SDLActivity.mIsPaused = false;
        } else {
            if (savedInstanceState.getBoolean("reallyQuit")) {
                android.os.Process.killProcess(android.os.Process.myPid());
                return;
            }

            boolean x = savedInstanceState.getBoolean("isPaused");
            if (x) {
                Log.v("SDL", "this is an old create, paused=true");
            } else {
                Log.v("SDL", "this is an old create, paused=false");
            }
            //SDLActivity.mIsPaused = savedInstanceState.getBoolean("isPaused");
            SDLActivity.mIsPaused = x;
            //this.mIsPaused = true;
        }

        super.onCreate(savedInstanceState);

        this.init_webview();

        // So we can call stuff from static callbacks
        mSingleton = this;

        mSingleton.requestWindowFeature(Window.FEATURE_NO_TITLE);
        mSingleton.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,WindowManager.LayoutParams.FLAG_FULLSCREEN);

        enableImmersiveMode();

        // Set up the surface
        mSurface = new SDLSurface(getApplication());
        setContentView(mSurface);
        this.mHolder = mSurface.getHolder();

        this.handle_intent(this.getIntent());

        // Try to use more data here. ANDROID_ID is a single point of attack.
        String deviceId = Secure.getString(getContentResolver(), Secure.ANDROID_ID);
        Log.v("Principia", "did: "+deviceId);

        SDLActivity.open_adapter = new ArrayAdapter<Level>(SDLActivity.mSingleton,
                android.R.layout.select_dialog_item);
        QuickaddDialog.object_adapter = new ArrayAdapter<String>(this, android.R.layout.simple_dropdown_item_1line);
    }

    public void enableImmersiveMode()
    {
        /// XXX: Immersive/fullscreen mode makes it difficult to use the sandbox menu drawer and causes some graphical
        /// glitches, comment this out for now (should probably be a toggle)
        /*
        WindowCompat.setDecorFitsSystemWindows(mSingleton.getWindow(), true);
        WindowInsetsControllerCompat controller = new WindowInsetsControllerCompat(mSingleton.getWindow(), mSingleton.getWindow().getDecorView());

        if (controller != null) {
            controller.hide(WindowInsetsCompat.Type.statusBars() | WindowInsetsCompat.Type.navigationBars());
            controller.setSystemBarsBehavior(WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        }*/
    }

    @Override
    public void onSaveInstanceState(Bundle savedInstanceState)
    {
        savedInstanceState.putBoolean("isPaused", SDLActivity.mPauseState);
        savedInstanceState.putBoolean("reallyQuit", SDLActivity.mSingleton.really_quit);
        super.onSaveInstanceState(savedInstanceState);
    }

    private void handle_intent(Intent i)
    {
        Log.v("Principia", "intent new!");

        if (i != null) {
            if (i.getScheme() != null && i.getScheme().equals("principia")) {
                PrincipiaBackend.setarg(i.getDataString());
            }
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        if (requestCode == 1) {
            this.cleanQuit();
        }
    }

    @Override
    public void onNewIntent(Intent i)
    {
        super.onNewIntent(i);

        handle_intent(i);
    }

    // Events
    /*protected void onPause() {
        Log.v("SDL", "onPause()");
        super.onPause();
        // Don't call SDLActivity.nativePause(); here, it will be called by SDLSurface::surfaceDestroyed
    }*/

    protected void onResume() {
        Log.v("SDL", "onResume()");
        SDLActivity.num_dialogs = 0;
        mSingleton.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,WindowManager.LayoutParams.FLAG_FULLSCREEN);
        super.onResume();
        // Don't call SDLActivity.nativeResume(); here, it will be called via SDLSurface::surfaceChanged->SDLActivity::startApp
    }

    protected void onDestroy() {
        super.onDestroy();
        Log.v("SDL", "onDestroy()");
        // Send a quit message to the application
        // /* XXX */
        if (really_quit) {
            SDLActivity.nativeQuit();

            if (mSDLThread != null) {
                try {
                    mSDLThread.join();
                } catch(Exception e) {
                    Log.v("SDL", "Problem stopping thread: " + e);
                }
                mSDLThread = null;

                Log.v("SDL", "Finished waiting for SDL thread");
            }
        } else {
            SDLActivity.nativePause();
        }

        // Now wait for the SDL thread to quit
        /*

        */
    }

    // Messages from the SDLMain thread
    static final int COMMAND_CHANGE_TITLE = 1;
    static final int COMMAND_KEYBOARD_SHOW = 2;

    // Handler for the messages
    static Handler commandHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.arg1) {
            case COMMAND_CHANGE_TITLE:
                SDLActivity.mSingleton.setTitle((String)msg.obj);
                break;
            case COMMAND_KEYBOARD_SHOW:
                /*
                InputMethodManager manager = (InputMethodManager) SDLActivity.mSingleton.getSystemService(INPUT_METHOD_SERVICE);
                if (manager != null) {
                    switch (((Integer)msg.obj).intValue()) {
                    case 0:
                        manager.hideSoftInputFromWindow(mSurface.getWindowToken(), 0);
                        break;
                    case 1:
                        manager.showSoftInput(mSurface, 0);
                        break;
                    case 2:
                        manager.toggleSoftInputFromWindow(mSurface.getWindowToken(), 0, 0);
                        break;
                    }
                }
                */
               break;
            }
        }
    };

    public static int num_dialogs = 0;

    // Send a message from the SDLMain thread
    void sendCommand(int command, Object data) {
        Message msg = commandHandler.obtainMessage();
        msg.arg1 = command;
        msg.obj = data;
        commandHandler.sendMessage(msg);
    }

    // C functions we call
    public static native void nativeInit();
    public static native void nativeQuit();
    public static native void nativePause();
    public static native void nativeResume();
    public static native void onNativeResize(int x, int y, int format, float xdpi, float ydpi);
    public static native void onNativeKeyDown(int keycode);
    public static native void onNativeKeyUp(int keycode);
    public static native void onNativeTouch(int touchDevId, int pointerFingerId,
                                            int action, float x,
                                            float y, float p);
    public static native void onNativeAccel(float x, float y, float z);
    public static native void nativeRunAudioThread();


    // Java functions called from C

    public static boolean createGLContext(int majorVersion, int minorVersion) {
        return initEGL(majorVersion, minorVersion);
    }

    public static void flipBuffers() {
        flipEGL();
    }

    public static void setActivityTitle(String title) {
        // Called from SDLMain() thread and can't directly affect the
        mSingleton.sendCommand(COMMAND_CHANGE_TITLE, title);
    }

    public static void sendMessage(int command, int param) {
        mSingleton.sendCommand(command, Integer.valueOf(param));
    }

    public static Context getContext() {
        return mSingleton;
    }

    public static void startApp() {
        // Start up the C app thread
        if (mSDLThread == null) {
            Log.v("SDL", "MainThread init");
            mSDLThread = new Thread(new SDLMain(), "SDLThread");
            mSDLThread.start();
        } else {
            /*
             * Some Android variants may send multiple surfaceChanged events, so we don't need to resume every time
             * every time we get one of those events, only if it comes after surfaceDestroyed
             */
            if (SDLActivity.mIsPaused) {
                SDLActivity.nativeResume();
                SDLActivity.mIsPaused = false;
                SDLActivity.mPauseState = false;
                PrincipiaBackend.setPaused(false);
                Log.v("SDL", "Resumed via startApp");
            }
            //if (mIsPaused) {
            //}
        }

        mSDLThread.setPriority(Thread.MAX_PRIORITY);
    }

    // EGL functions
    public static boolean initEGL(int majorVersion, int minorVersion) {
        Log.v("SDL", "initEGL");
        if (SDLActivity.mEGLDisplay == null) {
            Log.v("SDL", "Starting up OpenGL ES " + majorVersion + "." + minorVersion);

            try {
                EGL10 egl = (EGL10)EGLContext.getEGL();

                EGLDisplay dpy = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

                int[] version = new int[2];
                egl.eglInitialize(dpy, version);

                int EGL_OPENGL_ES_BIT = 1;
                int EGL_OPENGL_ES2_BIT = 4;
                int renderableType = 0;
                if (majorVersion == 2) {
                    renderableType = EGL_OPENGL_ES2_BIT;
                } else if (majorVersion == 1) {
                    renderableType = EGL_OPENGL_ES_BIT;
                }
                int[] configSpec = {
                    EGL10.EGL_DEPTH_SIZE,   16,
                    EGL10.EGL_RENDERABLE_TYPE, renderableType,
                    EGL10.EGL_NONE
                };
                EGLConfig[] configs = new EGLConfig[1];
                int[] num_config = new int[1];
                if (!egl.eglChooseConfig(dpy, configSpec, configs, 1, num_config) || num_config[0] == 0) {
                    Log.e("SDL", "No EGL config available");
                    return false;
                }
                EGLConfig config = configs[0];

                /*int EGL_CONTEXT_CLIENT_VERSION=0x3098;
                int contextAttrs[] = new int[] { EGL_CONTEXT_CLIENT_VERSION, majorVersion, EGL10.EGL_NONE };
                EGLContext ctx = egl.eglCreateContext(dpy, config, EGL10.EGL_NO_CONTEXT, contextAttrs);

                if (ctx == EGL10.EGL_NO_CONTEXT) {
                    Log.e("SDL", "Couldn't create context");
                    return false;
                }
                SDLActivity.mEGLContext = ctx;*/
                SDLActivity.mEGLDisplay = dpy;
                SDLActivity.mEGLConfig = config;
                SDLActivity.mGLMajor = majorVersion;
                SDLActivity.mGLMinor = minorVersion;

                SDLActivity.createEGLSurface();

            } catch(Exception e) {
                Log.v("SDL", e + "");
                for (StackTraceElement s : e.getStackTrace()) {
                    Log.v("SDL", s.toString());
                }
            }
        }
        else SDLActivity.createEGLSurface();

        return true;
    }

    public static boolean createEGLContext() {
        Log.v("SDL", "Creating new EGL CONTEXT");
        EGL10 egl = (EGL10)EGLContext.getEGL();
        int EGL_CONTEXT_CLIENT_VERSION=0x3098;
        int contextAttrs[] = new int[] { EGL_CONTEXT_CLIENT_VERSION, SDLActivity.mGLMajor, EGL10.EGL_NONE };
        SDLActivity.mEGLContext = egl.eglCreateContext(SDLActivity.mEGLDisplay, SDLActivity.mEGLConfig, EGL10.EGL_NO_CONTEXT, contextAttrs);
        if (SDLActivity.mEGLContext == EGL10.EGL_NO_CONTEXT) {
            Log.e("SDL", "Couldn't create context");
            return false;
        }
        return true;
    }

    public static boolean createEGLSurface() {
        if (SDLActivity.mEGLDisplay != null && SDLActivity.mEGLConfig != null) {
            EGL10 egl = (EGL10)EGLContext.getEGL();
            if (SDLActivity.mEGLContext == null) createEGLContext();


            Log.v("SDL", "Creating new EGL Surface");
            EGLSurface surface = egl.eglCreateWindowSurface(SDLActivity.mEGLDisplay, SDLActivity.mEGLConfig, SDLActivity.mSurface, null);
            if (surface == EGL10.EGL_NO_SURFACE) {
                Log.e("SDL", "Couldn't create surface");
                return false;
            }

            if (egl.eglGetCurrentContext() != SDLActivity.mEGLContext) {
                if (!egl.eglMakeCurrent(SDLActivity.mEGLDisplay, surface, surface, SDLActivity.mEGLContext)) {
                    Log.e("SDL", "Old EGL Context doesnt work, trying with a new one");
                    // TODO: Notify the user via a message that the old context could not be restored, and that textures need to be manually restored.
                    createEGLContext();
                    if (!egl.eglMakeCurrent(SDLActivity.mEGLDisplay, surface, surface, SDLActivity.mEGLContext)) {
                        Log.e("SDL", "Failed making EGL Context current");
                        return false;
                    }
                }
            }
            SDLActivity.mEGLSurface = surface;
            return true;
        }
        return false;
    }

    // EGL buffer flip
    public static void flipEGL() {
        try {
            EGL10 egl = (EGL10)EGLContext.getEGL();

            //egl.eglWaitNative(EGL10.EGL_CORE_NATIVE_ENGINE, null);

            // drawing here

            //egl.eglWaitGL();

            egl.eglSwapBuffers(SDLActivity.mEGLDisplay, SDLActivity.mEGLSurface);

        } catch(Exception e) {
            Log.v("SDL", "flipEGL(): " + e);
            for (StackTraceElement s : e.getStackTrace()) {
                Log.v("SDL", s.toString());
            }
        }
    }

    // Audio
    private static Object buf;

    public static Object audioInit(int sampleRate, boolean is16Bit, boolean isStereo, int desiredFrames) {
        int channelConfig = isStereo ? AudioFormat.CHANNEL_CONFIGURATION_STEREO : AudioFormat.CHANNEL_CONFIGURATION_MONO;
        int audioFormat = is16Bit ? AudioFormat.ENCODING_PCM_16BIT : AudioFormat.ENCODING_PCM_8BIT;
        int frameSize = (isStereo ? 2 : 1) * (is16Bit ? 2 : 1);

        Log.v("SDL", "SDL audio: wanted " + (isStereo ? "stereo" : "mono") + " " + (is16Bit ? "16-bit" : "8-bit") + " " + ((float)sampleRate / 1000f) + "kHz, " + desiredFrames + " frames buffer");

        // Let the user pick a larger buffer if they really want -- but ye
        // gods they probably shouldn't, the minimums are horrifyingly high
        // latency already
        desiredFrames = Math.max(desiredFrames, (AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat) + frameSize - 1) / frameSize);

        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
                channelConfig, audioFormat, desiredFrames * frameSize, AudioTrack.MODE_STREAM);

        audioStartThread();

        Log.v("SDL", "SDL audio: got " + ((mAudioTrack.getChannelCount() >= 2) ? "stereo" : "mono") + " " + ((mAudioTrack.getAudioFormat() == AudioFormat.ENCODING_PCM_16BIT) ? "16-bit" : "8-bit") + " " + ((float)mAudioTrack.getSampleRate() / 1000f) + "kHz, " + desiredFrames + " frames buffer");

        if (is16Bit) {
            buf = new short[desiredFrames * (isStereo ? 2 : 1)];
        } else {
            buf = new byte[desiredFrames * (isStereo ? 2 : 1)];
        }
        return buf;
    }

    public static void audioStartThread() {
        mAudioThread = new Thread(new Runnable() {
            public void run() {
                mAudioTrack.play();
                nativeRunAudioThread();
            }
        });

        // I'd take REALTIME if I could get it!
        mAudioThread.setPriority(Thread.MAX_PRIORITY);
        mAudioThread.start();
    }

    public static void audioWriteShortBuffer(short[] buffer) {
        for (int i = 0; i < buffer.length; ) {
            int result = mAudioTrack.write(buffer, i, buffer.length - i);
            if (result > 0) {
                i += result;
            } else if (result == 0) {
                try {
                    Thread.sleep(1);
                } catch(InterruptedException e) {
                    // Nom nom
                }
            } else {
                Log.w("SDL", "SDL audio: error return from write(short)");
                return;
            }
        }
    }

    public static void audioWriteByteBuffer(byte[] buffer) {
        for (int i = 0; i < buffer.length; ) {
            int result = mAudioTrack.write(buffer, i, buffer.length - i);
            if (result > 0) {
                i += result;
            } else if (result == 0) {
                try {
                    Thread.sleep(1);
                } catch(InterruptedException e) {
                    // Nom nom
                }
            } else {
                Log.w("SDL", "SDL audio: error return from write(short)");
                return;
            }
        }
    }

    public static void audioQuit() {
        if (mAudioThread != null) {
            try {
                mAudioThread.join();
            } catch(Exception e) {
                Log.v("SDL", "Problem stopping audio thread: " + e);
            }
            mAudioThread = null;

            //Log.v("SDL", "Finished waiting for audio thread");
        }

        if (mAudioTrack != null) {
            mAudioTrack.stop();
            mAudioTrack = null;
        }
    }

    /* -----------------------------------------------------------------------------------------------------  TMS PRINCIPIA */

    public static String get_storage_path()
    {
        File file_path[] = SDLActivity.getContext().getExternalFilesDirs(null);

        String path = file_path[0].toString();

        Log.v("FilesDir", path);

        return path;
    }

    public static void message(final String s, final int longd)
    {
        Log.v("tes message", s);
        SDLActivity.mSingleton.runOnUiThread(new Runnable(){
            public void run() {
                if (last_toast != null) {
                    last_toast.setText(s);
                } else {
                    last_toast = Toast.makeText(SDLActivity.mSingleton, s, longd==1?Toast.LENGTH_LONG:Toast.LENGTH_SHORT);
                }
                last_toast.show();
        }
        });
    }

    public static final int SIGNAL_QUICKADD_REFRESH     = 200;
    public static final int SIGNAL_CLICK_DISCOVER       = 400;
    public static final int SIGNAL_CLICK_SANDBOX        = 401;
    public static final int SIGNAL_SAVE_LEVEL           = 402;
    public static final int SIGNAL_PLAY_COMMUNITY_LEVEL = 403;
    public static final int SIGNAL_MAIN_LEVEL_COMPLETED = 404;
    protected static final String TAG = "Principia";

    public static void emit_signal(final int signal_id)
    {
        SDLActivity.mSingleton.runOnUiThread(new Runnable(){
            public void run() {
                switch (signal_id) {
                case SIGNAL_QUICKADD_REFRESH:
                    Log.v("Principia", "Quickadd refresh.");
                    QuickaddDialog.object_adapter.clear();
                    String[] objects = PrincipiaBackend.getObjects().split(",");

                    Log.v("Principia", String.format("Number of objects: %d", objects.length));

                    for (String name : objects) {
                        QuickaddDialog.object_adapter.add(name);
                    }
                    break;
                case SIGNAL_CLICK_DISCOVER:
                    break;
                case SIGNAL_CLICK_SANDBOX:
                    break;
                case SIGNAL_SAVE_LEVEL:
                    break;
                case SIGNAL_PLAY_COMMUNITY_LEVEL:
                    break;
                case SIGNAL_MAIN_LEVEL_COMPLETED:
                    break;
                }
            }
        });
    }

    public static void open_url(final String url)
    {
        SDLActivity.mSingleton.runOnUiThread(new Runnable(){
            public void run() {
                String community_host = PrincipiaBackend.getCommunityHost();
                if (SDLActivity.wv_cm != null) {
                    String curl_token = PrincipiaBackend.getCookies();

                    if (curl_token != null) {
                        // we got relevant cookies from curl!
                        SDLActivity.wv_cm.setCookie("."+community_host, "_PRINCSECURITY="+curl_token);
                    }
                }

                SDLActivity.wv.stopLoading();
                SDLActivity.wv.loadUrl(url);
                SDLActivity.wv_dialog.show();
            }
        });
    }

    public void init_webview()
    {
        SDLActivity.wv_dialog = new Dialog(this, android.R.style.Theme_NoTitleBar_Fullscreen) {
            @Override
            protected void onCreate(Bundle saved_instance) {
                super.onCreate(saved_instance);
                getWindow().setLayout(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
            }
        };

        View v = LayoutInflater.from(this).inflate(R.layout.webview, null);
        LinearLayout ll = (LinearLayout)v.findViewById(R.id.wv_ll);
        final ProgressBar pb = (ProgressBar)v.findViewById(R.id.wv_progress);
        final TextView pb_tv = (TextView)v.findViewById(R.id.wv_progresstext);

        SDLActivity.wv = new WebView(this);
        SDLActivity.wv.getSettings().setJavaScriptEnabled(true);
        int version = 0;
        PackageInfo pi;
        try {
            pi = getPackageManager().getPackageInfo(getPackageName(), 0);
            version = pi.versionCode;
        } catch (NameNotFoundException e) {
            version = 0;
        }
        SDLActivity.wv.getSettings().setUserAgentString(String.format(Locale.US, "Principia WebView/%d (Android)", version));
        SDLActivity.wv.setWebViewClient(new WebViewClient() {
            @Override
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                Uri uri = Uri.parse(url);
                String host = uri.getHost();

                if (uri.getScheme().equals("principia")) {
                    Log.v("Principia", "set arg "+url);
                    PrincipiaBackend.setarg(url);
                    SDLActivity.wv_dialog.dismiss();
                } else if (true) {
                    // FIXME: Also IF HOST CONTAINS QUERY ?printable=yes
                    Log.v("Principia", "Load url "+url);
                    view.stopLoading();
                    view.loadUrl(url);
                } else {
                    Log.v(TAG, "unhandled url " + url);
                    Log.v(TAG, "host: '" + uri.getHost()+"'");
                    Intent intent = new Intent(Intent.ACTION_VIEW, uri);
                    SDLActivity.mSingleton.startActivity(intent);
                    SDLActivity.wv_dialog.dismiss();
                }

                return true;
            }

            @Override
            public void onPageFinished(WebView view, String url)
            {
                Log.v("Principia", "page finished: " + url);
                pb.setVisibility(View.GONE);
                pb_tv.setVisibility(View.GONE);
            }

            @Override
            public void onPageStarted(WebView view, String url, Bitmap favicon)
            {
                Log.v("Principia", "page started: " + url);
                pb.setVisibility(View.VISIBLE);
                pb_tv.setVisibility(View.VISIBLE);
            }
        });
        ll.addView(SDLActivity.wv);
        Button wv_close = (Button)v.findViewById(R.id.wv_close);
        wv_close.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                SDLActivity.wv_dialog.dismiss();
            }
        });
        Button wv_reload = (Button)v.findViewById(R.id.wv_reload);
        wv_reload.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                SDLActivity.wv.reload();
            }
        });

        SDLActivity.wv_dialog.setContentView(v);
        SDLActivity.wv_dialog.setOnShowListener(this);
        SDLActivity.wv_dialog.setOnDismissListener(this);
        SDLActivity.wv_cm = CookieManager.getInstance();

        SDLActivity.wv_dialog.setOnKeyListener(new OnKeyListener() {
            @Override
            public boolean onKey(DialogInterface dialog, int keyCode,
                    KeyEvent event) {
                if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    if (keyCode == KeyEvent.KEYCODE_BACK) {
                        if (SDLActivity.wv.canGoBack()) {
                            SDLActivity.wv.goBack();
                        } else {
                            SDLActivity.wv_dialog.dismiss();
                        }

                        return true;
                    }
                }

                return false;
            }
        });
    }

    public static boolean is_cool = false;

    public static void open_dialog(final int num)
    {
        SDLActivity.open_dialog(num, false);
    }

    public static void open_dialog(final int num, final boolean is_cool)
    {
        SDLActivity.is_cool = is_cool;

        SDLActivity.mSingleton.runOnUiThread(new Runnable(){
            public void run() {
                if (num == DIALOG_PROMPT) {
                    try {SDLActivity.mSingleton.removeDialog(num);} catch(Exception e){};
                }
                if (num == DIALOG_PLAY_MENU) {
                    try {SDLActivity.mSingleton.removeDialog(num);} catch(Exception e){};
                }
                try {SDLActivity.mSingleton.removeDialog(DIALOG_OPEN);} catch(Exception e){};
                try {SDLActivity.mSingleton.removeDialog(DIALOG_OPEN_STATE);} catch(Exception e){};
                try {SDLActivity.mSingleton.removeDialog(DIALOG_MULTIEMITTER);} catch(Exception e){};
                try {SDLActivity.mSingleton.removeDialog(DIALOG_OPEN_OBJECT);} catch(Exception e){};
                SDLActivity.mSingleton.showDialog(num);
            }
        });
    }

    private List<Dialog> open_dialogs = new ArrayList<Dialog>();

    public static final int LEVEL_LOCAL   = 0;
    public static final int LEVEL_DB      = 1;
    public static final int LEVEL_MAIN    = 2;
    public static final int LEVEL_SYS     = 3;
    public static final int LEVEL_PARTIAL = 4;

    public static final int LEVEL_LOCAL_STATE = 100;
    public static final int LEVEL_DB_STATE    = 101;
    public static final int LEVEL_MAIN_STATE  = 102;

    public static final int ACTION_OPEN = 1;
    public static final int ACTION_RELOAD_GRAPHICS = 2;
    public static final int ACTION_RELOAD_LEVEL = 3;
    public static final int ACTION_SAVE = 4;
    public static final int ACTION_NEW_LEVEL = 5;
    public static final int ACTION_STICKY = 6;
    public static final int ACTION_LOGIN =  7;
    public static final int ACTION_SAVE_COPY = 8;
    public static final int ACTION_CONSTRUCT_ENTITY = 9;
    public static final int ACTION_OPEN_PLAY = 10;
    public static final int ACTION_PUBLISH = 11;
    public static final int ACTION_PLAY_PKG = 12;
    public static final int ACTION_WARP = 13;
    public static final int ACTION_PUBLISH_PKG = 14;
    public static final int ACTION_PING = 15;
    public static final int ACTION_UPGRADE_LEVEL = 16;
    public static final int ACTION_DERIVE = 17;
    public static final int ACTION_SET_STICKY_TEXT = 18;
    public static final int ACTION_IMPORT_OBJECT = 19;
    public static final int ACTION_EXPORT_OBJECT = 20;
    public static final int ACTION_MULTIEMITTER_SET = 21;
    public static final int ACTION_PUZZLEPLAY = 22;
    public static final int ACTION_EDIT = 23;
    public static final int ACTION_AUTOFIT_LEVEL_BORDERS = 24;
    public static final int ACTION_RESTART_LEVEL = 25;
    public static final int ACTION_BACK = 26;
    public static final int ACTION_RESELECT = 27;
    public static final int ACTION_HIGHLIGHT_SELECTED = 28;
    public static final int ACTION_OPEN_AUTOSAVE = 31;
    public static final int ACTION_REMOVE_AUTOSAVE = 32;
    public static final int ACTION_GOTO_MAINMENU = 33;
    public static final int ACTION_DELETE_LEVEL = 34;
    public static final int ACTION_DELETE_PARTIAL = 35;
    public static final int ACTION_SET_LEVEL_TYPE = 36;
    public static final int ACTION_RELOAD_DISPLAY = 37;
    public static final int ACTION_ENTITY_MODIFIED = 38;
    public static final int ACTION_SET_MODE = 39;
    public static final int ACTION_MAIN_MENU_PKG = 40;
    public static final int ACTION_WORLD_PAUSE = 41;
    public static final int ACTION_CONSTRUCT_ITEM = 45;
    public static final int ACTION_SUBMIT_SCORE = 46;
    public static final int ACTION_MULTI_DELETE = 47;
    public static final int ACTION_OPEN_STATE = 48;
    public static final int ACTION_AUTOSAVE = 49;

    public static final int ACTION_MULTI_JOINT_STRENGTH     = 50;
    public static final int ACTION_MULTI_PLASTIC_COLOR      = 51;
    public static final int ACTION_MULTI_PLASTIC_DENSITY    = 52;
    public static final int ACTION_MULTI_CHANGE_CONNECTION_RENDER_TYPE = 53;
    public static final int ACTION_MULTI_UNLOCK_ALL         = 54;
    public static final int ACTION_MULTI_DISCONNECT_ALL     = 55;

    public static final int ACTION_SAVE_STATE = 65;
    public static final int ACTION_SELF_DESTRUCT = 71;

    public static final int DIALOG_SANDBOX_MENU     = 1;
    public static final int DIALOG_QUICKADD         = 100;
    public static final int DIALOG_BEAM_COLOR       = 101;
    public static final int DIALOG_SAVE             = 102;
    public static final int DIALOG_OPEN             = 103;
    public static final int DIALOG_NEW_LEVEL        = 104;
    public static final int DIALOG_SET_FREQUENCY    = 105;
    public static final int DIALOG_PIXEL_COLOR      = 106;
    public static final int DIALOG_CONFIRM_QUIT     = 107;
    public static final int DIALOG_SET_COMMAND      = 108;
    public static final int DIALOG_STICKY           = 109;
    public static final int DIALOG_FXEMITTER        = 110;
    public static final int DIALOG_CAMTARGETER      = 111;
    public static final int DIALOG_SET_FREQ_RANGE   = 112;
    public static final int DIALOG_OPEN_OBJECT      = 113;
    public static final int DIALOG_EXPORT           = 114;
    public static final int DIALOG_SET_PKG_LEVEL    = 115;
    public static final int DIALOG_ROBOT            = 116;
    public static final int DIALOG_MULTIEMITTER     = 117;
    public static final int DIALOG_PUZZLE_PLAY      = 118;
    public static final int DIALOG_TIMER            = 119;
    public static final int DIALOG_EVENTLISTENER    = 120;
    public static final int DIALOG_SETTINGS         = 121;
    public static final int DIALOG_SAVE_COPY        = 122;
    public static final int DIALOG_LEVEL_PROPERTIES = 123;
    public static final int DIALOG_HELP             = 124;
    public static final int DIALOG_DIGITAL_DISPLAY  = 125;
    public static final int DIALOG_PLAY_MENU        = 126;
    public static final int DIALOG_OPEN_AUTOSAVE    = 127;
    public static final int DIALOG_COMMUNITY        = 128;
    public static final int DIALOG_PROMPT_SETTINGS  = 129;
    public static final int DIALOG_PROMPT           = 130;
    public static final int DIALOG_SFX_EMITTER      = 131;
    public static final int DIALOG_VARIABLE         = 132;
    public static final int DIALOG_SYNTHESIZER      = 133;
    public static final int DIALOG_SEQUENCER        = 134;
    public static final int DIALOG_SHAPEEXTRUDER    = 135;
    public static final int DIALOG_JUMPER           = 136;

    public static final int DIALOG_PUBLISHED        = 138;

    public static final int DIALOG_TOUCHFIELD       = 140;
    public static final int DIALOG_ESCRIPT          = 141;
    public static final int DIALOG_ITEM             = 142;

    public static final int DIALOG_SANDBOX_MODE     = 143;

    public static final int DIALOG_RUBBER           = 144;

    public static final int DIALOG_SOUNDMAN         = 148;

    public static final int DIALOG_FACTORY          = 149;

    public static final int DIALOG_SET_FACTION      = 150;
    public static final int DIALOG_RESOURCE         = 151;
    public static final int DIALOG_VENDOR           = 152;
    public static final int DIALOG_ANIMAL           = 153;
    public static final int DIALOG_POLYGON          = 154;
    public static final int DIALOG_KEY_LISTENER     = 155;
    public static final int DIALOG_OPEN_STATE       = 156;
    public static final int DIALOG_POLYGON_COLOR    = 157;
    public static final int DIALOG_MULTI_CONFIG     = 158;
    public static final int DIALOG_EMITTER          = 159;
    public static final int DIALOG_TREASURE_CHEST   = 160;
    public static final int DIALOG_DECORATION       = 161;
    public static final int DIALOG_SFX_EMITTER_2    = 162; // New SFX Emitter dialog (1.5.1+)

    public static final int DIALOG_PUBLISH          = 300;
    public static final int DIALOG_LOGIN            = 301;
    public static final int DIALOG_SANDBOX_TIPS     = 302;
    public static final int DIALOG_REGISTER         = 303;

    public static final int CLOSE_ALL_DIALOGS            = 200;
    public static final int CLOSE_ABSOLUTELY_ALL_DIALOGS = 201;
    public static final int CLOSE_REGISTER_DIALOG        = 202;
    public static final int DISABLE_REGISTER_LOADER      = 203;

    public static final int SIGNAL_LONG_PRESS    = 1000;

    public static final int PROMPT_RESPONSE_NONE = 0;
    public static final int PROMPT_RESPONSE_A    = 1;
    public static final int PROMPT_RESPONSE_B    = 2;
    public static final int PROMPT_RESPONSE_C    = 3;

    public static void showHelpDialog(final String title, final String description)
    {
        SDLActivity.mSingleton.runOnUiThread(new Runnable(){
            public void run() {
                try {SDLActivity.mSingleton.removeDialog(DIALOG_HELP);} catch(Exception e){};
                HelpDialog.title = title;
                HelpDialog.description = description;
                SDLActivity.mSingleton.showDialog(DIALOG_HELP);
            }


        });
    }

    public static void confirm(final String text, final String button1, final int action1, final long action1_data, final String button2, final int action2, final long action2_data, final String button3, final int action3, final long action3_data, final boolean dna_sandbox)
    {
        SDLActivity.mSingleton.runOnUiThread(new Runnable(){
            public void run() {
                new ConfirmDialog()
                .set_listener(new OnOptionSelectedListener() {
                    @Override
                    public void onOptionSelected(int option) {
                        if (option == ConfirmDialog.OPTION_YES) {
                            PrincipiaBackend.addActionAsInt(action1, action1_data);
                        } else if (option == ConfirmDialog.OPTION_NO) {
                            PrincipiaBackend.addActionAsInt(action2, action2_data);
                        } else if (option == ConfirmDialog.OPTION_3) {
                            PrincipiaBackend.addActionAsInt(action3, action3_data);
                        }
                    }
                })
                .run(text, button1, button2, button3, dna_sandbox);
            }
        });
    }

    public static void alert(final String text, final int alert_type)
    {
        SDLActivity.mSingleton.runOnUiThread(new Runnable(){
            public void run() {
                /* hack to prevent killing of immersive mode */
                //mSingleton.getWindow().setFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);

                new AlertDialog.Builder(mSingleton.getContext())
                    .setMessage(text)
                    .setPositiveButton(mSingleton.getString(R.string.close), new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface di, int which) {
                            //di.dismiss();
                        }
                    })
                    .setIcon(android.R.drawable.ic_dialog_info)
                    .show();

                //mSingleton.getWindow().getDecorView().setSystemUiVisibility(mSingleton.getWindow().getDecorView().getSystemUiVisibility());
                //mSingleton.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
            }
        });
    }

    public static void showErrorDialog(final String text)
    {
        SDLActivity.mSingleton.runOnUiThread(new Runnable(){
            public void run() {
                try {SDLActivity.mSingleton.removeDialog(DIALOG_HELP);} catch(Exception e){};
                HelpDialog.title = "Errors";
                HelpDialog.description = text;
                SDLActivity.mSingleton.showDialog(DIALOG_HELP);
            }
        });
    }

    public static void showSandboxTips()
    {
        SDLActivity.mSingleton.runOnUiThread(new Runnable(){
            public void run() {
                try {SDLActivity.mSingleton.removeDialog(DIALOG_SANDBOX_TIPS);} catch(Exception e){};
                SDLActivity.mSingleton.showDialog(DIALOG_SANDBOX_TIPS);
            }
        });
    }

    public static void cleanQuit()
    {
        SDLActivity.mSingleton.runOnUiThread(new Runnable(){
            public void run() {
                SDLActivity.nativeQuit();

                // Now wait for the SDL thread to quit
                if (mSDLThread != null) {
                    try {
                        mSDLThread.join();
                    } catch(Exception e) {
                        Log.v("SDL", "Problem stopping thread: " + e);
                    }
                    mSDLThread = null;

                    //Log.v("SDL", "Finished waiting for SDL thread");
                }


                android.os.Process.killProcess(android.os.Process.myPid());

                SDLActivity.mSingleton.finish();
            }
        });
    }

    @Override
    protected Dialog onCreateDialog(int num)
    {
        Dialog d = null;

        switch (num) {
        case DIALOG_SANDBOX_MENU:
            {
                AlertDialog.Builder bld = new AlertDialog.Builder(this);

                final CharSequence[] items;

                items = new CharSequence[] {
                    "Level properties",
                    "New level",
                    "Save",
                    "Save a copy",
                    "Open",
                    "Publish online",
                    "Settings",
                    "Log in",
                    "Help: Getting started",
                    "Help: Principia Wiki",
                    "Browse levels online",
                    "Back to menu",
                    "Quit"
                };

                bld.setItems(items, new DialogInterface.OnClickListener(){
                    public void onClick(DialogInterface dialog, int which) {
                        /* TODO: Use dialog fragments */
                        switch (which) {
                            case 0: showDialog(DIALOG_LEVEL_PROPERTIES); break;
                            case 1: showDialog(DIALOG_NEW_LEVEL); break;
                            case 2: if (PrincipiaBackend.getLevelName().length() == 0) {SaveAsDialog.refresh_name=true; SaveAsDialog.copy=false; showDialog(DIALOG_SAVE);} else PrincipiaBackend.triggerSave(false);  break;
                            case 3: SaveAsDialog.refresh_name = true; SaveAsDialog.copy = true; showDialog(DIALOG_SAVE_COPY); break;
                            case 4: try {SDLActivity.mSingleton.removeDialog(DIALOG_OPEN);} catch(Exception e){}; showDialog(DIALOG_OPEN); break;
                            case 5: showDialog(DIALOG_PUBLISH); break;
                            case 6: showDialog(DIALOG_SETTINGS); break;
                            case 7: showDialog(DIALOG_LOGIN); break;
                            case 8: SDLActivity.open_url("https://principia-web.se/wiki/Getting_Started"); break;
                            case 9: SDLActivity.open_url("https://principia-web.se/wiki/Main_Page"); break;
                            case 10: SDLActivity.open_url("https://principia-web.se/"); break;
                            case 11: PrincipiaBackend.addActionAsInt(ACTION_GOTO_MAINMENU, 0); break;
                            case 12: SDLActivity.cleanQuit(); break;
                        }

                        dialog.dismiss();
                    }
                });

                d = bld.create();
                break;
            }

        case DIALOG_SETTINGS:
            if (SDLActivity.settings_dialog == null) {
                d = (SDLActivity.settings_dialog = new SettingsDialog()).get_dialog();
            }
            break;

        case DIALOG_QUICKADD:           d = QuickaddDialog.get_dialog(); break;
        case DIALOG_OPEN:               d = (new OpenDialog(false)).get_dialog(); break;
        case DIALOG_LEVEL_PROPERTIES:   d = LevelDialog.get_dialog(); break;
        case DIALOG_SAVE_COPY:          d = SaveAsDialog.get_dialog(); break;
        case DIALOG_SAVE:               d = SaveAsDialog.get_dialog(); break;
        case DIALOG_HELP:               d = (new HelpDialog()).get_dialog(); break;
        case DIALOG_STICKY:             d = StickyDialog.get_dialog(); break;
        case DIALOG_NEW_LEVEL:          d = (new NewLevelDialog()).get_dialog(); break;
        case DIALOG_ROBOT:              d = RobotDialog.get_dialog(); break;
        case DIALOG_CAMTARGETER:        d = CamTargeterDialog.get_dialog(); break;
        case DIALOG_SET_COMMAND:        d = CommandPadDialog.get_dialog(); break;
        case DIALOG_FXEMITTER:          d = FxEmitterDialog.get_dialog(); break;
        case DIALOG_EVENTLISTENER:      d = EventListenerDialog.get_dialog(); break;
        case DIALOG_SET_PKG_LEVEL:      d = PkgLevelDialog.get_dialog(); break;
        case DIALOG_PIXEL_COLOR:        d = ColorChooserDialog.get_dialog(); break;
        case DIALOG_BEAM_COLOR:         d = ColorChooserDialog.get_dialog(); break;
        case DIALOG_POLYGON_COLOR:      d = ColorChooserDialog.get_dialog(); break;
        case DIALOG_DIGITAL_DISPLAY:    d = DigitalDisplayDialog.get_dialog(); break;
        case DIALOG_SET_FREQUENCY:      d = FrequencyDialog.get_dialog(); break;
        case DIALOG_SET_FREQ_RANGE:     d = FrequencyRangeDialog.get_dialog(); break;
        case DIALOG_EXPORT:             d = ExportDialog.get_dialog(); break;
        case DIALOG_MULTIEMITTER:       d = (new ImportDialog(true)).get_dialog(); break;
        case DIALOG_OPEN_OBJECT:        d = (new ImportDialog(false)).get_dialog(); break;
        case DIALOG_TIMER:              d = TimerDialog.get_dialog(); break;
        case DIALOG_PUZZLE_PLAY:        d = (new PuzzlePlayDialog()).get_dialog(); break;
        case DIALOG_PLAY_MENU:          d = (PlayDialog.create_dialog()); break;
        case DIALOG_OPEN_AUTOSAVE:      d = (new AutosaveDialog()).get_dialog(); break;
        case DIALOG_COMMUNITY:          d = (new CommunityDialog()).get_dialog(); break;
        case DIALOG_PROMPT_SETTINGS:    d = PromptSettingsDialog.get_dialog(); break;
        case DIALOG_PROMPT:             d = (new PromptDialog()).get_dialog(); break;
        case DIALOG_SFX_EMITTER:        d = SfxDialog.get_dialog(); break;
        case DIALOG_SFX_EMITTER_2:      d = Sfx2Dialog.get_dialog(); break;
        case DIALOG_VARIABLE:           d = VariableDialog.get_dialog(); break;
        case DIALOG_SYNTHESIZER:        d = SynthesizerDialog.get_dialog(); break;
        case DIALOG_SEQUENCER:          d = SequencerDialog.get_dialog(); break;
        case DIALOG_JUMPER:             d = JumperDialog.get_dialog(); break;
        case DIALOG_TOUCHFIELD:         d = TouchFieldDialog.get_dialog(); break;
        case DIALOG_ESCRIPT:            d = ScriptDialog.get_dialog(); break;
        case DIALOG_ITEM:               d = ConsumableDialog.get_dialog(); break;
        case DIALOG_SANDBOX_MODE:       d = (new ToolDialog()).get_dialog(); break;
        case DIALOG_RUBBER:             d = RubberDialog.get_dialog(); break;
        case DIALOG_SHAPEEXTRUDER:      d = ShapeExtruderDialog.get_dialog(); break;
        case DIALOG_POLYGON:            d = PolygonDialog.get_dialog(); break;
        case DIALOG_KEY_LISTENER:       d = KeyListenerDialog.get_dialog(); break;
        case DIALOG_EMITTER:            d = EmitterDialog.get_dialog(); break;
        case DIALOG_DECORATION:         d = DecorationDialog.get_dialog(); break;
        case DIALOG_ANIMAL:             d = AnimalDialog.get_dialog(); break;
        case DIALOG_SOUNDMAN:           d = SoundManDialog.get_dialog(); break;
        case DIALOG_MULTI_CONFIG:       d = MultiSelectDialog.get_dialog(); break;
        case DIALOG_VENDOR:             d = VendorDialog.get_dialog(); break;

        case DIALOG_FACTORY:            d = FactoryDialog.get_dialog(); break;

        case DIALOG_RESOURCE:           d = ResourceDialog.get_dialog(); break;
        case DIALOG_OPEN_STATE:         d = (new OpenDialog(true)).get_dialog(); break;

        case DIALOG_PUBLISH:            d = PublishDialog.get_dialog(); break;
        case DIALOG_PUBLISHED:          d = (new PublishedDialog()).get_dialog(); break;
        case DIALOG_LOGIN:              d = LoginDialog.get_dialog(); break;
        case DIALOG_SANDBOX_TIPS:       d = (new SandboxTipsDialog()).get_dialog(); break;
        case DIALOG_REGISTER:           d = RegisterDialog.get_dialog(); break;

        case CLOSE_ALL_DIALOGS:         break; /* do nothing */
        case CLOSE_ABSOLUTELY_ALL_DIALOGS:
            SDLActivity.mSingleton.runOnUiThread(new Runnable(){
                public void run() {
                    Log.v("Principia", "Closing all dialogs.");

                    for (Dialog open_dialog : open_dialogs) {
                        Log.v("Principia", "Closing a dialog["+open_dialog.toString()+"]");
                        //open_dialog.dismiss();
                    }
                    open_dialogs.clear();
                }
            });
            break;

        case CLOSE_REGISTER_DIALOG:
            SDLActivity.mSingleton.runOnUiThread(new Runnable(){
                public void run() {
                    RegisterDialog.get_dialog().dismiss();
                }
            });
            break;

        case DISABLE_REGISTER_LOADER:
            SDLActivity.mSingleton.runOnUiThread(new Runnable(){
                public void run() {
                    RegisterDialog.progressbar.setVisibility(View.GONE);
                }
            });
            break;

        case SIGNAL_LONG_PRESS:
            SDLActivity.mSingleton.runOnUiThread(new Runnable(){
                public void run() {
                    Log.v(TAG, "long press!");
                    Vibrator mVibrator = (Vibrator)SDLActivity.getContext().getSystemService(Context.VIBRATOR_SERVICE);

                    long[] mLongPressVibePattern = new long[] {
                        0, 1, 20, 21
                    };
                    mVibrator.vibrate(mLongPressVibePattern, -1);
                }
            });
            break;

        default: Log.e("Principia", "Unhandled UI Dialog: "+num); break;
        }

        if (d != null) {
            Log.v("Principia", "Adding dialog: "+d.toString());
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

            final Level level = SDLActivity.open_adapter.getItem(aInfo.position);

            menu.setHeaderTitle("Options for " + level.get_name());
            menu.add(1, 1, 1, "Delete")
                .setOnMenuItemClickListener(new OnMenuItemClickListener() {
                    @Override
                    public boolean onMenuItemClick(MenuItem item) {
                        new ConfirmDialog()
                        .set_listener(new OnOptionSelectedListener() {
                            @Override
                            public void onOptionSelected(int option) {
                                if (option == ConfirmDialog.OPTION_YES) {
                                    PrincipiaBackend.addActionAsTriple(ACTION_DELETE_LEVEL, level.get_level_type(), level.get_id(), level.get_save_id());
                                    SDLActivity.open_adapter.remove(level);
                                }
                            }
                        })
                        .run("Are you sure you want to delete this level?");

                        return false;
                    }
                });
        } else if (v == ImportDialog.lv) {
            AdapterContextMenuInfo aInfo = (AdapterContextMenuInfo) menuInfo;

            final Level level = ImportDialog.list_adapter.getItem(aInfo.position);

            menu.setHeaderTitle("Options for " + level.get_name());
            menu.add(1, 1, 1, "Delete")
                .setOnMenuItemClickListener(new OnMenuItemClickListener() {
                    @Override
                    public boolean onMenuItemClick(MenuItem item) {
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
                    }
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
                if (SDLActivity.settings_dialog != null) {
                    SDLActivity.settings_dialog.load();
                }
                break;

            case DIALOG_QUICKADD:           QuickaddDialog.prepare(dialog); break;
            case DIALOG_LEVEL_PROPERTIES:   LevelDialog.prepare(dialog); break;
            case DIALOG_SAVE:               SaveAsDialog.prepare(dialog); break;
            case DIALOG_SAVE_COPY:          SaveAsDialog.prepare(dialog); break;
            case DIALOG_ROBOT:              RobotDialog.prepare(dialog); break;
            case DIALOG_CAMTARGETER:        CamTargeterDialog.prepare(dialog); break;
            case DIALOG_SET_COMMAND:        CommandPadDialog.prepare(dialog); break;
            case DIALOG_FXEMITTER:          FxEmitterDialog.prepare(dialog); break;
            case DIALOG_EVENTLISTENER:      EventListenerDialog.prepare(dialog); break;
            case DIALOG_SET_PKG_LEVEL:      PkgLevelDialog.prepare(dialog); break;
            case DIALOG_PIXEL_COLOR:        ColorChooserDialog.prepare(dialog, true); break;
            case DIALOG_BEAM_COLOR:         ColorChooserDialog.prepare(dialog, false); break;
            case DIALOG_POLYGON_COLOR:      ColorChooserDialog.prepare(dialog, false); break;
            case DIALOG_DIGITAL_DISPLAY:    DigitalDisplayDialog.prepare(dialog); break;
            case DIALOG_SET_FREQUENCY:      FrequencyDialog.prepare(dialog); break;
            case DIALOG_SET_FREQ_RANGE:     FrequencyRangeDialog.prepare(dialog); break;
            case DIALOG_EXPORT:             ExportDialog.prepare(dialog); break;
            case DIALOG_STICKY:             StickyDialog.prepare(dialog); break;
            case DIALOG_TIMER:              TimerDialog.prepare(dialog); break;
            case DIALOG_PROMPT_SETTINGS:    PromptSettingsDialog.prepare(dialog); break;
            case DIALOG_SFX_EMITTER:        SfxDialog.prepare(dialog); break;
            case DIALOG_SFX_EMITTER_2:      Sfx2Dialog.prepare(dialog); break;
            case DIALOG_VARIABLE:           VariableDialog.prepare(dialog); break;
            case DIALOG_SYNTHESIZER:        SynthesizerDialog.prepare(dialog); break;
            case DIALOG_SEQUENCER:          SequencerDialog.prepare(dialog); break;
            case DIALOG_JUMPER:             JumperDialog.prepare(dialog); break;
            case DIALOG_TOUCHFIELD:         TouchFieldDialog.prepare(dialog); break;
            case DIALOG_ESCRIPT:            ScriptDialog.prepare(dialog); break;
            case DIALOG_ITEM:               ConsumableDialog.prepare(dialog); break;
            case DIALOG_RUBBER:             RubberDialog.prepare(dialog); break;
            case DIALOG_SHAPEEXTRUDER:      ShapeExtruderDialog.prepare(dialog); break;
            case DIALOG_POLYGON:            PolygonDialog.prepare(dialog); break;
            case DIALOG_KEY_LISTENER:       KeyListenerDialog.prepare(dialog); break;
            case DIALOG_EMITTER:            EmitterDialog.prepare(dialog); break;
            case DIALOG_DECORATION:         DecorationDialog.prepare(dialog); break;
            case DIALOG_ANIMAL:             AnimalDialog.prepare(dialog); break;
            case DIALOG_SOUNDMAN:           SoundManDialog.prepare(dialog); break;
            case DIALOG_MULTI_CONFIG:       MultiSelectDialog.prepare(dialog); break;
            case DIALOG_VENDOR:             VendorDialog.prepare(dialog); break;

            case DIALOG_FACTORY:            FactoryDialog.prepare(dialog); break;

            case DIALOG_RESOURCE:           ResourceDialog.prepare(dialog); break;

            case DIALOG_PUBLISH:            PublishDialog.prepare(dialog); break;
            case DIALOG_LOGIN:              LoginDialog.prepare(dialog); break;
        }

        /* Dialogs that need a separate onShowListener */
        switch (d) {
            case DIALOG_QUICKADD:
            case DIALOG_PUBLISH:
            case DIALOG_LOGIN:
            case DIALOG_SANDBOX_TIPS:
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
/*
    public void onCancel(DialogInterface dialog)
    {
        this.num_dialogs --;
        Log.v("CANCELLLLLLLLLLLLLLLLLLLLLLLLLLLL", "CANBCELLLLLLLLLLLLLLLLLLLLLL "+this.num_dialogs);
        if (this.num_dialogs <= 0){
            this.num_dialogs = 0;
            PrincipiaActivity.focusGL(true);
        }
    }
*/


    public void onDismiss(DialogInterface dialog)
    {
        Log.v("Principia", "dialog onDismiss called");
        open_dialogs.remove(dialog);
        SDLActivity.num_dialogs --;
        if (SDLActivity.num_dialogs <= 0){
            SDLActivity.num_dialogs = 0;
            PrincipiaBackend.focusGL(true);
        }

        enableImmersiveMode();
    }

    public void onShow(DialogInterface dialog) {
        Log.v("Principia", "dialog onShow called");
        this.open_dialogs.add((Dialog) dialog);
        SDLActivity.num_dialogs  ++;
        if (SDLActivity.num_dialogs == 1) {
            PrincipiaBackend.focusGL(false);
        }
    }

    public static void on_show(DialogInterface dialog) {
        Log.v("Principia", "dialog onShow called");
        SDLActivity.num_dialogs ++;
        if (SDLActivity.num_dialogs == 1) {
            PrincipiaBackend.focusGL(false);
        }
    }

    @Override
    public void onProgressChanged(SeekBar sb, int progress,
            boolean fromUser) {
        Log.v("Principia", "Progress changed");
        if (sb == SynthesizerDialog.synth_pulse_width) {
            SynthesizerDialog.synth_pulse_width_tv.setText(String.format(Locale.US, "%.3f", ((float)progress) / 100.f));
        } else if (sb == SynthesizerDialog.synth_bitcrushing) {
            SynthesizerDialog.synth_bitcrushing_tv.setText(Integer.toString(progress));
        } else if (sb == SynthesizerDialog.synth_volume_vibrato_hz) {
            SynthesizerDialog.synth_volume_vibrato_hz_tv.setText(Integer.toString(progress));
        } else if (sb == SynthesizerDialog.synth_volume_vibrato_extent) {
            SynthesizerDialog.synth_volume_vibrato_extent_tv.setText(String.format(Locale.US, "%.3f", ((float)progress) / 100.f));
        } else if (sb == SynthesizerDialog.synth_freq_vibrato_hz) {
            SynthesizerDialog.synth_freq_vibrato_hz_tv.setText(Integer.toString(progress));
        } else if (sb == SynthesizerDialog.synth_freq_vibrato_extent) {
            SynthesizerDialog.synth_freq_vibrato_extent_tv.setText(String.format(Locale.US, "%.3f", ((float)progress) / 100.f));
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        // TODO Auto-generated method stub

    }

    /*
    @Override
    public boolean onKeyDown(int keycode, KeyEvent event)
    {
        if (keycode == KeyEvent.KEYCODE_MENU) {
            SDLActivity.onNativeKeyDown(keycode);
            Log.v("MENU BUTTON", "MENU_BUTTON");
            return true;
        }
        return false;
    }*/
}

/**
    Simple nativeInit() runnable
*/
class SDLMain implements Runnable {
    public void run() {
        // Runs SDL_main()
        SDLActivity.nativeInit();

        //Log.v("SDL", "SDL thread terminated");
    }
}


/**
    SDLSurface. This is what we draw on, so we need to know when it's created
    in order to do anything useful.

    Because of this, that's where we set up the SDL thread
*/
class SDLSurface extends SurfaceView implements SurfaceHolder.Callback,
    View.OnKeyListener, View.OnTouchListener, SensorEventListener  {

    // Sensors
    private static SensorManager mSensorManager;

    // Keep track of the surface size to normalize touch events
    private static float mWidth, mHeight;

    // Startup
    public SDLSurface(Context context) {
        super(context);
        getHolder().addCallback(this);

        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
        setOnKeyListener(this);
        setOnTouchListener(this);

        mSensorManager = (SensorManager)context.getSystemService(Context.SENSOR_SERVICE);

        // Some arbitrary defaults to avoid a potential division by zero
        mWidth = 1.0f;
        mHeight = 1.0f;
    }


    // Called when we have a valid drawing surface
    public void surfaceCreated(SurfaceHolder holder) {
        Log.v("SDL", "surfaceCreated()");
        holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
        enableSensor(Sensor.TYPE_ACCELEROMETER, true);
    }

    // Called when we lose the surface
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.v("SDL", "surfaceDestroyed()");
        if (!SDLActivity.mIsPaused) {
            SDLActivity.mIsPaused = true;
            SDLActivity.mPauseState = true;
            SDLActivity.nativePause();
            PrincipiaBackend.setPaused(true);
        }
        enableSensor(Sensor.TYPE_ACCELEROMETER, false);
    }

    // Called when the surface is resized
    public void surfaceChanged(SurfaceHolder holder,
                               int format, int width, int height) {
        Log.v("SDL", "surfaceChanged()");

        int sdlFormat = 0x85151002; // SDL_PIXELFORMAT_RGB565 by default
        switch (format) {
        case PixelFormat.A_8:
            Log.v("SDL", "pixel format A_8");
            break;
        case PixelFormat.LA_88:
            Log.v("SDL", "pixel format LA_88");
            break;
        case PixelFormat.L_8:
            Log.v("SDL", "pixel format L_8");
            break;
        case PixelFormat.RGBA_4444:
            Log.v("SDL", "pixel format RGBA_4444");
            sdlFormat = 0x85421002; // SDL_PIXELFORMAT_RGBA4444
            break;
        case PixelFormat.RGBA_5551:
            Log.v("SDL", "pixel format RGBA_5551");
            sdlFormat = 0x85441002; // SDL_PIXELFORMAT_RGBA5551
            break;
        case PixelFormat.RGBA_8888:
            Log.v("SDL", "pixel format RGBA_8888");
            sdlFormat = 0x86462004; // SDL_PIXELFORMAT_RGBA8888
            break;
        case PixelFormat.RGBX_8888:
            Log.v("SDL", "pixel format RGBX_8888");
            sdlFormat = 0x86262004; // SDL_PIXELFORMAT_RGBX8888
            break;
        case PixelFormat.RGB_332:
            Log.v("SDL", "pixel format RGB_332");
            sdlFormat = 0x84110801; // SDL_PIXELFORMAT_RGB332
            break;
        case PixelFormat.RGB_565:
            Log.v("SDL", "pixel format RGB_565");
            sdlFormat = 0x85151002; // SDL_PIXELFORMAT_RGB565
            break;
        case PixelFormat.RGB_888:
            Log.v("SDL", "pixel format RGB_888");
            // Not sure this is right, maybe SDL_PIXELFORMAT_RGB24 instead?
            sdlFormat = 0x86161804; // SDL_PIXELFORMAT_RGB888
            break;
        default:
            Log.v("SDL", "pixel format unknown " + format);
            break;
        }

        mWidth = (float) width;
        mHeight = (float) height;
        /* get the screen's density */
        DisplayMetrics dm = new DisplayMetrics();
        ((SDLActivity)SDLActivity.getContext()).getWindowManager().getDefaultDisplay().getMetrics(dm);
        SDLActivity.onNativeResize(width, height, sdlFormat, dm.xdpi, dm.ydpi);
        Log.v("SDL", "Window size:" + width + "x"+height);

        SDLActivity.startApp();
        System.gc();
    }

    // unused
    public void onDraw(Canvas canvas) {}

    // Key events
    public boolean onKey(View  v, int keyCode, KeyEvent event)
    {
        /* Keycodes we should ignore */
        if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN || keyCode == KeyEvent.KEYCODE_VOLUME_UP || keyCode == KeyEvent.KEYCODE_VOLUME_MUTE) {
            return false;
        }

        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            Log.v("SDL", "key down: " + keyCode);
            SDLActivity.onNativeKeyDown(keyCode);
            return true;
        } else if (event.getAction() == KeyEvent.ACTION_UP) {
            Log.v("SDL", "key up: " + keyCode);
            SDLActivity.onNativeKeyUp(keyCode);
            return true;
        }

        return false;
    }

    // Touch events
    public boolean onTouch(View v, MotionEvent event) {
        {
             final int touchDevId = event.getDeviceId();
             final int pointerCount = event.getPointerCount();
             // touchId, pointerId, action, x, y, pressure
             int actionPointerIndex = (event.getAction() & MotionEvent.ACTION_POINTER_ID_MASK) >> MotionEvent. ACTION_POINTER_ID_SHIFT; /* API 8: event.getActionIndex(); */
             int pointerFingerId = event.getPointerId(actionPointerIndex);
             int action = (event.getAction() & MotionEvent.ACTION_MASK); /* API 8: event.getActionMasked(); */

             float x = event.getX(actionPointerIndex) / mWidth;
             float y = event.getY(actionPointerIndex) / mHeight;
             float p = event.getPressure(actionPointerIndex);

             if (action == MotionEvent.ACTION_MOVE && pointerCount > 1) {
                // TODO send motion to every pointer if its position has
                // changed since prev event.
                for (int i = 0; i < pointerCount; i++) {
                    pointerFingerId = event.getPointerId(i);
                    x = event.getX(i) / mWidth;
                    y = event.getY(i) / mHeight;
                    p = event.getPressure(i);
                    SDLActivity.onNativeTouch(touchDevId, pointerFingerId, action, x, y, p);
                }
             } else {
                SDLActivity.onNativeTouch(touchDevId, pointerFingerId, action, x, y, p);
             }
        }
      return true;
   }

    // Sensor events
    public void enableSensor(int sensortype, boolean enabled) {
        // TODO: This uses getDefaultSensor - what if we have >1 accels?
        if (enabled) {
            mSensorManager.registerListener(this,
                            mSensorManager.getDefaultSensor(sensortype),
                            SensorManager.SENSOR_DELAY_GAME, null);
        } else {
            mSensorManager.unregisterListener(this,
                            mSensorManager.getDefaultSensor(sensortype));
        }
    }

    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        // TODO
    }

    public void onSensorChanged(SensorEvent event) {
        if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
            SDLActivity.onNativeAccel(event.values[0] / SensorManager.GRAVITY_EARTH,
                                      event.values[1] / SensorManager.GRAVITY_EARTH,
                                      event.values[2] / SensorManager.GRAVITY_EARTH);
        }
    }
}
