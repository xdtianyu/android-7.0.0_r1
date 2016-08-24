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

package com.android.managedprovisioning;

import static android.app.admin.DeviceAdminReceiver.ACTION_PROFILE_PROVISIONING_COMPLETE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE;

import android.accounts.AccountManager;
import android.app.Activity;
import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.app.Service;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.IPackageManager;
import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.os.AsyncTask;
import android.os.IBinder;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.support.v4.content.LocalBroadcastManager;

import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.CrossProfileIntentFiltersHelper;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.task.DeleteNonRequiredAppsTask;
import com.android.managedprovisioning.task.DisableBluetoothSharingTask;
import com.android.managedprovisioning.task.DisableInstallShortcutListenersTask;
import com.android.managedprovisioning.task.ManagedProfileSettingsTask;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * Service that runs the profile owner provisioning.
 *
 * <p>This service is started from and sends updates to the {@link ProfileOwnerProvisioningActivity},
 * which contains the provisioning UI.
 */
public class ProfileOwnerProvisioningService extends Service {
    // Intent actions for communication with DeviceOwnerProvisioningService.
    public static final String ACTION_PROVISIONING_SUCCESS =
            "com.android.managedprovisioning.provisioning_success";
    public static final String ACTION_PROVISIONING_ERROR =
            "com.android.managedprovisioning.error";
    public static final String ACTION_PROVISIONING_CANCELLED =
            "com.android.managedprovisioning.cancelled";
    public static final String EXTRA_LOG_MESSAGE_KEY = "ProvisioningErrorLogMessage";

    // Maximum time we will wait for ACTION_USER_UNLOCK until we give up and continue without
    // account migration.
    private static final int USER_UNLOCKED_TIMEOUT_SECONDS = 120; // 2 minutes

    // Status flags for the provisioning process.
    /** Provisioning not started. */
    private static final int STATUS_UNKNOWN = 0;
    /** Provisioning started, no errors or cancellation requested received. */
    private static final int STATUS_STARTED = 1;
    /** Provisioning in progress, but user has requested cancellation. */
    private static final int STATUS_CANCELLING = 2;
    // Final possible states for the provisioning process.
    /** Provisioning completed successfully. */
    private static final int STATUS_DONE = 3;
    /** Provisioning failed and cleanup complete. */
    private static final int STATUS_ERROR = 4;
    /** Provisioning cancelled and cleanup complete. */
    private static final int STATUS_CANCELLED = 5;

    private IPackageManager mIpm;
    private UserInfo mManagedProfileOrUserInfo;
    private AccountManager mAccountManager;
    private UserManager mUserManager;
    private UserUnlockedReceiver mUnlockedReceiver;

    private AsyncTask<Intent, Void, Void> runnerTask;

    // MessageId of the last error message.
    private String mLastErrorMessage = null;

    // Current status of the provisioning process.
    private int mProvisioningStatus = STATUS_UNKNOWN;

    private ProvisioningParams mParams;

    private final Utils mUtils = new Utils();

    private class RunnerTask extends AsyncTask<Intent, Void, Void> {
        @Override
        protected Void doInBackground(Intent ... intents) {
            // Atomically move to STATUS_STARTED at most once.
            synchronized (ProfileOwnerProvisioningService.this) {
                if (mProvisioningStatus == STATUS_UNKNOWN) {
                    mProvisioningStatus = STATUS_STARTED;
                } else {
                    // Process already started, don't start again.
                    return null;
                }
            }

            try {
                initialize(intents[0]);
                startManagedProfileOrUserProvisioning();
            } catch (ProvisioningException e) {
                // Handle internal errors.
                error(e.getMessage(), e);
                finish();
            } catch (Exception e) {
                // General catch-all to ensure process cleans up in all cases.
                error("Failed to initialize managed profile, aborting.", e);
                finish();
            }

            return null;
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();

        mIpm = IPackageManager.Stub.asInterface(ServiceManager.getService("package"));
        mAccountManager = (AccountManager) getSystemService(Context.ACCOUNT_SERVICE);
        mUserManager = (UserManager) getSystemService(Context.USER_SERVICE);

        runnerTask = new RunnerTask();
    }

