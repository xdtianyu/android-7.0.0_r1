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

import java.io.IOException;
import android.content.Context;
import android.content.Intent;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothAdapter;
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
import android.test.suitebuilder.annotation.MediumTest;


public class BluetoothNetworkSettingsTests extends InstrumentationTestCase {

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
        mDevice.waitForIdle();
        super.tearDown();
    }

    @MediumTest
    public void testBluetoothEnabled() throws Exception {
        verifyBluetoothOnOrOff(true);
    }

    @MediumTest
    public void testBluetoothDisabled() throws Exception {
        verifyBluetoothOnOrOff(false);
    }

    @MediumTest
    public void testRefreshOverflowOption() throws Exception {
        verifyBluetoothOverflowOptions("Refresh", false, null);
    }

    @MediumTest
    public void testRenameOverflowOption() throws Exception {
        verifyBluetoothOverflowOptions("Rename this device", true, "RENAME");
    }

    @MediumTest
    public void testReceivedFilesOverflowOption() throws Exception {
        verifyBluetoothOverflowOptions("Show received files", true, "Bluetooth received");
    }

    @MediumTest
    public void testHelpFeedbackOverflowOption() throws Exception {
        verifyBluetoothOverflowOptions("Help & feedback", true, "Help");
    }

    public void launchBluetoothSettings() throws Exception {
        Intent btIntent = new Intent(Settings.ACTION_BLUETOOTH_SETTINGS);
        btIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getInstrumentation().getContext().startActivity(btIntent);
        Thread.sleep(TIMEOUT * 2);
    }

    /**
     * Verifies clicking on the BT overflow option and loading the right screen
     * @param overflowOptionText the text of the option to be clicked
     * @param verifyClick if you need a click to be verified
     * @param optionLoaded text of an element on the post click screen for verification
     */
    public void verifyBluetoothOverflowOptions(String overflowOptionText, boolean verifyClick,
            String optionLoaded) throws Exception {
        BluetoothAdapter bluetoothAdapter = ((BluetoothManager) getInstrumentation().getContext()
                .getSystemService(Context.BLUETOOTH_SERVICE)).getAdapter();
        bluetoothAdapter.enable();
        launchBluetoothSettings();
        mDevice.wait(Until.findObject(By.desc("More options")), TIMEOUT).click();
        Thread.sleep(TIMEOUT);
        UiObject2 overflowOption = mDevice.wait(Until.findObject(By.text(overflowOptionText)),
                TIMEOUT);
        assertNotNull(overflowOptionText + " option is not present in advanced Bluetooth menu",
                overflowOption);
        if (verifyClick) {
            overflowOption.click();
            // Adding an extra back press to deal with IME+UiAutomator bug
            if (optionLoaded.equals("RENAME")) {
                mDevice.pressBack();
            }
            UiObject2 loadOption = mDevice.wait(Until.findObject(By.text(optionLoaded)), TIMEOUT);
            assertNotNull(overflowOptionText + " option did not load correctly on tapping",
                    loadOption);
        }
    }

    /**
     * Toggles the Bluetooth switch and verifies that the change is reflected in Settings
     * @param verifyOn set to whether you want the setting turned On or Off
     */
    private void verifyBluetoothOnOrOff(boolean verifyOn) throws Exception {
        String switchText = "ON";
        BluetoothAdapter bluetoothAdapter = ((BluetoothManager) getInstrumentation().getContext()
                            .getSystemService(Context.BLUETOOTH_SERVICE)).getAdapter();
        if (verifyOn) {
            switchText = "OFF";
            bluetoothAdapter.disable();
         }
         else {
             bluetoothAdapter.enable();
         }
         launchBluetoothSettings();
         mDevice.wait(Until
                 .findObject(By.res(SETTINGS_PACKAGE, "switch_widget").text(switchText)), TIMEOUT)
                 .click();
         Thread.sleep(TIMEOUT);
         String bluetoothValue =
                 Settings.Global.getString(getInstrumentation().getContext().getContentResolver(),
                 Settings.Global.BLUETOOTH_ON);
         if (verifyOn) {
             assertEquals("1", bluetoothValue);
         }
         else {
             assertEquals("0", bluetoothValue);
         }
    }
}
