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
package com.android.managedprovisioning.uiflows;

import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE;
import static android.nfc.NfcAdapter.ACTION_NDEF_DISCOVERED;
import static com.android.managedprovisioning.common.Globals.ACTION_RESUME_PROVISIONING;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.verifyZeroInteractions;
import static org.mockito.Mockito.when;

import android.app.ActivityManager;
import android.app.KeyguardManager;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.UserManager;
import android.service.persistentdata.PersistentDataBlockManager;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.text.TextUtils;

import com.android.managedprovisioning.R;
import com.android.managedprovisioning.common.IllegalProvisioningArgumentException;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.model.WifiInfo;
import com.android.managedprovisioning.parser.MessageParser;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@SmallTest
public class PreProvisioningControllerTest extends AndroidTestCase {
    private static final String TEST_MDM_PACKAGE = "com.test.mdm";
    private static final ComponentName TEST_MDM_COMPONENT_NAME = new ComponentName(TEST_MDM_PACKAGE,
            "com.test.mdm.DeviceAdmin");
    private static final String TEST_BOGUS_PACKAGE = "com.test.bogus";
    private static final String TEST_WIFI_SSID = "TestNet";
    private static final String MP_PACKAGE_NAME = "com.android.managedprovisioning";
    private static final int TEST_USER_ID = 10;

    @Mock private Context mContext;
    @Mock private DevicePolicyManager mDevicePolicyManager;
    @Mock private UserManager mUserManager;
    @Mock private PackageManager mPackageManager;
    @Mock private ActivityManager mActivityManager;
    @Mock private KeyguardManager mKeyguardManager;
    @Mock private PersistentDataBlockManager mPdbManager;
    @Mock private PreProvisioningController.Ui mUi;
    @Mock private MessageParser mMessageParser;
    @Mock private Utils mUtils;
    @Mock private Intent mIntent;
    @Mock private EncryptionController mEncryptionController;

    private ProvisioningParams mParams;

    private PreProvisioningController mController;

    @Override
    public void setUp() {
        // this is necessary for mockito to work
        System.setProperty("dexmaker.dexcache", getContext().getCacheDir().toString());

        MockitoAnnotations.initMocks(this);

        when(mContext.getSystemService(Context.DEVICE_POLICY_SERVICE))
                .thenReturn(mDevicePolicyManager);
        when(mContext.getSystemService(Context.USER_SERVICE)).thenReturn(mUserManager);
        when(mContext.getPackageManager()).thenReturn(mPackageManager);
        when(mContext.getSystemService(Context.ACTIVITY_SERVICE)).thenReturn(mActivityManager);
        when(mContext.getSystemService(Context.KEYGUARD_SERVICE)).thenReturn(mKeyguardManager);
        when(mContext.getSystemService(Context.PERSISTENT_DATA_BLOCK_SERVICE))
                .thenReturn(mPdbManager);
        when(mContext.getPackageName()).thenReturn(MP_PACKAGE_NAME);

        when(mUserManager.getUserHandle()).thenReturn(TEST_USER_ID);

        when(mUtils.isFrpSupported(mContext)).thenReturn(true);
        when(mUtils.isSplitSystemUser()).thenReturn(false);
        when(mUtils.isEncryptionRequired()).thenReturn(false);
        when(mUtils.currentLauncherSupportsManagedProfiles(mContext)).thenReturn(true);
        when(mUtils.alreadyHasManagedProfile(mContext)).thenReturn(-1);

        when(mKeyguardManager.inKeyguardRestrictedInputMode()).thenReturn(false);
        when(mDevicePolicyManager.getStorageEncryptionStatus())
                .thenReturn(DevicePolicyManager.ENCRYPTION_STATUS_INACTIVE);

        mController = new PreProvisioningController(mContext, mUi, mMessageParser, mUtils,
                mEncryptionController);
    }