    @Override
    public int onStartCommand(final Intent intent, int flags, int startId) {
        if (ProfileOwnerProvisioningActivity.ACTION_CANCEL_PROVISIONING.equals(intent.getAction())) {
            ProvisionLogger.logd("Cancelling profile owner provisioning service");
            cancelProvisioning();
            return START_NOT_STICKY;
        }

        ProvisionLogger.logd("Starting profile owner provisioning service");

        try {
            runnerTask.execute(intent);
        } catch (IllegalStateException e) {
            // runnerTask is either in progress, or finished.
            ProvisionLogger.logd(
                    "ProfileOwnerProvisioningService: Provisioning already started, "
                    + "second provisioning intent not being processed, only reporting status.");
            reportStatus();
        }
        return START_NOT_STICKY;
    }

    private void reportStatus() {
        synchronized (this) {
            switch (mProvisioningStatus) {
                case STATUS_DONE:
                    notifyActivityOfSuccess();
                    break;
                case STATUS_CANCELLED:
                    notifyActivityCancelled();
                    break;
                case STATUS_ERROR:
                    notifyActivityError();
                    break;
                case STATUS_UNKNOWN:
                case STATUS_STARTED:
                case STATUS_CANCELLING:
                    // Don't notify UI of status when just-started/in-progress.
                    break;
            }
        }
    }

    private void cancelProvisioning() {
        synchronized (this) {
            switch (mProvisioningStatus) {
                case STATUS_DONE:
                    // Process completed, we should honor user request to cancel
                    // though.
                    mProvisioningStatus = STATUS_CANCELLING;
                    cleanupUserProfile();
                    mProvisioningStatus = STATUS_CANCELLED;
                    reportStatus();
                    break;
                case STATUS_UNKNOWN:
                    // Process hasn't started, move straight to cancelled state.
                    mProvisioningStatus = STATUS_CANCELLED;
                    reportStatus();
                    break;
                case STATUS_STARTED:
                    // Process is mid-flow, flag up that the user has requested
                    // cancellation.
                    mProvisioningStatus = STATUS_CANCELLING;
                    break;
                case STATUS_CANCELLING:
                    // Cancellation already being processed.
                    break;
                case STATUS_CANCELLED:
                case STATUS_ERROR:
                    // Process already completed, nothing left to cancel.
                    break;
            }
        }
    }

    private void initialize(Intent intent) {
        // Load the ProvisioningParams (from message in Intent).
        mParams = (ProvisioningParams) intent.getParcelableExtra(
                ProvisioningParams.EXTRA_PROVISIONING_PARAMS);
        if (mParams.accountToMigrate != null) {
            ProvisionLogger.logi("Migrating account to managed profile");
        }
    }

