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

package android.appsecurity.cts;

import android.appsecurity.cts.SplitTests.BaseInstallMultiple;

import com.android.ddmlib.Log;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;

/**
 * Set of tests that verify behavior of direct boot, if supported.
 * <p>
 * Note that these tests drive PIN setup manually instead of relying on device
 * administrators, which are not supported by all devices.
 */
public class DirectBootHostTest extends DeviceTestCase implements IAbiReceiver, IBuildReceiver {
    private static final String TAG = "DirectBootHostTest";

    private static final String PKG = "com.android.cts.encryptionapp";
    private static final String CLASS = ".EncryptionAppTest";
    private static final String APK = "CtsEncryptionApp.apk";

    private static final String OTHER_APK = "CtsSplitApp.apk";
    private static final String OTHER_PKG = "com.android.cts.splitapp";
    private static final String OTHER_CLASS = ".SplitAppTest";

    private static final String MODE_NATIVE = "native";
    private static final String MODE_EMULATED = "emulated";
    private static final String MODE_NONE = "none";

    private IAbi mAbi;
    private IBuildInfo mCtsBuild;

    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mCtsBuild = buildInfo;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        assertNotNull(mAbi);
        assertNotNull(mCtsBuild);

        getDevice().uninstallPackage(PKG);
        getDevice().uninstallPackage(OTHER_PKG);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();

        getDevice().uninstallPackage(PKG);
        getDevice().uninstallPackage(OTHER_PKG);
    }

    /**
     * If device has native FBE, verify lifecycle.
     */
    public void testDirectBootNative() throws Exception {
        if (!isSupportedDevice()) {
            Log.v(TAG, "Device not supported; skipping test");
            return;
        } else if (!MODE_NATIVE.equals(getFbeMode())) {
            Log.v(TAG, "Device doesn't have native FBE; skipping test");
            return;
        }

        doDirectBootTest(MODE_NATIVE);
    }

    /**
     * If device doesn't have native FBE, enable emulation and verify lifecycle.
     */
    public void testDirectBootEmulated() throws Exception {
        if (!isSupportedDevice()) {
            Log.v(TAG, "Device not supported; skipping test");
            return;
        } else if (MODE_NATIVE.equals(getFbeMode())) {
            Log.v(TAG, "Device has native FBE; skipping test");
            return;
        }

        doDirectBootTest(MODE_EMULATED);
    }

    /**
     * If device doesn't have native FBE, verify normal lifecycle.
     */
    public void testDirectBootNone() throws Exception {
        if (!isSupportedDevice()) {
            Log.v(TAG, "Device not supported; skipping test");
            return;
        } else if (MODE_NATIVE.equals(getFbeMode())) {
            Log.v(TAG, "Device has native FBE; skipping test");
            return;
        }

        doDirectBootTest(MODE_NONE);
    }

    public void doDirectBootTest(String mode) throws Exception {
        int[] users = {};
        try {
            users = createUsersForTest();

            // Set up test app and secure lock screens
            new InstallMultiple().addApk(APK).run();
            new InstallMultiple().addApk(OTHER_APK).run();

            // To receive boot broadcasts, kick our other app out of stopped state
            getDevice().executeShellCommand("am start -a android.intent.action.MAIN"
                    + " -c android.intent.category.LAUNCHER com.android.cts.splitapp/.MyActivity");

            // Give enough time for PackageManager to persist stopped state
            Thread.sleep(15000);

            runDeviceTests(PKG, CLASS, "testSetUp", users);

            // Give enough time for vold to update keys
            Thread.sleep(15000);

            // Reboot system into known state with keys ejected
            if (MODE_EMULATED.equals(mode)) {
                getDevice().executeShellCommand("sm set-emulate-fbe true");
                getDevice().waitForDeviceOnline();
            } else {
                getDevice().rebootUntilOnline();
            }
            waitForBootCompleted();

            if (MODE_NONE.equals(mode)) {
                runDeviceTests(PKG, CLASS, "testVerifyUnlockedAndDismiss", users);
            } else {
                runDeviceTests(PKG, CLASS, "testVerifyLockedAndDismiss", users);
            }

        } finally {
            try {
                // Remove secure lock screens and tear down test app
                runDeviceTests(PKG, CLASS, "testTearDown", users);
            } finally {
                getDevice().uninstallPackage(PKG);
                removeUsersForTest(users);

                // Get ourselves back into a known-good state
                if (MODE_EMULATED.equals(mode)) {
                    getDevice().executeShellCommand("sm set-emulate-fbe false");
                    getDevice().waitForDeviceOnline();
                } else {
                    getDevice().nonBlockingReboot();
                }
                waitForBootCompleted();
            }
        }
    }

    private int[] createUsersForTest() throws DeviceNotAvailableException {
        // TODO: enable test for multiple users
        return new int[] { 0 };
//        return Utils.createUsersForTest(getDevice());
    }

    private void removeUsersForTest(int[] users) throws DeviceNotAvailableException {
        Utils.removeUsersForTest(getDevice(), users);
    }

    private void runDeviceTests(String packageName, String testClassName, String testMethodName,
            int... users) throws DeviceNotAvailableException {
        for (int user : users) {
            Log.d(TAG, "runDeviceTests " + testMethodName + " u" + user);
            Utils.runDeviceTests(getDevice(), packageName, testClassName, testMethodName, user);
        }
    }

    private String getFbeMode() throws Exception {
        return getDevice().executeShellCommand("sm get-fbe-mode").trim();
    }

    private boolean isBootCompleted() throws Exception {
        return "1".equals(getDevice().executeShellCommand("getprop sys.boot_completed").trim());
    }

    private boolean isSupportedDevice() throws Exception {
        final String featureList = getDevice().executeShellCommand("pm list features");
        if (featureList.contains("feature:android.hardware.type.watch\n") ||
                featureList.contains("feature:android.hardware.type.television\n")) {
            return false;
        } else {
            return true;
        }
    }

    private void waitForBootCompleted() throws Exception {
        for (int i = 0; i < 45; i++) {
            if (isBootCompleted()) {
                Log.d(TAG, "Yay, system is ready!");
                return;
            }
            Log.d(TAG, "Waiting for system ready...");
            Thread.sleep(1000);
        }
        throw new AssertionError("System failed to become ready!");
    }

    private class InstallMultiple extends BaseInstallMultiple<InstallMultiple> {
        public InstallMultiple() {
            super(getDevice(), mCtsBuild, mAbi);
        }
    }
}
