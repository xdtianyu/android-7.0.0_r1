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

package com.android.cts.devicepolicy;

import com.android.cts.migration.MigrationHelper;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;

import java.io.File;
import java.io.FileNotFoundException;

/**
 * Set of tests for use cases that apply to profile and device owner.
 * This class is the base class of MixedProfileOwnerTest, MixedDeviceOwnerTest and
 * MixedManagedProfileOwnerTest and is abstract to avoid running spurious tests.
 *
 * NOTE: Not all tests are executed in the subclasses.  Sometimes, if a test is not applicable to
 * a subclass, they override it with an empty method.
 */
public abstract class DeviceAndProfileOwnerTest extends BaseDevicePolicyTest {

    protected static final String DEVICE_ADMIN_PKG = "com.android.cts.deviceandprofileowner";
    protected static final String DEVICE_ADMIN_APK = "CtsDeviceAndProfileOwnerApp.apk";
    protected static final String ADMIN_RECEIVER_TEST_CLASS
            = ".BaseDeviceAdminTest$BasicAdminReceiver";

    private static final String INTENT_RECEIVER_PKG = "com.android.cts.intent.receiver";
    private static final String INTENT_RECEIVER_APK = "CtsIntentReceiverApp.apk";

    private static final String INTENT_SENDER_PKG = "com.android.cts.intent.sender";
    private static final String INTENT_SENDER_APK = "CtsIntentSenderApp.apk";

    private static final String PERMISSIONS_APP_PKG = "com.android.cts.permissionapp";
    private static final String PERMISSIONS_APP_APK = "CtsPermissionApp.apk";

    private static final String SIMPLE_PRE_M_APP_PKG = "com.android.cts.launcherapps.simplepremapp";
    private static final String SIMPLE_PRE_M_APP_APK = "CtsSimplePreMApp.apk";

    private static final String APP_RESTRICTIONS_MANAGING_APP_PKG
            = "com.android.cts.apprestrictions.managingapp";
    private static final String APP_RESTRICTIONS_MANAGING_APP_APK
            = "CtsAppRestrictionsManagingApp.apk";
    private static final String APP_RESTRICTIONS_TARGET_APP_PKG
            = "com.android.cts.apprestrictions.targetapp";
    private static final String APP_RESTRICTIONS_TARGET_APP_APK = "CtsAppRestrictionsTargetApp.apk";

    private static final String CERT_INSTALLER_PKG = "com.android.cts.certinstaller";
    private static final String CERT_INSTALLER_APK = "CtsCertInstallerApp.apk";

    private static final String TEST_APP_APK = "CtsSimpleApp.apk";
    private static final String TEST_APP_PKG = "com.android.cts.launcherapps.simpleapp";
    private static final String TEST_APP_LOCATION = "/data/local/tmp/";

    private static final String PACKAGE_INSTALLER_PKG = "com.android.cts.packageinstaller";
    private static final String PACKAGE_INSTALLER_APK = "CtsPackageInstallerApp.apk";

    private static final String ACCOUNT_MANAGEMENT_PKG
            = "com.android.cts.devicepolicy.accountmanagement";
    private static final String ACCOUNT_MANAGEMENT_APK = "CtsAccountManagementDevicePolicyApp.apk";

    private static final String VPN_APP_PKG = "com.android.cts.vpnfirewall";
    private static final String VPN_APP_APK = "CtsVpnFirewallApp.apk";

    private static final String COMMAND_ADD_USER_RESTRICTION = "add-restriction";
    private static final String COMMAND_CLEAR_USER_RESTRICTION = "clear-restriction";
    private static final String COMMAND_BLOCK_ACCOUNT_TYPE = "block-accounttype";
    private static final String COMMAND_UNBLOCK_ACCOUNT_TYPE = "unblock-accounttype";

    private static final String DISALLOW_MODIFY_ACCOUNTS = "no_modify_accounts";
    private static final String ACCOUNT_TYPE
            = "com.android.cts.devicepolicy.accountmanagement.account.type";

    private static final String CUSTOMIZATION_APP_PKG = "com.android.cts.customizationapp";
    private static final String CUSTOMIZATION_APP_APK = "CtsCustomizationApp.apk";

    // ID of the user all tests are run as. For device owner this will be the primary user, for
    // profile owner it is the user id of the created profile.
    protected int mUserId;