    /**
     * This is the core method to create a managed profile or user. It goes through every
     * provisioning step.
     */
    private void startManagedProfileOrUserProvisioning() throws ProvisioningException {

        ProvisionLogger.logd("Starting managed profile or user provisioning");

        if(isProvisioningManagedUser()) {
            mManagedProfileOrUserInfo = mUserManager.getUserInfo(mUserManager.getUserHandle());
            if(mManagedProfileOrUserInfo == null) {
                throw raiseError("Couldn't get current user information");
            }
        } else {
            // Work through the provisioning steps in their corresponding order
            createProfile(getString(R.string.default_managed_profile_name));
        }
        if (mManagedProfileOrUserInfo != null) {
            final DeleteNonRequiredAppsTask deleteNonRequiredAppsTask;
            final DisableInstallShortcutListenersTask disableInstallShortcutListenersTask;
            final DisableBluetoothSharingTask disableBluetoothSharingTask;
            final ManagedProfileSettingsTask managedProfileSettingsTask =
                    new ManagedProfileSettingsTask(this, mManagedProfileOrUserInfo.id);

            disableInstallShortcutListenersTask = new DisableInstallShortcutListenersTask(this,
                    mManagedProfileOrUserInfo.id);
            disableBluetoothSharingTask = new DisableBluetoothSharingTask(
                    mManagedProfileOrUserInfo.id);
            // TODO Add separate set of apps for MANAGED_USER, currently same as of DEVICE_OWNER.
            deleteNonRequiredAppsTask = new DeleteNonRequiredAppsTask(this,
                    mParams.deviceAdminComponentName.getPackageName(),
                    (isProvisioningManagedUser() ? DeleteNonRequiredAppsTask.MANAGED_USER
                            : DeleteNonRequiredAppsTask.PROFILE_OWNER),
                    true /* creating new profile */,
                    mManagedProfileOrUserInfo.id, false /* delete non-required system apps */,
                    new DeleteNonRequiredAppsTask.Callback() {

                        @Override
                        public void onSuccess() {
                            // Need to explicitly handle exceptions here, as
                            // onError() is not invoked for failures in
                            // onSuccess().
                            try {
                                disableBluetoothSharingTask.run();
                                if (!isProvisioningManagedUser()) {
                                    managedProfileSettingsTask.run();
                                    disableInstallShortcutListenersTask.run();
                                }
                                setUpUserOrProfile();
                            } catch (ProvisioningException e) {
                                error(e.getMessage(), e);
                            } catch (Exception e) {
                                error("Provisioning failed", e);
                            }
                            finish();
                        }

                        @Override
                        public void onError() {
                            // Raise an error with a tracing exception attached.
                            error("Delete non required apps task failed.", new Exception());
                            finish();
                        }
                    });

            deleteNonRequiredAppsTask.run();
        }
    }

    /**
     * Called when the new profile or managed user is ready for provisioning (the profile is created
     * and all the apps not needed have been deleted).
     */
    private void setUpUserOrProfile() throws ProvisioningException {
        installMdmOnManagedProfile();
        setMdmAsActiveAdmin();
        setMdmAsManagedProfileOwner();

        if (!isProvisioningManagedUser()) {
            setOrganizationColor();
            setDefaultUserRestrictions();
            CrossProfileIntentFiltersHelper.setFilters(
                    getPackageManager(), getUserId(), mManagedProfileOrUserInfo.id);
            if (!startManagedProfile(mManagedProfileOrUserInfo.id)) {
                throw raiseError("Could not start user in background");
            }
            // Wait for ACTION_USER_UNLOCKED to be sent before trying to migrate the account.
            // Even if no account is present, we should not send the provisioning complete broadcast
            // before the managed profile user is properly started.
            if ((mUnlockedReceiver != null) && !mUnlockedReceiver.waitForUserUnlocked()) {
                return;
            }

            // Note: account migration must happen after setting the profile owner.
            // Otherwise, there will be a time interval where some apps may think that the account
            // does not have a profile owner.
            mUtils.maybeCopyAccount(this, mParams.accountToMigrate, Process.myUserHandle(),
                    mManagedProfileOrUserInfo.getUserHandle());
        }
    }

