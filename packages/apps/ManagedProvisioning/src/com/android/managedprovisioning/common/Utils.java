/*
 * Copyright 2014, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.managedprovisioning.common;

import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_SHAREABLE_DEVICE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_USER;
import static android.app.admin.DevicePolicyManager.MIME_TYPE_PROVISIONING_NFC;
import static android.nfc.NfcAdapter.ACTION_NDEF_DISCOVERED;
import static java.nio.charset.StandardCharsets.UTF_8;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerFuture;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.content.pm.UserInfo;
import android.graphics.Color;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.os.Build;
import android.os.Bundle;
import android.os.Parcelable;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.UserManager;
import android.os.storage.StorageManager;
import android.provider.Settings.Global;
import android.provider.Settings.Secure;
import android.text.TextUtils;
import android.util.Base64;

import java.io.IOException;
import java.lang.Integer;
import java.lang.Math;
import java.lang.String;
import java.nio.charset.StandardCharsets;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;

import com.android.internal.annotations.VisibleForTesting;
import com.android.managedprovisioning.FinalizationActivity;
import com.android.managedprovisioning.ProvisionLogger;
import com.android.managedprovisioning.TrampolineActivity;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.model.PackageDownloadInfo;

/**
 * Class containing various auxiliary methods.
 */
public class Utils {
    private static final int ACCOUNT_COPY_TIMEOUT_SECONDS = 60 * 3;  // 3 minutes

    private static final int THRESHOLD_BRIGHT_COLOR = 160; // A color needs a brightness of at least
    // this value to be considered bright. (brightness being between 0 and 255).
    public Utils() {}

    /**
     * Returns the currently installed system apps on a given user.
     *
     * <p>Calls into the {@link IPackageManager} to retrieve all installed packages on the given
     * user and returns the package names of all system apps.
     *
     * @param ipm an {@link IPackageManager} object
     * @param userId the id of the user we are interested in
     */
    public Set<String> getCurrentSystemApps(IPackageManager ipm, int userId) {
        Set<String> apps = new HashSet<String>();
        List<ApplicationInfo> aInfos = null;
        try {
            aInfos = ipm.getInstalledApplications(
                    PackageManager.GET_UNINSTALLED_PACKAGES, userId).getList();
        } catch (RemoteException neverThrown) {
            ProvisionLogger.loge("This should not happen.", neverThrown);
        }
        for (ApplicationInfo aInfo : aInfos) {
            if ((aInfo.flags & ApplicationInfo.FLAG_SYSTEM) != 0) {
                apps.add(aInfo.packageName);
            }
        }
        return apps;
    }

    /**
     * Disables a given component in a given user.
     *
     * @param toDisable the component that should be disabled
     * @param userId the id of the user where the component should be disabled.
     */
    public void disableComponent(ComponentName toDisable, int userId) {
        setComponentEnabledSetting(
                IPackageManager.Stub.asInterface(ServiceManager.getService("package")),
                toDisable,
                PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                userId);
    }

    /**
     * Enables a given component in a given user.
     *
     * @param toEnable the component that should be enabled
     * @param userId the id of the user where the component should be disabled.
     */
    public void enableComponent(ComponentName toEnable, int userId) {
        setComponentEnabledSetting(
                IPackageManager.Stub.asInterface(ServiceManager.getService("package")),
                toEnable,
                PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                userId);
    }

    /**
     * Disables a given component in a given user.
     *
     * @param ipm an {@link IPackageManager} object
     * @param toDisable the component that should be disabled
     * @param userId the id of the user where the component should be disabled.
     */
    @VisibleForTesting
    void setComponentEnabledSetting(IPackageManager ipm, ComponentName toDisable,
            int enabledSetting, int userId) {
        try {
            ipm.setComponentEnabledSetting(toDisable,
                    enabledSetting, PackageManager.DONT_KILL_APP,
                    userId);
        } catch (RemoteException neverThrown) {
            ProvisionLogger.loge("This should not happen.", neverThrown);
        } catch (Exception e) {
            ProvisionLogger.logw("Component not found, not changing enabled setting: "
                + toDisable.toShortString());
        }
    }

