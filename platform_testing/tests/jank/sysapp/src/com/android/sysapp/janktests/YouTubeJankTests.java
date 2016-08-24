/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.sysapp.janktests;

import java.io.File;
import java.io.IOException;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.jank.GfxMonitor;
import android.support.test.jank.JankTest;
import android.support.test.jank.JankTestBase;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.Until;
import junit.framework.Assert;
import android.support.test.timeresulthelper.TimeResultLogger;

/**
 * Jank test for YouTube recommendation window fling 3 times.
 */

public class YouTubeJankTests extends JankTestBase {
    private static final int LONG_TIMEOUT = 5000;
    private static final int SHORT_TIMEOUT = 1000;
    private static final int INNER_LOOP = 5;
    private static final int EXPECTED_FRAMES = 100;
    private static final String PACKAGE_NAME = "com.google.android.youtube";
    private static final File TIMESTAMP_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(), "autotester.log");
    private static final File RESULTS_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(), "results.log");

    private UiDevice mDevice;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        try {
            mDevice.setOrientationNatural();
        } catch (RemoteException e) {
            throw new RuntimeException("failed to freeze device orientaion", e);
        }
    }

    @Override
    protected void tearDown() throws Exception {
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    public void launchApp(String packageName) throws UiObjectNotFoundException {
        PackageManager pm = getInstrumentation().getContext().getPackageManager();
        Intent appIntent = pm.getLaunchIntentForPackage(packageName);
        appIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getInstrumentation().getContext().startActivity(appIntent);
        SystemClock.sleep(SHORT_TIMEOUT);
    }

    public void launchYouTube () throws UiObjectNotFoundException, IOException {
        launchApp(PACKAGE_NAME);
        dismissCling();
        UiObject2 uiObject = mDevice.wait(
            Until.findObject(By.res(PACKAGE_NAME, "pane_fragment_container")), LONG_TIMEOUT);
        Assert.assertNotNull("Recommendation container is null", uiObject);
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void afterTestYouTubeRecomendation(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    // Measures jank while fling YouTube recommendation
    @JankTest(beforeTest="launchYouTube", expectedFrames=EXPECTED_FRAMES,
            afterTest="afterTestYouTubeRecomendation")
    @GfxMonitor(processName=PACKAGE_NAME)
    public void testYouTubeRecomendationWindowFling() {
        UiObject2 uiObject = mDevice.wait(
                Until.findObject(By.res(PACKAGE_NAME, "pane_fragment_container")), LONG_TIMEOUT);
        Assert.assertNotNull("Recommendation container is null", uiObject);
        for (int i = 0; i < INNER_LOOP; i++) {
            uiObject.scroll(Direction.DOWN, 1.0f);
            SystemClock.sleep(100);
            uiObject.scroll(Direction.UP, 1.0f);
        }
    }

    private void dismissCling() {
        // Dismiss the dogfood splash screen that might appear on first start
        UiObject2 newNavigationDoneBtn = mDevice.wait(Until.findObject(
            By.res(PACKAGE_NAME, "done_button")), LONG_TIMEOUT);
        if (newNavigationDoneBtn != null) {
          newNavigationDoneBtn.click();
        }
        UiObject2 dialog_dismiss_btn = mDevice.wait(Until.findObject(
                By.res(PACKAGE_NAME, "dogfood_warning_dialog_dismiss_button").text("OK")), LONG_TIMEOUT);
        if (dialog_dismiss_btn != null) {
            dialog_dismiss_btn.click();
        }
        UiObject2 welcomeSkip = mDevice.wait(
            Until.findObject(By.res(PACKAGE_NAME, "skip_button").text("Skip")), LONG_TIMEOUT);
        if (welcomeSkip != null) {
            welcomeSkip.click();
        }
        UiObject2 musicFaster = mDevice.wait(
            Until.findObject(By.res(PACKAGE_NAME, "text").text("Find music faster")), LONG_TIMEOUT);
        if (musicFaster != null) {
            UiObject2 ok = mDevice.wait(
                    Until.findObject(By.res(PACKAGE_NAME, "ok").text("OK")), LONG_TIMEOUT);
            Assert.assertNotNull("No 'ok' button to bypass music", ok);
            ok.click();
        }
        UiObject2 laterButton = mDevice.wait(
            Until.findObject(By.res(PACKAGE_NAME, "later_button")), LONG_TIMEOUT);
        if (laterButton != null) {
            laterButton.click();
         }
    }
}