    @Override
    protected void tearDown() throws Exception {
        if (mHasFeature) {
            getDevice().uninstallPackage(DEVICE_ADMIN_PKG);
            getDevice().uninstallPackage(PERMISSIONS_APP_PKG);
            getDevice().uninstallPackage(SIMPLE_PRE_M_APP_PKG);
            getDevice().uninstallPackage(APP_RESTRICTIONS_MANAGING_APP_PKG);
            getDevice().uninstallPackage(APP_RESTRICTIONS_TARGET_APP_PKG);
            getDevice().uninstallPackage(CERT_INSTALLER_PKG);
            getDevice().uninstallPackage(ACCOUNT_MANAGEMENT_PKG);
            getDevice().uninstallPackage(VPN_APP_PKG);
            getDevice().uninstallPackage(INTENT_RECEIVER_PKG);
            getDevice().uninstallPackage(INTENT_SENDER_PKG);
            getDevice().uninstallPackage(CUSTOMIZATION_APP_PKG);
        }
        super.tearDown();
    }

    public void testResetPassword() throws Exception {
        if (!mHasFeature) {
            return;
        }
        executeDeviceTestClass(".ResetPasswordTest");
    }

    public void testApplicationRestrictions() throws Exception {
        if (!mHasFeature) {
            return;
        }

        installAppAsUser(APP_RESTRICTIONS_MANAGING_APP_APK, mUserId);
        installAppAsUser(APP_RESTRICTIONS_TARGET_APP_APK, mUserId);

        try {
            // Only the DPC can manage app restrictions by default.
            executeDeviceTestClass(".ApplicationRestrictionsTest");
            executeAppRestrictionsManagingPackageTest("testCannotManageAppRestrictions");

            // Letting the APP_RESTRICTIONS_MANAGING_APP_PKG manage app restrictions too.
            changeApplicationRestrictionsManagingPackage(APP_RESTRICTIONS_MANAGING_APP_PKG);
            executeAppRestrictionsManagingPackageTest("testCanManageAppRestrictions");
            executeAppRestrictionsManagingPackageTest("testSettingComponentNameThrowsException");

            // The DPC should still be able to manage app restrictions normally.
            executeDeviceTestClass(".ApplicationRestrictionsTest");

            // The app shouldn't be able to manage app restrictions for other users.
            int parentUserId = getPrimaryUser();
            if (parentUserId != mUserId) {
                installAppAsUser(APP_RESTRICTIONS_MANAGING_APP_APK, parentUserId);
                installAppAsUser(APP_RESTRICTIONS_TARGET_APP_APK, parentUserId);
                assertTrue(runDeviceTestsAsUser(
                        APP_RESTRICTIONS_MANAGING_APP_PKG, ".ApplicationRestrictionsManagerTest",
                        "testCannotManageAppRestrictions", parentUserId));
            }

            // Revoking the permission for APP_RESTRICTIONS_MANAGING_APP_PKG to manage restrictions.
            changeApplicationRestrictionsManagingPackage(null);
            executeAppRestrictionsManagingPackageTest("testCannotManageAppRestrictions");

            // The DPC should still be able to manage app restrictions normally.
            executeDeviceTestClass(".ApplicationRestrictionsTest");
        } finally {
            changeApplicationRestrictionsManagingPackage(null);
        }
    }

    public void testPermissionGrant() throws Exception {
        if (!mHasFeature) {
            return;
        }
        installAppPermissionAppAsUser();
        executeDeviceTestMethod(".PermissionsTest", "testPermissionGrantState");
    }

    public void testAlwaysOnVpn() throws Exception {
        if (!mHasFeature) {
            return;
        }
        installAppAsUser(VPN_APP_APK, mUserId);
        executeDeviceTestClass(".AlwaysOnVpnTest");
    }

    public void testPermissionPolicy() throws Exception {
        if (!mHasFeature) {
            return;
        }
        installAppPermissionAppAsUser();
        executeDeviceTestMethod(".PermissionsTest", "testPermissionPolicy");
    }

    public void testPermissionMixedPolicies() throws Exception {
        if (!mHasFeature) {
            return;
        }
        installAppPermissionAppAsUser();
        executeDeviceTestMethod(".PermissionsTest", "testPermissionMixedPolicies");
    }

    // Test flakey; suppressed.
//    public void testPermissionPrompts() throws Exception {
//        if (!mHasFeature) {
//            return;
//        }
//        installAppPermissionAppAsUser();
//        executeDeviceTestMethod(".PermissionsTest", "testPermissionPrompts");
//    }

