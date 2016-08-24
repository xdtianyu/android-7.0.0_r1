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
package com.android.cts.devicepolicy;

import com.android.tradefed.device.DeviceNotAvailableException;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

public class UserRestrictionsTest extends BaseDevicePolicyTest {
    private static final String DEVICE_ADMIN_PKG = "com.android.cts.deviceandprofileowner";
    private static final String DEVICE_ADMIN_APK = "CtsDeviceAndProfileOwnerApp.apk";
    private static final String ADMIN_RECEIVER_TEST_CLASS
            = ".BaseDeviceAdminTest$BasicAdminReceiver";

    private boolean mRemoveOwnerInTearDown;
    private int mDeviceOwnerUserId;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mRemoveOwnerInTearDown = false;
        mDeviceOwnerUserId = mPrimaryUserId;
    }

    @Override
    protected void tearDown() throws Exception {
        if (mHasFeature) {
            if (mRemoveOwnerInTearDown) {
                assertTrue("Failed to clear owner",
                        removeAdmin(DEVICE_ADMIN_PKG + "/" + ADMIN_RECEIVER_TEST_CLASS,
                                mDeviceOwnerUserId));
            }
            assertTrue("Some user restrictions are still set",
                    runTests("userrestrictions.CheckNoOwnerRestrictionsTest", mDeviceOwnerUserId));

            // DO/PO might have set DISALLOW_REMOVE_USER, so it needs to be done after removing
            // them.
            removeTestUsers();
        }
        super.tearDown();
    }

    private boolean runTests(@Nonnull String className,
            @Nullable String method, int userId) throws DeviceNotAvailableException {
        return runDeviceTestsAsUser(DEVICE_ADMIN_PKG, "." + className, method, userId);
    }

    private boolean runTests(@Nonnull String className, int userId)
            throws DeviceNotAvailableException {
        return runTests(className, null, userId);
    }

    public void testUserRestrictions_deviceOwnerOnly() throws Exception {
        if (!mHasFeature) {
            return;
        }
        installAppAsUser(DEVICE_ADMIN_APK, mDeviceOwnerUserId);
        assertTrue("Failed to set device owner",
                setDeviceOwner(DEVICE_ADMIN_PKG + "/" + ADMIN_RECEIVER_TEST_CLASS,
                        mDeviceOwnerUserId, /*expectFailure*/ false));
        mRemoveOwnerInTearDown = true;

        runTests("userrestrictions.DeviceOwnerUserRestrictionsTest",
                "testSetAllRestrictions", mDeviceOwnerUserId);
    }

    public void testUserRestrictions_primaryProfileOwnerOnly() throws Exception {
        if (!mHasFeature) {
            return;
        }
        if (hasUserSplit()) {
            // Can't set PO on user-0 in this mode.
            return;
        }

        installAppAsUser(DEVICE_ADMIN_APK, mDeviceOwnerUserId);
        assertTrue("Failed to set profile owner",
                setProfileOwner(DEVICE_ADMIN_PKG + "/" + ADMIN_RECEIVER_TEST_CLASS,
                        mDeviceOwnerUserId, /* expectFailure */ false));
        mRemoveOwnerInTearDown = true;

        runTests("userrestrictions.PrimaryProfileOwnerUserRestrictionsTest",
                "testSetAllRestrictions", mDeviceOwnerUserId);
    }

    public void testUserRestrictions_secondaryProfileOwnerOnly() throws Exception {
        if (!mHasFeature || !mSupportsMultiUser) {
            return;
        }
        final int secondaryUserId = createUser();

        installAppAsUser(DEVICE_ADMIN_APK, secondaryUserId);
        assertTrue("Failed to set profile owner",
                setProfileOwner(DEVICE_ADMIN_PKG + "/" + ADMIN_RECEIVER_TEST_CLASS,
                        secondaryUserId, /* expectFailure */ false));

        runTests("userrestrictions.SecondaryProfileOwnerUserRestrictionsTest",
                "testSetAllRestrictions", secondaryUserId);
    }

    /**
     * DO + PO combination.  Make sure global DO restrictions are visible on secondary users.
     */
    public void testUserRestrictions_layering() throws Exception {
        if (!mHasFeature || !mSupportsMultiUser) {
            return;
        }
        // Set DO
        installAppAsUser(DEVICE_ADMIN_APK, mDeviceOwnerUserId);
        assertTrue("Failed to set device owner",
                setDeviceOwner(DEVICE_ADMIN_PKG + "/" + ADMIN_RECEIVER_TEST_CLASS,
                        mDeviceOwnerUserId, /*expectFailure*/ false));
        mRemoveOwnerInTearDown = true;

        // Create another user and set PO.
        final int secondaryUserId = createUser();

        installAppAsUser(DEVICE_ADMIN_APK, secondaryUserId);
        assertTrue("Failed to set profile owner",
                setProfileOwner(DEVICE_ADMIN_PKG + "/" + ADMIN_RECEIVER_TEST_CLASS,
                        secondaryUserId, /* expectFailure */ false));

        // Let DO set all restrictions.
        runTests("userrestrictions.DeviceOwnerUserRestrictionsTest",
                "testSetAllRestrictions", mDeviceOwnerUserId);

        // Make sure the global restrictions are visible to secondary users.
        runTests("userrestrictions.SecondaryProfileOwnerUserRestrictionsTest",
                "testHasGlobalRestrictions", secondaryUserId);

        // Then let PO set all restrictions.
        runTests("userrestrictions.SecondaryProfileOwnerUserRestrictionsTest",
                "testSetAllRestrictions", secondaryUserId);

        // Make sure both local and global restrictions are visible on secondary users.
        runTests("userrestrictions.SecondaryProfileOwnerUserRestrictionsTest",
                "testHasBothGlobalAndLocalRestrictions", secondaryUserId);

        // Let DO clear all restrictions.
        runTests("userrestrictions.DeviceOwnerUserRestrictionsTest",
                "testClearAllRestrictions", mDeviceOwnerUserId);

        // Now only PO restrictions should be set on the secondary user.
        runTests("userrestrictions.SecondaryProfileOwnerUserRestrictionsTest",
                "testLocalRestrictionsOnly", secondaryUserId);
    }

    /**
     * PO on user-0.  It can set DO restrictions too, but they shouldn't leak to other users.
     */
    public void testUserRestrictions_layering_profileOwnerNoLeaking() throws Exception {
        if (!mHasFeature || !mSupportsMultiUser) {
            return;
        }
        if (hasUserSplit()) {
            // Can't set PO on user-0 in this mode.
            return;
        }
        // Set DO on user 0
        installAppAsUser(DEVICE_ADMIN_APK, mDeviceOwnerUserId);
        assertTrue("Failed to set profile owner",
                setProfileOwner(DEVICE_ADMIN_PKG + "/" + ADMIN_RECEIVER_TEST_CLASS,
                        mDeviceOwnerUserId, /* expectFailure */ false));
        mRemoveOwnerInTearDown = true;

        // Create another user and set PO.
        final int secondaryUserId = createUser();

        installAppAsUser(DEVICE_ADMIN_APK, secondaryUserId);
        assertTrue("Failed to set profile owner",
                setProfileOwner(DEVICE_ADMIN_PKG + "/" + ADMIN_RECEIVER_TEST_CLASS,
                        secondaryUserId, /* expectFailure */ false));

        // Let user-0 PO sets all restrictions.
        runTests("userrestrictions.PrimaryProfileOwnerUserRestrictionsTest",
                "testSetAllRestrictions", mDeviceOwnerUserId);

        // Secondary users shouldn't see any of them.
        runTests("userrestrictions.SecondaryProfileOwnerUserRestrictionsTest",
                "testDefaultRestrictionsOnly", secondaryUserId);
    }
}
