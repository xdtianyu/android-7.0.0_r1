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
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.MediumTest;


public class MoreWirelessSettingsTests extends InstrumentationTestCase {

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
    public void testAirplaneModeEnabled() throws Exception {
        verifyAirplaneModeOnOrOff(true);
    }

    @MediumTest
    public void testAirplaneModeDisabled() throws Exception {
        verifyAirplaneModeOnOrOff(false);
    }

    // This NFC toggle test is set up this way since there's no way to set
    // the NFC flag to enabled or disabled without touching UI.
    // This way, we get coverage for whether or not the toggle button works.
    @MediumTest
    public void testNFCToggle() throws Exception {
        NfcManager manager = (NfcManager) getInstrumentation().getContext()
               .getSystemService(Context.NFC_SERVICE);
        NfcAdapter nfcAdapter = manager.getDefaultAdapter();
        boolean nfcInitiallyEnabled = nfcAdapter.isEnabled();
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(),
                Settings.ACTION_WIRELESS_SETTINGS);
        UiObject2 nfcSetting = mDevice.wait(Until.findObject(By.text("NFC")), TIMEOUT);
        nfcSetting.click();
        Thread.sleep(TIMEOUT*2);
        if (nfcInitiallyEnabled) {
            assertFalse("NFC wasn't disabled on toggle", nfcAdapter.isEnabled());
            nfcSetting.click();
            Thread.sleep(TIMEOUT*2);
            assertTrue("NFC wasn't enabled on toggle", nfcAdapter.isEnabled());
        }
        else {
            assertTrue("NFC wasn't enabled on toggle", nfcAdapter.isEnabled());
            nfcSetting.click();
            Thread.sleep(TIMEOUT*2);
            assertFalse("NFC wasn't disabled on toggle", nfcAdapter.isEnabled());
        }
    }

    @MediumTest
    public void testTetheringMenuLoad() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(),
                Settings.ACTION_WIRELESS_SETTINGS);
        mDevice.wait(Until
                 .findObject(By.text("Tethering & portable hotspot")), TIMEOUT)
                 .click();
        Thread.sleep(TIMEOUT);
        UiObject2 usbTethering = mDevice.wait(Until
                 .findObject(By.text("USB tethering")), TIMEOUT);
        assertNotNull("Tethering screen did not load correctly", usbTethering);
    }

    @MediumTest
    public void testVPNMenuLoad() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(),
                Settings.ACTION_WIRELESS_SETTINGS);
        mDevice.wait(Until
                 .findObject(By.text("VPN")), TIMEOUT)
                 .click();
        Thread.sleep(TIMEOUT);
        UiObject2 usbTethering = mDevice.wait(Until
                 .findObject(By.res(SETTINGS_PACKAGE, "vpn_create")), TIMEOUT);
        assertNotNull("VPN screen did not load correctly", usbTethering);
    }

    private void verifyAirplaneModeOnOrOff(boolean verifyOn) throws Exception {
        if (verifyOn) {
            Settings.Global.putString(getInstrumentation().getContext().getContentResolver(),
                    Settings.Global.AIRPLANE_MODE_ON, "0");
        }
        else {
            Settings.Global.putString(getInstrumentation().getContext().getContentResolver(),
                    Settings.Global.AIRPLANE_MODE_ON, "1");
        }
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(),
                Settings.ACTION_WIRELESS_SETTINGS);
        mDevice.wait(Until
                .findObject(By.text("Airplane mode")), TIMEOUT)
                .click();
        Thread.sleep(TIMEOUT);
        String airplaneModeValue = Settings.Global
                .getString(getInstrumentation().getContext().getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON);
        if (verifyOn) {
            assertEquals("1", airplaneModeValue);
        }
        else {
            assertEquals("0", airplaneModeValue);
        }
    }
}
