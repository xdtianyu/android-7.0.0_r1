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

import android.app.Instrumentation;
import android.app.NotificationManager;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.test.InstrumentationTestCase;

/**
 * Test class for remote bugreports.
 *
 * This class also handles making sure that the test is the device owner
 * and that it has an active admin registered, so that all tests may
 * assume these are done. The admin component can be accessed through
 * {@link BaseDeviceOwnerTest#getWho()}.
 */
public class RemoteBugreportTest extends InstrumentationTestCase {

    private static final String MESSAGE_ONLY_ONE_MANAGED_USER_ALLOWED =
            "There should only be one user, managed by Device Owner";


    private DevicePolicyManager mDevicePolicyManager;
    private Context mContext;
    private ComponentName mComponentName;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        Instrumentation instrumentation = getInstrumentation();
        mContext = instrumentation.getTargetContext();
        mDevicePolicyManager = (DevicePolicyManager)
                mContext.getSystemService(Context.DEVICE_POLICY_SERVICE);
        BaseDeviceOwnerTest.assertDeviceOwner(mDevicePolicyManager);
        mComponentName = BaseDeviceOwnerTest.getWho();
    }

    /**
     * Test: remote bugreport flow can only be started if there's one user on the device.
     */
    public void testRequestBugreportNotStartedIfMoreThanOneUserPresent() {
        boolean startedSuccessfully = false;
        try {
            startedSuccessfully = mDevicePolicyManager.requestBugreport(mComponentName);
            fail("did not throw expected SecurityException");
        } catch (SecurityException e) {
            assertEquals(e.getMessage(), MESSAGE_ONLY_ONE_MANAGED_USER_ALLOWED);
        }
    }

}
