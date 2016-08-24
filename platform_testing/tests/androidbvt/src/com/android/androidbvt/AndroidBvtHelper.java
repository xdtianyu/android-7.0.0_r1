/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.androidbvt;

import android.app.DownloadManager;
import android.app.UiAutomation;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.ParcelFileDescriptor;
import android.support.test.uiautomator.UiDevice;
import android.telecom.TelecomManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

/**
 * Defines constants & implements common methods to be used by Framework, SysUI, System e2e BVT
 * tests. Also ensures single instance of this object
 */
public class AndroidBvtHelper {
    public static final String TEST_TAG = "AndroidBVT";
    public static final int SHORT_TIMEOUT = 1000;
    public static final int LONG_TIMEOUT = 5000;
    // 600dp is the threshold value for 7-inch tablets.
    private static final int TABLET_DP_THRESHOLD = 600;
    private static AndroidBvtHelper sInstance = null;
    private Context mContext = null;
    private UiDevice mDevice = null;
    private UiAutomation mUiAutomation = null;

    public AndroidBvtHelper(UiDevice device, Context context, UiAutomation uiAutomation) {
        mContext = context;
        mDevice = device;
        mUiAutomation = uiAutomation;
    }

    public static AndroidBvtHelper getInstance(UiDevice device, Context context,
            UiAutomation uiAutomation) {
        if (sInstance == null) {
            sInstance = new AndroidBvtHelper(device, context, uiAutomation);
        }
        return sInstance;
    }

    public TelecomManager getTelecomManager() {
        return (TelecomManager) mContext.getSystemService(Context.TELECOM_SERVICE);
    }

    public WifiManager getWifiManager() {
        return (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
    }

    public ConnectivityManager getConnectivityManager() {
        return (ConnectivityManager) (ConnectivityManager) mContext
                .getSystemService(Context.CONNECTIVITY_SERVICE);
    }

    public DownloadManager getDownloadManager() {
        return (DownloadManager) (DownloadManager) mContext
                .getSystemService(Context.DOWNLOAD_SERVICE);
    }

    /**
     * Only executes 'adb shell' commands that run in the same process as the runner. Converts output
     * of the command from ParcelFileDescriptior to user friendly list of strings
     * https://developer.android.com/reference/android/app/UiAutomation.html#executeShellCommand(
     * java.lang.String)
     */
    public List<String> executeShellCommand(String cmd) {
        if (cmd == null || cmd.isEmpty()) {
            return null;
        }
        List<String> output = new ArrayList<String>();
        ParcelFileDescriptor pfd = mUiAutomation.executeShellCommand(cmd);
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(new FileInputStream(pfd.getFileDescriptor())))) {
            String line;
            while ((line = reader.readLine()) != null) {
                output.add(line);
            }
        } catch (IOException e) {
            Log.e(TEST_TAG, e.getMessage());
            return null;
        }
        return output;
    }

    /** Returns true if the device is a tablet */
    public boolean isTablet() {
        // Get screen density & screen size from window manager
        WindowManager wm = (WindowManager) mContext.getSystemService(
                Context.WINDOW_SERVICE);
        DisplayMetrics metrics = new DisplayMetrics();
        wm.getDefaultDisplay().getMetrics(metrics);
        // Determines the smallest screen width DP which is
        // calculated as ( pixels * density-independent pixel unit ) / density.
        // http://developer.android.com/guide/practices/screens_support.html.
        int screenDensity = metrics.densityDpi;
        int screenWidth = Math.min(
                metrics.widthPixels, metrics.heightPixels);
        int screenHeight = Math.max(
                metrics.widthPixels, metrics.heightPixels);
        int smallestScreenWidthDp = (Math.min(screenWidth, screenHeight)
                * DisplayMetrics.DENSITY_DEFAULT) / screenDensity;
        return smallestScreenWidthDp >= TABLET_DP_THRESHOLD;
    }
}
