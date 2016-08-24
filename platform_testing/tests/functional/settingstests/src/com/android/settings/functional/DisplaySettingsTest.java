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

import android.content.ContentResolver;
import android.provider.Settings;
import android.platform.test.helpers.SettingsHelperImpl;
import android.platform.test.helpers.SettingsHelperImpl.SettingsType;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.Suppress;

import java.util.regex.Pattern;

public class DisplaySettingsTest extends InstrumentationTestCase {

    private static final String PAGE = Settings.ACTION_DISPLAY_SETTINGS;
    private static final int TIMEOUT = 2000;
    private static final FontSetting FONT_SMALL = new FontSetting("Small", 0.85f);
    private static final FontSetting FONT_NORMAL = new FontSetting("Default", 1.00f);
    private static final FontSetting FONT_LARGE = new FontSetting("Large", 1.15f);
    private static final FontSetting FONT_HUGE = new FontSetting("Largest", 1.30f);

    private UiDevice mDevice;
    private ContentResolver mResolver;
    private SettingsHelperImpl mHelper;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mDevice.setOrientationNatural();
        mResolver = getInstrumentation().getContext().getContentResolver();
        mHelper = new SettingsHelperImpl(getInstrumentation());
    }

    @Override
    public void tearDown() throws Exception {
        // reset settings we touched that may impact others
        Settings.System.putFloat(mResolver, Settings.System.FONT_SCALE, 1.00f);
        mDevice.waitForIdle();
        super.tearDown();
    }

    @MediumTest
    public void testAdaptiveBrightness() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        mHelper.scrollVert(true);
        Thread.sleep(1000);
        assertTrue(mHelper.verifyToggleSetting(SettingsType.SYSTEM, PAGE, "Adaptive brightness",
                Settings.System.SCREEN_BRIGHTNESS_MODE));
        assertTrue(mHelper.verifyToggleSetting(SettingsType.SYSTEM, PAGE, "Adaptive brightness",
                Settings.System.SCREEN_BRIGHTNESS_MODE));
    }

    @MediumTest
    public void testCameraDoubleTap() throws Exception {
        assertTrue(mHelper.verifyToggleSetting(SettingsType.SECURE, PAGE,
                "Press power button twice for camera",
                Settings.Secure.CAMERA_DOUBLE_TAP_POWER_GESTURE_DISABLED));
        assertTrue(mHelper.verifyToggleSetting(SettingsType.SECURE, PAGE,
                "Press power button twice for camera",
                Settings.Secure.CAMERA_DOUBLE_TAP_POWER_GESTURE_DISABLED));
    }

    @MediumTest
    public void testAmbientDisplay() throws Exception {
        // unique to the ambient display setting, null is equivalent to "on",
        // so we need to populate the setting if it hasn't been yet
        String initialSetting = Settings.Secure.getString(mResolver, Settings.Secure.DOZE_ENABLED);
        if (initialSetting == null) {
            Settings.Secure.putString(mResolver, Settings.Secure.DOZE_ENABLED, "1");
        }
        assertTrue(mHelper.verifyToggleSetting(SettingsType.SECURE, PAGE, "Ambient display",
                Settings.Secure.DOZE_ENABLED));
        assertTrue(mHelper.verifyToggleSetting(SettingsType.SECURE, PAGE, "Ambient display",
                Settings.Secure.DOZE_ENABLED));
    }

    // blocked on b/27487224
    @MediumTest
    @Suppress
    public void testDaydreamToggle() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        Pattern p = Pattern.compile("On|Off");
        mHelper.clickSetting("Screen saver");
        Thread.sleep(1000);
        try {
            assertTrue(mHelper.verifyToggleSetting(SettingsType.SECURE, PAGE, p,
                    Settings.Secure.SCREENSAVER_ENABLED, false));
            assertTrue(mHelper.verifyToggleSetting(SettingsType.SECURE, PAGE, p,
                    Settings.Secure.SCREENSAVER_ENABLED, false));
        } finally {
            mDevice.pressBack();
        }
    }

    @MediumTest
    public void testAccelRotation() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        mHelper.scrollVert(true);
        Thread.sleep(4000);
        String[] buttons = {
                "Rotate the contents of the screen",
                "Stay in portrait view"
        };
        int currentAccelSetting = Settings.System.getInt(
                mResolver, Settings.System.ACCELEROMETER_ROTATION);
        mHelper.scrollVert(false);
        mHelper.clickSetting("When device is rotated");
        assertTrue(mHelper.verifyToggleSetting(SettingsType.SYSTEM, PAGE,
                buttons[currentAccelSetting], Settings.System.ACCELEROMETER_ROTATION, false));
        mHelper.scrollVert(false);
        mHelper.clickSetting("When device is rotated");
        assertTrue(mHelper.verifyToggleSetting(SettingsType.SYSTEM, PAGE,
                buttons[1 - currentAccelSetting], Settings.System.ACCELEROMETER_ROTATION, false));
    }

    @MediumTest
    public void testDaydream() throws Exception {
        Settings.Secure.putInt(mResolver, Settings.Secure.SCREENSAVER_ENABLED, 1);
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        try {
            assertTrue(mHelper.verifyRadioSetting(SettingsType.SECURE, PAGE,
                    "Screen saver", "Clock", Settings.Secure.SCREENSAVER_COMPONENTS,
                    "com.google.android.deskclock/com.android.deskclock.Screensaver"));
            assertTrue(mHelper.verifyRadioSetting(SettingsType.SECURE, PAGE,
                    null, "Colors", Settings.Secure.SCREENSAVER_COMPONENTS,
                    "com.android.dreams.basic/com.android.dreams.basic.Colors"));
            assertTrue(mHelper.verifyRadioSetting(SettingsType.SECURE, PAGE,
                    null, "Photos", Settings.Secure.SCREENSAVER_COMPONENTS,
                    "com.google.android.apps.photos/com.google.android.apps.photos.daydream.PhotosDreamService"));
        } finally {
            mDevice.pressBack();
            Thread.sleep(2000);
        }
    }

    @MediumTest
    public void testSleep15Seconds() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        assertTrue(mHelper.verifyRadioSetting(SettingsType.SYSTEM, PAGE,
                "Sleep", "15 seconds", Settings.System.SCREEN_OFF_TIMEOUT, "15000"));
    }

    @MediumTest
    public void testSleep30Seconds() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        assertTrue(mHelper.verifyRadioSetting(SettingsType.SYSTEM, PAGE,
                "Sleep", "30 seconds", Settings.System.SCREEN_OFF_TIMEOUT, "30000"));
    }

    @MediumTest
    public void testSleep1Minute() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        assertTrue(mHelper.verifyRadioSetting(SettingsType.SYSTEM, PAGE,
                "Sleep", "1 minute", Settings.System.SCREEN_OFF_TIMEOUT, "60000"));
    }

    @MediumTest
    public void testSleep2Minutes() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        assertTrue(mHelper.verifyRadioSetting(SettingsType.SYSTEM, PAGE,
                "Sleep", "2 minutes", Settings.System.SCREEN_OFF_TIMEOUT, "120000"));
    }

    @MediumTest
    public void testSleep5Minutes() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        assertTrue(mHelper.verifyRadioSetting(SettingsType.SYSTEM, PAGE,
                "Sleep", "5 minutes", Settings.System.SCREEN_OFF_TIMEOUT, "300000"));
    }

    @MediumTest
    public void testSleep10Minutes() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        assertTrue(mHelper.verifyRadioSetting(SettingsType.SYSTEM, PAGE,
                "Sleep", "10 minutes", Settings.System.SCREEN_OFF_TIMEOUT, "600000"));
    }

    @MediumTest
    public void testSleep30Minutes() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        assertTrue(mHelper.verifyRadioSetting(SettingsType.SYSTEM, PAGE,
                "Sleep", "30 minutes", Settings.System.SCREEN_OFF_TIMEOUT, "1800000"));
    }

    @MediumTest
    public void testFontSizeLarge() throws Exception {
        verifyFontSizeSetting(1.00f, FONT_LARGE);
        // Leaving the font size at large can make later tests fail, so reset it
        Settings.System.putFloat(mResolver, Settings.System.FONT_SCALE, 1.00f);
        // It takes a second for the new font size to be picked up
        Thread.sleep(2000);
    }

    @MediumTest
    public void testFontSizeDefault() throws Exception {
        verifyFontSizeSetting(1.15f, FONT_NORMAL);
    }

    @MediumTest
    public void testFontSizeLargest() throws Exception {
        verifyFontSizeSetting(1.00f, FONT_HUGE);
        // Leaving the font size at huge can make later tests fail, so reset it
        Settings.System.putFloat(mResolver, Settings.System.FONT_SCALE, 1.00f);
        // It takes a second for the new font size to be picked up
        Thread.sleep(2000);
    }

    @MediumTest
    public void testFontSizeSmall() throws Exception {
        verifyFontSizeSetting(1.00f, FONT_SMALL);
    }

    private void verifyFontSizeSetting(float resetValue, FontSetting setting)
            throws Exception {
        Settings.System.putFloat(mResolver, Settings.System.FONT_SCALE, resetValue);
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        mHelper.clickSetting("Font size");
        try {
            mDevice.wait(Until.findObject(By.desc(setting.getName())), TIMEOUT).click();
            Thread.sleep(1000);
            float changedValue = Settings.System.getFloat(
                    mResolver, Settings.System.FONT_SCALE);
            assertEquals(setting.getSize(), changedValue, 0.0001);
        } finally {
            // Make sure to back out of the font menu
            mDevice.pressBack();
        }
    }

    private static class FontSetting {
        private final String mSizeName;
        private final float mSizeVal;

        public FontSetting(String sizeName, float sizeVal) {
            mSizeName = sizeName;
            mSizeVal = sizeVal;
        }

        public String getName() {
            return mSizeName;
        }

        public float getSize() {
            return mSizeVal;
        }
    }
}
