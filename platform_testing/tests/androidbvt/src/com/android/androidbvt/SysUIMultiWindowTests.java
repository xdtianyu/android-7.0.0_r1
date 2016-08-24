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

import android.app.UiAutomation;
import android.content.Context;
import android.content.Intent;
import android.os.RemoteException;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.UiDevice;
import android.test.suitebuilder.annotation.LargeTest;
import android.view.accessibility.AccessibilityWindowInfo;

import junit.framework.TestCase;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import android.util.Log;
public class SysUIMultiWindowTests extends TestCase {
    private static final String CALCULATOR_PACKAGE = "com.google.android.calculator";
    private static final String CALCULATOR_ACTIVITY = "com.android.calculator2.Calculator";
    private static final String SETTINGS_PACKAGE = "com.android.settings";
    private static final int FULLSCREEN = 1;
    private static final int SPLITSCREEN = 3;
    private UiAutomation mUiAutomation = null;
    private UiDevice mDevice;
    private Context mContext = null;
    private AndroidBvtHelper mABvtHelper = null;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mDevice.setOrientationNatural();
        mContext = InstrumentationRegistry.getTargetContext();
        mUiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        mABvtHelper = AndroidBvtHelper.getInstance(mDevice, mContext, mUiAutomation);
    }

    @Override
    public void tearDown() throws Exception {
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    /**
     * Following test ensures any app can be docked from full-screen to split-screen, another can be
     * launched to multiwindow mode and finally, initial app can be brought back to full-screen
     */
    @LargeTest
    public void testLaunchInMultiwindow() throws InterruptedException, RemoteException {
        // Launch calculator in full screen
        Intent launchIntent = mContext.getPackageManager()
                .getLaunchIntentForPackage(CALCULATOR_PACKAGE);
        mContext.startActivity(launchIntent);
        Thread.sleep(mABvtHelper.SHORT_TIMEOUT);
        int taskId = -1;
        // Find task id for launched Calculator package
        List<String> cmdOut = mABvtHelper.executeShellCommand("am stack list");
        for (String line : cmdOut) {
            Pattern pattern = Pattern.compile(String.format(".*taskId=([0-9]+): %s/%s.*",CALCULATOR_PACKAGE, CALCULATOR_ACTIVITY));
            Matcher matcher = pattern.matcher(line);
            if (matcher.find()) {
                taskId = Integer.parseInt(matcher.group(1));
                break;
            }
        }
        assertTrue("Taskid hasn't been found", taskId != -1);
        // Convert calculator to multiwindow mode
        mUiAutomation.executeShellCommand(
                String.format("am stack movetask %d %d true", taskId, SPLITSCREEN));
        Thread.sleep(mABvtHelper.SHORT_TIMEOUT * 2);
        // Launch Settings
        launchIntent = mContext.getPackageManager().getLaunchIntentForPackage(SETTINGS_PACKAGE);
        mContext.startActivity(launchIntent);
        Thread.sleep(mABvtHelper.SHORT_TIMEOUT * 2);
        // Ensure settings is active window
        List<AccessibilityWindowInfo> windows = mUiAutomation.getWindows();
        AccessibilityWindowInfo window = windows.get(windows.size() - 1);
        assertTrue("Settings isn't active window",
                window.getRoot().getPackageName().equals(SETTINGS_PACKAGE));
        // Calculate midpoint for Calculator window and click
        mDevice.click(mDevice.getDisplayHeight() / 4, mDevice.getDisplayWidth() / 2);
        Thread.sleep(mABvtHelper.SHORT_TIMEOUT);
        windows = mUiAutomation.getWindows();
        window = windows.get(windows.size() - 2);
        assertTrue("Calcualtor isn't active window",
                window.getRoot().getPackageName().equals(CALCULATOR_PACKAGE));
        // Make Calculator FullWindow again and ensure Settings package isn't found on window
        mUiAutomation.executeShellCommand(
                String.format("am stack movetask %d %d true", taskId, FULLSCREEN));
        Thread.sleep(mABvtHelper.SHORT_TIMEOUT * 2);
        windows = mUiAutomation.getWindows();
        for(int i = 0; i < windows.size() && windows.get(i).getRoot() != null; ++i) {
            assertFalse("Settings have been found",
                    windows.get(i).getRoot().getPackageName().equals(SETTINGS_PACKAGE));
        }
        mDevice.pressHome();
    }
}
