/*
 * Copyright (C) 2015 The Android Open Source Project
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

/**
 * Set of tests for managed profile owner use cases that also apply to device owners.
 * Tests that should be run identically in both cases are added in DeviceAndProfileOwnerTest.
 */
public class MixedManagedProfileOwnerTest extends DeviceAndProfileOwnerTest {

    protected static final String CLEAR_PROFILE_OWNER_NEGATIVE_TEST_CLASS =
            DEVICE_ADMIN_PKG + ".ClearProfileOwnerNegativeTest";

    private int mParentUserId = -1;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        // We need managed users to be supported in order to create a profile of the user owner.
        mHasFeature &= hasDeviceFeature("android.software.managed_users");

        if (mHasFeature) {
            removeTestUsers();
            mParentUserId = mPrimaryUserId;
            createManagedProfile();
        }
    }

    private void createManagedProfile() throws Exception {
        mUserId = createManagedProfile(mParentUserId);
        switchUser(mParentUserId);
        startUser(mUserId);

        installAppAsUser(DEVICE_ADMIN_APK, mUserId);
        setProfileOwnerOrFail(DEVICE_ADMIN_PKG + "/" + ADMIN_RECEIVER_TEST_CLASS, mUserId);
        startUser(mUserId);
    }

    @Override
    protected void tearDown() throws Exception {
        if (mHasFeature) {
            removeUser(mUserId);
        }
        super.tearDown();
    }

    // Most tests for this class are defined in DeviceAndProfileOwnerTest

    /**
     * Verify that screenshots are still possible for activities in the primary user when the policy
     * is set on the profile owner.
     */
    public void testScreenCaptureDisabled_allowedPrimaryUser() throws Exception {
        if (!mHasFeature) {
            return;
        }
        executeDeviceTestMethod(".ScreenCaptureDisabledTest", "testSetScreenCaptureDisabled_true");
        // start the ScreenCaptureDisabledActivity in the parent
        installAppAsUser(DEVICE_ADMIN_APK, mParentUserId);
        String command = "am start -W --user " + mParentUserId + " " + DEVICE_ADMIN_PKG + "/"
                + DEVICE_ADMIN_PKG + ".ScreenCaptureDisabledActivity";
        getDevice().executeShellCommand(command);
        executeDeviceTestMethod(".ScreenCaptureDisabledTest", "testScreenCapturePossible");
    }

    @Override
    public void testResetPassword() {
        // Managed profile owner can't call resetPassword().
    }

    public void testCannotClearProfileOwner() throws Exception {
        if (mHasFeature) {
            assertTrue("Managed profile owner shouldn't be removed",
                    runDeviceTestsAsUser(DEVICE_ADMIN_PKG, CLEAR_PROFILE_OWNER_NEGATIVE_TEST_CLASS,
                            mUserId));
        }
    }

    @Override
    public void testDisallowSetWallpaper_allowed() throws Exception {
        // Managed profile doesn't have wallpaper.
    }

    @Override
    public void testAudioRestriction() throws Exception {
        // DISALLOW_UNMUTE_MICROPHONE and DISALLOW_ADJUST_VOLUME can only be set by device owners
        // and profile owners on the primary user.
    }

    @Override
    public void testDelegatedCertInstaller() throws Exception {
        if (!mHasFeature) {
            return;
        }

        try {
            super.testDelegatedCertInstaller();
        } finally {
            // In managed profile, clearing password through dpm is not allowed. Recreate user to
            // clear password instead.
            removeUser(mUserId);
            createManagedProfile();
        }
    }
}
