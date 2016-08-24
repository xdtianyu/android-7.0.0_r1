/*
 * Copyright 2014, The Android Open Source Project
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

package com.android.managedprovisioning;

import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_SHAREABLE_DEVICE;

import android.app.AlarmManager;
import android.app.Service;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Process;
import android.os.UserHandle;
import android.os.UserManager;
import android.support.v4.content.LocalBroadcastManager;

import com.android.internal.app.LocalePicker;
import com.android.managedprovisioning.common.IllegalProvisioningArgumentException;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.task.AddWifiNetworkTask;
import com.android.managedprovisioning.task.DeleteNonRequiredAppsTask;
import com.android.managedprovisioning.task.DisallowAddUserTask;
import com.android.managedprovisioning.task.DownloadPackageTask;
import com.android.managedprovisioning.task.InstallPackageTask;
import com.android.managedprovisioning.task.SetDevicePolicyTask;

import java.util.Locale;

/**
 * This service does the work for the DeviceOwnerProvisioningActivity.
 * Feedback is sent back to the activity via intents.
 *
 * <p>
 * If the corresponding activity is killed and restarted, the service is
 * called twice. The service will not start the provisioning flow a second time, but instead
 * send a status update to the activity.
 * </p>
 */
public class DeviceOwnerProvisioningService extends Service {
    private static final boolean DEBUG = false; // To control logging.

    private static final String DEVICE_OWNER = "deviceOwner";

    /**
     * Intent action to activate the CDMA phone connection by OTASP.
     * This is not necessary for a GSM phone connection, which is activated automatically.
     * String must agree with the constants in com.android.phone.InCallScreenShowActivation.
     */
    private static final String ACTION_PERFORM_CDMA_PROVISIONING =
            "com.android.phone.PERFORM_CDMA_PROVISIONING";

    // Intent actions and extras for communication from DeviceOwnerProvisioningService to Activity.
    protected static final String ACTION_PROVISIONING_SUCCESS =
            "com.android.managedprovisioning.provisioning_success";
    protected static final String ACTION_PROVISIONING_ERROR =
            "com.android.managedprovisioning.error";
    protected static final String EXTRA_USER_VISIBLE_ERROR_ID_KEY =
            "UserVisibleErrorMessage-Id";
    protected static final String EXTRA_FACTORY_RESET_REQUIRED =
            "FactoryResetRequired";
    protected static final String ACTION_PROGRESS_UPDATE =
            "com.android.managedprovisioning.progress_update";
    protected static final String EXTRA_PROGRESS_MESSAGE_ID_KEY =
            "ProgressMessageId";
    protected static final String ACTION_REQUEST_WIFI_PICK =
            "com.android.managedprovisioning.request_wifi_pick";

    // Indicates whether provisioning has started.
    private boolean mProvisioningInFlight = false;

    // MessageId of the last progress message.
    private int mLastProgressMessage = -1;

    // MessageId of the last error message.
    private int mLastErrorMessage = -1;

    // Indicates whether reverting the provisioning process up till now requires a factory reset.
    // Is false at the start and flips to true after the first irrevertible action.
    private boolean mFactoryResetRequired = false;

    // Indicates whether provisioning has finished successfully (service waiting to stop).
    private volatile boolean mDone = false;

    // Provisioning tasks.
    private AddWifiNetworkTask mAddWifiNetworkTask;
    private DownloadPackageTask mDownloadPackageTask;
    private InstallPackageTask mInstallPackageTask;
    private SetDevicePolicyTask mSetDevicePolicyTask;
    private DeleteNonRequiredAppsTask mDeleteNonRequiredAppsTask;
    private DisallowAddUserTask mDisallowAddUserTask;

    private ProvisioningParams mParams;

    private final Utils mUtils = new Utils();

    @Override
    public int onStartCommand(final Intent intent, int flags, int startId) {
        if (DEBUG) ProvisionLogger.logd("Device owner provisioning service ONSTARTCOMMAND.");

        synchronized (this) { // Make operations on mProvisioningInFlight atomic.
            if (mProvisioningInFlight) {
                if (DEBUG) ProvisionLogger.logd("Provisioning already in flight.");

                sendProgressUpdateToActivity();

                // Send error message if currently in error state.
                if (mLastErrorMessage >= 0) {
                    sendError();
                }

                // Send success if provisioning was successful.
                if (mDone) {
                    onProvisioningSuccess();
                }
            } else {
                mProvisioningInFlight = true;
                if (DEBUG) ProvisionLogger.logd("First start of the service.");
                progressUpdate(R.string.progress_data_process);

                // Load the ProvisioningParams (from message in Intent).
                mParams = (ProvisioningParams) intent.getParcelableExtra(
                        ProvisioningParams.EXTRA_PROVISIONING_PARAMS);

                // Do the work on a separate thread.
                new Thread(new Runnable() {
                    public void run() {
                        initializeProvisioningEnvironment(mParams);
                        startDeviceOwnerProvisioning(mParams);
                    }
                }).start();
            }
        }
        return START_NOT_STICKY;
    }

