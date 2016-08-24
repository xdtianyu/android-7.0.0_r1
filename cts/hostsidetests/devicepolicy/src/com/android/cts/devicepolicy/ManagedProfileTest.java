/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;

import junit.framework.AssertionFailedError;

import java.util.concurrent.Callable;

/**
 * Set of tests for Managed Profile use cases.
 */
public class ManagedProfileTest extends BaseDevicePolicyTest {

    private static final String MANAGED_PROFILE_PKG = "com.android.cts.managedprofile";
    private static final String MANAGED_PROFILE_APK = "CtsManagedProfileApp.apk";

    private static final String DEVICE_OWNER_PKG = "com.android.cts.deviceowner";
    private static final String DEVICE_OWNER_APK = "CtsDeviceOwnerApp.apk";
    private static final String DEVICE_OWNER_ADMIN =
            DEVICE_OWNER_PKG + ".BaseDeviceOwnerTest$BasicAdminReceiver";

    private static final String INTENT_SENDER_PKG = "com.android.cts.intent.sender";
    private static final String INTENT_SENDER_APK = "CtsIntentSenderApp.apk";

    private static final String INTENT_RECEIVER_PKG = "com.android.cts.intent.receiver";
    private static final String INTENT_RECEIVER_APK = "CtsIntentReceiverApp.apk";

    private static final String WIFI_CONFIG_CREATOR_PKG = "com.android.cts.wificonfigcreator";
    private static final String WIFI_CONFIG_CREATOR_APK = "CtsWifiConfigCreator.apk";

    private static final String WIDGET_PROVIDER_APK = "CtsWidgetProviderApp.apk";
    private static final String WIDGET_PROVIDER_PKG = "com.android.cts.widgetprovider";

    private static final String DIRECTORY_PROVIDER_APK = "CtsContactDirectoryProvider.apk";
    private static final String DIRECTORY_PROVIDER_PKG
            = "com.android.cts.contactdirectoryprovider";
    private static final String PRIMARY_DIRECTORY_PREFIX = "Primary";
    private static final String MANAGED_DIRECTORY_PREFIX = "Managed";
    private static final String DIRECTORY_PRIVOIDER_URI
            = "content://com.android.cts.contact.directory.provider/";
    private static final String SET_CUSTOM_DIRECTORY_PREFIX_METHOD = "set_prefix";

    private static final String ADMIN_RECEIVER_TEST_CLASS =
            MANAGED_PROFILE_PKG + ".BaseManagedProfileTest$BasicAdminReceiver";

    private static final String FEATURE_BLUETOOTH = "android.hardware.bluetooth";
    private static final String FEATURE_CAMERA = "android.hardware.camera";
    private static final String FEATURE_WIFI = "android.hardware.wifi";
    private static final String FEATURE_TELEPHONY = "android.hardware.telephony";
    private static final String FEATURE_CONNECTION_SERVICE = "android.software.connectionservice";

    private static final String ADD_RESTRICTION_COMMAND = "add-restriction";

    private int mParentUserId;

    // ID of the profile we'll create. This will always be a profile of the parent.
    private int mProfileUserId;
    private String mPackageVerifier;

    private boolean mHasNfcFeature;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        // We need multi user to be supported in order to create a profile of the user owner.
        mHasFeature = mHasFeature && hasDeviceFeature(
                "android.software.managed_users");
        mHasNfcFeature = hasDeviceFeature("android.hardware.nfc");

