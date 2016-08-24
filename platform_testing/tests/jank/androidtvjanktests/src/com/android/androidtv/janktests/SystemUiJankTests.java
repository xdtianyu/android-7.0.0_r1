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

package com.android.androidtv.janktests;

import android.os.Bundle;
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
import android.util.Log;
import java.io.IOException;

/*
 * This class contains the tests for Android TV jank.
 */
public class SystemUiJankTests extends JankTestBase {

    private static final int SHORT_TIMEOUT = 1000;
    private static final int LONG_TIMEOUT = 3000;
    private static final int INNER_LOOP = 4;
    private static final int FLING_SPEED = 12000;
    private static final String LEANBACK_LAUNCHER = "com.google.android.leanbacklauncher";
    private static final String SETTINGS_PACKAGE = "com.android.tv.settings";
    private UiDevice mDevice;

    @Override
    public void setUp() {
        mDevice = UiDevice.getInstance(getInstrumentation());
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
    }

    public void goHome() {
        mDevice.pressHome();
        // Ensure that Home screen is being displayed
        UiObject2 homeScreen = mDevice.wait(
                Until.findObject(By.scrollable(true).res(LEANBACK_LAUNCHER, "main_list_view")),
                SHORT_TIMEOUT);
    }

    public void afterTestHomeScreenNavigation(Bundle metrics) throws IOException {
        super.afterTest(metrics);
    }

    // Measures jank while scrolling down the Home screen
    @JankTest(expectedFrames=100, beforeTest = "goHome",
            afterTest="afterTestHomeScreenNavigation")
    @GfxMonitor(processName=LEANBACK_LAUNCHER)
    public void testHomeScreenNavigation() throws UiObjectNotFoundException {
        // We've already verified that Home screen is being displayed.
        // Scroll up and down the home screen.
        navigateDownAndUpCurrentScreen();
    }

    // Navigates to the Settings row on the Home screen
    public void goToSettingsRow() {
        // Navigate to Home screen and verify that it is being displayed.
        goHome();
        mDevice.wait(Until.findObject(By.scrollable(true).res(LEANBACK_LAUNCHER, "main_list_view")),
                SHORT_TIMEOUT);
        // Look for the row with 'Settings' text.
        // This will ensure that the DPad focus is on the Settings icon.
        int count = 0;
        while (count <= 5 && !(mDevice.hasObject(By.res(LEANBACK_LAUNCHER, "label")
                .text("Settings")))) {
            mDevice.pressDPadDown();
            count++;
        }
        if (!mDevice.hasObject(By.res(LEANBACK_LAUNCHER, "label").text("Settings"))) {
            Log.d(LEANBACK_LAUNCHER, "Couldn't navigate to settings");
        }
    }

    public void afterTestSettings(Bundle metrics) throws IOException {
        // Navigate back home
        goHome();
        super.afterTest(metrics);
    }

    // Measures jank while navigating to Settings from Home and back
    @JankTest(expectedFrames=100, beforeTest="goToSettingsRow",
            afterTest="afterTestSettings")
    @GfxMonitor(processName=SETTINGS_PACKAGE)
    public void testNavigateToSettings() throws UiObjectNotFoundException {
        for (int i = 0; i < INNER_LOOP * 10; i++) {
            // Press DPad center button to navigate to settings.
            mDevice.pressDPadCenter();
            // Press Back button to go back to the Home screen with focus on Settings
            mDevice.pressBack();
        }
    }

    // Navigates to the Settings Screen
    public void goToSettings() {
        goToSettingsRow();
        mDevice.pressDPadCenter();
    }

    // Measures jank while scrolling on the Settings screen
    @JankTest(expectedFrames=100, beforeTest="goToSettings",
            afterTest="afterTestSettings")
    @GfxMonitor(processName=SETTINGS_PACKAGE)
    public void testSettingsScreenNavigation() throws UiObjectNotFoundException {
        // Ensure that Settings screen is being displayed
        mDevice.wait(Until.findObject(By.scrollable(true).res(SETTINGS_PACKAGE, "container_list")),
                SHORT_TIMEOUT);
        navigateDownAndUpCurrentScreen();
    }

    public void navigateDownAndUpCurrentScreen() {
        for (int i = 0; i < INNER_LOOP; i++) {
            // Press DPad button down eight times in succession
            mDevice.pressDPadDown();
        }
        for (int i = 0; i < INNER_LOOP; i++) {
            // Press DPad button up eight times in succession.
            mDevice.pressDPadUp();
        }
    }
}