    /**
     * This is the core method of this class. It goes through every provisioning step.
     * Each task checks if it is required and executes if it is.
     */
    private void startDeviceOwnerProvisioning(final ProvisioningParams params) {
        if (DEBUG) ProvisionLogger.logd("Starting device owner provisioning");

        // Construct Tasks. Do not start them yet.
        mAddWifiNetworkTask = new AddWifiNetworkTask(this, params.wifiInfo,
                new AddWifiNetworkTask.Callback() {
                    @Override
                    public void onSuccess() {
                        progressUpdate(R.string.progress_download);
                        mDownloadPackageTask.run();
                    }

                    @Override
                    public void onError(){
                        error(R.string.device_owner_error_wifi,
                                false /* do not require factory reset */);
                    }
                });

        mDownloadPackageTask = new DownloadPackageTask(this,
                new DownloadPackageTask.Callback() {
                    @Override
                    public void onSuccess() {
                        progressUpdate(R.string.progress_install);
                        mInstallPackageTask.addInstallIfNecessary(
                                params.inferDeviceAdminPackageName(),
                                mDownloadPackageTask.getDownloadedPackageLocation(DEVICE_OWNER));
                        mInstallPackageTask.run();
                    }

                    @Override
                    public void onError(int errorCode) {
                        switch(errorCode) {
                            case DownloadPackageTask.ERROR_HASH_MISMATCH:
                                error(R.string.device_owner_error_hash_mismatch);
                                break;
                            case DownloadPackageTask.ERROR_DOWNLOAD_FAILED:
                                error(R.string.device_owner_error_download_failed);
                                break;
                            default:
                                error(R.string.device_owner_error_general);
                                break;
                        }
                    }
                });

        // Add packages to download to the DownloadPackageTask.
        mDownloadPackageTask.addDownloadIfNecessary(params.inferDeviceAdminPackageName(),
                params.deviceAdminDownloadInfo, DEVICE_OWNER);

        mInstallPackageTask = new InstallPackageTask(this,
                new InstallPackageTask.Callback() {
                    @Override
                    public void onSuccess() {
                        progressUpdate(R.string.progress_set_owner);
                        try {
                            // Now that the app has been installed, we can look for the device admin
                            // component in it.
                            mSetDevicePolicyTask.run(mParams.inferDeviceAdminComponentName(
                                    DeviceOwnerProvisioningService.this));
                        } catch (IllegalProvisioningArgumentException e) {
                            error(R.string.device_owner_error_general);
                            ProvisionLogger.loge("Failed to infer the device admin component name",
                                    e);
                            return;
                        }
                    }

                    @Override
                    public void onError(int errorCode) {
                        switch(errorCode) {
                            case InstallPackageTask.ERROR_PACKAGE_INVALID:
                                error(R.string.device_owner_error_package_invalid);
                                break;
                            case InstallPackageTask.ERROR_INSTALLATION_FAILED:
                                error(R.string.device_owner_error_installation_failed);
                                break;
                            default:
                                error(R.string.device_owner_error_general);
                                break;
                        }
                    }
                });

        mSetDevicePolicyTask = new SetDevicePolicyTask(this,
                getResources().getString(R.string.default_owned_device_username),
                new SetDevicePolicyTask.Callback() {
                    @Override
                    public void onSuccess() {
                        mDeleteNonRequiredAppsTask.run();
                    }

                    @Override
                    public void onError() {
                        error(R.string.device_owner_error_general);
                    }
                });

        // For split system user devices that will have a system device owner, don't adjust the set
        // of enabled packages in the system user as we expect the right set of packages to be
        // enabled for the system user out of the box. For other devices, the set of available
        // packages can vary depending on management state.
        boolean leaveAllSystemAppsEnabled = params.leaveAllSystemAppsEnabled ||
                params.provisioningAction.equals(ACTION_PROVISION_MANAGED_SHAREABLE_DEVICE);
        mDeleteNonRequiredAppsTask = new DeleteNonRequiredAppsTask(
                this, params.inferDeviceAdminPackageName(),
                DeleteNonRequiredAppsTask.DEVICE_OWNER, true /* creating new profile */,
                UserHandle.myUserId(), leaveAllSystemAppsEnabled,
                new DeleteNonRequiredAppsTask.Callback() {
                    @Override
                    public void onSuccess() {
                        mDisallowAddUserTask.maybeDisallowAddUsers();

                        // Done with provisioning. Success.
                        onProvisioningSuccess();
                    }

                    @Override
                    public void onError() {
                        error(R.string.device_owner_error_general);
                    }
                });

        mDisallowAddUserTask = new DisallowAddUserTask((UserManager) getSystemService(USER_SERVICE),
                UserHandle.myUserId(), UserManager.isSplitSystemUser());

        // Start first task, which starts next task in its callback, etc.
        progressUpdate(R.string.progress_connect_to_wifi);
        mAddWifiNetworkTask.run();
    }

