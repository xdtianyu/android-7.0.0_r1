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

package android.appsecurity.cts;

import com.android.compatibility.common.util.AbiUtils;
import com.android.cts.migration.MigrationHelper;
import com.android.ddmlib.Log;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;

import java.io.File;
import java.io.FileNotFoundException;

/**
 * Set of tests that verify behavior of external storage devices.
 */
public class ExternalStorageHostTest extends DeviceTestCase
        implements IAbiReceiver, IBuildReceiver {
    private static final String TAG = "ExternalStorageHostTest";

    private static final String COMMON_CLASS =
            "com.android.cts.externalstorageapp.CommonExternalStorageTest";

    private static final String NONE_APK = "CtsExternalStorageApp.apk";
    private static final String NONE_PKG = "com.android.cts.externalstorageapp";
    private static final String NONE_CLASS = ".ExternalStorageTest";
    private static final String READ_APK = "CtsReadExternalStorageApp.apk";
    private static final String READ_PKG = "com.android.cts.readexternalstorageapp";
    private static final String READ_CLASS = ".ReadExternalStorageTest";
    private static final String WRITE_APK = "CtsWriteExternalStorageApp.apk";
    private static final String WRITE_PKG = "com.android.cts.writeexternalstorageapp";
    private static final String WRITE_CLASS = ".WriteExternalStorageTest";
    private static final String MULTIUSER_APK = "CtsMultiUserStorageApp.apk";
    private static final String MULTIUSER_PKG = "com.android.cts.multiuserstorageapp";
    private static final String MULTIUSER_CLASS = ".MultiUserStorageTest";

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

    private File getTestAppFile(String fileName) throws FileNotFoundException {
        return MigrationHelper.getTestFile(mCtsBuild, fileName);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        assertNotNull(mAbi);
        assertNotNull(mCtsBuild);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
    }

    /**
     * Verify that app with no external storage permissions works correctly.
     */
    public void testExternalStorageNone() throws Exception {
        final int[] users = createUsersForTest();
        try {
            wipePrimaryExternalStorage();

            getDevice().uninstallPackage(NONE_PKG);
            String[] options = {AbiUtils.createAbiFlag(mAbi.getName())};
            assertNull(getDevice().installPackage(getTestAppFile(NONE_APK), false, options));

            for (int user : users) {
                runDeviceTests(NONE_PKG, COMMON_CLASS, user);
                runDeviceTests(NONE_PKG, NONE_CLASS, user);
            }
        } finally {
            getDevice().uninstallPackage(NONE_PKG);
            removeUsersForTest(users);
        }
    }

    /**
     * Verify that app with
     * {@link android.Manifest.permission#READ_EXTERNAL_STORAGE} works
     * correctly.
     */
    public void testExternalStorageRead() throws Exception {
        final int[] users = createUsersForTest();
        try {
            wipePrimaryExternalStorage();

            getDevice().uninstallPackage(READ_PKG);
            String[] options = {AbiUtils.createAbiFlag(mAbi.getName())};
            assertNull(getDevice().installPackage(getTestAppFile(READ_APK), false, options));

            for (int user : users) {
                runDeviceTests(READ_PKG, COMMON_CLASS, user);
                runDeviceTests(READ_PKG, READ_CLASS, user);
            }
        } finally {
            getDevice().uninstallPackage(READ_PKG);
            removeUsersForTest(users);
        }
    }

    /**
     * Verify that app with
     * {@link android.Manifest.permission#WRITE_EXTERNAL_STORAGE} works
     * correctly.
     */
    public void testExternalStorageWrite() throws Exception {
        final int[] users = createUsersForTest();
        try {
            wipePrimaryExternalStorage();

            getDevice().uninstallPackage(WRITE_PKG);
            String[] options = {AbiUtils.createAbiFlag(mAbi.getName())};
            assertNull(getDevice().installPackage(getTestAppFile(WRITE_APK), false, options));

            for (int user : users) {
                runDeviceTests(WRITE_PKG, COMMON_CLASS, user);
                runDeviceTests(WRITE_PKG, WRITE_CLASS, user);
            }
        } finally {
            getDevice().uninstallPackage(WRITE_PKG);
            removeUsersForTest(users);
        }
    }

    /**
     * Verify that app with WRITE_EXTERNAL can leave gifts in external storage
     * directories belonging to other apps, and those apps can read.
     */
    public void testExternalStorageGifts() throws Exception {
        final int[] users = createUsersForTest();
        try {
            wipePrimaryExternalStorage();

            getDevice().uninstallPackage(NONE_PKG);
            getDevice().uninstallPackage(READ_PKG);
            getDevice().uninstallPackage(WRITE_PKG);
            final String[] options = {AbiUtils.createAbiFlag(mAbi.getName())};

            // We purposefully delay the installation of the reading apps to
            // verify that the daemon correctly invalidates any caches.
            assertNull(getDevice().installPackage(getTestAppFile(WRITE_APK), false, options));
            for (int user : users) {
                runDeviceTests(WRITE_PKG, ".WriteGiftTest", user);
            }

            assertNull(getDevice().installPackage(getTestAppFile(NONE_APK), false, options));
            assertNull(getDevice().installPackage(getTestAppFile(READ_APK), false, options));
            for (int user : users) {
                runDeviceTests(READ_PKG, ".ReadGiftTest", user);
                runDeviceTests(NONE_PKG, ".GiftTest", user);
            }
        } finally {
            getDevice().uninstallPackage(NONE_PKG);
            getDevice().uninstallPackage(READ_PKG);
            getDevice().uninstallPackage(WRITE_PKG);
            removeUsersForTest(users);
        }
    }

    /**
     * Test multi-user emulated storage environment, ensuring that each user has
     * isolated storage.
     */
    public void testMultiUserStorageIsolated() throws Exception {
        final int[] users = createUsersForTest();
        try {
            if (users.length == 1) {
                Log.d(TAG, "Single user device; skipping isolated storage tests");
                return;
            }

            final int owner = users[0];
            final int secondary = users[1];

            // Install our test app
            getDevice().uninstallPackage(MULTIUSER_PKG);
            String[] options = {AbiUtils.createAbiFlag(mAbi.getName())};
            final String installResult = getDevice()
                    .installPackage(getTestAppFile(MULTIUSER_APK), false, options);
            assertNull("Failed to install: " + installResult, installResult);

            // Clear data from previous tests
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testCleanIsolatedStorage", owner);
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testCleanIsolatedStorage", secondary);

            // Have both users try writing into isolated storage
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testWriteIsolatedStorage", owner);
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testWriteIsolatedStorage", secondary);

            // Verify they both have isolated view of storage
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testReadIsolatedStorage", owner);
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testReadIsolatedStorage", secondary);

            // Verify they can't poke at each other
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testUserIsolation", owner);
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testUserIsolation", secondary);
        } finally {
            getDevice().uninstallPackage(MULTIUSER_PKG);
            removeUsersForTest(users);
        }
    }

    private void wipePrimaryExternalStorage() throws DeviceNotAvailableException {
        getDevice().executeShellCommand("rm -rf /sdcard/Android");
        getDevice().executeShellCommand("rm -rf /sdcard/DCIM");
        getDevice().executeShellCommand("rm -rf /sdcard/MUST_*");
    }

    private int[] createUsersForTest() throws DeviceNotAvailableException {
        return Utils.createUsersForTest(getDevice());
    }

    private void removeUsersForTest(int[] users) throws DeviceNotAvailableException {
        Utils.removeUsersForTest(getDevice(), users);
    }

    private void runDeviceTests(String packageName, String testClassName, int userId)
            throws DeviceNotAvailableException {
        Utils.runDeviceTests(getDevice(), packageName, testClassName, userId);
    }

    private void runDeviceTests(String packageName, String testClassName, String testMethodName,
            int userId) throws DeviceNotAvailableException {
        Utils.runDeviceTests(getDevice(), packageName, testClassName, testMethodName, userId);
    }
}
