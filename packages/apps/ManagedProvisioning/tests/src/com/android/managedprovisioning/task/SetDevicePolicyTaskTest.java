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

package com.android.managedprovisioning.task;

import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.Mock;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import org.mockito.MockitoAnnotations;

public class SetDevicePolicyTaskTest extends AndroidTestCase {
    private static final String ADMIN_PACKAGE_NAME = "com.admin.test";
    private static final String ADMIN_RECEIVER_NAME = ADMIN_PACKAGE_NAME + ".AdminReceiver";
    private static final ComponentName ADMIN_COMPONENT_NAME = new ComponentName(ADMIN_PACKAGE_NAME,
            ADMIN_RECEIVER_NAME);
    private static final String OWNER_NAME = "Test Owner";
    private static final int TEST_USER_ID = 123;

    @Mock private Context mContext;
    @Mock private PackageManager mPackageManager;
    @Mock private DevicePolicyManager mDevicePolicyManager;
    @Mock private SetDevicePolicyTask.Callback mCallback;

    private SetDevicePolicyTask mTask;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // This is necessary for mockito to work
        System.setProperty("dexmaker.dexcache", getContext().getCacheDir().toString());
        MockitoAnnotations.initMocks(this);

        when(mContext.getPackageManager()).thenReturn(mPackageManager);
        when(mContext.getSystemService(Context.DEVICE_POLICY_SERVICE))
                .thenReturn(mDevicePolicyManager);
        when(mPackageManager.getApplicationEnabledSetting(ADMIN_PACKAGE_NAME))
                .thenReturn(PackageManager.COMPONENT_ENABLED_STATE_DEFAULT);
        when(mDevicePolicyManager.getDeviceOwnerComponentOnCallingUser()).thenReturn(null);
        when(mDevicePolicyManager.setDeviceOwner(ADMIN_COMPONENT_NAME, OWNER_NAME, TEST_USER_ID))
                .thenReturn(true);

        mTask = new SetDevicePolicyTask(mContext, OWNER_NAME, mCallback, TEST_USER_ID);
    }

    @SmallTest
    public void testEnableDevicePolicyApp() {
        when(mPackageManager.getApplicationEnabledSetting(ADMIN_PACKAGE_NAME))
                .thenReturn(PackageManager.COMPONENT_ENABLED_STATE_DISABLED);
        mTask.run(ADMIN_COMPONENT_NAME);
        verify(mPackageManager).setApplicationEnabledSetting(ADMIN_PACKAGE_NAME,
                PackageManager.COMPONENT_ENABLED_STATE_DEFAULT,
                PackageManager.DONT_KILL_APP);
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testEnableDevicePolicyApp_PackageNotFound() {
        when(mPackageManager.getApplicationEnabledSetting(ADMIN_PACKAGE_NAME))
                .thenThrow(new IllegalArgumentException());
        mTask.run(ADMIN_COMPONENT_NAME);
        verify(mCallback, times(1)).onError();
    }

    @SmallTest
    public void testSetActiveAdmin() {
        mTask.run(ADMIN_COMPONENT_NAME);
        verify(mDevicePolicyManager).setActiveAdmin(ADMIN_COMPONENT_NAME, true, TEST_USER_ID);
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testSetDeviceOwner() {
        mTask.run(ADMIN_COMPONENT_NAME);
        verify(mDevicePolicyManager).setDeviceOwner(ADMIN_COMPONENT_NAME, OWNER_NAME, TEST_USER_ID);
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testSetDeviceOwner_PreconditionsNotMet() {
        when(mDevicePolicyManager.setDeviceOwner(ADMIN_COMPONENT_NAME, OWNER_NAME, TEST_USER_ID))
                .thenThrow(new IllegalStateException());
        mTask.run(ADMIN_COMPONENT_NAME);
        verify(mDevicePolicyManager).setDeviceOwner(ADMIN_COMPONENT_NAME, OWNER_NAME, TEST_USER_ID);
        verify(mCallback, times(1)).onError();
    }

    @SmallTest
    public void testSetDeviceOwner_ReturnFalse() {
        when(mDevicePolicyManager.setDeviceOwner(ADMIN_COMPONENT_NAME, OWNER_NAME, TEST_USER_ID))
                .thenReturn(false);
        mTask.run(ADMIN_COMPONENT_NAME);
        verify(mDevicePolicyManager).setDeviceOwner(ADMIN_COMPONENT_NAME, OWNER_NAME, TEST_USER_ID);
        verify(mCallback, times(1)).onError();
    }
}