    public void testPermissionAppUpdate() throws Exception {
        if (!mHasFeature) {
            return;
        }
        installAppPermissionAppAsUser();
        executeDeviceTestMethod(".PermissionsTest", "testPermissionUpdate_setDeniedState");
        executeDeviceTestMethod(".PermissionsTest", "testPermissionUpdate_checkDenied");
        installAppPermissionAppAsUser();
        executeDeviceTestMethod(".PermissionsTest", "testPermissionUpdate_checkDenied");

        assertNull(getDevice().uninstallPackage(PERMISSIONS_APP_PKG));
        installAppPermissionAppAsUser();
        executeDeviceTestMethod(".PermissionsTest", "testPermissionUpdate_setGrantedState");
        executeDeviceTestMethod(".PermissionsTest", "testPermissionUpdate_checkGranted");
        installAppPermissionAppAsUser();
        executeDeviceTestMethod(".PermissionsTest", "testPermissionUpdate_checkGranted");

        assertNull(getDevice().uninstallPackage(PERMISSIONS_APP_PKG));
        installAppPermissionAppAsUser();
        executeDeviceTestMethod(".PermissionsTest", "testPermissionUpdate_setAutoDeniedPolicy");
        executeDeviceTestMethod(".PermissionsTest", "testPermissionUpdate_checkDenied");
        installAppPermissionAppAsUser();
        executeDeviceTestMethod(".PermissionsTest", "testPermissionUpdate_checkDenied");

        assertNull(getDevice().uninstallPackage(PERMISSIONS_APP_PKG));
        installAppPermissionAppAsUser();
        executeDeviceTestMethod(".PermissionsTest", "testPermissionUpdate_setAutoGrantedPolicy");
        executeDeviceTestMethod(".PermissionsTest", "testPermissionUpdate_checkGranted");
        installAppPermissionAppAsUser();
        executeDeviceTestMethod(".PermissionsTest", "testPermissionUpdate_checkGranted");
    }

    public void testPermissionGrantPreMApp() throws Exception {
        if (!mHasFeature) {
            return;
        }
        installAppAsUser(SIMPLE_PRE_M_APP_APK, mUserId);
        executeDeviceTestMethod(".PermissionsTest", "testPermissionGrantStatePreMApp");
    }

    public void testPersistentIntentResolving() throws Exception {
        if (!mHasFeature) {
            return;
        }
        executeDeviceTestClass(".PersistentIntentResolvingTest");
    }

    public void testScreenCaptureDisabled() throws Exception {
        if (!mHasFeature) {
            return;
        }
        // We need to ensure that the policy is deactivated for the device owner case, so making
        // sure the second test is run even if the first one fails
        try {
            executeDeviceTestMethod(".ScreenCaptureDisabledTest",
                    "testSetScreenCaptureDisabled_true");
        } finally {
            executeDeviceTestMethod(".ScreenCaptureDisabledTest",
                    "testSetScreenCaptureDisabled_false");
        }
    }

    public void testSupportMessage() throws Exception {
        if (!mHasFeature) {
            return;
        }
        installAppAsUser(APP_RESTRICTIONS_MANAGING_APP_APK, mUserId);
        executeDeviceTestClass(".SupportMessageTest");
    }

    public void testApplicationHidden() throws Exception {
        if (!mHasFeature) {
            return;
        }
        installAppPermissionAppAsUser();
        executeDeviceTestClass(".ApplicationHiddenTest");
    }

    public void testAccountManagement_deviceAndProfileOwnerAlwaysAllowed() throws Exception {
        if (!mHasFeature) {
            return;
        }

        installAppAsUser(ACCOUNT_MANAGEMENT_APK, mUserId);
        executeDeviceTestClass(".DpcAllowedAccountManagementTest");
    }

    public void testAccountManagement_userRestrictionAddAccount() throws Exception {
        if (!mHasFeature) {
            return;
        }

        installAppAsUser(ACCOUNT_MANAGEMENT_APK, mUserId);
        try {
            changeUserRestrictionForUser(DISALLOW_MODIFY_ACCOUNTS, COMMAND_ADD_USER_RESTRICTION,
                    mUserId);
            executeAccountTest("testAddAccount_blocked");
        } finally {
            // Ensure we clear the user restriction
            changeUserRestrictionForUser(DISALLOW_MODIFY_ACCOUNTS, COMMAND_CLEAR_USER_RESTRICTION,
                    mUserId);
        }
        executeAccountTest("testAddAccount_allowed");
    }

