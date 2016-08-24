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
package com.android.cts.deviceandprofileowner;

public class ResetPasswordTest extends BaseDeviceAdminTest {
    public void testResetPassword() {
        try {
            // DO/PO can set a password.
            mDevicePolicyManager.resetPassword("12345abcdef!!##1", 0);

            // DO/PO can change the password, even if one is set already.
            mDevicePolicyManager.resetPassword("12345abcdef!!##2", 0);
        } finally {
            // DO/PO can clear the password.
            mDevicePolicyManager.resetPassword("", 0);
        }
    }
}
