/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.cts.deviceandprofileowner;

import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * Tests for {@link DevicePolicyManager#setScreenCaptureDisabled} and
 * {@link DevicePolicyManager#getScreenCaptureDisabled} APIs.
 */
public class ScreenCaptureDisabledTest extends BaseDeviceAdminTest {

    private static final String TAG = "ScreenCaptureDisabledTest";

    private ScreenCaptureBroadcastReceiver mReceiver = new ScreenCaptureBroadcastReceiver();

    protected void setUp() throws Exception {
        super.setUp();
        mContext.registerReceiver(mReceiver, new IntentFilter(
                ScreenCaptureDisabledActivity.ACTIVITY_RESUMED));
    }

    protected void tearDown() throws Exception {
        mContext.unregisterReceiver(mReceiver);
        super.tearDown();
    }

    public void testSetScreenCaptureDisabled_false() throws Exception {
        mDevicePolicyManager.setScreenCaptureDisabled(ADMIN_RECEIVER_COMPONENT, false);
        assertFalse(mDevicePolicyManager.getScreenCaptureDisabled(ADMIN_RECEIVER_COMPONENT));
        assertFalse(mDevicePolicyManager.getScreenCaptureDisabled(null /* any admin */));
        startTestActivity();
        assertNotNull(getInstrumentation().getUiAutomation().takeScreenshot());
    }

    public void testSetScreenCaptureDisabled_true() throws Exception {
        mDevicePolicyManager.setScreenCaptureDisabled(ADMIN_RECEIVER_COMPONENT, true);
        assertTrue(mDevicePolicyManager.getScreenCaptureDisabled(ADMIN_RECEIVER_COMPONENT));
        assertTrue(mDevicePolicyManager.getScreenCaptureDisabled(null /* any admin */));
        startTestActivity();
        assertNull(getInstrumentation().getUiAutomation().takeScreenshot());
    }

    public void testScreenCapturePossible() throws Exception {
        assertNotNull(getInstrumentation().getUiAutomation().takeScreenshot());
    }

    // We need to launch an activity before trying to take a screen shot, because screenshots are
    // only blocked on a per-user basis in the profile owner case depending on the owner of the
    // foreground activity.
    private void startTestActivity() throws Exception {
        Intent launchIntent = new Intent();
        launchIntent.setComponent(new ComponentName(PACKAGE_NAME,
                ScreenCaptureDisabledActivity.class.getName()));
        launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(launchIntent);
        assertTrue(mReceiver.waitForBroadcast());
        Thread.sleep(1000);
    }

    private class ScreenCaptureBroadcastReceiver extends BroadcastReceiver {
        private final Semaphore mSemaphore = new Semaphore(0);

        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "Broadcast received");
            mSemaphore.release();
        }

        public boolean waitForBroadcast() throws Exception {
            if (mSemaphore.tryAcquire(5, TimeUnit.SECONDS)) {
                return true;
            }
            return false;
        }
    }
}