    public void testAccountManagement_userRestrictionRemoveAccount() throws Exception {
        if (!mHasFeature) {
            return;
        }

        installAppAsUser(ACCOUNT_MANAGEMENT_APK, mUserId);
        try {
            changeUserRestrictionForUser(DISALLOW_MODIFY_ACCOUNTS, COMMAND_ADD_USER_RESTRICTION,
                    mUserId);
            executeAccountTest("testRemoveAccount_blocked");
        } finally {
            // Ensure we clear the user restriction
            changeUserRestrictionForUser(DISALLOW_MODIFY_ACCOUNTS, COMMAND_CLEAR_USER_RESTRICTION,
                    mUserId);
        }
        executeAccountTest("testRemoveAccount_allowed");
    }

    public void testAccountManagement_disabledAddAccount() throws Exception {
        if (!mHasFeature) {
            return;
        }

        installAppAsUser(ACCOUNT_MANAGEMENT_APK, mUserId);
        try {
            changeAccountManagement(COMMAND_BLOCK_ACCOUNT_TYPE, ACCOUNT_TYPE, mUserId);
            executeAccountTest("testAddAccount_blocked");
        } finally {
            // Ensure we remove account management policies
            changeAccountManagement(COMMAND_UNBLOCK_ACCOUNT_TYPE, ACCOUNT_TYPE, mUserId);
        }
        executeAccountTest("testAddAccount_allowed");
    }

    public void testAccountManagement_disabledRemoveAccount() throws Exception {
        if (!mHasFeature) {
            return;
        }

        installAppAsUser(ACCOUNT_MANAGEMENT_APK, mUserId);
        try {
            changeAccountManagement(COMMAND_BLOCK_ACCOUNT_TYPE, ACCOUNT_TYPE, mUserId);
            executeAccountTest("testRemoveAccount_blocked");
        } finally {
            // Ensure we remove account management policies
            changeAccountManagement(COMMAND_UNBLOCK_ACCOUNT_TYPE, ACCOUNT_TYPE, mUserId);
        }
        executeAccountTest("testRemoveAccount_allowed");
    }

    public void testDelegatedCertInstaller() throws Exception {
        if (!mHasFeature) {
            return;
        }

        installAppAsUser(CERT_INSTALLER_APK, mUserId);

        boolean isManagedProfile = (mPrimaryUserId != mUserId);

        try {
            // Set a non-empty device lockscreen password, which is a precondition for installing
            // private key pairs.
            assertTrue("Set lockscreen password failed", runDeviceTestsAsUser(DEVICE_ADMIN_PKG,
                    ".ResetPasswordHelper", "testSetPassword", mUserId));
            assertTrue("DelegatedCertInstaller failed", runDeviceTestsAsUser(DEVICE_ADMIN_PKG,
                    ".DelegatedCertInstallerTest", mUserId));
        } finally {
            if (!isManagedProfile) {
                // Skip managed profile as dpm doesn't allow clear password
                assertTrue("Clear lockscreen password failed", runDeviceTestsAsUser(DEVICE_ADMIN_PKG,
                        ".ResetPasswordHelper", "testClearPassword", mUserId));
            }
        }
    }

    // Sets restrictions and launches non-admin app, that tries to set wallpaper.
    // Non-admin apps must not violate any user restriction.
    public void testSetWallpaper_disallowed() throws Exception {
        // UserManager.DISALLOW_SET_WALLPAPER
        final String DISALLOW_SET_WALLPAPER = "no_set_wallpaper";
        if (!mHasFeature) {
            return;
        }

        installAppAsUser(CUSTOMIZATION_APP_APK, mUserId);
        try {
            changeUserRestrictionForUser(DISALLOW_SET_WALLPAPER, COMMAND_ADD_USER_RESTRICTION,
                    mUserId);
            assertTrue(runDeviceTestsAsUser(CUSTOMIZATION_APP_PKG, ".CustomizationTest",
                "testSetWallpaper_disallowed", mUserId));
        } finally {
            changeUserRestrictionForUser(DISALLOW_SET_WALLPAPER, COMMAND_CLEAR_USER_RESTRICTION,
                    mUserId);
        }
    }

    // Runs test with admin privileges. The test methods set all the tested restrictions
    // inside. But these restrictions must have no effect on the device/profile owner behavior.
    public void testDisallowSetWallpaper_allowed() throws Exception {
        if (!mHasFeature) {
            return;
        }
        executeDeviceTestMethod(".CustomizationRestrictionsTest",
                "testDisallowSetWallpaper_allowed");
    }

