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

package com.android.test.util.dismissdialogs;

import android.app.Activity;
import android.app.Instrumentation;
import android.os.Bundle;
import android.os.Environment;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.aupt.UiWatchers;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.Until;
import android.util.Log;

import android.platform.test.helpers.IStandardAppHelper;
import android.platform.test.helpers.ChromeHelperImpl;
import android.platform.test.helpers.GoogleCameraHelperImpl;
import android.platform.test.helpers.GoogleKeyboardHelperImpl;
import android.platform.test.helpers.GmailHelperImpl;
import android.platform.test.helpers.MapsHelperImpl;
import android.platform.test.helpers.PhotosHelperImpl;
import android.platform.test.helpers.PlayMoviesHelperImpl;
import android.platform.test.helpers.PlayMusicHelperImpl;
import android.platform.test.helpers.PlayStoreHelperImpl;
import android.platform.test.helpers.YouTubeHelperImpl;
import android.support.test.launcherhelper.ILauncherStrategy;
import android.support.test.launcherhelper.LauncherStrategyFactory;

import java.io.File;
import java.io.IOException;
import java.lang.NoSuchMethodException;
import java.lang.InstantiationException;
import java.lang.IllegalAccessException;
import java.lang.ReflectiveOperationException;
import java.lang.reflect.InvocationTargetException;
import java.util.HashMap;
import java.util.Map;

/**
 * A utility to dismiss all predictable, relevant one-time dialogs
 */
public class DismissDialogsInstrumentation extends Instrumentation {
    private static final String LOG_TAG = DismissDialogsInstrumentation.class.getSimpleName();
    private static final String IMAGE_SUBFOLDER = "dialog-dismissal";

    private static final long INIT_TIMEOUT = 20000;
    private static final long MAX_INIT_RETRIES = 5;

    // Comma-separated value indicating for which apps to dismiss dialogs
    private static final String PARAM_APP = "apps";
    // Boolean to indicate if this should take screenshots to document dismissal
    private static final String PARAM_SCREENSHOTS = "screenshots";
    // Boolean to indicate if this should quit if any failure occurs
    private static final String PARAM_QUIT_ON_ERROR = "quitOnError";

    // Key for status bundles provided when running the preparer
    private static final String BUNDLE_DISMISSED_APP_KEY = "dismissedApp";
    private static final String BUNDLE_APP_ERROR_KEY = "appError";

    private Map<String, Class<? extends IStandardAppHelper>> mKeyHelperMap;
    private String[] mApps;
    private boolean mScreenshots;
    private boolean mQuitOnError;
    private UiDevice mDevice;

    @Override
    public void onCreate(Bundle arguments) {
        super.onCreate(arguments);

        mKeyHelperMap = new HashMap<String, Class<? extends IStandardAppHelper>>();
        mKeyHelperMap.put("Chrome", ChromeHelperImpl.class);
        mKeyHelperMap.put("GoogleCamera", GoogleCameraHelperImpl.class);
        mKeyHelperMap.put("GoogleKeyboard", GoogleKeyboardHelperImpl.class);
        mKeyHelperMap.put("Gmail", GmailHelperImpl.class);
        mKeyHelperMap.put("Maps", MapsHelperImpl.class);
        mKeyHelperMap.put("Photos", PhotosHelperImpl.class);
        mKeyHelperMap.put("PlayMovies", PlayMoviesHelperImpl.class);
        mKeyHelperMap.put("PlayMusic", PlayMusicHelperImpl.class);
        mKeyHelperMap.put("PlayStore", PlayStoreHelperImpl.class);
        //mKeyHelperMap.put("Settings", SettingsHelperImpl.class);
        mKeyHelperMap.put("YouTube", YouTubeHelperImpl.class);

        String appsString = arguments.getString(PARAM_APP);
        if (appsString == null) {
            throw new IllegalArgumentException("Missing 'apps' parameter.");
        }
        mApps = appsString.split(",");

        String screenshotsString = arguments.getString(PARAM_SCREENSHOTS);
        if (screenshotsString == null) {
            Log.i(LOG_TAG, "No 'screenshots' parameter. Defaulting to true.");
            mScreenshots = true;
        } else {
            mScreenshots = "true".equals(screenshotsString);
        }

        String quitString = arguments.getString(PARAM_QUIT_ON_ERROR);
        if (quitString == null) {
            Log.i(LOG_TAG, "No 'quitOnError' parameter. Defaulting to quit on error.");
            mQuitOnError = true;
        } else {
            mQuitOnError = "true".equals(quitString);
        }

        start();
    }