    /**
     * Check the validity of the admin component name supplied, or try to infer this componentName
     * from the package.
     *
     * We are supporting lookup by package name for legacy reasons.
     *
     * If mdmComponentName is supplied (not null):
     * mdmPackageName is ignored.
     * Check that the package of mdmComponentName is installed, that mdmComponentName is a
     * receiver in this package, and return it. The receiver can be in disabled state.
     *
     * Otherwise:
     * mdmPackageName must be supplied (not null).
     * Check that this package is installed, try to infer a potential device admin in this package,
     * and return it.
     */
    // TODO: Add unit tests
    public ComponentName findDeviceAdmin(String mdmPackageName,
            ComponentName mdmComponentName, Context c) throws IllegalProvisioningArgumentException {
        if (mdmComponentName != null) {
            mdmPackageName = mdmComponentName.getPackageName();
        }
        if (mdmPackageName == null) {
            throw new IllegalProvisioningArgumentException("Neither the package name nor the"
                    + " component name of the admin are supplied");
        }
        PackageInfo pi;
        try {
            pi = c.getPackageManager().getPackageInfo(mdmPackageName,
                    PackageManager.GET_RECEIVERS | PackageManager.MATCH_DISABLED_COMPONENTS);
        } catch (NameNotFoundException e) {
            throw new IllegalProvisioningArgumentException("Mdm "+ mdmPackageName
                    + " is not installed. ", e);
        }
        if (mdmComponentName != null) {
            // If the component was specified in the intent: check that it is in the manifest.
            checkAdminComponent(mdmComponentName, pi);
            return mdmComponentName;
        } else {
            // Otherwise: try to find a potential device admin in the manifest.
            return findDeviceAdminInPackage(mdmPackageName, pi);
        }
    }

    /**
     * Verifies that an admin component is part of a given package.
     *
     * @param mdmComponentName the admin component to be checked
     * @param pi the {@link PackageInfo} of the package to be checked.
     *
     * @throws IllegalProvisioningArgumentException if the given component is not part of the
     *         package
     */
    private void checkAdminComponent(ComponentName mdmComponentName, PackageInfo pi)
            throws IllegalProvisioningArgumentException{
        for (ActivityInfo ai : pi.receivers) {
            if (mdmComponentName.getClassName().equals(ai.name)) {
                return;
            }
        }
        throw new IllegalProvisioningArgumentException("The component " + mdmComponentName
                + " cannot be found");
    }

    private ComponentName findDeviceAdminInPackage(String mdmPackageName, PackageInfo pi)
            throws IllegalProvisioningArgumentException {
        ComponentName mdmComponentName = null;
        for (ActivityInfo ai : pi.receivers) {
            if (!TextUtils.isEmpty(ai.permission) &&
                    ai.permission.equals(android.Manifest.permission.BIND_DEVICE_ADMIN)) {
                if (mdmComponentName != null) {
                    throw new IllegalProvisioningArgumentException("There are several "
                            + "device admins in " + mdmPackageName + " but no one in specified");
                } else {
                    mdmComponentName = new ComponentName(mdmPackageName, ai.name);
                }
            }
        }
        if (mdmComponentName == null) {
            throw new IllegalProvisioningArgumentException("There are no device admins in"
                    + mdmPackageName);
        }
        return mdmComponentName;
    }

    /**
     * Returns whether the current user is the system user.
     */
    public boolean isCurrentUserSystem() {
        return UserHandle.myUserId() == UserHandle.USER_SYSTEM;
    }

    /**
     * Returns whether the device is currently managed.
     */
    public boolean isDeviceManaged(Context context) {
        DevicePolicyManager dpm =
                (DevicePolicyManager) context.getSystemService(Context.DEVICE_POLICY_SERVICE);
        return dpm.isDeviceManaged();
    }

    /**
     * Returns whether the calling user is a managed profile.
     */
    public boolean isManagedProfile(Context context) {
        UserManager um = (UserManager) context.getSystemService(Context.USER_SERVICE);
        UserInfo user = um.getUserInfo(UserHandle.myUserId());
        return user != null ? user.isManagedProfile() : false;
    }

    /**
     * Returns true if the given package requires an update.
     *
     * <p>There are two cases where an update is required:
     * 1. The package is not currently present on the device.
     * 2. The package is present, but the version is below the minimum supported version.
     *
     * @param packageName the package to be checked for updates
     * @param minSupportedVersion the minimum supported version
     * @param context a {@link Context} object
     */
    public boolean packageRequiresUpdate(String packageName, int minSupportedVersion,
            Context context) {
        try {
            PackageInfo packageInfo = context.getPackageManager().getPackageInfo(packageName, 0);
            // Always download packages if no minimum version given.
            if (minSupportedVersion != PackageDownloadInfo.DEFAULT_MINIMUM_VERSION
                    && packageInfo.versionCode >= minSupportedVersion) {
                return false;
            }
        } catch (NameNotFoundException e) {
            // Package not on device.
        }

        return true;
    }

