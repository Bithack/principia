package com.bithack.principia;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.webkit.CookieManager;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.LinearLayout;
import android.view.ViewGroup.LayoutParams;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.util.Locale;

public class WebViewManager {
    private static final String TAG = "PrincipiaActivity";
    public Dialog wv_dialog;
    public WebView wv;
    public CookieManager wv_cm;

    private PrincipiaActivity mSingleton;

    public WebViewManager() {
        mSingleton = PrincipiaActivity.mSingleton;
        init_webview();
    }

    private int getPackageVersion() {
        PackageInfo pi;
        try {
            pi = mSingleton.getPackageManager().getPackageInfo(mSingleton.getPackageName(), 0);
            return pi.versionCode;
        } catch (NameNotFoundException e) {
            return -1;
        }
    }

    private String getUserAgent() {
        return String.format(Locale.US, "Principia WebView/%d (Android)", getPackageVersion());
    }

    @SuppressLint("SetJavaScriptEnabled")
    private void init_webview() {
        wv_dialog = new Dialog(mSingleton, android.R.style.Theme_NoTitleBar_Fullscreen) {
            @Override
            protected void onCreate(Bundle saved_instance) {
                super.onCreate(saved_instance);
                getWindow().setLayout(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
            }
        };

        View v = LayoutInflater.from(mSingleton).inflate(R.layout.webview, null);
        LinearLayout ll = v.findViewById(R.id.wv_ll);
        final ProgressBar pb = v.findViewById(R.id.wv_progress);
        final TextView pb_tv = v.findViewById(R.id.wv_progresstext);

        wv = new WebView(mSingleton);
        wv.getSettings().setJavaScriptEnabled(true);

        wv.getSettings().setUserAgentString(getUserAgent());

        wv.setWebViewClient(new WebViewClient() {
            @Override
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                Uri uri = Uri.parse(url);
                String host = uri.getHost();

                if (uri.getScheme().equals("principia")) {
                    Log.v("Principia", "set arg "+url);
                    PrincipiaBackend.setarg(url);
                    wv_dialog.dismiss();
                } else if (host.contains(PrincipiaBackend.getCommunityHost())) {
                    Log.v("Principia", "Load url "+url);
                    view.stopLoading();
                    view.loadUrl(url);
                } else {
                    Log.v(TAG, "unhandled url " + url);
                    Log.v(TAG, "host: '" + uri.getHost()+"'");
                    Intent intent = new Intent(Intent.ACTION_VIEW, uri);
                    try {
                        PrincipiaActivity.mSingleton.startActivity(intent);
                    } catch (ActivityNotFoundException e) {
                        Log.v(TAG, "No app found to open url: " + url);
                    }
                    wv_dialog.dismiss();
                }

                return true;
            }

            @Override
            public void onPageFinished(WebView view, String url) {
                Log.v("Principia", "page finished: " + url);
                pb.setVisibility(View.GONE);
                pb_tv.setVisibility(View.GONE);
            }

            @Override
            public void onPageStarted(WebView view, String url, Bitmap favicon) {
                Log.v("Principia", "page started: " + url);
                pb.setVisibility(View.VISIBLE);
                pb_tv.setVisibility(View.VISIBLE);
            }
        });
        ll.addView(wv);
        Button wv_close = v.findViewById(R.id.wv_close);
        wv_close.setOnClickListener(v1 -> wv_dialog.dismiss());
        Button wv_reload = v.findViewById(R.id.wv_reload);
        wv_reload.setOnClickListener(v2 -> wv.reload());

        wv_dialog.setContentView(v);
        wv_dialog.setOnShowListener(mSingleton);
        wv_dialog.setOnDismissListener(mSingleton);
        wv_cm = CookieManager.getInstance();

        wv_dialog.setOnKeyListener((dialog, keyCode, event) -> {
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                if (keyCode == KeyEvent.KEYCODE_BACK) {
                    if (wv.canGoBack())
                        wv.goBack();
                    else
                        wv_dialog.dismiss();

                    return true;
                }
            }

            return false;
        });
    }

    public void open_url(final String url) {
        PrincipiaActivity.mSingleton.runOnUiThread(() -> {
            String community_host = PrincipiaBackend.getCommunityHost();
            if (wv_cm != null) {
                String curl_token = PrincipiaBackend.getCookies();

                if (curl_token != null) // we got relevant cookies from curl!
                    wv_cm.setCookie("."+community_host, "_PRINCSECURITY="+curl_token);
            }

            wv.stopLoading();
            wv.loadUrl(url);
            wv_dialog.show();
        });
    }
}