    @Override
    public void onStart() {
        super.onStart();

        mDevice = UiDevice.getInstance(this);

        UiWatchers watcherManager = new UiWatchers();
        watcherManager.registerAnrAndCrashWatchers(this);

        takeScreenDump("init", "pre-setup");

        try {
            mDevice.setOrientationNatural();
        } catch (RemoteException e) {
            Log.e(LOG_TAG, "Unable to set device orientation.", e);
        }

        for (int retry = 1; retry <= MAX_INIT_RETRIES; retry++) {
            ILauncherStrategy launcherStrategy =
                    LauncherStrategyFactory.getInstance(mDevice).getLauncherStrategy();
            boolean foundHome = mDevice.wait(Until.hasObject(
                    launcherStrategy.getWorkspaceSelector()), INIT_TIMEOUT);
            if (foundHome) {
                sendStatusUpdate(Activity.RESULT_OK, "launcher");
                break;
            } else {
                takeScreenDump("init", String.format("launcher-selection-failure-%d", retry));
                if (retry == MAX_INIT_RETRIES && mQuitOnError) {
                    throw new RuntimeException("Unable to select launcher workspace. Quitting.");
                } else {
                    sendStatusUpdate(Activity.RESULT_CANCELED, "launcher");
                    Log.e(LOG_TAG, "Failed to find home selector; try #" + retry);
                    // HACK: Try to poke at UI to fix accessibility issue (b/21448825)
                    try {
                        mDevice.sleep();
                        SystemClock.sleep(1000);
                        mDevice.wakeUp();
                        mDevice.pressMenu();
                        UiDevice.getInstance(this).pressHome();
                    } catch (RemoteException e) {
                        Log.e(LOG_TAG, "Failed to avoid UI bug b/21448825.", e);
                    }
                }
            }
        }

        for (String app : mApps) {
            Log.i(LOG_TAG, String.format("Dismissing dialogs for app, %s.", app));
            try {
                if (!dismissDialogs(app)) {
                    throw new IllegalArgumentException(
                            String.format("Unrecognized app \"%s\"", mApps));
                } else {
                    sendStatusUpdate(Activity.RESULT_OK, app);
                }
            } catch (ReflectiveOperationException e) {
                if (mQuitOnError) {
                    quitWithError(app, e);
                } else {
                    sendStatusUpdate(Activity.RESULT_CANCELED, app);
                    Log.w(LOG_TAG, "ReflectiveOperationException. Continuing with dismissal.", e);
                }
            } catch (RuntimeException e) {
                if (mQuitOnError) {
                    quitWithError(app, e);
                } else {
                    sendStatusUpdate(Activity.RESULT_CANCELED, app);
                    Log.w(LOG_TAG, "RuntimeException. Continuing with dismissal.", e);
                }
            } catch (AssertionError e) {
                if (mQuitOnError) {
                    quitWithError(app, new Exception(e));
                } else {
                    sendStatusUpdate(Activity.RESULT_CANCELED, app);
                    Log.w(LOG_TAG, "AssertionError. Continuing with dismissal.", e);
                }
            }

            // Always return to the home page after dismissal
            UiDevice.getInstance(this).pressHome();
        }

        watcherManager.removeAnrAndCrashWatchers(this);

        finish(Activity.RESULT_OK, new Bundle());
    }

    private boolean dismissDialogs(String app) throws NoSuchMethodException, InstantiationException,
            IllegalAccessException, InvocationTargetException {
        try {
            if (mKeyHelperMap.containsKey(app)) {
                Class<? extends IStandardAppHelper> appHelperClass = mKeyHelperMap.get(app);
                IStandardAppHelper helper =
                        appHelperClass.getDeclaredConstructor(Instrumentation.class).newInstance(this);
                takeScreenDump(app, "-dialog1-pre-open");
                helper.open();
                takeScreenDump(app, "-dialog2-pre-dismissal");
                helper.dismissInitialDialogs();
                takeScreenDump(app, "-dialog3-post-dismissal");
                helper.exit();
                takeScreenDump(app, "-dialog4-post-exit");
                return true;
            } else {
                return false;
            }
        } catch (Exception | AssertionError e) {
            takeScreenDump(app, "-exception");
            throw e;
        }
    }

    private void sendStatusUpdate(int code, String app) {
        Bundle result = new Bundle();
        result.putString(BUNDLE_DISMISSED_APP_KEY, app);
        sendStatus(code, result);
    }

    private void quitWithError(String app, Exception exception) {
        Log.e(LOG_TAG, "Quitting with exception.", exception);
        // Pass Bundle with debugging information to TF
        Bundle result = new Bundle();
        result.putString(BUNDLE_DISMISSED_APP_KEY, app);
        result.putString(BUNDLE_APP_ERROR_KEY, exception.toString());
        finish(Activity.RESULT_CANCELED, result);
    }

    private void takeScreenDump(String app, String suffix) {
        if (!mScreenshots) {
            return;
        }

        try {
            File dir = new File(Environment.getExternalStorageDirectory(), IMAGE_SUBFOLDER);
            if (!dir.exists() && !dir.mkdirs()) {
                    throw new RuntimeException(String.format(
                            "Unable to create or find directory, %s.", dir.getPath()));
            }
            File scr = new File(dir, "dd-" + app + suffix + ".png");
            File uix = new File(dir, "dd-" + app + suffix + ".uix");
            Log.v(LOG_TAG, String.format("Screen file path: %s", scr.getPath()));
            Log.v(LOG_TAG, String.format("UI XML file path: %s", uix.getPath()));
            scr.createNewFile();
            uix.createNewFile();
            UiDevice.getInstance(this).takeScreenshot(scr);
            UiDevice.getInstance(this).dumpWindowHierarchy(uix);
        } catch (IOException e) {
            Log.e(LOG_TAG, "Failed screen dump.", e);
        }
    }
}