    /**
     * Transforms a string into a byte array.
     *
     * @param s the string to be transformed
     */
    public byte[] stringToByteArray(String s)
        throws NumberFormatException {
        try {
            return Base64.decode(s, Base64.URL_SAFE);
        } catch (IllegalArgumentException e) {
            throw new NumberFormatException("Incorrect format. Should be Url-safe Base64 encoded.");
        }
    }

    /**
     * Transforms a byte array into a string.
     *
     * @param bytes the byte array to be transformed
     */
    public String byteArrayToString(byte[] bytes) {
        return Base64.encodeToString(bytes, Base64.URL_SAFE | Base64.NO_PADDING | Base64.NO_WRAP);
    }

    /**
     * Sets user setup complete on a given user.
     *
     * <p>This will set USER_SETUP_COMPLETE to 1 on the given user.
     */
    public void markUserSetupComplete(Context context, int userId) {
        ProvisionLogger.logd("Setting USER_SETUP_COMPLETE to 1 for user " + userId);
        Secure.putIntForUser(context.getContentResolver(), Secure.USER_SETUP_COMPLETE, 1, userId);
    }

    /**
     * Returns whether USER_SETUP_COMPLETE is set on the calling user.
     */
    public boolean isUserSetupCompleted(Context context) {
        return Secure.getInt(context.getContentResolver(), Secure.USER_SETUP_COMPLETE, 0) != 0;
    }

    /**
     * Returns whether DEVICE_PROVISIONED is set.
     */
    public boolean isDeviceProvisioned(Context context) {
        return Global.getInt(context.getContentResolver(), Global.DEVICE_PROVISIONED, 0) != 0;
    }

    /**
     * Set the current users userProvisioningState depending on the following factors:
     * <ul>
     *     <li>We're setting up a managed-profile - need to set state on two users.</li>
     *     <li>User-setup has previously been completed or not - skip states relating to
     *     communicating with setup-wizard</li>
     *     <li>DPC requested we skip the rest of setup-wizard.</li>
     * </ul>
     *
     * @param context a {@link Context} object
     * @param params configuration for current provisioning attempt
     */
    // TODO: Add unit tests
    public void markUserProvisioningStateInitiallyDone(Context context,
            ProvisioningParams params) {
        int currentUserId = UserHandle.myUserId();
        int managedProfileUserId = UserHandle.USER_NULL;
        DevicePolicyManager dpm = context.getSystemService(DevicePolicyManager.class);

        // new provisioning state for current user, if non-null
        Integer newState = null;
         // New provisioning state for managed-profile of current user, if non-null.
        Integer newProfileState = null;

        boolean userSetupCompleted = isUserSetupCompleted(context);
        if (params.provisioningAction.equals(ACTION_PROVISION_MANAGED_PROFILE)) {
            // Managed profiles are a special case as two users are involved.
            managedProfileUserId = getManagedProfile(context).getIdentifier();
            if (userSetupCompleted) {
                // SUW on current user is complete, so nothing much to do beyond indicating we're
                // all done.
                newProfileState = DevicePolicyManager.STATE_USER_SETUP_FINALIZED;
            } else {
                // We're still in SUW, so indicate that a managed-profile was setup on current user,
                // and that we're awaiting finalization on both.
                newState = DevicePolicyManager.STATE_USER_PROFILE_COMPLETE;
                newProfileState = DevicePolicyManager.STATE_USER_SETUP_COMPLETE;
            }
        } else if (userSetupCompleted) {
            // User setup was previously completed this is an unexpected case.
            ProvisionLogger.logw("user_setup_complete set, but provisioning was started");
        } else if (params.skipUserSetup) {
            // DPC requested setup-wizard is skipped, indicate this to SUW.
            newState = DevicePolicyManager.STATE_USER_SETUP_COMPLETE;
        } else {
            // DPC requested setup-wizard is not skipped, indicate this to SUW.
            newState = DevicePolicyManager.STATE_USER_SETUP_INCOMPLETE;
        }

        if (newState != null) {
            setUserProvisioningState(dpm, newState, currentUserId);
        }
        if (newProfileState != null) {
            setUserProvisioningState(dpm, newProfileState, managedProfileUserId);
        }
        if (!userSetupCompleted) {
            // We expect a PROVISIONING_FINALIZATION intent to finish setup if we're still in
            // user-setup.
            FinalizationActivity.storeProvisioningParams(context, params);
        }
    }