        if (mHasFeature) {
            removeTestUsers();
            mParentUserId = mPrimaryUserId;
            mProfileUserId = createManagedProfile(mParentUserId);

            installAppAsUser(MANAGED_PROFILE_APK, mParentUserId);
            installAppAsUser(MANAGED_PROFILE_APK, mProfileUserId);
            setProfileOwnerOrFail(MANAGED_PROFILE_PKG + "/" + ADMIN_RECEIVER_TEST_CLASS,
                    mProfileUserId);
            startUser(mProfileUserId);
        }
    }

    @Override
    protected void tearDown() throws Exception {
        if (mHasFeature) {
            removeUser(mProfileUserId);
            getDevice().uninstallPackage(MANAGED_PROFILE_PKG);
            getDevice().uninstallPackage(INTENT_SENDER_PKG);
            getDevice().uninstallPackage(INTENT_RECEIVER_PKG);
        }
        super.tearDown();
    }

    public void testManagedProfileSetup() throws Exception {
        if (!mHasFeature) {
            return;
        }
        assertTrue(runDeviceTestsAsUser(
                MANAGED_PROFILE_PKG, MANAGED_PROFILE_PKG + ".ManagedProfileSetupTest",
                mProfileUserId));
    }

    /**
     *  wipeData() test removes the managed profile, so it needs to separated from other tests.
     */
    public void testWipeData() throws Exception {
        if (!mHasFeature) {
            return;
        }
        assertTrue(listUsers().contains(mProfileUserId));
        assertTrue(runDeviceTestsAsUser(
                MANAGED_PROFILE_PKG, MANAGED_PROFILE_PKG + ".WipeDataTest", mProfileUserId));
        // Note: the managed profile is removed by this test, which will make removeUserCommand in
        // tearDown() to complain, but that should be OK since its result is not asserted.
        assertFalse(listUsers().contains(mProfileUserId));
    }

    public void testMaxOneManagedProfile() throws Exception {
        int newUserId = -1;
        try {
            newUserId = createManagedProfile(mParentUserId);
        } catch (AssertionFailedError expected) {
        }
        if (newUserId > 0) {
            removeUser(newUserId);
            fail(mHasFeature ? "Device must allow creating only one managed profile"
                    : "Device must not allow creating a managed profile");
        }
    }

    /**
     * Verify that removing a managed profile will remove all networks owned by that profile.
     */
    public void testProfileWifiCleanup() throws Exception {
        if (!mHasFeature || !hasDeviceFeature(FEATURE_WIFI)) {
            return;
        }
        assertTrue("WiFi config already exists and could not be removed", runDeviceTestsAsUser(
                MANAGED_PROFILE_PKG, ".WifiTest", "testRemoveWifiNetworkIfExists", mParentUserId));

        installAppAsUser(WIFI_CONFIG_CREATOR_APK, mProfileUserId);
        assertTrue("Failed to add WiFi config", runDeviceTestsAsUser(
                MANAGED_PROFILE_PKG, ".WifiTest", "testAddWifiNetwork", mProfileUserId));

        // Now delete the user - should undo the effect of testAddWifiNetwork.
        removeUser(mProfileUserId);
        assertTrue("WiFi config not removed after deleting profile", runDeviceTestsAsUser(
                MANAGED_PROFILE_PKG, ".WifiTest", "testWifiNetworkDoesNotExist",
                mParentUserId));
    }

    public void testCrossProfileIntentFilters() throws Exception {
        if (!mHasFeature) {
            return;
        }
        // Set up activities: ManagedProfileActivity will only be enabled in the managed profile and
        // PrimaryUserActivity only in the primary one
        disableActivityForUser("ManagedProfileActivity", mParentUserId);
        disableActivityForUser("PrimaryUserActivity", mProfileUserId);

        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG,
                MANAGED_PROFILE_PKG + ".ManagedProfileTest", mProfileUserId));

        // Set up filters from primary to managed profile
        String command = "am start -W --user " + mProfileUserId  + " " + MANAGED_PROFILE_PKG
                + "/.PrimaryUserFilterSetterActivity";
        CLog.d("Output for command " + command + ": "
              + getDevice().executeShellCommand(command));
        assertTrue(runDeviceTestsAsUser(
                MANAGED_PROFILE_PKG, MANAGED_PROFILE_PKG + ".PrimaryUserTest", mParentUserId));
        // TODO: Test with startActivity
    }

    public void testAppLinks() throws Exception {
        if (!mHasFeature) {
            return;
        }
        // Disable all pre-existing browsers in the managed profile so they don't interfere with
        // intents resolution.
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "testDisableAllBrowsers", mProfileUserId));
        installAppAsUser(INTENT_RECEIVER_APK, mParentUserId);
        installAppAsUser(INTENT_SENDER_APK, mParentUserId);
        installAppAsUser(INTENT_RECEIVER_APK, mProfileUserId);
        installAppAsUser(INTENT_SENDER_APK, mProfileUserId);

        changeVerificationStatus(mParentUserId, INTENT_RECEIVER_PKG, "ask");
        changeVerificationStatus(mProfileUserId, INTENT_RECEIVER_PKG, "ask");
        // We should have two receivers: IntentReceiverActivity and BrowserActivity in the
        // managed profile
        assertAppLinkResult("testTwoReceivers");

        changeUserRestrictionForUser("allow_parent_profile_app_linking", ADD_RESTRICTION_COMMAND,
                mProfileUserId);
        // Now we should also have one receiver in the primary user, so three receivers in total.
        assertAppLinkResult("testThreeReceivers");

        changeVerificationStatus(mParentUserId, INTENT_RECEIVER_PKG, "never");
        // The primary user one has been set to never: we should only have the managed profile ones.
        assertAppLinkResult("testTwoReceivers");

        changeVerificationStatus(mProfileUserId, INTENT_RECEIVER_PKG, "never");
        // Now there's only the browser in the managed profile left
        assertAppLinkResult("testReceivedByBrowserActivityInManaged");

        changeVerificationStatus(mProfileUserId, INTENT_RECEIVER_PKG, "always");
        changeVerificationStatus(mParentUserId, INTENT_RECEIVER_PKG, "always");
        // We have one always in the primary user and one always in the managed profile: the managed
        // profile one should have precedence.
        assertAppLinkResult("testReceivedByAppLinkActivityInManaged");
    }


    public void testSettingsIntents() throws Exception {
        if (!mHasFeature) {
            return;
        }

        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".SettingsIntentsTest",
                mProfileUserId));
    }

    public void testCrossProfileContent() throws Exception {
        if (!mHasFeature) {
            return;
        }
        installAppAsUser(INTENT_RECEIVER_APK, mParentUserId);
        installAppAsUser(INTENT_SENDER_APK, mParentUserId);
        installAppAsUser(INTENT_RECEIVER_APK, mProfileUserId);
        installAppAsUser(INTENT_SENDER_APK, mProfileUserId);

        // Test from parent to managed
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "testRemoveAllFilters", mProfileUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "testAddManagedCanAccessParentFilters", mProfileUserId));
        assertTrue(runDeviceTestsAsUser(INTENT_SENDER_PKG, ".ContentTest", mParentUserId));

        // Test from managed to parent
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "testRemoveAllFilters", mProfileUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "testAddParentCanAccessManagedFilters", mProfileUserId));
        assertTrue(runDeviceTestsAsUser(INTENT_SENDER_PKG, ".ContentTest", mProfileUserId));

    }

    public void testCrossProfileCopyPaste() throws Exception {
        if (!mHasFeature) {
            return;
        }
        installAppAsUser(INTENT_RECEIVER_APK, mParentUserId);
        installAppAsUser(INTENT_SENDER_APK, mParentUserId);
        installAppAsUser(INTENT_RECEIVER_APK, mProfileUserId);
        installAppAsUser(INTENT_SENDER_APK, mProfileUserId);

        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "testAllowCrossProfileCopyPaste", mProfileUserId));
        // Test that managed can see what is copied in the parent.
        testCrossProfileCopyPasteInternal(mProfileUserId, true);
        // Test that the parent can see what is copied in managed.
        testCrossProfileCopyPasteInternal(mParentUserId, true);

        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "testDisallowCrossProfileCopyPaste", mProfileUserId));
        // Test that managed can still see what is copied in the parent.
        testCrossProfileCopyPasteInternal(mProfileUserId, true);
        // Test that the parent cannot see what is copied in managed.
        testCrossProfileCopyPasteInternal(mParentUserId, false);
    }

    private void testCrossProfileCopyPasteInternal(int userId, boolean shouldSucceed)
            throws DeviceNotAvailableException {
        final String direction = (userId == mParentUserId)
                ? "testAddManagedCanAccessParentFilters"
                : "testAddParentCanAccessManagedFilters";
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "testRemoveAllFilters", mProfileUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                direction, mProfileUserId));
        if (shouldSucceed) {
            assertTrue(runDeviceTestsAsUser(INTENT_SENDER_PKG, ".CopyPasteTest",
                    "testCanReadAcrossProfiles", userId));
            assertTrue(runDeviceTestsAsUser(INTENT_SENDER_PKG, ".CopyPasteTest",
                    "testIsNotified", userId));
        } else {
            assertTrue(runDeviceTestsAsUser(INTENT_SENDER_PKG, ".CopyPasteTest",
                    "testCannotReadAcrossProfiles", userId));
        }
    }

    // TODO: This test is not specific to managed profiles, but applies to multi-user in general.
    // Move it to a MultiUserTest class when there is one. Should probably move
    // SetPolicyActivity to a more generic apk too as it might be useful for different kinds
    // of tests (same applies to ComponentDisablingActivity).
    public void testNoDebuggingFeaturesRestriction() throws Exception {
        if (!mHasFeature) {
            return;
        }
        // If adb is running as root, then the adb uid is 0 instead of SHELL_UID,
        // so the DISALLOW_DEBUGGING_FEATURES restriction does not work and this test
        // fails.
        if (getDevice().isAdbRoot()) {
            CLog.logAndDisplay(LogLevel.WARN,
                    "Cannot test testNoDebuggingFeaturesRestriction() in eng/userdebug build");
            return;
        }
        String restriction = "no_debugging_features";  // UserManager.DISALLOW_DEBUGGING_FEATURES

        String addRestrictionCommandOutput =
                changeUserRestrictionForUser(restriction, ADD_RESTRICTION_COMMAND, mProfileUserId);
        assertTrue("Command was expected to succeed " + addRestrictionCommandOutput,
                addRestrictionCommandOutput.contains("Status: ok"));

        // This should now fail, as the shell is not available to start activities under a different
        // user once the restriction is in place.
        addRestrictionCommandOutput =
                changeUserRestrictionForUser(restriction, ADD_RESTRICTION_COMMAND, mProfileUserId);
        assertTrue(
                "Expected SecurityException when starting the activity "
                        + addRestrictionCommandOutput,
                addRestrictionCommandOutput.contains("SecurityException"));
    }

    // Test the bluetooth API from a managed profile.
    public void testBluetooth() throws Exception {
        boolean mHasBluetooth = hasDeviceFeature(FEATURE_BLUETOOTH);
        if (!mHasFeature || !mHasBluetooth) {
            return ;
        }

        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".BluetoothTest",
                "testEnableDisable", mProfileUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".BluetoothTest",
                "testGetAddress", mProfileUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".BluetoothTest",
                "testListenUsingRfcommWithServiceRecord", mProfileUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".BluetoothTest",
                "testGetRemoteDevice", mProfileUserId));
    }

    public void testCameraPolicy() throws Exception {
        boolean hasCamera = hasDeviceFeature(FEATURE_CAMERA);
        if (!mHasFeature || !hasCamera) {
            return;
        }
        try {
            setDeviceAdmin(MANAGED_PROFILE_PKG + "/.PrimaryUserDeviceAdmin", mParentUserId);

            // Disable managed profile camera.
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CameraPolicyTest",
                    "testDisableCameraInManagedProfile",
                    mProfileUserId));
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CameraPolicyTest",
                    "testIsCameraEnabledInPrimaryProfile",
                    mParentUserId));

            // Enable managed profile camera.
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CameraPolicyTest",
                    "testEnableCameraInManagedProfile",
                    mProfileUserId));
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CameraPolicyTest",
                    "testIsCameraEnabledInPrimaryProfile",
                    mParentUserId));

            // Disable primary profile camera.
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CameraPolicyTest",
                    "testDisableCameraInPrimaryProfile",
                    mParentUserId));
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CameraPolicyTest",
                    "testIsCameraEnabledInManagedProfile",
                    mProfileUserId));

            // Enable primary profile camera.
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CameraPolicyTest",
                    "testEnableCameraInPrimaryProfile",
                    mParentUserId));
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CameraPolicyTest",
                    "testIsCameraEnabledInManagedProfile",
                    mProfileUserId));
        } finally {
            final String adminHelperClass = ".PrimaryUserAdminHelper";
            assertTrue("Clear device admin failed", runDeviceTestsAsUser(MANAGED_PROFILE_PKG,
                    adminHelperClass, "testClearDeviceAdmin", mParentUserId));
        }
    }


    public void testManagedContactsUris() throws Exception {
        runManagedContactsTest(new Callable<Void>() {
            @Override
            public Void call() throws Exception {
                ContactsTestSet contactsTestSet = new ContactsTestSet(ManagedProfileTest.this,
                        MANAGED_PROFILE_PKG, mParentUserId, mProfileUserId);

                contactsTestSet.setCallerIdEnabled(true);
                contactsTestSet.setContactsSearchEnabled(true);
                contactsTestSet.checkIfCanLookupEnterpriseContacts(true);
                contactsTestSet.checkIfCanFilterEnterpriseContacts(true);
                contactsTestSet.checkIfCanFilterSelfContacts();
                return null;
            }
        });
    }

    public void testManagedQuickContacts() throws Exception {
        runManagedContactsTest(new Callable<Void>() {
            @Override
            public Void call() throws Exception {
                assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                        "testQuickContact", mParentUserId));
                return null;
            }
        });
    }

    public void testManagedContactsPolicies() throws Exception {
        runManagedContactsTest(new Callable<Void>() {
            @Override
            public Void call() throws Exception {
                ContactsTestSet contactsTestSet = new ContactsTestSet(ManagedProfileTest.this,
                        MANAGED_PROFILE_PKG, mParentUserId, mProfileUserId);
                try {
                    contactsTestSet.setCallerIdEnabled(true);
                    contactsTestSet.setContactsSearchEnabled(false);
                    contactsTestSet.checkIfCanLookupEnterpriseContacts(true);
                    contactsTestSet.checkIfCanFilterEnterpriseContacts(false);
                    contactsTestSet.checkIfCanFilterSelfContacts();
                    contactsTestSet.setCallerIdEnabled(false);
                    contactsTestSet.setContactsSearchEnabled(true);
                    contactsTestSet.checkIfCanLookupEnterpriseContacts(false);
                    contactsTestSet.checkIfCanFilterEnterpriseContacts(true);
                    contactsTestSet.checkIfCanFilterSelfContacts();
                    contactsTestSet.setCallerIdEnabled(false);
                    contactsTestSet.setContactsSearchEnabled(false);
                    contactsTestSet.checkIfCanLookupEnterpriseContacts(false);
                    contactsTestSet.checkIfCanFilterEnterpriseContacts(false);
                    contactsTestSet.checkIfCanFilterSelfContacts();
                    contactsTestSet.checkIfNoEnterpriseDirectoryFound();
                    return null;
                } finally {
                    // reset policies
                    contactsTestSet.setCallerIdEnabled(true);
                    contactsTestSet.setContactsSearchEnabled(true);
                }
            }
        });
    }

    public void testOrganizationInfo() throws Exception {
        if (!mHasFeature) {
            return;
        }
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".OrganizationInfoTest",
                "testDefaultOrganizationColor", mProfileUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".OrganizationInfoTest",
                "testDefaultOrganizationNameIsNull", mProfileUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".OrganizationInfoTest",
                mProfileUserId));
    }

    public void testPasswordMinimumRestrictions() throws Exception {
        if (!mHasFeature) {
            return;
        }
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PasswordMinimumRestrictionsTest",
                mProfileUserId));
    }

    public void testBluetoothContactSharingDisabled() throws Exception {
        if (!mHasFeature) {
            return;
        }
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                "testSetBluetoothContactSharingDisabled_setterAndGetter", mProfileUserId));
    }

    public void testCannotSetProfileOwnerAgain() throws Exception {
        if (!mHasFeature) {
            return;
        }
        // verify that we can't set the same admin receiver as profile owner again
        assertFalse(setProfileOwner(
                MANAGED_PROFILE_PKG + "/" + ADMIN_RECEIVER_TEST_CLASS, mProfileUserId,
                /*expectFailure*/ true));

        // verify that we can't set a different admin receiver as profile owner
        installAppAsUser(DEVICE_OWNER_APK, mProfileUserId);
        assertFalse(setProfileOwner(DEVICE_OWNER_PKG + "/" + DEVICE_OWNER_ADMIN, mProfileUserId,
                /*expectFailure*/ true));
    }

    public void testCannotSetDeviceOwnerWhenProfilePresent() throws Exception {
        if (!mHasFeature) {
            return;
        }

        try {
            installAppAsUser(DEVICE_OWNER_APK, mParentUserId);
            assertFalse(setDeviceOwner(DEVICE_OWNER_PKG + "/" + DEVICE_OWNER_ADMIN, mParentUserId,
                    /*expectFailure*/ true));
        } finally {
            // make sure we clean up in case we succeeded in setting the device owner
            removeAdmin(DEVICE_OWNER_PKG + "/" + DEVICE_OWNER_ADMIN, mParentUserId);
            getDevice().uninstallPackage(DEVICE_OWNER_PKG);
        }
    }

    public void testNfcRestriction() throws Exception {
        if (!mHasFeature || !mHasNfcFeature) {
            return;
        }

        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".NfcTest",
                "testNfcShareEnabled", mProfileUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".NfcTest",
                "testNfcShareEnabled", mParentUserId));

        String restriction = "no_outgoing_beam";  // UserManager.DISALLOW_OUTGOING_BEAM
        String command = "add-restriction";

        String addRestrictionCommandOutput =
                changeUserRestrictionForUser(restriction, command, mProfileUserId);
        assertTrue("Command was expected to succeed " + addRestrictionCommandOutput,
                addRestrictionCommandOutput.contains("Status: ok"));

        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".NfcTest",
                "testNfcShareDisabled", mProfileUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".NfcTest",
                "testNfcShareEnabled", mParentUserId));
    }

    public void testCrossProfileWidgets() throws Exception {
        if (!mHasFeature) {
            return;
        }

        try {
            installAppAsUser(WIDGET_PROVIDER_APK, mProfileUserId);
            installAppAsUser(WIDGET_PROVIDER_APK, mParentUserId);
            getDevice().executeShellCommand("appwidget grantbind --user " + mParentUserId
                    + " --package " + WIDGET_PROVIDER_PKG);
            startWidgetHostService();

            String commandOutput = changeCrossProfileWidgetForUser(WIDGET_PROVIDER_PKG,
                    "add-cross-profile-widget", mProfileUserId);
            assertTrue("Command was expected to succeed " + commandOutput,
                    commandOutput.contains("Status: ok"));

            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileWidgetTest",
                    "testCrossProfileWidgetProviderAdded", mProfileUserId));
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG,
                    ".CrossProfileWidgetPrimaryUserTest",
                    "testHasCrossProfileWidgetProvider_true", mParentUserId));
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG,
                    ".CrossProfileWidgetPrimaryUserTest",
                    "testHostReceivesWidgetUpdates_true", mParentUserId));

            commandOutput = changeCrossProfileWidgetForUser(WIDGET_PROVIDER_PKG,
                    "remove-cross-profile-widget", mProfileUserId);
            assertTrue("Command was expected to succeed " + commandOutput,
                    commandOutput.contains("Status: ok"));

            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileWidgetTest",
                    "testCrossProfileWidgetProviderRemoved", mProfileUserId));
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG,
                    ".CrossProfileWidgetPrimaryUserTest",
                    "testHasCrossProfileWidgetProvider_false", mParentUserId));
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG,
                    ".CrossProfileWidgetPrimaryUserTest",
                    "testHostReceivesWidgetUpdates_false", mParentUserId));
        } finally {
            changeCrossProfileWidgetForUser(WIDGET_PROVIDER_PKG, "remove-cross-profile-widget",
                    mProfileUserId);
            getDevice().uninstallPackage(WIDGET_PROVIDER_PKG);
        }
    }

    public void testIsProvisioningAllowed() throws DeviceNotAvailableException {
        if (!mHasFeature) {
            return;
        }
        // In Managed profile user when managed profile is provisioned
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PreManagedProfileTest",
                "testIsProvisioningAllowedFalse", mProfileUserId));

        // In parent user when managed profile is provisioned
        // It's allowed to provision again by removing the previous profile
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PreManagedProfileTest",
                "testIsProvisioningAllowedTrue", mParentUserId));
    }

    private void setDirectoryPrefix(String directoryName, int userId)
            throws DeviceNotAvailableException {
        String command = "content call --uri " + DIRECTORY_PRIVOIDER_URI
                + " --user " + userId
                + " --method " + SET_CUSTOM_DIRECTORY_PREFIX_METHOD
                + " --arg " + directoryName;
        CLog.d("Output for command " + command + ": "
                + getDevice().executeShellCommand(command));
    }

    public void testPhoneAccountVisibility() throws Exception  {
        if (!mHasFeature) {
            return;
        }
        if (!shouldRunTelecomTest()) {
            return;
        }
        try {
            // Register phone account in parent user.
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PhoneAccountTest",
                    "testRegisterPhoneAccount",
                    mParentUserId));
            // The phone account should not be visible in managed user.
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PhoneAccountTest",
                    "testPhoneAccountNotRegistered",
                    mProfileUserId));
        } finally {
            // Unregister the phone account.
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PhoneAccountTest",
                    "testUnregisterPhoneAccount",
                    mParentUserId));
        }

        try {
            // Register phone account in profile user.
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PhoneAccountTest",
                    "testRegisterPhoneAccount",
                    mProfileUserId));
            // The phone account should not be visible in parent user.
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PhoneAccountTest",
                    "testPhoneAccountNotRegistered",
                    mParentUserId));
        } finally {
            // Unregister the phone account.
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PhoneAccountTest",
                    "testUnregisterPhoneAccount",
                    mProfileUserId));
        }
    }

    public void testManagedCall() throws Exception {
        if (!mHasFeature) {
            return;
        }
        if (!shouldRunTelecomTest()) {
            return;
        }
        // Place a outgoing call through work phone account using TelecomManager and verify the
        // call is inserted properly.
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PhoneAccountTest",
                "testOutgoingCallUsingTelecomManager",
                mProfileUserId));
        // Make sure the call is not inserted into parent user.
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PhoneAccountTest",
                "testEnsureCallNotInserted",
                mParentUserId));

        // Place a outgoing call through work phone account using ACTION_CALL and verify the call
        // is inserted properly.
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PhoneAccountTest",
                "testOutgoingCallUsingActionCall",
                mProfileUserId));
        // Make sure the call is not inserted into parent user.
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PhoneAccountTest",
                "testEnsureCallNotInserted",
                mParentUserId));

        // Add an incoming call with parent user's phone account and verify the call is inserted
        // properly.
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PhoneAccountTest",
                "testIncomingCall",
                mProfileUserId));
        // Make sure the call is not inserted into parent user.
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PhoneAccountTest",
                "testEnsureCallNotInserted",
                mParentUserId));
    }

    public void testTrustAgentInfo() throws Exception {
        if (!mHasFeature) {
            return;
        }
        // Set and get trust agent config using child dpm instance.
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".TrustAgentInfoTest",
                "testSetAndGetTrustAgentConfiguration_child",
                mProfileUserId));
        // Set and get trust agent config using parent dpm instance.
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".TrustAgentInfoTest",
                "testSetAndGetTrustAgentConfiguration_parent",
                mProfileUserId));
        // Unified case
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".TrustAgentInfoTest",
                "testSetTrustAgentConfiguration_bothHaveTrustAgentConfigAndUnified",
                mProfileUserId));
        // Non-unified case, this test must run last because we have no way to clear work side
        // password.
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".TrustAgentInfoTest",
                "testSetTrustAgentConfiguration_bothHaveTrustAgentConfigAndNonUnified",
                mProfileUserId));
    }

    private void disableActivityForUser(String activityName, int userId)
            throws DeviceNotAvailableException {
        String command = "am start -W --user " + userId
                + " --es extra-package " + MANAGED_PROFILE_PKG
                + " --es extra-class-name " + MANAGED_PROFILE_PKG + "." + activityName
                + " " + MANAGED_PROFILE_PKG + "/.ComponentDisablingActivity ";
        CLog.d("Output for command " + command + ": "
                + getDevice().executeShellCommand(command));
    }

    private String changeUserRestrictionForUser(String key, String command, int userId)
            throws DeviceNotAvailableException {
        String adbCommand = "am start -W --user " + userId
                + " -c android.intent.category.DEFAULT "
                + " --es extra-command " + command
                + " --es extra-restriction-key " + key
                + " " + MANAGED_PROFILE_PKG + "/.SetPolicyActivity";
        // Don't log output because sometimes used expecting failures.
        CLog.d("Running command " + adbCommand);
        String commandOutput = getDevice().executeShellCommand(adbCommand);
        return commandOutput;
    }

    private String changeCrossProfileWidgetForUser(String packageName, String command, int userId)
            throws DeviceNotAvailableException {
        String adbCommand = "am start -W --user " + userId
                + " -c android.intent.category.DEFAULT "
                + " --es extra-command " + command
                + " --es extra-package-name " + packageName
                + " " + MANAGED_PROFILE_PKG + "/.SetPolicyActivity";
        String commandOutput = getDevice().executeShellCommand(adbCommand);
        CLog.d("Output for command " + adbCommand + ": " + commandOutput);
        return commandOutput;
    }

    // status should be one of never, undefined, ask, always
    private void changeVerificationStatus(int userId, String packageName, String status)
            throws DeviceNotAvailableException {
        String command = "pm set-app-link --user " + userId + " " + packageName + " " + status;
        CLog.d("Output for command " + command + ": "
                + getDevice().executeShellCommand(command));
    }

    protected void startWidgetHostService() throws Exception {
        String command = "am startservice --user " + mParentUserId
                + " -a " + WIDGET_PROVIDER_PKG + ".REGISTER_CALLBACK "
                + "--ei user-extra " + getUserSerialNumber(mProfileUserId)
                + " " + WIDGET_PROVIDER_PKG + "/.SimpleAppWidgetHostService";
        CLog.d("Output for command " + command + ": "
              + getDevice().executeShellCommand(command));
    }

    private void assertAppLinkResult(String methodName) throws DeviceNotAvailableException {
        assertTrue(runDeviceTestsAsUser(INTENT_SENDER_PKG, ".AppLinkTest", methodName,
                mProfileUserId));
    }

    private boolean shouldRunTelecomTest() throws DeviceNotAvailableException {
        return hasDeviceFeature(FEATURE_TELEPHONY) && hasDeviceFeature(FEATURE_CONNECTION_SERVICE);
    }

    private void runManagedContactsTest(Callable<Void> callable) throws Exception {
        if (!mHasFeature) {
            return;
        }

        try {
            // Allow cross profile contacts search.
            // TODO test both on and off.
            getDevice().executeShellCommand(
                    "settings put --user " + mProfileUserId
                    + " secure managed_profile_contact_remote_search 1");

            // Add test account
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testAddTestAccount", mParentUserId));
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testAddTestAccount", mProfileUserId));

            // Install directory provider to both primary and managed profile
            installAppAsUser(DIRECTORY_PROVIDER_APK, mProfileUserId);
            installAppAsUser(DIRECTORY_PROVIDER_APK, mParentUserId);
            setDirectoryPrefix(PRIMARY_DIRECTORY_PREFIX, mParentUserId);
            setDirectoryPrefix(MANAGED_DIRECTORY_PREFIX, mProfileUserId);

            // Check enterprise directory API works
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testGetDirectoryListInPrimaryProfile", mParentUserId));

            // Insert Primary profile Contacts
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testPrimaryProfilePhoneAndEmailLookup_insertedAndfound", mParentUserId));
            // Insert Managed profile Contacts
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testManagedProfilePhoneAndEmailLookup_insertedAndfound", mProfileUserId));
            // Insert a primary contact with same phone & email as other
            // enterprise contacts
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testPrimaryProfileDuplicatedPhoneEmailContact_insertedAndfound",
                    mParentUserId));
            // Insert a enterprise contact with same phone & email as other
            // primary contacts
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testManagedProfileDuplicatedPhoneEmailContact_insertedAndfound",
                    mProfileUserId));

            callable.call();

        } finally {
            // Clean up in managed profile and primary profile
            runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testCurrentProfileContacts_removeContacts", mProfileUserId);
            runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testCurrentProfileContacts_removeContacts", mParentUserId);
            getDevice().uninstallPackage(DIRECTORY_PROVIDER_PKG);
        }
    }


    /*
     * Container for running ContactsTest under multi-user environment
     */
    private static class ContactsTestSet {

        private ManagedProfileTest mManagedProfileTest;
        private String mManagedProfilePackage;
        private int mParentUserId;
        private int mProfileUserId;

        public ContactsTestSet(ManagedProfileTest managedProfileTest, String managedProfilePackage,
                int parentUserId, int profileUserId) {
            mManagedProfileTest = managedProfileTest;
            mManagedProfilePackage = managedProfilePackage;
            mParentUserId = parentUserId;
            mProfileUserId = profileUserId;
        }

        private boolean runDeviceTestsAsUser(String pkgName, String testClassName,
                String testMethodName, Integer userId) throws DeviceNotAvailableException {
            return mManagedProfileTest.runDeviceTestsAsUser(pkgName, testClassName, testMethodName,
                    userId);
        }

        // Enable / Disable cross profile caller id
        public void setCallerIdEnabled(boolean enabled) throws DeviceNotAvailableException {
            if (enabled) {
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testSetCrossProfileCallerIdDisabled_false", mProfileUserId));
            } else {
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testSetCrossProfileCallerIdDisabled_true", mProfileUserId));
            }
        }

        // Enable / Disable cross profile contacts search
        public void setContactsSearchEnabled(boolean enabled) throws DeviceNotAvailableException {
            if (enabled) {
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testSetCrossProfileContactsSearchDisabled_false", mProfileUserId));
            } else {
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testSetCrossProfileContactsSearchDisabled_true", mProfileUserId));
            }
        }

        public void checkIfCanLookupEnterpriseContacts(boolean expected)
                throws DeviceNotAvailableException {
            // Primary user cannot use ordinary phone/email lookup api to access
            // managed contacts
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfilePhoneLookup_canNotAccessEnterpriseContact", mParentUserId));
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfileEmailLookup_canNotAccessEnterpriseContact", mParentUserId));
            // Primary user can use ENTERPRISE_CONTENT_FILTER_URI to access
            // primary contacts
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfileEnterprisePhoneLookup_canAccessPrimaryContact",
                    mParentUserId));
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfileEnterpriseEmailLookup_canAccessPrimaryContact",
                    mParentUserId));
            // When there exist contacts with the same phone/email in primary &
            // enterprise,
            // primary user can use ENTERPRISE_CONTENT_FILTER_URI to access the
            // primary contact.
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfileEnterpriseEmailLookupDuplicated_canAccessPrimaryContact",
                    mParentUserId));
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfileEnterprisePhoneLookupDuplicated_canAccessPrimaryContact",
                    mParentUserId));

            // Managed user cannot use ordinary phone/email lookup api to access
            // primary contacts
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testManagedProfilePhoneLookup_canNotAccessPrimaryContact", mProfileUserId));
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testManagedProfileEmailLookup_canNotAccessPrimaryContact", mProfileUserId));
            // Managed user can use ENTERPRISE_CONTENT_FILTER_URI to access
            // enterprise contacts
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testManagedProfileEnterprisePhoneLookup_canAccessEnterpriseContact",
                    mProfileUserId));
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testManagedProfileEnterpriseEmailLookup_canAccessEnterpriseContact",
                    mProfileUserId));
            // Managed user cannot use ENTERPRISE_CONTENT_FILTER_URI to access
            // primary contacts
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testManagedProfileEnterprisePhoneLookup_canNotAccessPrimaryContact",
                    mProfileUserId));
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testManagedProfileEnterpriseEmailLookup_canNotAccessPrimaryContact",
                    mProfileUserId));
            // When there exist contacts with the same phone/email in primary &
            // enterprise,
            // managed user can use ENTERPRISE_CONTENT_FILTER_URI to access the
            // enterprise contact.
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testManagedProfileEnterpriseEmailLookupDuplicated_canAccessEnterpriseContact",
                    mProfileUserId));
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testManagedProfileEnterprisePhoneLookupDuplicated_canAccessEnterpriseContact",
                    mProfileUserId));

            // Check if phone lookup can access primary directories
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfileEnterprisePhoneLookup_canAccessPrimaryDirectories",
                    mParentUserId));

            // Check if email lookup can access primary directories
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfileEnterpriseEmailLookup_canAccessPrimaryDirectories",
                    mParentUserId));

            if (expected) {
                // Primary user can use ENTERPRISE_CONTENT_FILTER_URI to access
                // managed profile contacts
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterprisePhoneLookup_canAccessEnterpriseContact",
                        mParentUserId));
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterpriseEmailLookup_canAccessEnterpriseContact",
                        mParentUserId));

                // Make sure SIP enterprise lookup works too.
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterpriseSipLookup_canAccessEnterpriseContact",
                        mParentUserId));

                // Check if phone lookup can access enterprise directories
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterprisePhoneLookup_canAccessManagedDirectories",
                        mParentUserId));

                // Check if email lookup can access enterprise directories
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterpriseEmailLookup_canAccessManagedDirectories",
                        mParentUserId));
            } else {
                // Primary user cannot use ENTERPRISE_CONTENT_FILTER_URI to
                // access managed contacts
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterprisePhoneLookup_canNotAccessEnterpriseContact",
                        mParentUserId));
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterprisePhoneLookup_canNotAccessManagedDirectories",
                        mParentUserId));

                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterpriseEmailLookup_canNotAccessManagedDirectories",
                        mParentUserId));
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterprisePhoneLookup_canNotAccessManagedDirectories",
                        mParentUserId));
            }
        }

        public void checkIfCanFilterSelfContacts() throws DeviceNotAvailableException {
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfileEnterpriseCallableFilter_canAccessPrimaryDirectories",
                    mParentUserId));
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testManagedProfileEnterpriseCallableFilter_canAccessManagedDirectories",
                    mProfileUserId));

            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfileEnterpriseEmailFilter_canAccessPrimaryDirectories",
                    mParentUserId));
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testEnterpriseProfileEnterpriseEmailFilter_canAccessManagedDirectories",
                    mProfileUserId));

            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfileEnterpriseContactFilter_canAccessPrimaryDirectories",
                    mParentUserId));
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testManagedProfileEnterpriseContactFilter_canAccessManagedDirectories",
                    mProfileUserId));

            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfileEnterprisePhoneFilter_canAccessPrimaryDirectories",
                    mParentUserId));
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testManagedProfileEnterprisePhoneFilter_canAccessManagedDirectories",
                    mProfileUserId));
        }

        public void checkIfCanFilterEnterpriseContacts(boolean expected)
                throws DeviceNotAvailableException {
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testFilterUriWhenDirectoryParamMissing", mParentUserId));
            if (expected) {
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterpriseCallableFilter_canAccessManagedDirectories",
                        mParentUserId));
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterpriseEmailFilter_canAccessManagedDirectories",
                        mParentUserId));
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterpriseContactFilter_canAccessManagedDirectories",
                        mParentUserId));
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterprisePhoneFilter_canAccessManagedDirectories",
                        mParentUserId));
            } else {
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterpriseCallableFilter_canNotAccessManagedDirectories",
                        mParentUserId));
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterpriseEmailFilter_canNotAccessManagedDirectories",
                        mParentUserId));
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterpriseContactFilter_canNotAccessManagedDirectories",
                        mParentUserId));
                assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                        "testPrimaryProfileEnterprisePhoneFilter_canNotAccessManagedDirectories",
                        mParentUserId));
            }
        }

        public void checkIfNoEnterpriseDirectoryFound() throws DeviceNotAvailableException {
            assertTrue(runDeviceTestsAsUser(mManagedProfilePackage, ".ContactsTest",
                    "testPrimaryProfileEnterpriseDirectories_canNotAccessManagedDirectories",
                    mParentUserId));
        }
    }
}