    /**
     * Notify the calling activity of our final status, perform any cleanup if
     * the process didn't succeed.
     */
    private void finish() {
        ProvisionLogger.logi("Finishing provisioning process, status: "
                             + mProvisioningStatus);
        // Reached the end of the provisioning process, take appropriate action
        // based on current mProvisioningStatus.
        synchronized (this) {
            switch (mProvisioningStatus) {
                case STATUS_STARTED:
                    // Provisioning process completed normally.
                    notifyMdmAndCleanup();
                    mProvisioningStatus = STATUS_DONE;
                    break;
                case STATUS_UNKNOWN:
                    // No idea how we could end up in finish() in this state,
                    // but for safety treat it as an error and fall-through to
                    // STATUS_ERROR.
                    mLastErrorMessage = "finish() invoked in STATUS_UNKNOWN";
                    mProvisioningStatus = STATUS_ERROR;
                    break;
                case STATUS_ERROR:
                    // Process errored out, cleanup partially created managed
                    // profile.
                    cleanupUserProfile();
                    break;
                case STATUS_CANCELLING:
                    // User requested cancellation during processing, remove
                    // the successfully created profile.
                    cleanupUserProfile();
                    mProvisioningStatus = STATUS_CANCELLED;
                    break;
                case STATUS_CANCELLED:
                case STATUS_DONE:
                    // Shouldn't be possible to already be in this state?!?
                    ProvisionLogger.logw("finish() invoked multiple times?");
                    break;
            }
        }

        ProvisionLogger.logi("Finished provisioning process, final status: "
                + mProvisioningStatus);

        // Notify UI activity of final status reached.
        reportStatus();
    }

    /**
     * Initialize the user that underlies the managed profile.
     * This is required so that the provisioning complete broadcast can be sent across to the
     * profile and apps can run on it.
     */
    private boolean startManagedProfile(int userId)  {
        ProvisionLogger.logd("Starting user in background");
        IActivityManager iActivityManager = ActivityManagerNative.getDefault();
        // Register a receiver for the Intent.ACTION_USER_UNLOCKED to know when the managed profile
        // has been started and unlocked.
        mUnlockedReceiver = new UserUnlockedReceiver(this, userId);
        try {
            return iActivityManager.startUserInBackground(userId);
        } catch (RemoteException neverThrown) {
            // Never thrown, as we are making local calls.
            ProvisionLogger.loge("This should not happen.", neverThrown);
        }
        return false;
    }

    private void notifyActivityOfSuccess() {
        Intent successIntent = new Intent(ACTION_PROVISIONING_SUCCESS);
        LocalBroadcastManager.getInstance(ProfileOwnerProvisioningService.this)
                .sendBroadcast(successIntent);
    }

    /**
     * Notify the mdm that provisioning has completed. When the mdm has received the intent, stop
     * the service and notify the {@link ProfileOwnerProvisioningActivity} so that it can finish
     * itself.
     */
    private void notifyMdmAndCleanup() {
        // Set DPM userProvisioningState appropriately and persist mParams for use during
        // FinalizationActivity if necessary.
        mUtils.markUserProvisioningStateInitiallyDone(this, mParams);

        if (mParams.provisioningAction.equals(ACTION_PROVISION_MANAGED_PROFILE)) {
            // Set the user_setup_complete flag on the managed-profile as setup-wizard is never run
            // for that user. This is not relevant for other cases since
            // Utils.markUserProvisioningStateInitiallyDone() communicates provisioning state to
            // setup-wizard via DPM.setUserProvisioningState() if necessary.
            mUtils.markUserSetupComplete(this, mManagedProfileOrUserInfo.id);
        }

        // If profile owner provisioning was started after current user setup is completed, then we
        // can directly send the ACTION_PROFILE_PROVISIONING_COMPLETE broadcast to the MDM.
        // But if the provisioning was started as part of setup wizard flow, we signal setup-wizard
        // should shutdown via DPM.setUserProvisioningState(), which will result in a finalization
        // intent being sent to us once setup-wizard finishes. As part of the finalization intent
        // handling we then broadcast ACTION_PROFILE_PROVISIONING_COMPLETE.
        if (mUtils.isUserSetupCompleted(this)) {
            UserHandle managedUserHandle = new UserHandle(mManagedProfileOrUserInfo.id);

            // Use an ordered broadcast, so that we only finish when the mdm has received it.
            // Avoids a lag in the transition between provisioning and the mdm.
            BroadcastReceiver mdmReceivedSuccessReceiver = new MdmReceivedSuccessReceiver(
                    mParams.accountToMigrate, mParams.deviceAdminComponentName.getPackageName());

            Intent completeIntent = new Intent(ACTION_PROFILE_PROVISIONING_COMPLETE);
            completeIntent.setComponent(mParams.deviceAdminComponentName);
            completeIntent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES |
                    Intent.FLAG_RECEIVER_FOREGROUND);
            if (mParams.adminExtrasBundle != null) {
                completeIntent.putExtra(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE,
                        mParams.adminExtrasBundle);
            }

            sendOrderedBroadcastAsUser(completeIntent, managedUserHandle, null,
                    mdmReceivedSuccessReceiver, null, Activity.RESULT_OK, null, null);
            ProvisionLogger.logd("Provisioning complete broadcast has been sent to user "
                    + managedUserHandle.getIdentifier());
        }
    }