    /**
     * Finalize the current users userProvisioningState depending on the following factors:
     * <ul>
     *     <li>We're setting up a managed-profile - need to set state on two users.</li>
     * </ul>
     *
     * @param context a {@link Context} object
     * @param params configuration for current provisioning attempt - if null (because
     *               ManagedProvisioning wasn't used for first phase of provisioning) aassumes we
     *               can just mark current user as being in finalized provisioning state
     */
    // TODO: Add unit tests
    public void markUserProvisioningStateFinalized(Context context,
            ProvisioningParams params) {
        int currentUserId = UserHandle.myUserId();
        int managedProfileUserId = -1;
        DevicePolicyManager dpm = context.getSystemService(DevicePolicyManager.class);
        Integer newState = null;
        Integer newProfileState = null;

        if (params != null && params.provisioningAction.equals(ACTION_PROVISION_MANAGED_PROFILE)) {
            // Managed profiles are a special case as two users are involved.
            managedProfileUserId = getManagedProfile(context).getIdentifier();

            newState = DevicePolicyManager.STATE_USER_UNMANAGED;
            newProfileState = DevicePolicyManager.STATE_USER_SETUP_FINALIZED;
        } else {
            newState = DevicePolicyManager.STATE_USER_SETUP_FINALIZED;
        }

        if (newState != null) {
            setUserProvisioningState(dpm, newState, currentUserId);
        }
        if (newProfileState != null) {
            setUserProvisioningState(dpm, newProfileState, managedProfileUserId);
        }
    }

    private void setUserProvisioningState(DevicePolicyManager dpm, int state, int userId) {
        ProvisionLogger.logi("Setting userProvisioningState for user " + userId + " to: " + state);
        dpm.setUserProvisioningState(state, userId);
    }

    /**
     * Returns the first existing managed profile if any present, null otherwise.
     *
     * <p>Note that we currently only support one managed profile per device.
     */
    // TODO: Add unit tests
    public UserHandle getManagedProfile(Context context) {
        UserManager userManager = (UserManager) context.getSystemService(Context.USER_SERVICE);
        int currentUserId = userManager.getUserHandle();
        List<UserInfo> userProfiles = userManager.getProfiles(currentUserId);
        for (UserInfo profile : userProfiles) {
            if (profile.isManagedProfile()) {
                return new UserHandle(profile.id);
            }
        }
        return null;
    }

    /**
     * Returns the user id of an already existing managed profile or -1 if none exists.
     */
    // TODO: Add unit tests
    public int alreadyHasManagedProfile(Context context) {
        UserHandle managedUser = getManagedProfile(context);
        if (managedUser != null) {
            return managedUser.getIdentifier();
        } else {
            return -1;
        }
    }

    /**
     * Removes an account.
     *
     * <p>This removes the given account from the calling user's list of accounts.
     *
     * @param context a {@link Context} object
     * @param account the account to be removed
     */
    // TODO: Add unit tests
    public void removeAccount(Context context, Account account) {
        try {
            AccountManager accountManager =
                    (AccountManager) context.getSystemService(Context.ACCOUNT_SERVICE);
            AccountManagerFuture<Bundle> bundle = accountManager.removeAccount(account,
                    null, null /* callback */, null /* handler */);
            // Block to get the result of the removeAccount operation
            if (bundle.getResult().getBoolean(AccountManager.KEY_BOOLEAN_RESULT, false)) {
                ProvisionLogger.logw("Account removed from the primary user.");
            } else {
                Intent removeIntent = (Intent) bundle.getResult().getParcelable(
                        AccountManager.KEY_INTENT);
                if (removeIntent != null) {
                    ProvisionLogger.logi("Starting activity to remove account");
                    TrampolineActivity.startActivity(context, removeIntent);
                } else {
                    ProvisionLogger.logw("Could not remove account from the primary user.");
                }
            }
        } catch (OperationCanceledException | AuthenticatorException | IOException e) {
            ProvisionLogger.logw("Exception removing account from the primary user.", e);
        }
    }

