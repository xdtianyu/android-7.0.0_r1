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

/**
 * Base class for DeviceAdmin host side tests.
 *
 * We have subclasses for device admins for different API levels.
 */
public abstract class BaseDeviceAdminHostSideTest extends BaseDevicePolicyTest {

    private static final String ADMIN_RECEIVER_TEST_CLASS = "BaseDeviceAdminTest$AdminReceiver";

    private static final String UNPROTECTED_ADMIN_RECEIVER_TEST_CLASS =
            "DeviceAdminReceiverWithNoProtection";

    protected int mUserId;

    /** returns "com.android.cts.deviceadmin" */
    protected final String getDeviceAdminJavaPackage() {
        return "com.android.cts.deviceadmin";
    }

    /** e.g. 23, 24, etc. */
    protected abstract int getTargetApiVersion();

    /** e.g. CtsDeviceAdminApp24.apk */
    protected final String getDeviceAdminApkFileName() {
        return "CtsDeviceAdminApp" + getTargetApiVersion() + ".apk";
    }

    /** e.g. "com.android.cts.deviceadmin24" */
    protected final String getDeviceAdminApkPackage() {
        return getDeviceAdminJavaPackage() + getTargetApiVersion();
    }

    /**
     * e.g.
     * "com.android.cts.deviceadmin24/com.android.cts.deviceadmin.BaseDeviceAdminTest$AdminReceiver"
     */
    protected final String getAdminReceiverComponent() {
        return getDeviceAdminApkPackage() + "/" + getDeviceAdminJavaPackage() + "." +
                ADMIN_RECEIVER_TEST_CLASS;
    }

    /**
     * e.g.
     * "com.android.cts.deviceadmin24/com.android.cts.deviceadmin.DeviceAdminReceiverWithNoProtection"
     */
    protected final String getUnprotectedAdminReceiverComponent() {
        return getDeviceAdminApkPackage() + "/" + getDeviceAdminJavaPackage() + "." +
                UNPROTECTED_ADMIN_RECEIVER_TEST_CLASS;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mUserId = mPrimaryUserId;

        if (mHasFeature) {
            installAppAsUser(getDeviceAdminApkFileName(), mUserId);
            setDeviceAdmin(getAdminReceiverComponent(), mUserId);
        }
    }

    @Override
    protected void tearDown() throws Exception {
        if (mHasFeature) {
            assertTrue("Failed to remove admin",
                    removeAdmin(getAdminReceiverComponent(), mUserId));
            getDevice().uninstallPackage(getDeviceAdminApkPackage());
        }

        super.tearDown();
    }

    protected boolean runTests(@Nonnull String apk, @Nonnull String className,
            @Nullable String method) throws DeviceNotAvailableException {
        return runDeviceTestsAsUser(apk,
                getDeviceAdminJavaPackage() + "." + className, method, mUserId);
    }

    protected boolean runTests(@Nonnull String apk, @Nonnull String className)
            throws DeviceNotAvailableException {
        return runTests(apk, className, null);
    }

    /**
     * Run all tests in DeviceAdminTest.java (as device admin).
     */
    public void testRunDeviceAdminTest() throws Exception {
        if (!mHasFeature) {
            return;
        }

        assertTrue("Some of device side tests failed",
                runTests(getDeviceAdminApkPackage(), "DeviceAdminTest"));
    }

    private void clearPasswordForDeviceOwner() throws Exception {
        assertTrue("Failed to clear password",
                runTests(getDeviceAdminApkPackage(), "ClearPasswordTest"));
    }

    private void makeDoAndClearPassword() throws Exception {
        // Clear the password.  We do it by promoting the DA to DO.
        setDeviceOwner(getAdminReceiverComponent(), mUserId, /*expectFailure*/ false);
        try {
            clearPasswordForDeviceOwner();
        } finally {
            assertTrue("Failed to clear device owner",
                    removeAdmin(getAdminReceiverComponent(), mUserId));
            // Clearing DO removes the DA too, so we need to set it again.
            setDeviceAdmin(getAdminReceiverComponent(), mUserId);
        }
    }

    public void testResetPassword_nycRestrictions() throws Exception {
        if (!mHasFeature) {
            return;
        }

        // If there's a password, clear it.
        makeDoAndClearPassword();
        try {
            assertTrue(runTests(getDeviceAdminApkPackage(), "DeviceAdminPasswordTest",
                            "testResetPassword_nycRestrictions"));
        } finally {
            makeDoAndClearPassword();
        }
    }

    /**
     * Run the tests in DeviceOwnerPasswordTest.java (as device owner).
     */
    public void testRunDeviceOwnerPasswordTest() throws Exception {
        if (!mHasFeature) {
            return;
        }

        setDeviceOwner(getAdminReceiverComponent(), mUserId, /*expectFailure*/ false);

        clearPasswordForDeviceOwner();

        try {
            assertTrue("Some of device side tests failed",
                    runTests(getDeviceAdminApkPackage(), "DeviceOwnerPasswordTest"));
        } finally {
            clearPasswordForDeviceOwner();
        }
    }
}