    private void createProfile(String profileName) throws ProvisioningException {

        ProvisionLogger.logd("Creating managed profile with name " + profileName);

        mManagedProfileOrUserInfo = mUserManager.createProfileForUser(profileName,
                UserInfo.FLAG_MANAGED_PROFILE | UserInfo.FLAG_DISABLED,
                Process.myUserHandle().getIdentifier());

        if (mManagedProfileOrUserInfo == null) {
            throw raiseError("Couldn't create profile.");
        }
    }

    private void installMdmOnManagedProfile() throws ProvisioningException {
        ProvisionLogger.logd("Installing mobile device management app "
                + mParams.deviceAdminComponentName + " on managed profile");

        try {
            int status = mIpm.installExistingPackageAsUser(
                mParams.deviceAdminComponentName.getPackageName(), mManagedProfileOrUserInfo.id);
            switch (status) {
              case PackageManager.INSTALL_SUCCEEDED:
                  return;
              case PackageManager.INSTALL_FAILED_USER_RESTRICTED:
                  // Should not happen because we're not installing a restricted user
                  throw raiseError("Could not install mobile device management app on managed "
                          + "profile because the user is restricted");
              case PackageManager.INSTALL_FAILED_INVALID_URI:
                  // Should not happen because we already checked
                  throw raiseError("Could not install mobile device management app on managed "
                          + "profile because the package could not be found");
              default:
                  throw raiseError("Could not install mobile device management app on managed "
                          + "profile. Unknown status: " + status);
            }
        } catch (RemoteException neverThrown) {
            // Never thrown, as we are making local calls.
            ProvisionLogger.loge("This should not happen.", neverThrown);
        }
    }

    private void setMdmAsManagedProfileOwner() throws ProvisioningException {
        ProvisionLogger.logd("Setting package " + mParams.deviceAdminComponentName
                + " as managed profile owner.");

        DevicePolicyManager dpm =
                (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);
        if (!dpm.setProfileOwner(
                mParams.deviceAdminComponentName,
                mParams.deviceAdminComponentName.getPackageName(),
                mManagedProfileOrUserInfo.id)) {
            ProvisionLogger.logw("Could not set profile owner.");
            throw raiseError("Could not set profile owner.");
        }
    }

    private void setMdmAsActiveAdmin() {
        ProvisionLogger.logd("Setting package " + mParams.deviceAdminComponentName
                + " as active admin.");

        DevicePolicyManager dpm =
                (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);
        dpm.setActiveAdmin(mParams.deviceAdminComponentName, true /* refreshing*/,
                mManagedProfileOrUserInfo.id);
    }

    private void setOrganizationColor() {
        if (mParams.mainColor != null) {
            DevicePolicyManager dpm =
                    (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);
            dpm.setOrganizationColorForUser(mParams.mainColor, mManagedProfileOrUserInfo.id);
        }
    }

    private ProvisioningException raiseError(String message) throws ProvisioningException {
        throw new ProvisioningException(message);
    }

