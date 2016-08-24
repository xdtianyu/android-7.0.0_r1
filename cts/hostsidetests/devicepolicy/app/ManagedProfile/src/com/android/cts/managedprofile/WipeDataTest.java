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
package com.android.cts.managedprofile;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.os.Process;
import android.os.UserHandle;
import android.os.UserManager;

import com.android.cts.managedprofile.BaseManagedProfileTest.BasicAdminReceiver;

import org.junit.Ignore;

/**
 * Test wipeData() for use in managed profile. If called from a managed profile, wipeData() should
 * remove the current managed profile. Also, no erasing of external storage should be allowed.
 */
public class WipeDataTest extends BaseManagedProfileTest {

    private UserManager mUserManager;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // Make sure we are running in a managed profile, otherwise risk wiping the primary user's
        // data.
        assertTrue(mDevicePolicyManager.isAdminActive(ADMIN_RECEIVER_COMPONENT));
        assertTrue(mDevicePolicyManager.isProfileOwnerApp(ADMIN_RECEIVER_COMPONENT.getPackageName()));
        mUserManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
    }

    public void testWipeData() throws InterruptedException {
        UserHandle currentUser = Process.myUserHandle();
        assertTrue(mUserManager.getUserProfiles().contains(currentUser));

        mDevicePolicyManager.wipeData(0);

        // ACTION_MANAGED_PROFILE_REMOVED is only sent to parent user.
        // As a result, we have to poll in order to know when the profile
        // is actually removed.
        long epoch = System.currentTimeMillis();
        while (System.currentTimeMillis() - epoch <= 10 * 1000) {
            if (!mUserManager.getUserProfiles().contains(currentUser)) {
                break;
            }
            Thread.sleep(250);
        }

        // Verify the profile is deleted
        assertFalse(mUserManager.getUserProfiles().contains(currentUser));
    }
}
