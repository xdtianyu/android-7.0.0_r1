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

import com.android.cts.migration.MigrationHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;

/**
 * Set of tests that verify behavior of runtime permissions, including both
 * dynamic granting and behavior of legacy apps.
 */
public class PermissionsHostTest extends DeviceTestCase implements IAbiReceiver, IBuildReceiver {
    private static final String PKG = "com.android.cts.usepermission";

    private static final String APK_22 = "CtsUsePermissionApp22.apk";
    private static final String APK_23 = "CtsUsePermissionApp23.apk";
    private static final String APK_24 = "CtsUsePermissionApp24.apk";

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
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();

        getDevice().uninstallPackage(PKG);
    }

    public void testFail() throws Exception {
        // Sanity check that remote failure is host failure
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        try {
            runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                    "testFail");
            fail("Expected remote failure");
        } catch (AssertionError expected) {
        }
    }

    public void testKill() throws Exception {
        // Sanity check that remote kill is host failure
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        try {
            runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                    "testKill");
            fail("Expected remote failure");
        } catch (AssertionError expected) {
        }
    }

    public void testCompatDefault22() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_22),
                false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest22",
                "testCompatDefault");
    }

    public void testCompatRevoked22() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_22),
                false, false));
        try {
            runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest22",
                    "testCompatRevoked_part1");
            fail("App must be killed on a permission revoke");
        } catch (AssertionError expected) {
        }
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest22",
                "testCompatRevoked_part2");
    }

    public void testNoRuntimePrompt22() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_22),
                false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest22",
                "testNoRuntimePrompt");
    }

    public void testDefault23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testDefault");
    }

    public void testGranted23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testGranted");
    }

    public void testInteractiveGrant23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testInteractiveGrant");
    }

    public void testRuntimeGroupGrantSpecificity23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testRuntimeGroupGrantSpecificity");
    }

    public void testRuntimeGroupGrantExpansion23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testRuntimeGroupGrantExpansion");
    }

    public void testCancelledPermissionRequest23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testCancelledPermissionRequest");
    }

    public void testRequestGrantedPermission23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testRequestGrantedPermission");
    }

    public void testDenialWithPrejudice23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testDenialWithPrejudice");
    }

    public void testRevokeAffectsWholeGroup23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        try {
            runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                    "testRevokeAffectsWholeGroup_part1");
        } catch (AssertionError expected) {
        }
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testRevokeAffectsWholeGroup_part2");
    }

    public void testGrantPreviouslyRevokedWithPrejudiceShowsPrompt23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        try {
            runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                    "testGrantPreviouslyRevokedWithPrejudiceShowsPrompt_part1");
            fail("App must be killed on a permission revoke");
        } catch (Throwable expected) {
        }
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testGrantPreviouslyRevokedWithPrejudiceShowsPrompt_part2");
    }

    public void testRequestNonRuntimePermission23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testRequestNonRuntimePermission");
    }

    public void testRequestNonExistentPermission23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testRequestNonExistentPermission");
    }

    public void testRequestPermissionFromTwoGroups23() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testRequestPermissionFromTwoGroups");
    }

//    public void testOnlyRequestedPermissionsGranted24() throws Exception {
//        assertNull(getDevice().installPackage(
//                MigrationHelper.getTestFile(mCtsBuild, APK_24), false, false));
//        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest24",
//                "testOnlyRequestedPermissionsGranted");
//    }

    public void testUpgradeKeepsPermissions() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_22), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest22",
                "testAllPermissionsGrantedByDefault");
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), true, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testAllPermissionsGrantedOnUpgrade");
    }

    public void testNoDowngradePermissionModel() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        try {
            assertNull(getDevice().installPackage(
                    MigrationHelper.getTestFile(mCtsBuild, APK_22), true, false));
            fail("Permission mode downgrade not allowed");
        } catch (AssertionError expected) {
        }
    }

    public void testNoResidualPermissionsOnUninstall() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testNoResidualPermissionsOnUninstall_part1");
        assertNull(getDevice().uninstallPackage(PKG));
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testNoResidualPermissionsOnUninstall_part2");
    }

    public void testRevokePropagatedOnUpgradeOldToNewModel() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_22), false, false));
        try {
            runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest22",
                    "testRevokePropagatedOnUpgradeOldToNewModel_part1");
            fail("App must be killed on a permission revoke");
        } catch (AssertionError expected) {
        }
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), true, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testRevokePropagatedOnUpgradeOldToNewModel_part2");
    }

    public void testRevokePropagatedOnUpgradeNewToNewModel() throws Exception {
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), false, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testRevokePropagatedOnUpgradeNewToNewModel_part1");
        assertNull(getDevice().installPackage(
                MigrationHelper.getTestFile(mCtsBuild, APK_23), true, false));
        runDeviceTests(PKG, "com.android.cts.usepermission.UsePermissionTest23",
                "testRevokePropagatedOnUpgradeNewToNewModel_part2");
    }

    private void runDeviceTests(String packageName, String testClassName, String testMethodName)
            throws DeviceNotAvailableException {
        Utils.runDeviceTests(getDevice(), packageName, testClassName, testMethodName);
    }
}
