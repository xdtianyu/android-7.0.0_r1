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

package com.android.cts.usepermission;

import android.Manifest;
import android.content.pm.PackageManager;
import org.junit.Test;

import static junit.framework.Assert.assertEquals;

/**
 * Runtime permission behavior tests for apps targeting API 24
 */
public class UsePermissionTest24 extends BasePermissionsTest {
    private static final int REQUEST_CODE_PERMISSIONS = 42;

    @Test
    public void testOnlyRequestedPermissionsGranted() throws Exception {
        // Start out without permission
        assertEquals(PackageManager.PERMISSION_DENIED, getInstrumentation().getContext()
                .checkSelfPermission(Manifest.permission.RECEIVE_SMS));
        assertEquals(PackageManager.PERMISSION_DENIED, getInstrumentation().getContext()
                .checkSelfPermission(Manifest.permission.SEND_SMS));

        String[] firstPermissions = new String[] {Manifest.permission.RECEIVE_SMS};

        // Request only one permission and confirm
        BasePermissionActivity.Result firstResult = requestPermissions(firstPermissions,
                REQUEST_CODE_PERMISSIONS,
                BasePermissionActivity.class,
                () -> {
                    try {
                        clickAllowButton();
                        getUiDevice().waitForIdle();
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                });

        // Expect the permission is granted
        assertPermissionRequestResult(firstResult, REQUEST_CODE_PERMISSIONS,
                firstPermissions, new boolean[] {true});

        // We should not have the other permission in the group
        assertEquals(PackageManager.PERMISSION_DENIED, getInstrumentation().getContext()
                .checkSelfPermission(Manifest.permission.SEND_SMS));

        String[] secondPermissions = new String[] {Manifest.permission.SEND_SMS};

        // Request the other permission which should be auto-granted
        BasePermissionActivity.Result secondResult = requestPermissions(secondPermissions,
                REQUEST_CODE_PERMISSIONS + 1, BasePermissionActivity.class, null);

        // Expect the permission is granted
        assertPermissionRequestResult(secondResult, REQUEST_CODE_PERMISSIONS + 1,
                secondPermissions, new boolean[] {true});

        // We now should have both permissions
        assertEquals(PackageManager.PERMISSION_GRANTED, getInstrumentation().getContext()
                .checkSelfPermission(Manifest.permission.RECEIVE_SMS));
        assertEquals(PackageManager.PERMISSION_GRANTED, getInstrumentation().getContext()
                .checkSelfPermission(Manifest.permission.SEND_SMS));
    }
}
