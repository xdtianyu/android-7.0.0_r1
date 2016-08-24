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

package com.android.cts.profileowner;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.test.AndroidTestCase;

public class ClearProfileOwnerTest extends AndroidTestCase {

    private DevicePolicyManager mDevicePolicyManager;

    @Override
    protected void tearDown() throws Exception {
        mDevicePolicyManager = (DevicePolicyManager)
                mContext.getSystemService(Context.DEVICE_POLICY_SERVICE);
        if (mDevicePolicyManager != null) {
            if (mDevicePolicyManager.isProfileOwnerApp(BaseProfileOwnerTest.PACKAGE_NAME)) {
                mDevicePolicyManager.clearProfileOwner(BaseProfileOwnerTest.getWho());
            }
            assertFalse(mDevicePolicyManager.isProfileOwnerApp(BaseProfileOwnerTest.PACKAGE_NAME));

            waitForActiveAdminRemoved(BaseProfileOwnerTest.getWho());
        }

        super.tearDown();
    }

    // This test clears the profile owner and active admin on tearDown(). To be called from the host
    // side test once a test case is finished.
    public void testClearProfileOwner() {
    }

    private void waitForActiveAdminRemoved(ComponentName cn) throws InterruptedException {
        for (int i = 0; i < 1000 && mDevicePolicyManager.isAdminActive(cn); i++) {
            Thread.sleep(100);
        }
        assertFalse(mDevicePolicyManager.isAdminActive(cn));
    }
}
