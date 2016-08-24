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

package com.android.bluetooth.tests;

import android.app.admin.DevicePolicyManager;
import android.content.Context;
import android.content.pm.UserInfo;
import android.net.Uri;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.test.AndroidTestCase;

import com.android.bluetooth.util.DevicePolicyUtils;
import com.google.testing.littlemock.LittleMock;

import java.util.Arrays;
import java.util.List;

import static com.google.testing.littlemock.LittleMock.mock;
import static com.google.testing.littlemock.LittleMock.doReturn;
import static com.google.testing.littlemock.LittleMock.anyInt;

public class DevicePolicyUtilsTest extends AndroidTestCase {
    private static final String TAG = "DevicePolicyUtilsTest";

    private static final String SYSTEM_PROPERTY_DEXMAKER_DEXCACHE = "dexmaker.dexcache";

    private String mOriginalDexcache;

    @Override
    protected void setUp() {
        mOriginalDexcache = System.getProperty(SYSTEM_PROPERTY_DEXMAKER_DEXCACHE);
        System.setProperty(SYSTEM_PROPERTY_DEXMAKER_DEXCACHE, getContext().getCacheDir().getPath());
    }

    @Override
    protected void tearDown() {
        if (mOriginalDexcache == null) {
            System.clearProperty(SYSTEM_PROPERTY_DEXMAKER_DEXCACHE);
        } else {
            System.setProperty(SYSTEM_PROPERTY_DEXMAKER_DEXCACHE, mOriginalDexcache);
        }
    }

    public void testIsBluetoothWorkContactSharingDisabled() {
        {
            // normal user only with bluetoothContacts is disabled
            Context mockContext = getMockContext(false, true);
            Uri uri = DevicePolicyUtils.getEnterprisePhoneUri(mockContext);
            assertEquals("expected: " + Phone.CONTENT_URI + " value = " + uri,
                    Phone.CONTENT_URI, uri);
        }
        {
            // normal user only with bluetoothContacts is not disabled
            Context mockContext = getMockContext(false, false);
            Uri uri = DevicePolicyUtils.getEnterprisePhoneUri(mockContext);
            assertEquals("expected: " + Phone.CONTENT_URI + " value = " + uri,
                    Phone.CONTENT_URI, uri);
        }
        {
            // managedProfile with bluetoothContacts is disabled
            Context mockContext = getMockContext(true, true);
            Uri uri = DevicePolicyUtils.getEnterprisePhoneUri(mockContext);
            assertEquals("expected: " + Phone.CONTENT_URI + " value = " + uri,
                    Phone.CONTENT_URI, uri);
        }
        {
            // managedProfile with bluetoothContacts is not disabled
            Context mockContext = getMockContext(true, false);
            Uri uri = DevicePolicyUtils.getEnterprisePhoneUri(mockContext);
            assertEquals("expected: " + Phone.ENTERPRISE_CONTENT_URI + " value = " + uri,
                    Phone.ENTERPRISE_CONTENT_URI, uri);
        }
    }

    private static final List<UserInfo> NORMAL_USERINFO_LIST = Arrays.asList(
            new UserInfo[]{ new UserInfo(0, "user0", 0)});

    private static final List<UserInfo> MANAGED_USERINFO_LIST = Arrays.asList(new UserInfo[]{
            new UserInfo(0, "user0", 0),
            new UserInfo(10, "user10", UserInfo.FLAG_MANAGED_PROFILE)});

    private Context getMockContext(boolean managedProfileExists,
            boolean isBluetoothContactsSharingDisabled) {
        DevicePolicyManager mockDpm = mock(DevicePolicyManager.class);
        doReturn(isBluetoothContactsSharingDisabled).when(mockDpm)
                .getBluetoothContactSharingDisabled(LittleMock.<UserHandle>anyObject());

        UserManager mockUm = mock(UserManager.class);
        doReturn(managedProfileExists ? MANAGED_USERINFO_LIST : NORMAL_USERINFO_LIST)
                .when(mockUm).getProfiles(anyInt());

        Context mockContext = mock(Context.class);
        doReturn(mockDpm).when(mockContext).getSystemService(Context.DEVICE_POLICY_SERVICE);
        doReturn(mockUm).when(mockContext).getSystemService(Context.USER_SERVICE);

        return mockContext;
    }
}

