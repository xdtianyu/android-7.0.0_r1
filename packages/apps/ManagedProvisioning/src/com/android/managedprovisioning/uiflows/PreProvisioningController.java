/*
 * Copyright 2016, The Android Open Source Project
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
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_SHAREABLE_DEVICE;
import static com.android.internal.util.Preconditions.checkNotNull;
import static com.android.managedprovisioning.common.Globals.ACTION_RESUME_PROVISIONING;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.ActivityManager;
import android.app.admin.DevicePolicyManager;
import android.app.KeyguardManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.os.AsyncTask;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings.Global;
import android.service.persistentdata.PersistentDataBlockManager;
import android.text.TextUtils;

import com.android.internal.annotations.VisibleForTesting;
import com.android.managedprovisioning.common.IllegalProvisioningArgumentException;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.parser.MessageParser;
import com.android.managedprovisioning.ProvisionLogger;
import com.android.managedprovisioning.R;

import java.util.List;

public class PreProvisioningController {
    private final Context mContext;
    private final Ui mUi;
    private final MessageParser mMessageParser;
    private final Utils mUtils;
    private final EncryptionController mEncryptionController;

    // used system services
    private final DevicePolicyManager mDevicePolicyManager;
    private final UserManager mUserManager;
    private final PackageManager mPackageManager;
    private final ActivityManager mActivityManager;
    private final KeyguardManager mKeyguardManager;
    private final PersistentDataBlockManager mPdbManager;

    private ProvisioningParams mParams;
    private boolean mIsProfileOwnerProvisioning;

    public PreProvisioningController(
            @NonNull Context context,
            @NonNull Ui ui) {
        this(context, ui, new MessageParser(), new Utils(),
                EncryptionController.getInstance(context));
    }

    @VisibleForTesting
    PreProvisioningController(
            @NonNull Context context,
            @NonNull Ui ui,
            @NonNull MessageParser parser,
            @NonNull Utils utils,
            @NonNull EncryptionController encryptionController) {
        mContext = checkNotNull(context, "Context must not be null");
        mUi = checkNotNull(ui, "Ui must not be null");
        mMessageParser = checkNotNull(parser, "MessageParser must not be null");
        mUtils = checkNotNull(utils, "Utils must not be null");
        mEncryptionController = checkNotNull(encryptionController,
                "EncryptionController must not be null");

        mDevicePolicyManager = (DevicePolicyManager) mContext.getSystemService(
                Context.DEVICE_POLICY_SERVICE);
        mUserManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        mPackageManager = mContext.getPackageManager();
        mActivityManager = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        mKeyguardManager = (KeyguardManager) mContext.getSystemService(Context.KEYGUARD_SERVICE);
        mPdbManager = (PersistentDataBlockManager) mContext.getSystemService(
                Context.PERSISTENT_DATA_BLOCK_SERVICE);
    }

    interface Ui {
        /**
         * Show an error message and cancel provisioning.
         *
         * @param resId resource id used to form the user facing error message
         * @param errorMessage an error message that gets logged for debugging
         */
        void showErrorAndClose(int resId, String errorMessage);

        /**
         * Request the user to encrypt the device.
         *
         * @param params the {@link ProvisioningParams} object related to the ongoing provisioning
         */
        void requestEncryption(ProvisioningParams params);

        /**
         * Request the user to choose a wifi network.
         */
        void requestWifiPick();

        /**
         * Initialize the pre provisioning UI with the mdm info and the relevant strings.
         *
         * @param headerRes resource id for the header text
         * @param titleRes resource id for the title text
         * @param consentRes resource id of the consent text
         * @param mdmInfoRes resource id for the mdm info text
         * @param params the {@link ProvisioningParams} object related to the ongoing provisioning
         */
        void initiateUi(int headerRes, int titleRes, int consentRes, int mdmInfoRes,
                ProvisioningParams params);

        /**
         * Start device owner provisioning.
         *
         * @param userId the id of the user we want to start provisioning on
         * @param params the {@link ProvisioningParams} object related to the ongoing provisioning
         */
        void startDeviceOwnerProvisioning(int userId, ProvisioningParams params);

        /**
         * Start profile owner provisioning.
         *
         * @param params the {@link ProvisioningParams} object related to the ongoing provisioning
         */
        void startProfileOwnerProvisioning(ProvisioningParams params);

        /**
         * Show a user consent dialog.
         *
         * @param params the {@link ProvisioningParams} object related to the ongoing provisioning
         * @param isProfileOwnerProvisioning whether we're provisioning a profile owner
         */
        void showUserConsentDialog(ProvisioningParams params, boolean isProfileOwnerProvisioning);

        /**
         * Show a dialog to delete an existing managed profile.
         *
         * @param mdmPackageName the {@link ComponentName} of the existing profile's profile owner
         * @param domainName domain name of the organization which owns the managed profile
         *
         * @param userId the user id of the existing profile
         */
        void showDeleteManagedProfileDialog(ComponentName mdmPackageName, String domainName,
                int userId);

        /**
         * Show an error dialog indicating that the current launcher does not support managed
         * profiles and ask the user to choose a different one.
         */
        void showCurrentLauncherInvalid();
    }

    public void initiateProvisioning(Intent intent, String callingPackage) {
        // Check factory reset protection as the first thing
        if (factoryResetProtected()) {
            mUi.showErrorAndClose(R.string.device_owner_error_frp,
                    "Factory reset protection blocks provisioning.");
            return;
        }

        try {
            // Read the provisioning params from the provisioning intent
            mParams = mMessageParser.parse(intent, mContext);

            // If this is a resume after encryption or trusted intent, we don't need to verify the
            // caller. Otherwise, verify that the calling app is trying to set itself as
            // Device/ProfileOwner
            if (!ACTION_RESUME_PROVISIONING.equals(intent.getAction()) &&
                    !mParams.startedByTrustedSource) {
                verifyCaller(callingPackage);
            }
        } catch (IllegalProvisioningArgumentException e) {
            // TODO: make this a generic error message
            mUi.showErrorAndClose(R.string.device_owner_error_general, e.getMessage());
            return;
        }

        mIsProfileOwnerProvisioning = mUtils.isProfileOwnerAction(mParams.provisioningAction);
        // Check whether provisioning is allowed for the current action
        if (!mDevicePolicyManager.isProvisioningAllowed(mParams.provisioningAction)) {
            showProvisioningError(mParams.provisioningAction);
            return;
        }

        // Initiate the corresponding provisioning mode
        if (mIsProfileOwnerProvisioning) {
            initiateProfileOwnerProvisioning(intent);
        } else {
            initiateDeviceOwnerProvisioning(intent);
        }
    }

    /**
     * Verify that the caller is trying to set itself as owner.
     *
     * @throws IllegalProvisioningArgumentException if the caller is trying to set a different
     * package as owner.
     */
    private void verifyCaller(@NonNull String callingPackage)
            throws IllegalProvisioningArgumentException {
        checkNotNull(callingPackage,
                "Calling package is null. Was startActivityForResult used to start this activity?");
        if (!callingPackage.equals(mParams.inferDeviceAdminPackageName())) {
            throw new IllegalProvisioningArgumentException("Permission denied, "
                    + "calling package tried to set a different package as owner. ");
        }
    }

    private void initiateDeviceOwnerProvisioning(Intent intent) {
        if (!mParams.startedByTrustedSource) {
            mUi.initiateUi(
                    R.string.setup_work_device,
                    R.string.setup_device_start_setup,
                    R.string.company_controls_device,
                    R.string.the_following_is_your_mdm_for_device,
                    mParams);
        }

        // Ask to encrypt the device before proceeding
        if (isEncryptionRequired()) {
            maybeTriggerEncryption();
            return;
        }

        // Have the user pick a wifi network if necessary.
        // It is not possible to ask the user to pick a wifi network if
        // the screen is locked.
        // TODO: remove this check once we know the screen will not be locked.
        if (mKeyguardManager.inKeyguardRestrictedInputMode()) {
            ProvisionLogger.logi("Cannot pick wifi because the screen is locked.");
            // Have the user pick a wifi network if necessary.
        } else if (!mUtils.isConnectedToNetwork(mContext) && mParams.wifiInfo == null) {
            if (canRequestWifiPick()) {
                mUi.requestWifiPick();
                return;
            } else {
                ProvisionLogger.logi(
                        "Cannot pick wifi because there is no handler to the intent");
            }
        }
        askForConsentOrStartDeviceOwnerProvisioning();
    }

    private void initiateProfileOwnerProvisioning(Intent intent) {
        mUi.initiateUi(
                R.string.setup_work_profile,
                R.string.setup_profile_start_setup,
                R.string.company_controls_workspace,
                R.string.the_following_is_your_mdm,
                mParams);

        // If there is already a managed profile, setup the profile deletion dialog.
        int existingManagedProfileUserId = mUtils.alreadyHasManagedProfile(mContext);
        if (existingManagedProfileUserId != -1) {
            ComponentName mdmPackageName = mDevicePolicyManager
                    .getProfileOwnerAsUser(existingManagedProfileUserId);
            String domainName = mDevicePolicyManager
                    .getProfileOwnerNameAsUser(existingManagedProfileUserId);
            mUi.showDeleteManagedProfileDialog(mdmPackageName, domainName,
                    existingManagedProfileUserId);
        }
    }

    /**
     * Start provisioning for real. In profile owner case, double check that the launcher
     * supports managed profiles if necessary. In device owner case, possibly create a new user
     * before starting provisioning.
     */
    public void continueProvisioningAfterUserConsent() {
        if (isProfileOwnerProvisioning()) {
            checkLauncherAndStartProfileOwnerProvisioning();
        } else {
            maybeCreateUserAndStartDeviceOwnerProvisioning();
        }
    }

    /**
     * Invoked when the user continues provisioning by pressing the next button.
     *
     * <p>If device hasn't been encrypted yet, invoke the encryption flow. Otherwise, show a user
     * consent before starting provisioning.
     */
    public void afterNavigateNext() {
        if (isEncryptionRequired()) {
            maybeTriggerEncryption();
        } else {
            // Notify the user once more that the admin will have full control over the profile,
            // then start provisioning.
            mUi.showUserConsentDialog(mParams, mIsProfileOwnerProvisioning);
        }
    }

    /**
     * Returns whether the device needs encryption.
     *
     * @param skip indicating whether the parameter to skip encryption was given.
     */
    private boolean isEncryptionRequired() {
        return !mParams.skipEncryption && mUtils.isEncryptionRequired();
    }

    /**
     * Check whether the device supports encryption. If it does not support encryption, but
     * encryption is requested, show an error dialog.
     */
    private void maybeTriggerEncryption() {
        if (mDevicePolicyManager.getStorageEncryptionStatus() ==
                DevicePolicyManager.ENCRYPTION_STATUS_UNSUPPORTED) {
            mUi.showErrorAndClose(R.string.preprovisioning_error_encryption_not_supported,
                    "This device does not support encryption, but "
                    + DevicePolicyManager.EXTRA_PROVISIONING_SKIP_ENCRYPTION
                    + " was not passed.");
        } else {
            mUi.requestEncryption(mParams);
        }
    }

    private void checkLauncherAndStartProfileOwnerProvisioning() {
        // Check whether the current launcher supports managed profiles.
        if (!mUtils.currentLauncherSupportsManagedProfiles(mContext)) {
            mUi.showCurrentLauncherInvalid();
        } else {
            // Cancel the boot reminder as provisioning has now started.
            mEncryptionController.cancelEncryptionReminder();
            mUi.startProfileOwnerProvisioning(mParams);
        }
    }

    public void askForConsentOrStartDeviceOwnerProvisioning() {
        // If we are started by Nfc and the device supports FRP, we need to ask for user consent
        // since FRP will not be activated at the end of the flow.
        if (mParams.startedByTrustedSource) {
            if (mUtils.isFrpSupported(mContext)) {
                mUi.showUserConsentDialog(mParams, false);
            } else {
                maybeCreateUserAndStartDeviceOwnerProvisioning();
            }
        }
        // In other provisioning modes we wait for the user to press next.
    }

    private void maybeCreateUserAndStartDeviceOwnerProvisioning() {
        // Cancel the boot reminder as provisioning has now started.
        mEncryptionController.cancelEncryptionReminder();
        if (isMeatUserCreationRequired(mParams.provisioningAction)) {
            // Create the primary user, and continue the provisioning in this user.
            new CreatePrimaryUserTask().execute();
        } else {
            mUi.startDeviceOwnerProvisioning(mUserManager.getUserHandle(), mParams);
        }
    }

    private boolean factoryResetProtected() {
        // If we are started during setup wizard, check for factory reset protection.
        // If the device is already setup successfully, do not check factory reset protection.
        if (mUtils.isDeviceProvisioned(mContext)) {
            ProvisionLogger.logd("Device is provisioned, FRP not required.");
            return false;
        }

        if (mPdbManager == null) {
            ProvisionLogger.logd("Reset protection not supported.");
            return false;
        }
        int size = mPdbManager.getDataBlockSize();
        ProvisionLogger.logd("Data block size: " + size);
        return size > 0;
    }

    public boolean isMeatUserCreationRequired(String action) {
        if (mUtils.isSplitSystemUser()
                && ACTION_PROVISION_MANAGED_DEVICE.equals(action)) {
            List<UserInfo> users = mUserManager.getUsers();
            if (users.size() > 1) {
                mUi.showErrorAndClose(R.string.device_owner_error_general,
                        "Cannot start Device Owner Provisioning because there are already "
                        + users.size() + " users");
                return false;
            }
            return true;
        } else {
            return false;
        }
    }

    private boolean canRequestWifiPick() {
        return mPackageManager.resolveActivity(mUtils.getWifiPickIntent(), 0) != null;
    }

    private boolean systemHasManagedProfileFeature() {
        return mPackageManager.hasSystemFeature(PackageManager.FEATURE_MANAGED_USERS);
    }

    /**
     * Returns whether the provisioning process is a profile owner provisioning process.
     */
    public boolean isProfileOwnerProvisioning() {
        return mIsProfileOwnerProvisioning;
    }

    @NonNull
    public ProvisioningParams getParams() {
        if (mParams == null) {
            throw new IllegalStateException("ProvisioningParams are null");
        }
        return mParams;
    }

    // TODO: review the use of async task for the case where the activity might have got killed
    private class CreatePrimaryUserTask extends AsyncTask<Void, Void, UserInfo> {
        @Override
        protected UserInfo doInBackground(Void... args) {
            // Create the user where we're going to install the device owner.
            UserInfo userInfo = mUserManager.createUser(
                    mContext.getString(R.string.default_first_meat_user_name),
                    UserInfo.FLAG_PRIMARY | UserInfo.FLAG_ADMIN);

            if (userInfo != null) {
                ProvisionLogger.logi("Created user " + userInfo.id + " to hold the device owner");
            }
            return userInfo;
        }

        @Override
        protected void onPostExecute(UserInfo userInfo) {
            if (userInfo == null) {
                mUi.showErrorAndClose(R.string.device_owner_error_general,
                        "Could not create user to hold the device owner");
            } else {
                mActivityManager.switchUser(userInfo.id);
                mUi.startDeviceOwnerProvisioning(userInfo.id, mParams);
            }
        }
    }

    private void showProvisioningError(String action) {
        UserInfo userInfo = mUserManager.getUserInfo(mUserManager.getUserHandle());
        if (DevicePolicyManager.ACTION_PROVISION_MANAGED_USER.equals(action)) {
            mUi.showErrorAndClose(R.string.user_setup_incomplete,
                        "Exiting managed user provisioning, setup incomplete");
        } else if (DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE.equals(action)) {
            // Try to show an error message explaining why provisioning is not allowed.
            if (!systemHasManagedProfileFeature()) {
                mUi.showErrorAndClose(R.string.managed_provisioning_not_supported,
                        "Exiting managed profile provisioning, "
                        + "managed profiles feature is not available");
            } else if (!userInfo.canHaveProfile()) {
                mUi.showErrorAndClose(R.string.user_cannot_have_work_profile,
                        "Exiting managed profile provisioning, calling user cannot have managed"
                        + "profiles.");
            } else if (mUtils.isDeviceManaged(mContext)) {
                // The actual check in isProvisioningAllowed() is more than just "is there DO?",
                // but for error message showing purpose, isDeviceManaged() will do.
                mUi.showErrorAndClose(R.string.device_owner_exists,
                        "Exiting managed profile provisioning, a device owner exists");
            } else if (!mUserManager.canAddMoreManagedProfiles(UserHandle.myUserId(),
                    true /* after removing one eventual existing managed profile */)) {
                mUi.showErrorAndClose(R.string.maximum_user_limit_reached,
                        "Exiting managed profile provisioning, cannot add more managed profiles.");
            } else {
                mUi.showErrorAndClose(R.string.managed_provisioning_error_text, "Managed profile"
                        + " provisioning not allowed for an unknown reason.");
            }
        } else if (mUtils.isDeviceProvisioned(mContext)) {
            mUi.showErrorAndClose(R.string.device_owner_error_already_provisioned,
                    "Device already provisioned.");
        } else if (!mUtils.isCurrentUserSystem()) {
            mUi.showErrorAndClose(R.string.device_owner_error_general,
                    "Device owner can only be set up for USER_SYSTEM.");
        } else if (action.equals(ACTION_PROVISION_MANAGED_SHAREABLE_DEVICE) &&
                !UserManager.isSplitSystemUser()) {
            mUi.showErrorAndClose(R.string.device_owner_error_general,
                    "System User Device owner can only be set on a split-user system.");
        } else {
            // TODO: show generic error
            mUi.showErrorAndClose(R.string.device_owner_error_general,
                    "Device Owner provisioning not allowed for an unknown reason.");
        }
    }
}
