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
package com.android.cts.deviceandprofileowner;

import android.app.admin.DevicePolicyManager;
import android.content.Context;
import android.test.AndroidTestCase;

/**
 * This test is used to set and clear the lockscreen password. This is required to use the keystore
 * by the host side delegated cert installer test.
 */
public class ResetPasswordHelper extends AndroidTestCase {

    private DevicePolicyManager mDpm;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDpm = (DevicePolicyManager) mContext.getSystemService(Context.DEVICE_POLICY_SERVICE);
    }

    /**
     * Set lockscreen password.
     */
    public void testSetPassword() {
        // Enable credential storage by setting a nonempty password.
        assertTrue(mDpm.resetPassword("test", 0));
    }

    /**
     * Clear lockscreen password.
     */
    public void testClearPassword() {
        mDpm.setPasswordQuality(BaseDeviceAdminTest.ADMIN_RECEIVER_COMPONENT,
                DevicePolicyManager.PASSWORD_QUALITY_UNSPECIFIED);
        mDpm.setPasswordMinimumLength(
                BaseDeviceAdminTest.ADMIN_RECEIVER_COMPONENT, 0);
        assertTrue(mDpm.resetPassword("", 0));
    }
}