    public void testDisallowSetUserIcon_allowed() throws Exception {
        if (!mHasFeature) {
            return;
        }
        executeDeviceTestMethod(".CustomizationRestrictionsTest",
                "testDisallowSetUserIcon_allowed");
    }

    public void testPackageInstallUserRestrictions() throws Exception {
        if (!mHasFeature) {
            return;
        }
        // UserManager.DISALLOW_INSTALL_UNKNOWN_SOURCES
        final String DISALLOW_INSTALL_UNKNOWN_SOURCES = "no_install_unknown_sources";
        final String UNKNOWN_SOURCES_SETTING = "install_non_market_apps";
        final String PACKAGE_VERIFIER_USER_CONSENT_SETTING = "package_verifier_user_consent";
        final String PACKAGE_VERIFIER_ENABLE_SETTING = "package_verifier_enable";
        final String SECURE_SETTING_CATEGORY = "secure";
        final String GLOBAL_SETTING_CATEGORY = "global";
        final File apk = MigrationHelper.getTestFile(mCtsBuild, TEST_APP_APK);
        String unknownSourceSetting = null;
        String packageVerifierEnableSetting = null;
        String packageVerifierUserConsentSetting = null;
        try {
            // Install the test and prepare the test apk.
            installAppAsUser(PACKAGE_INSTALLER_APK, mUserId);
            assertTrue(getDevice().pushFile(apk, TEST_APP_LOCATION + apk.getName()));

            // Add restrictions and test if we can install the apk.
            getDevice().uninstallPackage(TEST_APP_PKG);
            changeUserRestrictionForUser(DISALLOW_INSTALL_UNKNOWN_SOURCES,
                    COMMAND_ADD_USER_RESTRICTION, mUserId);
            assertTrue(runDeviceTestsAsUser(PACKAGE_INSTALLER_PKG, ".ManualPackageInstallTest",
                    "testManualInstallBlocked", mUserId));

            // Clear restrictions and test if we can install the apk.
            changeUserRestrictionForUser(DISALLOW_INSTALL_UNKNOWN_SOURCES,
                    COMMAND_CLEAR_USER_RESTRICTION, mUserId);

            // Enable Unknown sources in Settings.
            unknownSourceSetting =
                    getSettings(SECURE_SETTING_CATEGORY, UNKNOWN_SOURCES_SETTING, mUserId);
            packageVerifierUserConsentSetting = getSettings(SECURE_SETTING_CATEGORY,
                    PACKAGE_VERIFIER_USER_CONSENT_SETTING, mUserId);
            packageVerifierEnableSetting = getSettings(GLOBAL_SETTING_CATEGORY,
                    PACKAGE_VERIFIER_ENABLE_SETTING, mUserId);

            putSettings(SECURE_SETTING_CATEGORY, UNKNOWN_SOURCES_SETTING, "1", mUserId);
            putSettings(SECURE_SETTING_CATEGORY, PACKAGE_VERIFIER_USER_CONSENT_SETTING, "-1",
                    mUserId);
            putSettings(GLOBAL_SETTING_CATEGORY, PACKAGE_VERIFIER_ENABLE_SETTING, "0", mUserId);
            assertEquals("1",
                    getSettings(SECURE_SETTING_CATEGORY, UNKNOWN_SOURCES_SETTING, mUserId));
            assertEquals("-1", getSettings(SECURE_SETTING_CATEGORY,
                    PACKAGE_VERIFIER_USER_CONSENT_SETTING, mUserId));
            assertEquals("0", getSettings(GLOBAL_SETTING_CATEGORY,
                    PACKAGE_VERIFIER_ENABLE_SETTING, mUserId));
            assertTrue(runDeviceTestsAsUser(PACKAGE_INSTALLER_PKG, ".ManualPackageInstallTest",
                    "testManualInstallSucceeded", mUserId));
        } finally {
            String command = "rm " + TEST_APP_LOCATION + apk.getName();
            getDevice().executeShellCommand(command);
            getDevice().uninstallPackage(TEST_APP_PKG);
            getDevice().uninstallPackage(PACKAGE_INSTALLER_APK);
            if (unknownSourceSetting != null) {
                putSettings(SECURE_SETTING_CATEGORY, UNKNOWN_SOURCES_SETTING, unknownSourceSetting,
                        mUserId);
            }
            if (packageVerifierEnableSetting != null) {
                putSettings(GLOBAL_SETTING_CATEGORY, PACKAGE_VERIFIER_ENABLE_SETTING,
                        packageVerifierEnableSetting, mUserId);
            }
            if (packageVerifierUserConsentSetting != null) {
                putSettings(SECURE_SETTING_CATEGORY, PACKAGE_VERIFIER_USER_CONSENT_SETTING,
                        packageVerifierUserConsentSetting, mUserId);
            }
        }
    }

