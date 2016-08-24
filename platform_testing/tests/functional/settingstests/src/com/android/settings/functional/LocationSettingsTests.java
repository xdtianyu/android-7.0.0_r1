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
import android.net.wifi.WifiManager;
import android.nfc.NfcAdapter;
import android.nfc.NfcManager;
import android.os.RemoteException;
import android.provider.Settings;
import android.platform.test.helpers.SettingsHelperImpl;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.MediumTest;


public class LocationSettingsTests extends InstrumentationTestCase {

    private static final String SETTINGS_PACKAGE = "com.android.settings";
    private static final int TIMEOUT = 2000;
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
        mDevice.pressBack();
        mDevice.pressHome();
        super.tearDown();
    }

    @MediumTest
    public void testLoadingLocationSettings () throws Exception {
        // Load settings
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(),
                Settings.ACTION_SETTINGS);
        // Tap on location
        UiObject2 settingsPanel = mDevice.wait(Until.findObject
                (By.res(SETTINGS_PACKAGE, "dashboard_container")), TIMEOUT);
        int count = 0;
        UiObject2 locationTitle = null;
        while(count < 6 && locationTitle == null) {
            locationTitle = mDevice.wait(Until.findObject(By.text("Location")), TIMEOUT);
            if (locationTitle == null) {
                settingsPanel.scroll(Direction.DOWN, 1.0f);
            }
            count++;
        }
        // Verify location settings loads.
        locationTitle.click();
        Thread.sleep(TIMEOUT);
        assertNotNull("Location screen has not loaded correctly",
                mDevice.wait(Until.findObject(By.text("Location services")), TIMEOUT));
    }

    @MediumTest
    public void testLocationSettingOn() throws Exception {
        verifyLocationSettingsOnOrOff(true);
    }

    @MediumTest
    public void testLocationSettingOff() throws Exception {
        verifyLocationSettingsOnOrOff(false);
    }

    @MediumTest
    public void testLocationDeviceOnlyMode() throws Exception {
        // Changing the value from default before testing the toggle to Device only mode
        Settings.Secure.putInt(getInstrumentation().getContext().getContentResolver(),
                Settings.Secure.LOCATION_MODE, Settings.Secure.LOCATION_MODE_BATTERY_SAVING);
        Thread.sleep(TIMEOUT);
        verifyLocationSettingsMode(Settings.Secure.LOCATION_MODE_SENSORS_ONLY);
    }

    @MediumTest
    public void testLocationBatterySavingMode() throws Exception {
        Settings.Secure.putInt(getInstrumentation().getContext().getContentResolver(),
                Settings.Secure.LOCATION_MODE, Settings.Secure.LOCATION_MODE_SENSORS_ONLY);
        Thread.sleep(TIMEOUT);
        verifyLocationSettingsMode(Settings.Secure.LOCATION_MODE_BATTERY_SAVING);
    }

    @MediumTest
    public void testLocationHighAccuracyMode() throws Exception {
        Settings.Secure.putInt(getInstrumentation().getContext().getContentResolver(),
                Settings.Secure.LOCATION_MODE, Settings.Secure.LOCATION_MODE_SENSORS_ONLY);
        Thread.sleep(TIMEOUT);
        verifyLocationSettingsMode(Settings.Secure.LOCATION_MODE_HIGH_ACCURACY);
    }

    @MediumTest
    public void testLocationSettingsElements() throws Exception {
        String[] textElements = {"Location", "On", "Mode", "Recent location requests",
                "Location services"};
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(),
                Settings.ACTION_LOCATION_SOURCE_SETTINGS);
        Thread.sleep(TIMEOUT);
        for (String element : textElements) {
            assertNotNull(element + " item not found under Location Settings",
                    mDevice.wait(Until.findObject(By.text(element)), TIMEOUT));
        }
    }

    @MediumTest
    public void testLocationSettingsOverflowMenuElements() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(),
                            Settings.ACTION_LOCATION_SOURCE_SETTINGS);
        // Tap on overflow menu
        mDevice.wait(Until.findObject(By.desc("More options")), TIMEOUT).click();
        // Verify scanning
        assertNotNull("Scanning item not found under Location Settings",
                mDevice.wait(Until.findObject(By.text("Scanning")), TIMEOUT));
        // Verify help & feedback
        assertNotNull("Help & feedback item not found under Location Settings",
                mDevice.wait(Until.findObject(By.text("Help & feedback")), TIMEOUT));
    }

    private void verifyLocationSettingsMode(int mode) throws Exception {
        int modeIntValue = 1;
        String textMode = "Device only";
        if (mode == Settings.Secure.LOCATION_MODE_HIGH_ACCURACY) {
            modeIntValue = 3;
            textMode = "High accuracy";
        }
        else if (mode == Settings.Secure.LOCATION_MODE_BATTERY_SAVING) {
            modeIntValue = 2;
            textMode = "Battery saving";
        }
        // Load location settings
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(),
                Settings.ACTION_LOCATION_SOURCE_SETTINGS);
        // Tap on mode
        mDevice.wait(Until.findObject(By.text("Mode")), TIMEOUT).click();
        Thread.sleep(TIMEOUT);
        assertNotNull("Location mode screen not loaded", mDevice.wait(Until.findObject
                (By.text("Location mode")), TIMEOUT));
        // Choose said mode
        mDevice.wait(Until.findObject(By.text(textMode)), TIMEOUT).click();
        Thread.sleep(TIMEOUT);
        if (mode == Settings.Secure.LOCATION_MODE_HIGH_ACCURACY ||
                mode == Settings.Secure.LOCATION_MODE_BATTERY_SAVING) {
            // Expect another dialog here at improving location tracking
            // for the first time
            UiObject2 agreeDialog = mDevice.wait(Until.findObject
                    (By.text("Improve location accuracy?")), TIMEOUT);
            if (agreeDialog != null) {
            mDevice.wait(Until.findObject
                                (By.text("AGREE")), TIMEOUT).click();
            Thread.sleep(TIMEOUT);
            assertNull("Improve location dialog not dismissed", mDevice.wait(Until.findObject
                    (By.text("Improve location accuracy?")), TIMEOUT));
            }
        }
        // get setting and verify value
        // Verify change of mode
        int locationSettingMode =
                Settings.Secure.getInt(getInstrumentation().getContext().getContentResolver(),
                Settings.Secure.LOCATION_MODE);
        assertEquals(mode + " value not set correctly for location.", modeIntValue,
                locationSettingMode);
    }

    private void verifyLocationSettingsOnOrOff(boolean verifyOn) throws Exception {
        // Set location flag
        if (verifyOn) {
            Settings.Secure.putInt(getInstrumentation().getContext().getContentResolver(),
                    Settings.Secure.LOCATION_MODE, Settings.Secure.LOCATION_MODE_OFF);
        }
        else {
            Settings.Secure.putInt(getInstrumentation().getContext().getContentResolver(),
                    Settings.Secure.LOCATION_MODE, Settings.Secure.LOCATION_MODE_SENSORS_ONLY);
        }
        // Load location settings
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(),
                Settings.ACTION_LOCATION_SOURCE_SETTINGS);
        // Toggle UI
        mDevice.wait(Until.findObject(By.res(SETTINGS_PACKAGE, "switch_widget")), TIMEOUT).click();
        Thread.sleep(TIMEOUT);
        // Verify change in setting
        int locationEnabled = Settings.Secure.getInt(getInstrumentation()
                 .getContext().getContentResolver(),
                 Settings.Secure.LOCATION_MODE);
        if (verifyOn) {
            assertFalse("Location not enabled correctly", locationEnabled == 0);
        }
        else {
            assertEquals("Location not disabled correctly", 0, locationEnabled);
        }
    }
}