    /**
     * Copies an account to a given user.
     *
     * <p>Copies a given account form {@code sourceUser} to {@code targetUser}. This call is
     * blocking until the operation has succeeded. If within a timeout the account hasn't been
     * successfully copied to the new user, we give up.
     *
     * @param context a {@link Context} object
     * @param accountToMigrate the account to be migrated
     * @param sourceUser the {@link UserHandle} of the user to copy from
     * @param targetUser the {@link UserHandle} of the user to copy to
     * @return whether account migration successfully completed
     */
    public boolean maybeCopyAccount(Context context, Account accountToMigrate,
            UserHandle sourceUser, UserHandle targetUser) {
        if (accountToMigrate == null) {
            ProvisionLogger.logd("No account to migrate.");
            return false;
        }
        if (sourceUser.equals(targetUser)) {
            ProvisionLogger.loge("sourceUser and targetUser are the same, won't migrate account.");
            return false;
        }
        ProvisionLogger.logd("Attempting to copy account from " + sourceUser + " to " + targetUser);
        try {
            AccountManager accountManager =
                    (AccountManager) context.getSystemService(Context.ACCOUNT_SERVICE);
            boolean copySucceeded = accountManager.copyAccountToUser(
                    accountToMigrate,
                    sourceUser,
                    targetUser,
                    /* callback= */ null, /* handler= */ null)
                    .getResult(ACCOUNT_COPY_TIMEOUT_SECONDS, TimeUnit.SECONDS);
            if (copySucceeded) {
                ProvisionLogger.logi("Copied account to " + targetUser);
                return true;
            } else {
                ProvisionLogger.loge("Could not copy account to " + targetUser);
            }
        } catch (OperationCanceledException | AuthenticatorException | IOException e) {
            ProvisionLogger.loge("Exception copying account to " + targetUser, e);
        }
        return false;
    }

    /**
     * Returns whether FRP is supported on the device.
     */
    public boolean isFrpSupported(Context context) {
        Object pdbManager = context.getSystemService(Context.PERSISTENT_DATA_BLOCK_SERVICE);
        return pdbManager != null;
    }

    /**
     * Translates a given managed provisioning intent to its corresponding provisioning flow, using
     * the action from the intent.
     *
     * <p/>This is necessary because, unlike other provisioning actions which has 1:1 mapping, there
     * are multiple actions that can trigger the device owner provisioning flow. This includes
     * {@link ACTION_PROVISION_MANAGED_DEVICE}, {@link ACTION_NDEF_DISCOVERED} and
     * {@link ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE}. These 3 actions are equivalent
     * excepts they are sent from a different source.
     *
     * @return the appropriate DevicePolicyManager declared action for the given incoming intent.
     * @throws IllegalProvisioningArgumentException if intent is malformed
     */
    // TODO: Add unit tests
    public String mapIntentToDpmAction(Intent intent)
            throws IllegalProvisioningArgumentException {
        if (intent == null || intent.getAction() == null) {
            throw new IllegalProvisioningArgumentException("Null intent action.");
        }

        // Map the incoming intent to a DevicePolicyManager.ACTION_*, as there is a N:1 mapping in
        // some cases.
        String dpmProvisioningAction;
        switch (intent.getAction()) {
            // Trivial cases.
            case ACTION_PROVISION_MANAGED_DEVICE:
            case ACTION_PROVISION_MANAGED_SHAREABLE_DEVICE:
            case ACTION_PROVISION_MANAGED_USER:
            case ACTION_PROVISION_MANAGED_PROFILE:
                dpmProvisioningAction = intent.getAction();
                break;

            // NFC cases which need to take mime-type into account.
            case ACTION_NDEF_DISCOVERED:
                String mimeType = intent.getType();
                switch (mimeType) {
                    case MIME_TYPE_PROVISIONING_NFC:
                        dpmProvisioningAction = ACTION_PROVISION_MANAGED_DEVICE;
                        break;

                    default:
                        throw new IllegalProvisioningArgumentException(
                                "Unknown NFC bump mime-type: " + mimeType);
                }
                break;

            // Device owner provisioning from a trusted app.
            // TODO (b/27217042): review for new management modes in split system-user model
            case ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE:
                dpmProvisioningAction = ACTION_PROVISION_MANAGED_DEVICE;
                break;

            default:
                throw new IllegalProvisioningArgumentException("Unknown intent action "
                        + intent.getAction());
        }
        return dpmProvisioningAction;
    }

