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
package com.android.cts.apprestrictions.managingapp;

import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Parcelable;
import android.os.UserManager;
import android.test.InstrumentationTestCase;
import android.test.MoreAsserts;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * Tests that a package other than the DPC can manage app restrictions if allowed by the DPC
 * via {@link DevicePolicyManager#setApplicationRestrictionsManagingPackage(ComponentName, String)}
 */
public class ApplicationRestrictionsManagerTest extends InstrumentationTestCase {

    private static final String APP_RESTRICTIONS_TARGET_PKG =
            "com.android.cts.apprestrictions.targetapp";
    private static final String APP_RESTRICTIONS_ACTIVITY_NAME =
            APP_RESTRICTIONS_TARGET_PKG + ".ApplicationRestrictionsActivity";
    private static final String ACTION_RESTRICTIONS_VALUE =
            "com.android.cts.apprestrictions.targetapp.RESTRICTIONS_VALUE";

    private static final ComponentName TEST_COMPONENT_NAME = new ComponentName(
            APP_RESTRICTIONS_TARGET_PKG, ApplicationRestrictionsManagerTest.class.getName());

    private static final Bundle BUNDLE_0 = createBundle0();
    private static final Bundle BUNDLE_1 = createBundle1();

    private static final long WAIT_FOR_ACTIVITY_TIMEOUT_SECONDS = 10;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ACTION_RESTRICTIONS_VALUE.equals(action)) {
                mReceivedRestrictions = intent.getBundleExtra("value");
                mOnRestrictionsSemaphore.release();
            }
        }
    };

    private Context mContext;
    private DevicePolicyManager mDevicePolicyManager;
    private UserManager mUserManager;
    private final Semaphore mOnRestrictionsSemaphore = new Semaphore(0);
    private Bundle mReceivedRestrictions;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mContext = getInstrumentation().getContext();
        mDevicePolicyManager = (DevicePolicyManager)
                mContext.getSystemService(Context.DEVICE_POLICY_SERVICE);
        mUserManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);

        mContext.registerReceiver(mReceiver, new IntentFilter(ACTION_RESTRICTIONS_VALUE));
    }

    @Override
    protected void tearDown() throws Exception {
        mContext.unregisterReceiver(mReceiver);

        super.tearDown();
    }

    public void testCannotManageAppRestrictions() {
        assertFalse(mDevicePolicyManager.isCallerApplicationRestrictionsManagingPackage());
        try {
            mDevicePolicyManager.setApplicationRestrictions(
                    null, APP_RESTRICTIONS_TARGET_PKG, null);
            fail("Expected SecurityException not thrown");
        } catch (SecurityException expected) {
            MoreAsserts.assertContainsRegex(
                    "cannot manage application restrictions", expected.getMessage());
        }
        try {
            mDevicePolicyManager.getApplicationRestrictions(null, APP_RESTRICTIONS_TARGET_PKG);
            fail("Expected SecurityException not thrown");
        } catch (SecurityException expected) {
            MoreAsserts.assertContainsRegex(
                    "cannot manage application restrictions", expected.getMessage());
        }

        // Should still be able to retrieve our own restrictions via user manager
        mUserManager.getApplicationRestrictions(mContext.getPackageName());
    }

    public void testCanManageAppRestrictions() {
        assertTrue(mDevicePolicyManager.isCallerApplicationRestrictionsManagingPackage());
        try {
            mDevicePolicyManager.setApplicationRestrictions(
                    null, APP_RESTRICTIONS_TARGET_PKG, BUNDLE_0);
            assertBundle0(mDevicePolicyManager.getApplicationRestrictions(
                    null, APP_RESTRICTIONS_TARGET_PKG));

            // Check that the target app can retrieve the same restrictions.
            assertBundle0(waitForChangedRestriction());

            // Test overwriting
            mDevicePolicyManager.setApplicationRestrictions(
                    null, APP_RESTRICTIONS_TARGET_PKG, BUNDLE_1);
            assertBundle1(mDevicePolicyManager.getApplicationRestrictions(
                    null, APP_RESTRICTIONS_TARGET_PKG));
            assertBundle1(waitForChangedRestriction());
        } finally {
            mDevicePolicyManager.setApplicationRestrictions(
                    null, APP_RESTRICTIONS_TARGET_PKG, new Bundle());
            assertTrue(mDevicePolicyManager.getApplicationRestrictions(
                    null, APP_RESTRICTIONS_TARGET_PKG).isEmpty());
        }
    }

    public void testSettingComponentNameThrowsException() {
        assertTrue(mDevicePolicyManager.isCallerApplicationRestrictionsManagingPackage());
        try {
            mDevicePolicyManager.setApplicationRestrictions(
                    TEST_COMPONENT_NAME, APP_RESTRICTIONS_TARGET_PKG, null);
            fail("Expected SecurityException not thrown");
        } catch (SecurityException expected) {
            MoreAsserts.assertContainsRegex("No active admin", expected.getMessage());
        }
        try {
            mDevicePolicyManager.getApplicationRestrictions(
                    TEST_COMPONENT_NAME, APP_RESTRICTIONS_TARGET_PKG);
            fail("Expected SecurityException not thrown");
        } catch (SecurityException expected) {
            MoreAsserts.assertContainsRegex("No active admin", expected.getMessage());
        }
    }

    // Should be consistent with assertBundle0
    private static Bundle createBundle0() {
        Bundle result = new Bundle();
        result.putString("dummyString", "value");
        return result;
    }

    // Should be consistent with createBundle0
    private void assertBundle0(Bundle bundle) {
        assertEquals(1, bundle.size());
        assertEquals("value", bundle.getString("dummyString"));
    }

    // Should be consistent with assertBundle1
    private static Bundle createBundle1() {
        Bundle result = new Bundle();
        result.putInt("dummyInt", 1);
        return result;
    }

    // Should be consistent with createBundle1
    private void assertBundle1(Bundle bundle) {
        assertEquals(1, bundle.size());
        assertEquals(1, bundle.getInt("dummyInt"));
    }

    private void startTestActivity() {
        mContext.startActivity(new Intent()
                .setComponent(new ComponentName(
                        APP_RESTRICTIONS_TARGET_PKG, APP_RESTRICTIONS_ACTIVITY_NAME))
                .addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK));
    }

    private Bundle waitForChangedRestriction() {
        startTestActivity();

        try {
            assertTrue(mOnRestrictionsSemaphore.tryAcquire(
                    WAIT_FOR_ACTIVITY_TIMEOUT_SECONDS, TimeUnit.SECONDS));
        } catch (InterruptedException e) {
            fail("waitForChangedRestriction() interrupted");
        }

        return mReceivedRestrictions;
    }
}
