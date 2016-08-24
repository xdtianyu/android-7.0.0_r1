/*
 * Copyright (C) 2011 The Android Open Source Project
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

package android.admin.cts;

import android.app.admin.DeviceAdminReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.platform.test.annotations.Presubmit;
import android.test.AndroidTestCase;
import android.util.Log;

public class DeviceAdminReceiverTest extends AndroidTestCase {

    private static final String TAG = DeviceAdminReceiverTest.class.getSimpleName();
    private static final String DISABLE_WARNING = "Disable Warning";
    private static final String BUGREPORT_HASH = "f4k3h45h";

    private static final String ACTION_BUGREPORT_SHARING_DECLINED =
            "android.app.action.BUGREPORT_SHARING_DECLINED";
    private static final String ACTION_BUGREPORT_FAILED = "android.app.action.BUGREPORT_FAILED";
    private static final String ACTION_BUGREPORT_SHARE =
            "android.app.action.BUGREPORT_SHARE";
    private static final String ACTION_SECURITY_LOGS_AVAILABLE
            = "android.app.action.SECURITY_LOGS_AVAILABLE";
    private static final String EXTRA_BUGREPORT_FAILURE_REASON =
            "android.app.extra.BUGREPORT_FAILURE_REASON";
    private static final String EXTRA_BUGREPORT_HASH = "android.app.extra.BUGREPORT_HASH";

    private static final int PASSWORD_CHANGED = 0x1;
    private static final int PASSWORD_FAILED = 0x2;
    private static final int PASSWORD_SUCCEEDED = 0x4;
    private static final int DEVICE_ADMIN_ENABLED = 0x8;
    private static final int DEVICE_ADMIN_DISABLE_REQUESTED = 0x10;
    private static final int DEVICE_ADMIN_DISABLED = 0x20;
    private static final int BUGREPORT_SHARING_DECLINED = 0x40;
    private static final int BUGREPORT_FAILED = 0x80;
    private static final int BUGREPORT_SHARED = 0x100;
    private static final int SECURITY_LOGS_AVAILABLE = 0x200;

    private TestReceiver mReceiver;
    private boolean mDeviceAdmin;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mReceiver = new TestReceiver();
        mDeviceAdmin =
                mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_DEVICE_ADMIN);
    }

    @Presubmit
    public void testOnReceive() {
        if (!mDeviceAdmin) {
            Log.w(TAG, "Skipping testOnReceive");
            return;
        }
        mReceiver.reset();
        mReceiver.onReceive(mContext, new Intent(DeviceAdminReceiver.ACTION_PASSWORD_CHANGED));
        assertTrue(mReceiver.hasFlags(PASSWORD_CHANGED));

        mReceiver.reset();
        mReceiver.onReceive(mContext, new Intent(DeviceAdminReceiver.ACTION_PASSWORD_FAILED));
        assertTrue(mReceiver.hasFlags(PASSWORD_FAILED));

        mReceiver.reset();
        mReceiver.onReceive(mContext, new Intent(DeviceAdminReceiver.ACTION_PASSWORD_SUCCEEDED));
        assertTrue(mReceiver.hasFlags(PASSWORD_SUCCEEDED));

        mReceiver.reset();
        mReceiver.onReceive(mContext, new Intent(DeviceAdminReceiver.ACTION_DEVICE_ADMIN_ENABLED));
        assertTrue(mReceiver.hasFlags(DEVICE_ADMIN_ENABLED));

        mReceiver.reset();
        mReceiver.onReceive(mContext, new Intent(DeviceAdminReceiver.ACTION_DEVICE_ADMIN_DISABLED));
        assertTrue(mReceiver.hasFlags(DEVICE_ADMIN_DISABLED));

        mReceiver.reset();
        mReceiver.onReceive(mContext, new Intent(ACTION_BUGREPORT_SHARING_DECLINED));
        assertTrue(mReceiver.hasFlags(BUGREPORT_SHARING_DECLINED));

        mReceiver.reset();
        Intent bugreportFailedIntent = new Intent(ACTION_BUGREPORT_FAILED);
        bugreportFailedIntent.putExtra(EXTRA_BUGREPORT_FAILURE_REASON,
                DeviceAdminReceiver.BUGREPORT_FAILURE_FAILED_COMPLETING);
        mReceiver.onReceive(mContext, bugreportFailedIntent);
        assertTrue(mReceiver.hasFlags(BUGREPORT_FAILED));
        assertEquals(DeviceAdminReceiver.BUGREPORT_FAILURE_FAILED_COMPLETING,
                mReceiver.getBugreportFailureCode());

        mReceiver.reset();
        Intent bugreportSharedIntent = new Intent(ACTION_BUGREPORT_SHARE);
        bugreportSharedIntent.putExtra(EXTRA_BUGREPORT_HASH, BUGREPORT_HASH);
        mReceiver.onReceive(mContext, bugreportSharedIntent);
        assertTrue(mReceiver.hasFlags(BUGREPORT_SHARED));
        assertEquals(BUGREPORT_HASH, mReceiver.getBugreportHash());

        mReceiver.reset();
        mReceiver.onReceive(mContext, new Intent(ACTION_SECURITY_LOGS_AVAILABLE));
        assertTrue(mReceiver.hasFlags(SECURITY_LOGS_AVAILABLE));
    }

    private class TestReceiver extends DeviceAdminReceiver {

        private int mFlags = 0;
        private int bugreportFailureCode = -1;
        private String bugreportHash;

        void reset() {
            mFlags = 0;
            bugreportFailureCode = -1;
            bugreportHash = null;
        }

        boolean hasFlags(int flags) {
            return mFlags == flags;
        }

        int getBugreportFailureCode() {
            return bugreportFailureCode;
        }

        String getBugreportHash() {
            return bugreportHash;
        }

        @Override
        public void onPasswordChanged(Context context, Intent intent) {
            super.onPasswordChanged(context, intent);
            mFlags |= PASSWORD_CHANGED;
        }

        @Override
        public void onPasswordFailed(Context context, Intent intent) {
            super.onPasswordFailed(context, intent);
            mFlags |= PASSWORD_FAILED;
        }

        @Override
        public void onPasswordSucceeded(Context context, Intent intent) {
            super.onPasswordSucceeded(context, intent);
            mFlags |= PASSWORD_SUCCEEDED;
        }

        @Override
        public void onEnabled(Context context, Intent intent) {
            super.onEnabled(context, intent);
            mFlags |= DEVICE_ADMIN_ENABLED;
        }

        @Override
        public CharSequence onDisableRequested(Context context, Intent intent) {
            mFlags |= DEVICE_ADMIN_DISABLE_REQUESTED;
            return DISABLE_WARNING;
        }

        @Override
        public void onDisabled(Context context, Intent intent) {
            super.onDisabled(context, intent);
            mFlags |= DEVICE_ADMIN_DISABLED;
        }

        @Override
        public void onBugreportSharingDeclined(Context context, Intent intent) {
            super.onBugreportSharingDeclined(context, intent);
            mFlags |= BUGREPORT_SHARING_DECLINED;
        }

        @Override
        public void onBugreportFailed(Context context, Intent intent, int failureCode) {
            super.onBugreportFailed(context, intent, failureCode);
            mFlags |= BUGREPORT_FAILED;
            bugreportFailureCode = failureCode;
        }

        @Override
        public void onBugreportShared(Context context, Intent intent, String bugreportHash) {
            super.onBugreportShared(context, intent, bugreportHash);
            mFlags |= BUGREPORT_SHARED;
            this.bugreportHash = bugreportHash;
        }

        @Override
        public void onSecurityLogsAvailable(Context context, Intent intent) {
            super.onSecurityLogsAvailable(context, intent);
            mFlags |= SECURITY_LOGS_AVAILABLE;
        }
    }
}
