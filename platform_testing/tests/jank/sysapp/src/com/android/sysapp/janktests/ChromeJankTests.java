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
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.Until;
import junit.framework.Assert;
import android.platform.test.helpers.ChromeHelperImpl;
import android.support.test.timeresulthelper.TimeResultLogger;

/**
 * Jank test for Chorme apps
 * Open overflow menu
 */

public class ChromeJankTests extends JankTestBase {
    private static final int SHORT_TIMEOUT = 1000;
    private static final int LONG_TIMEOUT = 30000;
    private static final int INNER_LOOP = 5;
    private static final int EXPECTED_FRAMES = 100;
    private static final String PACKAGE_NAME = "com.android.chrome";
    private UiDevice mDevice;
    private ChromeHelperImpl chromeHelper;
    private static final File TIMESTAMP_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(), "autotester.log");
    private static final File RESULTS_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(), "results.log");

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

    public void launchApp(String packageName) throws UiObjectNotFoundException{
        PackageManager pm = getInstrumentation().getContext().getPackageManager();
        Intent appIntent = pm.getLaunchIntentForPackage(packageName);
        appIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getInstrumentation().getContext().startActivity(appIntent);
        SystemClock.sleep(SHORT_TIMEOUT);
    }

    public void launchChrome() throws UiObjectNotFoundException, IOException{
        launchApp(PACKAGE_NAME);
        chromeHelper = new ChromeHelperImpl(getInstrumentation());
        chromeHelper.dismissInitialDialogs();
        getOverflowMenu();
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void afterTestChromeOverflowMenuTap(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    // Measures jank window render for overflow menu tap
    @JankTest(beforeTest="launchChrome", expectedFrames=EXPECTED_FRAMES,
            afterTest="afterTestChromeOverflowMenuTap")
    @GfxMonitor(processName=PACKAGE_NAME)
    public void testChromeOverflowMenuTap() {
        for (int i = 0; i < INNER_LOOP; i++) {
            UiObject2 overflow = getOverflowMenu();
            overflow.click();
            SystemClock.sleep(100);
            mDevice.pressBack();
        }
    }

    public UiObject2 getOverflowMenu() {
        UiObject2 overflow = mDevice.wait(
            Until.findObject(By.desc("More options")), 5 * SHORT_TIMEOUT);
        Assert.assertNotNull("Failed to locate overflow menu", overflow);
        return overflow;
    }
}