    public void testAudioRestriction() throws Exception {
        if (!mHasFeature) {
            return;
        }
        executeDeviceTestClass(".AudioRestrictionTest");
    }

    public void testSuspendPackage() throws Exception {
        if (!mHasFeature) {
            return;
        }
        installAppAsUser(INTENT_SENDER_APK, mUserId);
        installAppAsUser(INTENT_RECEIVER_APK, mUserId);
        // Suspend a testing package.
        executeDeviceTestMethod(".SuspendPackageTest", "testSetPackagesSuspended");
        // Verify that the package is suspended.
        executeSuspendPackageTestMethod("testPackageSuspended");
        // Undo the suspend.
        executeDeviceTestMethod(".SuspendPackageTest", "testSetPackagesNotSuspended");
        // Verify that the package is not suspended.
        executeSuspendPackageTestMethod("testPackageNotSuspended");
        // Verify we cannot suspend not suspendable packages.
        executeDeviceTestMethod(".SuspendPackageTest", "testSuspendNotSuspendablePackages");
    }

    public void testTrustAgentInfo() throws Exception {
        if (!mHasFeature) {
            return;
        }
        executeDeviceTestClass(".TrustAgentInfoTest");
    }

    protected void executeDeviceTestClass(String className) throws Exception {
        assertTrue(runDeviceTestsAsUser(DEVICE_ADMIN_PKG, className, mUserId));
    }

    protected void executeDeviceTestMethod(String className, String testName) throws Exception {
        assertTrue(runDeviceTestsAsUser(DEVICE_ADMIN_PKG, className, testName, mUserId));
    }

    private void installAppPermissionAppAsUser()
            throws FileNotFoundException, DeviceNotAvailableException {
        installAppAsUser(PERMISSIONS_APP_APK, false, mUserId);
    }

    private void executeSuspendPackageTestMethod(String testName) throws Exception {
        assertTrue(runDeviceTestsAsUser(INTENT_SENDER_PKG, ".SuspendPackageTest",
                testName, mUserId));
    }

    private void executeAccountTest(String testName) throws DeviceNotAvailableException {
        assertTrue(runDeviceTestsAsUser(ACCOUNT_MANAGEMENT_PKG, ".AccountManagementTest",
                testName, mUserId));
        // Send a home intent to dismiss an error dialog.
        String command = "am start -a android.intent.action.MAIN"
                + " -c android.intent.category.HOME";
        CLog.i("Output for command " + command + ": " + getDevice().executeShellCommand(command));
    }

    private void executeAppRestrictionsManagingPackageTest(String testName) throws Exception {
        assertTrue(runDeviceTestsAsUser(APP_RESTRICTIONS_MANAGING_APP_PKG,
                ".ApplicationRestrictionsManagerTest", testName, mUserId));
    }

    private void changeUserRestrictionForUser(String key, String command, int userId)
            throws DeviceNotAvailableException {
        changePolicy(command, "--es extra-restriction-key " + key, userId);
    }

    private void changeAccountManagement(String command, String accountType, int userId)
            throws DeviceNotAvailableException {
        changePolicy(command, "--es extra-account-type " + accountType, userId);
    }

    private void changeApplicationRestrictionsManagingPackage(String packageName)
            throws DeviceNotAvailableException {
        String packageNameExtra = (packageName != null)
                ? "--es extra-package-name " + packageName : "";
        changePolicy("set-app-restrictions-manager", packageNameExtra, mUserId);
    }

    private void changePolicy(String command, String extras, int userId)
            throws DeviceNotAvailableException {
        String adbCommand = "am start -W --user " + userId
                + " -c android.intent.category.DEFAULT "
                + " --es extra-command " + command
                + " " + extras
                + " " + DEVICE_ADMIN_PKG + "/.SetPolicyActivity";
        String commandOutput = getDevice().executeShellCommand(adbCommand);
        CLog.d("Output for command " + adbCommand + ": " + commandOutput);
        assertTrue("Command was expected to succeed " + commandOutput,
                commandOutput.contains("Status: ok"));
    }
}