    private void error(int dialogMessage) {
        error(dialogMessage, true /* require factory reset */);
    }

    private void error(int dialogMessage, boolean factoryResetRequired) {
        mLastErrorMessage = dialogMessage;
        if (factoryResetRequired) {
            mFactoryResetRequired = true;
        }
        sendError();
        // Wait for stopService() call from the activity.
    }

    private void sendError() {
        if (DEBUG) {
            ProvisionLogger.logd("Reporting Error: " + getResources()
                .getString(mLastErrorMessage));
        }
        Intent intent = new Intent(ACTION_PROVISIONING_ERROR);
        intent.setClass(this, DeviceOwnerProvisioningActivity.ServiceMessageReceiver.class);
        intent.putExtra(EXTRA_USER_VISIBLE_ERROR_ID_KEY, mLastErrorMessage);
        intent.putExtra(EXTRA_FACTORY_RESET_REQUIRED, mFactoryResetRequired);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private void progressUpdate(int progressMessage) {
        if (DEBUG) {
            ProvisionLogger.logd("Reporting progress update: " + getResources()
                .getString(progressMessage));
        }
        mLastProgressMessage = progressMessage;
        sendProgressUpdateToActivity();
    }

    private void sendProgressUpdateToActivity() {
        Intent intent = new Intent(ACTION_PROGRESS_UPDATE);
        intent.putExtra(EXTRA_PROGRESS_MESSAGE_ID_KEY, mLastProgressMessage);
        intent.setClass(this, DeviceOwnerProvisioningActivity.ServiceMessageReceiver.class);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private void onProvisioningSuccess() {
        // Copying an account needs to happen late in the provisioning process to allow the current
        // user to be started, but before we tell the MDM that provisioning succeeded.
        maybeCopyAccount();

        if (DEBUG) ProvisionLogger.logd("Reporting success.");
        mDone = true;

        // Set DPM userProvisioningState appropriately and persists mParams for use during
        // FinalizationActivity if necessary.
        mUtils.markUserProvisioningStateInitiallyDone(this, mParams);

        Intent successIntent = new Intent(ACTION_PROVISIONING_SUCCESS);
        successIntent.setClass(this, DeviceOwnerProvisioningActivity.ServiceMessageReceiver.class);
        LocalBroadcastManager.getInstance(this).sendBroadcast(successIntent);
        // Wait for stopService() call from the activity.
    }

    private void maybeCopyAccount() {
        if (!UserManager.isSplitSystemUser()) {
            // Only one user involved in this case.
            return;
        }

        mUtils.maybeCopyAccount(DeviceOwnerProvisioningService.this,
                mParams.accountToMigrate, UserHandle.SYSTEM,
                Process.myUserHandle());
    }

    private void initializeProvisioningEnvironment(ProvisioningParams params) {
        setTimeAndTimezone(params.timeZone, params.localTime);
        setLocale(params.locale);

        // Start CDMA activation to enable phone calls.
        final Intent intent = new Intent(ACTION_PERFORM_CDMA_PROVISIONING);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        if (DEBUG) ProvisionLogger.logd("Starting cdma activation activity");
        startActivity(intent); // Activity will be a Nop if not a CDMA device.
    }

    private void setTimeAndTimezone(String timeZone, long localTime) {
        try {
            final AlarmManager am = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
            if (timeZone != null) {
                if (DEBUG) ProvisionLogger.logd("Setting time zone to " + timeZone);
                am.setTimeZone(timeZone);
            }
            if (localTime > 0) {
                if (DEBUG) ProvisionLogger.logd("Setting time to " + localTime);
                am.setTime(localTime);
            }
        } catch (Exception e) {
            ProvisionLogger.loge("Alarm manager failed to set the system time/timezone.");
            // Do not stop provisioning process, but ignore this error.
        }
    }

    private void setLocale(Locale locale) {
        if (locale == null || locale.equals(Locale.getDefault())) {
            return;
        }
        try {
            if (DEBUG) ProvisionLogger.logd("Setting locale to " + locale);
            // If locale is different from current locale this results in a configuration change,
            // which will trigger the restarting of the activity.
            LocalePicker.updateLocale(locale);
        } catch (Exception e) {
            ProvisionLogger.loge("Failed to set the system locale.");
            // Do not stop provisioning process, but ignore this error.
        }
    }

    @Override
    public void onCreate () {
        if (DEBUG) ProvisionLogger.logd("Device owner provisioning service ONCREATE.");
    }

    @Override
    public void onDestroy () {
        if (DEBUG) ProvisionLogger.logd("Device owner provisioning service ONDESTROY");
        if (mAddWifiNetworkTask != null) {
            mAddWifiNetworkTask.cleanUp();
        }
        if (mDownloadPackageTask != null) {
            mDownloadPackageTask.cleanUp();
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
