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
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.LargeTest;
import android.util.Log;

import junit.framework.Assert;

public class MainSettingsLargeTests extends InstrumentationTestCase {
    private static final String SETTINGS_PACKAGE = "com.android.settings";
    private static final int TIMEOUT = 2000;
    private static final String PERSONAL_CATEGORY = "Personal";
    private static final String[] sPersonalItems = new String[] {
            "Location", "Security", "Accounts", "Google", "Languages & input", "Backup & reset"
    };
    private static final String SYSTEM_CATEGORY = "System";
    private static final String[] sSystemItems = new String[] {
            "Date & time", "Accessibility", "Printing", "About phone"
    };
    private UiDevice mDevice;
    private Context mContext = null;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mContext = getInstrumentation().getContext();
        try {
            mDevice.setOrientationNatural();
        } catch (RemoteException e) {
            throw new RuntimeException("failed to freeze device orientaion", e);
        }
    }

    @Override
    public void tearDown() throws Exception {
        // Need to finish settings activity
        mDevice.pressHome();
        super.tearDown();
    }

    @LargeTest
    public void testPersonalSettingCategory() throws Exception {
        launchMainSettingsCategory(PERSONAL_CATEGORY, sPersonalItems);
    }

    @LargeTest
    public void testSystemSettingCategory() throws Exception {
        launchMainSettingsCategory(SYSTEM_CATEGORY, sSystemItems);
    }

    private void launchMainSettingsCategory(String category, String[] items) throws Exception {
        launchMainSettings(Settings.ACTION_SETTINGS);
        launchSettingItems(category);
        for (String i : items) {
            launchSettingItems(i);
            Log.d(SETTINGS_PACKAGE, String.format("launch setting: %s", i));
        }
    }

    private void launchMainSettings(String mainSetting) throws Exception {
        mDevice.pressHome();
        Intent settingIntent = new Intent(mainSetting);
        settingIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getInstrumentation().getContext().startActivity(settingIntent);
        Thread.sleep(TIMEOUT * 2);
    }

    private void launchSettingItems(String title) throws Exception {
        int maxAttempt = 5;
        UiObject2 item = null;
        UiObject2 view = null;
        while (maxAttempt-- > 0) {
            item = mDevice.wait(Until.findObject(By.res("android:id/title").text(title)), TIMEOUT);
            if (item == null) {
                view = mDevice.wait(
                        Until.findObject(By.res(SETTINGS_PACKAGE, "main_content")),
                        TIMEOUT);
                view.scroll(Direction.DOWN, 1.0f);
            } else {
                return;
            }
        }
        assertNotNull(String.format("%s in Setting has not been loaded correctly", title), item);
    }
}