    /**
     * Record the fact that an error occurred, change mProvisioningStatus to
     * reflect the fact the provisioning process failed
     */
    private void error(String dialogMessage, Exception e) {
        synchronized (this) {
            // Only case where an error condition should be notified is if we
            // are in the normal flow for provisioning. If the process has been
            // cancelled or already completed, then the fact there is an error
            // is almost irrelevant.
            if (mProvisioningStatus == STATUS_STARTED) {
                mProvisioningStatus = STATUS_ERROR;
                mLastErrorMessage = dialogMessage;

                ProvisionLogger.logw(
                        "Error occured during provisioning process: "
                        + dialogMessage,
                        e);
            } else {
                ProvisionLogger.logw(
                        "Unexpected error occured in status ["
                        + mProvisioningStatus + "]: " + dialogMessage,
                        e);
            }
        }
    }

    private void setDefaultUserRestrictions() {
        mUserManager.setUserRestriction(UserManager.DISALLOW_WALLPAPER, true,
                mManagedProfileOrUserInfo.getUserHandle());
    }

    private void notifyActivityError() {
        Intent intent = new Intent(ACTION_PROVISIONING_ERROR);
        intent.putExtra(EXTRA_LOG_MESSAGE_KEY, mLastErrorMessage);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private void notifyActivityCancelled() {
        Intent cancelIntent = new Intent(ACTION_PROVISIONING_CANCELLED);
        LocalBroadcastManager.getInstance(this).sendBroadcast(cancelIntent);
    }

    /**
     * Performs cleanup of any created user-profile on failure/cancellation.
     */
    private void cleanupUserProfile() {
        if (mManagedProfileOrUserInfo != null && !isProvisioningManagedUser()) {
            ProvisionLogger.logd("Removing managed profile");
            mUserManager.removeUser(mManagedProfileOrUserInfo.id);
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    /**
     * Internal exception to allow provisioning process to terminal quickly and
     * cleanly on first error, rather than continuing to process despite errors
     * occurring.
     */
    private static class ProvisioningException extends Exception {
        public ProvisioningException(String detailMessage) {
            super(detailMessage);
        }
    }

    public boolean isProvisioningManagedUser() {
        return mParams.provisioningAction.equals(DevicePolicyManager.ACTION_PROVISION_MANAGED_USER);
    }

    /**
     * BroadcastReceiver that listens to {@link Intent#ACTION_USER_UNLOCKED} in order to provide
     * a blocking wait until the managed profile has been started and unlocked.
     */
    private static class UserUnlockedReceiver extends BroadcastReceiver {
        private static final IntentFilter FILTER = new IntentFilter(Intent.ACTION_USER_UNLOCKED);

        private final Semaphore semaphore = new Semaphore(0);
        private final Context mContext;
        private final int mUserId;

        UserUnlockedReceiver(Context context, int userId) {
            mContext = context;
            mUserId = userId;
            mContext.registerReceiverAsUser(this, new UserHandle(userId), FILTER, null, null);
        }

        @Override
        public void onReceive(Context context, Intent intent ) {
            if (!Intent.ACTION_USER_UNLOCKED.equals(intent.getAction())) {
                ProvisionLogger.logw("Unexpected intent: " + intent);
                return;
            }
            if (intent.getIntExtra(Intent.EXTRA_USER_HANDLE, UserHandle.USER_NULL) == mUserId) {
                ProvisionLogger.logd("Received ACTION_USER_UNLOCKED for user " + mUserId);
                semaphore.release();
                mContext.unregisterReceiver(this);
            }
        }

        public boolean waitForUserUnlocked() {
            ProvisionLogger.logd("Waiting for ACTION_USER_UNLOCKED");
            try {
                return semaphore.tryAcquire(USER_UNLOCKED_TIMEOUT_SECONDS, TimeUnit.SECONDS);
            } catch (InterruptedException ie) {
                mContext.unregisterReceiver(this);
                return false;
            }
        }
    }
 }