    /**
     * Sends an intent to trigger a factory reset.
     */
    // TODO: Move the FR intent into a Globals class.
    public void sendFactoryResetBroadcast(Context context, String reason) {
        Intent intent = new Intent(Intent.ACTION_MASTER_CLEAR);
        intent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);
        intent.putExtra(Intent.EXTRA_REASON, reason);
        context.sendBroadcast(intent);
    }

    /**
     * Returns whether the given provisioning action is a profile owner action.
     */
    // TODO: Move the list of device owner actions into a Globals class.
    public final boolean isProfileOwnerAction(String action) {
        return action.equals(ACTION_PROVISION_MANAGED_PROFILE)
                || action.equals(ACTION_PROVISION_MANAGED_USER);
    }

    /**
     * Returns whether the given provisioning action is a device owner action.
     */
    // TODO: Move the list of device owner actions into a Globals class.
    public final boolean isDeviceOwnerAction(String action) {
        return action.equals(ACTION_PROVISION_MANAGED_DEVICE)
                || action.equals(ACTION_PROVISION_MANAGED_SHAREABLE_DEVICE);
    }

    /**
     * Returns whether the device currently has connectivity.
     */
    public boolean isConnectedToNetwork(Context context) {
        NetworkInfo info = getActiveNetworkInfo(context);
        return info != null && info.isConnected();
    }

    /**
     * Returns whether the device is currently connected to a wifi.
     */
    public boolean isConnectedToWifi(Context context) {
        NetworkInfo info = getActiveNetworkInfo(context);
        return info != null
                && info.isConnected()
                && info.getType() == ConnectivityManager.TYPE_WIFI;
    }

    private NetworkInfo getActiveNetworkInfo(Context context) {
        ConnectivityManager cm =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (cm != null) {
            return cm.getActiveNetworkInfo();
        }
        return null;
    }

    /**
     * Returns whether encryption is required on this device.
     *
     * <p>Encryption is required if the device is not currently encrypted and the persistent
     * system flag {@code persist.sys.no_req_encrypt} is not set.
     */
    public boolean isEncryptionRequired() {
        return !isPhysicalDeviceEncrypted()
                && !SystemProperties.getBoolean("persist.sys.no_req_encrypt", false);
    }

    /**
     * Returns whether the device is currently encrypted.
     */
    public boolean isPhysicalDeviceEncrypted() {
        return StorageManager.isEncrypted();
    }

    /**
     * Returns the wifi pick intent.
     */
    // TODO: Move this intent into a Globals class.
    public Intent getWifiPickIntent() {
        Intent wifiIntent = new Intent(WifiManager.ACTION_PICK_WIFI_NETWORK);
        wifiIntent.putExtra("extra_prefs_show_button_bar", true);
        wifiIntent.putExtra("wifi_enable_next_on_connect", true);
        return wifiIntent;
    }

    /**
     * Returns whether the device has a split system user.
     *
     * <p>Split system user means that user 0 is system only and all meat users are separate from
     * the system user.
     */
    public boolean isSplitSystemUser() {
        return UserManager.isSplitSystemUser();
    }

    /**
     * Returns whether the currently chosen launcher supports managed profiles.
     *
     * <p>A launcher is deemed to support managed profiles when its target API version is at least
     * {@link Build.VERSION_CODES#LOLLIPOP}.
     */
    public boolean currentLauncherSupportsManagedProfiles(Context context) {
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_HOME);

        PackageManager pm = context.getPackageManager();
        ResolveInfo launcherResolveInfo = pm.resolveActivity(intent,
                PackageManager.MATCH_DEFAULT_ONLY);
        if (launcherResolveInfo == null) {
            return false;
        }
        try {
            // If the user has not chosen a default launcher, then launcherResolveInfo will be
            // referring to the resolver activity. It is fine to create a managed profile in
            // this case since there will always be at least one launcher on the device that
            // supports managed profile feature.
            ApplicationInfo launcherAppInfo = pm.getApplicationInfo(
                    launcherResolveInfo.activityInfo.packageName, 0 /* default flags */);
            return versionNumberAtLeastL(launcherAppInfo.targetSdkVersion);
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        }
    }

    /**
     * Returns whether the given version number is at least lollipop.
     *
     * @param versionNumber the version number to be verified.
     */
    private boolean versionNumberAtLeastL(int versionNumber) {
        return versionNumber >= Build.VERSION_CODES.LOLLIPOP;
    }

    public boolean isBrightColor(int color) {
        // we're using the brightness formula: (r * 299 + g * 587 + b * 144) / 1000
        return Color.red(color) * 299 + Color.green(color) * 587 + Color.blue(color) * 114
                >= 1000 * THRESHOLD_BRIGHT_COLOR;
    }
}
