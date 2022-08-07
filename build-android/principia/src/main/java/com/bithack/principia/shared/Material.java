package com.bithack.principia.shared;

import java.util.ArrayList;

import org.libsdl.app.PrincipiaBackend;

import android.util.Log;

public class Material {
    private static ArrayList<String> bgs = null;

    public static ArrayList<String> get_bgs()
    {
        if (bgs == null) {
            bgs = new ArrayList<String>();
            String bg_str = PrincipiaBackend.getAvailableBgs();
            String[] bg_arr = bg_str.split("\n");

            for (int x=0; x<bg_arr.length; ++x) {
                Log.v("p", bg_arr[x]);
                bgs.add(bg_arr[x]);
            }
        }

        return bgs;
    }
}
