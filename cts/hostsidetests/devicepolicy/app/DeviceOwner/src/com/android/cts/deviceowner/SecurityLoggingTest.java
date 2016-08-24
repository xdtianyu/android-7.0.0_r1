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
package com.android.cts.deviceowner;

import android.app.admin.SecurityLog.SecurityEvent;
import android.os.UserHandle;

import java.util.List;

public class SecurityLoggingTest extends BaseDeviceOwnerTest {

    private static final String MESSAGE_ONLY_ONE_MANAGED_USER_ALLOWED =
            "There should only be one user, managed by Device Owner";

    /**
     * Test: setting security logging can only be done if there's one user on the device.
     */
    public void testSetSecurityLoggingEnabledNotPossibleIfMoreThanOneUserPresent() {
        try {
            mDevicePolicyManager.setSecurityLoggingEnabled(getWho(), true);
            fail("did not throw expected SecurityException");
        } catch (SecurityException e) {
            assertEquals(e.getMessage(), MESSAGE_ONLY_ONE_MANAGED_USER_ALLOWED);
        }
    }

    /**
     * Test: retrieving security logs can only be done if there's one user on the device.
     */
    public void testRetrievingSecurityLogsNotPossibleIfMoreThanOneUserPresent() {
        try {
            mDevicePolicyManager.retrieveSecurityLogs(getWho());
            fail("did not throw expected SecurityException");
        } catch (SecurityException e) {
            assertEquals(e.getMessage(), MESSAGE_ONLY_ONE_MANAGED_USER_ALLOWED);
        }
    }

    /**
     * Test: retrieving previous security logs can only be done if there's one user on the device.
     */
    public void testRetrievingPreviousSecurityLogsNotPossibleIfMoreThanOneUserPresent() {
        try {
            mDevicePolicyManager.retrievePreRebootSecurityLogs(getWho());
            fail("did not throw expected SecurityException");
        } catch (SecurityException e) {
            assertEquals(e.getMessage(), MESSAGE_ONLY_ONE_MANAGED_USER_ALLOWED);
        }
    }

    /**
     * Test: Test enabling and disabling of security logging.
     */
    public void testEnablingAndDisablingSecurityLogging() {
        mDevicePolicyManager.setSecurityLoggingEnabled(getWho(), true);
        assertTrue(mDevicePolicyManager.isSecurityLoggingEnabled(getWho()));
        mDevicePolicyManager.setSecurityLoggingEnabled(getWho(), false);
        assertFalse(mDevicePolicyManager.isSecurityLoggingEnabled(getWho()));
    }

    /**
     * Test: retrieving security logs should be rate limited - subsequent attempts should return
     * null.
     */
    public void testRetrievingSecurityLogsNotPossibleImmediatelyAfterPreviousSuccessfulRetrieval() {
        List<SecurityEvent> logs = mDevicePolicyManager.retrieveSecurityLogs(getWho());
        // if logs is null it means that that attempt was rate limited => test PASS
        if (logs != null) {
            assertNull(mDevicePolicyManager.retrieveSecurityLogs(getWho()));
            assertNull(mDevicePolicyManager.retrieveSecurityLogs(getWho()));
        }
    }
}