    public void testManagedProfile() throws Exception {
        // GIVEN an intent to provision a managed profile
        prepareMocksForManagedProfileIntent(false);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN the UI elements should be updated accordingly
        verifyInitiateProfileOwnerUi();
        // WHEN the user clicks next
        mController.afterNavigateNext();
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, true);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start profile provisioning
        verify(mUi).startProfileOwnerProvisioning(mParams);
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_provisioningNotAllowed() throws Exception {
        // GIVEN an intent to provision a managed profile, but provisioning mode is not allowed
        prepareMocksForManagedProfileIntent(false);
        when(mDevicePolicyManager.isProvisioningAllowed(ACTION_PROVISION_MANAGED_PROFILE))
                .thenReturn(false);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN show an error dialog
        verify(mUi).showErrorAndClose(anyInt(), anyString());
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_nullCallingPackage() throws Exception {
        // GIVEN a device that is not currently encrypted
        prepareMocksForManagedProfileIntent(false);
        try {
            // WHEN initiating provisioning
            mController.initiateProvisioning(mIntent, null);
            fail("Expected NullPointerException not thrown");
        } catch (NullPointerException ne) {
            // THEN a NullPointerException is thrown
        }
        // THEN no user interaction occurs
        verifyZeroInteractions(mUi);
    }

    public void testManagedProfile_withEncryption() throws Exception {
        // GIVEN a device that is not currently encrypted
        prepareMocksForManagedProfileIntent(false);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating managed profile provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN the UI elements should be updated accordingly
        verifyInitiateProfileOwnerUi();
        // WHEN the user clicks next
        mController.afterNavigateNext();
        // THEN show encryption screen
        verify(mUi).requestEncryption(mParams);
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_afterEncryption() throws Exception {
        // GIVEN managed profile provisioning continues after successful encryption. In this case
        // we don't set the startedByTrustedSource flag.
        prepareMocksForAfterEncryption(ACTION_PROVISION_MANAGED_PROFILE, false);
        // WHEN initiating with a continuation intent
        mController.initiateProvisioning(mIntent, MP_PACKAGE_NAME);
        // THEN the UI elements should be updated accordingly
        verifyInitiateProfileOwnerUi();
        // WHEN the user clicks next
        mController.afterNavigateNext();
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, true);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start profile provisioning
        verify(mUi).startProfileOwnerProvisioning(mParams);
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_withExistingProfile() throws Exception {
        // GIVEN a managed profile currently exist on the device
        prepareMocksForManagedProfileIntent(false);
        when(mUtils.alreadyHasManagedProfile(mContext)).thenReturn(TEST_USER_ID);
        // WHEN initiating managed profile provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN the UI elements should be updated accordingly and a dialog to remove the existing
        // profile should be shown
        verifyInitiateProfileOwnerUi();
        verify(mUi).showDeleteManagedProfileDialog(any(ComponentName.class),
                anyString(), eq(TEST_USER_ID));
        // WHEN the user clicks next
        mController.afterNavigateNext();
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, true);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start profile provisioning
        verify(mUi).startProfileOwnerProvisioning(mParams);
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_badLauncher() throws Exception {
        // GIVEN that the current launcher does not support managed profiles
        prepareMocksForManagedProfileIntent(false);
        when(mUtils.currentLauncherSupportsManagedProfiles(mContext)).thenReturn(false);
        // WHEN initiating managed profile provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN the UI elements should be updated accordingly
        verifyInitiateProfileOwnerUi();
        // WHEN the user clicks next
        mController.afterNavigateNext();
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, true);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN show a dialog indicating that the current launcher is invalid
        verify(mUi).showCurrentLauncherInvalid();
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_wrongPackage() throws Exception {
        // GIVEN that the provisioning intent tries to set a package different from the caller
        // as owner of the profile
        prepareMocksForManagedProfileIntent(false);
        // WHEN initiating managed profile provisioning
        mController.initiateProvisioning(mIntent, TEST_BOGUS_PACKAGE);
        // THEN show an error dialog and do not continue
        verify(mUi).showErrorAndClose(eq(R.string.device_owner_error_general), anyString());
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_frp() throws Exception {
        // GIVEN managed profile provisioning is invoked from SUW with FRP active
        prepareMocksForManagedProfileIntent(false);
        when(mUtils.isDeviceProvisioned(mContext)).thenReturn(false);
        // setting the data block size to any number greater than 0 should invoke FRP.
        when(mPdbManager.getDataBlockSize()).thenReturn(4);
        // WHEN initiating managed profile provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN show an error dialog and do not continue
        verify(mUi).showErrorAndClose(eq(R.string.device_owner_error_frp), anyString());
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_skipEncryption() throws Exception {
        // GIVEN an intent to provision a managed profile with skip encryption
        prepareMocksForManagedProfileIntent(true);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN the UI elements should be updated accordingly
        verifyInitiateProfileOwnerUi();
        // WHEN the user clicks next
        mController.afterNavigateNext();
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, true);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start profile provisioning
        verify(mUi).startProfileOwnerProvisioning(mParams);
        verify(mUi, never()).requestEncryption(any(ProvisioningParams.class));
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_encryptionNotSupported() throws Exception {
        // GIVEN an intent to provision a managed profile on an unencrypted device that does not
        // support encryption
        prepareMocksForManagedProfileIntent(false);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        when(mDevicePolicyManager.getStorageEncryptionStatus())
                .thenReturn(DevicePolicyManager.ENCRYPTION_STATUS_UNSUPPORTED);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN the UI elements should be updated accordingly
        verifyInitiateProfileOwnerUi();
        // WHEN the user clicks next
        mController.afterNavigateNext();
        // THEN show an error indicating that this device does not support encryption
        verify(mUi).showErrorAndClose(eq(R.string.preprovisioning_error_encryption_not_supported),
                anyString());
        verifyNoMoreInteractions(mUi);
    }

    public void testNfc() throws Exception {
        // GIVEN provisioning was started via an NFC tap and device is already encrypted
        prepareMocksForNfcIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        // WHEN initiating NFC provisioning
        mController.initiateProvisioning(mIntent, null);
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, false);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verify(mUi).startDeviceOwnerProvisioning(TEST_USER_ID, mParams);
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testNfc_skipEncryption() throws Exception {
        // GIVEN provisioning was started via an NFC tap with encryption skipped
        prepareMocksForNfcIntent(ACTION_PROVISION_MANAGED_DEVICE, true);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating NFC provisioning
        mController.initiateProvisioning(mIntent, null);
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, false);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verify(mUi).startDeviceOwnerProvisioning(TEST_USER_ID, mParams);
        verify(mUi, never()).requestEncryption(any(ProvisioningParams.class));
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testNfc_withEncryption() throws Exception {
        // GIVEN provisioning was started via an NFC tap with encryption necessary
        prepareMocksForNfcIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating NFC provisioning
        mController.initiateProvisioning(mIntent, null);
        // THEN show encryption screen
        verify(mUi).requestEncryption(mParams);
        verifyNoMoreInteractions(mUi);
    }

    public void testNfc_afterEncryption() throws Exception {
        // GIVEN provisioning was started via an NFC tap and we have gone through encryption
        // in this case the device gets resumed with the DO intent and startedByTrustedSource flag
        // set
        prepareMocksForAfterEncryption(ACTION_PROVISION_MANAGED_DEVICE, true);
        // WHEN continuing NFC provisioning after encryption
        mController.initiateProvisioning(mIntent, null);
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, false);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verify(mUi).startDeviceOwnerProvisioning(TEST_USER_ID, mParams);
        verifyNoMoreInteractions(mUi);
    }

    public void testNfc_frp() throws Exception {
        // GIVEN provisioning was started via an NFC tap, but the device is locked with FRP
        prepareMocksForNfcIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        // setting the data block size to any number greater than 0 should invoke FRP.
        when(mPdbManager.getDataBlockSize()).thenReturn(4);
        // WHEN initiating NFC provisioning
        mController.initiateProvisioning(mIntent, null);
        // THEN show an error dialog
        verify(mUi).showErrorAndClose(eq(R.string.device_owner_error_frp), anyString());
        verifyNoMoreInteractions(mUi);
    }

    public void testNfc_encryptionNotSupported() throws Exception {
        // GIVEN provisioning was started via an NFC tap, the device is not encrypted and encryption
        // is not supported on the device
        prepareMocksForNfcIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        when(mDevicePolicyManager.getStorageEncryptionStatus())
                .thenReturn(DevicePolicyManager.ENCRYPTION_STATUS_UNSUPPORTED);
        // WHEN initiating NFC provisioning
        mController.initiateProvisioning(mIntent, null);
        // THEN show an error dialog
        verify(mUi).showErrorAndClose(eq(R.string.preprovisioning_error_encryption_not_supported),
                anyString());
        verifyNoMoreInteractions(mUi);
    }

    public void testQr() throws Exception {
        // GIVEN provisioning was started via a QR code and device is already encrypted
        prepareMocksForQrIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        // WHEN initiating QR provisioning
        mController.initiateProvisioning(mIntent, null);
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, false);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verify(mUi).startDeviceOwnerProvisioning(TEST_USER_ID, mParams);
        verifyNoMoreInteractions(mUi);
    }

    public void testQr_skipEncryption() throws Exception {
        // GIVEN provisioning was started via a QR code with encryption skipped
        prepareMocksForQrIntent(ACTION_PROVISION_MANAGED_DEVICE, true);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating QR provisioning
        mController.initiateProvisioning(mIntent, null);
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, false);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verify(mUi).startDeviceOwnerProvisioning(TEST_USER_ID, mParams);
        verify(mUi, never()).requestEncryption(any(ProvisioningParams.class));
        verifyNoMoreInteractions(mUi);
    }

    public void testQr_withEncryption() throws Exception {
        // GIVEN provisioning was started via a QR code with encryption necessary
        prepareMocksForQrIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating QR provisioning
        mController.initiateProvisioning(mIntent, null);
        // THEN show encryption screen
        verify(mUi).requestEncryption(mParams);
        verifyNoMoreInteractions(mUi);
    }

    public void testQr_frp() throws Exception {
        // GIVEN provisioning was started via a QR code, but the device is locked with FRP
        prepareMocksForQrIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        // setting the data block size to any number greater than 0 should invoke FRP.
        when(mPdbManager.getDataBlockSize()).thenReturn(4);
        // WHEN initiating QR provisioning
        mController.initiateProvisioning(mIntent, null);
        // THEN show an error dialog
        verify(mUi).showErrorAndClose(eq(R.string.device_owner_error_frp), anyString());
        verifyNoMoreInteractions(mUi);
    }

    public void testDeviceOwner() throws Exception {
        // GIVEN device owner provisioning was started and device is already encrypted
        prepareMocksForDoIntent(true);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN the UI elements should be updated accordingly
        verifyInitiateDeviceOwnerUi();
        // WHEN the user clicks next
        mController.afterNavigateNext();
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, false);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verify(mUi).startDeviceOwnerProvisioning(TEST_USER_ID, mParams);
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testDeviceOwner_skipEncryption() throws Exception {
        // GIVEN device owner provisioning was started with skip encryption flag
        prepareMocksForDoIntent(true);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN the UI elements should be updated accordingly
        verifyInitiateDeviceOwnerUi();
        // WHEN the user clicks next
        mController.afterNavigateNext();
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, false);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verify(mUi).startDeviceOwnerProvisioning(TEST_USER_ID, mParams);
        verify(mUi, never()).requestEncryption(any(ProvisioningParams.class));
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    // TODO: There is a difference in behaviour here between the managed profile and the device
    // owner case: In managed profile case, we invoke encryption after user clicks next, but in
    // device owner mode we invoke it straight away. Also in theory no need to update
    // the UI elements if we're moving away from this activity straight away.
    public void testDeviceOwner_withEncryption() throws Exception {
        // GIVEN device owner provisioning is started with encryption needed
        prepareMocksForDoIntent(false);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN update the UI elements and show encryption screen
        verifyInitiateDeviceOwnerUi();
        verify(mUi).requestEncryption(mParams);
        verifyNoMoreInteractions(mUi);
    }

    public void testDeviceOwner_afterEncryption() throws Exception {
        // GIVEN device owner provisioning is continued after encryption. In this case we do not set
        // the startedByTrustedSource flag.
        prepareMocksForAfterEncryption(ACTION_PROVISION_MANAGED_DEVICE, false);
        // WHEN provisioning is continued
        mController.initiateProvisioning(mIntent, null);
        // THEN the UI elements should be updated accordingly
        verifyInitiateDeviceOwnerUi();
        // WHEN the user clicks next
        mController.afterNavigateNext();
        // THEN show a user consent dialog
        verify(mUi).showUserConsentDialog(mParams, false);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verify(mUi).startDeviceOwnerProvisioning(TEST_USER_ID, mParams);
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testDeviceOwner_frp() throws Exception {
        // GIVEN device owner provisioning is invoked with FRP active
        prepareMocksForDoIntent(false);
        // setting the data block size to any number greater than 0 should invoke FRP.
        when(mPdbManager.getDataBlockSize()).thenReturn(4);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN show an error dialog
        verify(mUi).showErrorAndClose(eq(R.string.device_owner_error_frp), anyString());
        verifyNoMoreInteractions(mUi);
    }

    private void prepareMocksForManagedProfileIntent(boolean skipEncryption) throws Exception {
        final String action = ACTION_PROVISION_MANAGED_PROFILE;
        when(mIntent.getAction()).thenReturn(action);
        when(mUtils.findDeviceAdmin(TEST_MDM_PACKAGE, null, mContext))
                .thenReturn(TEST_MDM_COMPONENT_NAME);
        when(mUtils.isDeviceProvisioned(mContext)).thenReturn(true);
        when(mDevicePolicyManager.isProvisioningAllowed(action)).thenReturn(true);
        when(mMessageParser.parse(mIntent, mContext)).thenReturn(
                createParams(false, skipEncryption, null, action, TEST_MDM_PACKAGE));
    }

    private void prepareMocksForNfcIntent(String action, boolean skipEncryption) throws Exception {
        when(mIntent.getAction()).thenReturn(ACTION_NDEF_DISCOVERED);
        when(mDevicePolicyManager.isProvisioningAllowed(action)).thenReturn(true);
        when(mMessageParser.parse(mIntent, mContext)).thenReturn(
                createParams(true, skipEncryption, TEST_WIFI_SSID, action, TEST_MDM_PACKAGE));
    }

    private void prepareMocksForQrIntent(String action, boolean skipEncryption) throws Exception {
        when(mIntent.getAction())
                .thenReturn(ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE);
        when(mDevicePolicyManager.isProvisioningAllowed(action)).thenReturn(true);
        when(mMessageParser.parse(mIntent, mContext)).thenReturn(
                createParams(true, skipEncryption, TEST_WIFI_SSID, action, TEST_MDM_PACKAGE));
    }

    private void prepareMocksForDoIntent(boolean skipEncryption) throws Exception {
        final String action = ACTION_PROVISION_MANAGED_DEVICE;
        when(mIntent.getAction()).thenReturn(action);
        when(mDevicePolicyManager.isProvisioningAllowed(action)).thenReturn(true);
        when(mMessageParser.parse(mIntent, mContext)).thenReturn(
                createParams(false, skipEncryption, TEST_WIFI_SSID, action, TEST_MDM_PACKAGE));
    }

    private void prepareMocksForAfterEncryption(String action, boolean startedByTrustedSource)
            throws Exception {
        when(mIntent.getAction()).thenReturn(ACTION_RESUME_PROVISIONING);
        when(mDevicePolicyManager.isProvisioningAllowed(action)).thenReturn(true);
        when(mMessageParser.parse(mIntent, mContext)).thenReturn(
                createParams(
                        startedByTrustedSource, false, TEST_WIFI_SSID, action, TEST_MDM_PACKAGE));
    }

    private ProvisioningParams createParams(boolean startedByTrustedSource, boolean skipEncryption,
            String wifiSsid, String action, String packageName) {
        ProvisioningParams.Builder builder = ProvisioningParams.Builder.builder()
                .setStartedByTrustedSource(startedByTrustedSource)
                .setSkipEncryption(skipEncryption)
                .setProvisioningAction(action)
                .setDeviceAdminPackageName(packageName);
        if (!TextUtils.isEmpty(wifiSsid)) {
            builder.setWifiInfo(WifiInfo.Builder.builder().setSsid(wifiSsid).build());
        }
        return mParams = builder.build();
    }

    private void verifyInitiateProfileOwnerUi() {
        verify(mUi).initiateUi(
                R.string.setup_work_profile,
                R.string.setup_profile_start_setup,
                R.string.company_controls_workspace,
                R.string.the_following_is_your_mdm,
                mParams);
    }

    private void verifyInitiateDeviceOwnerUi() {
        verify(mUi).initiateUi(
                R.string.setup_work_device,
                R.string.setup_device_start_setup,
                R.string.company_controls_device,
                R.string.the_following_is_your_mdm_for_device,
                mParams);
    }
}
