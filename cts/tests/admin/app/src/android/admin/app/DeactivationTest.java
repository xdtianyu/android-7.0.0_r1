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

package android.admin.app;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.test.AndroidTestCase;

/**
 * Helper to deactivate Device Admins.
 */
public class DeactivationTest extends AndroidTestCase {
    private static final String PACKAGE = CtsDeviceAdminReceiver.class.getPackage().getName();
    private static final ComponentName RECEIVER1 = new ComponentName(PACKAGE,
            CtsDeviceAdminReceiver.class.getName());
    private static final ComponentName RECEIVER2 = new ComponentName(PACKAGE,
            CtsDeviceAdminReceiver2.class.getName());

    public void testDeactivateAdmins() throws Exception {
        DevicePolicyManager manager = (DevicePolicyManager)
                getContext().getSystemService(Context.DEVICE_POLICY_SERVICE);
        assertNotNull(manager);

        manager.removeActiveAdmin(RECEIVER1);
        manager.removeActiveAdmin(RECEIVER2);

        for (int i = 0; i < 1000 && isActive(manager); i++) {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        assertFalse(isActive(manager));
    }

    private boolean isActive(DevicePolicyManager manager) {
        return manager.isAdminActive(RECEIVER1) ||
                manager.isAdminActive(RECEIVER2);
    }
}