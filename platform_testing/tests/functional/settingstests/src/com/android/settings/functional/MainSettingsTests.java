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

package android.settings.functional;

import android.content.Context;
import android.content.Intent;
import android.os.RemoteException;
import android.os.SystemClock;
import android.platform.test.helpers.SettingsHelperImpl;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.MediumTest;

import junit.framework.Assert;

/** Component test for verifying basic functionality of Main Setting screen */
public class MainSettingsTests extends InstrumentationTestCase {
    private static final String SETTINGS_PACKAGE = "com.android.settings";
    private static final int TIMEOUT = 2000;
    private static final String WIFI_CATEGORY = "Wireless & networks";
    private static final String[] sWifiItems = new String[] {
            "Wi‑Fi", "Bluetooth", "Data usage", "More"
    };
    private static final String DEVICE_CATEGORY = "Device";
    private static final String[] sDeviceItems = new String[] {
            "Display", "Notifications", "Sound", "Apps", "Storage", "Battery", "Memory",
            "Users"
    };
    private UiDevice mDevice;
    private Context mContext = null;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mContext = getInstrumentation().getContext();
        mDevice.setOrientationNatural();
    }

    @Override
    public void tearDown() throws Exception {
        // Need to finish settings activity
        mDevice.pressHome();
        mDevice.unfreezeRotation();
        mDevice.waitForIdle();
        super.tearDown();
    }

    @MediumTest
    public void testLoadSetting() throws Exception {
        launchMainSettings(Settings.ACTION_SETTINGS);
        UiObject2 settingHeading = mDevice.wait(Until.findObject(By.text("Settings")),
                TIMEOUT);
        assertNotNull("Setting menu has not loaded correctly", settingHeading);
    }

    @MediumTest
    public void testWifiSettingCategory() throws Exception {
        launchMainSettingsCategory(WIFI_CATEGORY, sWifiItems);
    }

    @MediumTest
    public void testDeviceSettingCategory() throws Exception {
        launchMainSettingsCategory(DEVICE_CATEGORY, sDeviceItems);
    }

    private void launchMainSettingsCategory(String category, String[] items) throws Exception {
        SettingsHelper.launchSettingsPage(getInstrumentation().getContext(),
                Settings.ACTION_SETTINGS);
        launchSettingItems(category);
        for (String i : items) {
            launchSettingItems(i);
            Log.d(SETTINGS_PACKAGE, String.format("launch setting: %s", i));
        }
        UiObject2 mainSettings = mDevice.wait(
                Until.findObject(By.res("com.android.settings:id/main_content")),
                TIMEOUT);
        // Scrolling back up twice so we're at the top of the settings list.
        for ( int i = 0; i < 2; i++) {
            mainSettings.scroll(Direction.UP, 1.0f);
        }
    }

    @MediumTest
    public void testSearchSetting() throws Exception {
        launchMainSettings(Settings.ACTION_SETTINGS);
        mDevice.wait(Until.findObject(By.desc("Search settings")), TIMEOUT).click();
        UiObject2 searchBox = mDevice.wait(Until.findObject(By.res("android:id/search_src_text")),
                TIMEOUT);
        InputMethodManager imm = (InputMethodManager) mContext
                .getSystemService(Context.INPUT_METHOD_SERVICE);
        if (!imm.isAcceptingText()) {
            mDevice.wait(Until.findObject(By.desc("Collapse")), TIMEOUT).click();
            assertNotNull("Search Setting has not loaded correctly", searchBox);
        } else {
            mDevice.wait(Until.findObject(By.desc("Collapse")), TIMEOUT).click();
        }
    }

    @MediumTest
    public void testOverflowSetting() throws Exception {
        launchMainSettings(Settings.ACTION_SETTINGS);
        mDevice.wait(Until.findObject(By.desc("More options")), TIMEOUT).click();
        mDevice.wait(Until.findObject(By.text("Help & feedback")), TIMEOUT).click();
        UiObject2 help = mDevice.wait(Until.findObject(By.text("Help")),
                TIMEOUT);
        assertNotNull("Overflow setting has not loaded correctly", help);
    }

    private void launchMainSettings(String mainSetting) throws Exception {
        mDevice.pressHome();
        Intent settingIntent = new Intent(mainSetting);
        settingIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getInstrumentation().getContext().startActivity(settingIntent);
        Thread.sleep(TIMEOUT * 3);
    }

    private void launchSettingItems(String title) throws Exception {
        int maxAttempt = 5;
        UiObject2 item = null;
        UiObject2 view = null;
        while (maxAttempt-- > 0) {
            item = mDevice.wait(Until.findObject(By.res("android:id/title").text(title)), TIMEOUT);
            if (item == null) {
                view = mDevice.wait(
                        Until.findObject(By.res("com.android.settings:id/main_content")),
                        TIMEOUT);
                view.scroll(Direction.DOWN, 1.0f);
            } else {
                return;
            }
        }
        assertNotNull(String.format("%s in Setting has not been loaded correctly", title), item);
    }
}
