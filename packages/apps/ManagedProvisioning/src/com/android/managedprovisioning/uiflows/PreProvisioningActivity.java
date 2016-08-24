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

import android.annotation.NonNull;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.managedprovisioning.common.MdmPackageInfo;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.DeleteManagedProfileDialog;
import com.android.managedprovisioning.DeviceOwnerProvisioningActivity;
import com.android.managedprovisioning.LogoUtils;
import com.android.managedprovisioning.ProfileOwnerProvisioningActivity;
import com.android.managedprovisioning.ProvisionLogger;
import com.android.managedprovisioning.R;
import com.android.managedprovisioning.SetupLayoutActivity;
import com.android.managedprovisioning.UserConsentDialog;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.parser.MessageParser;

public class PreProvisioningActivity extends SetupLayoutActivity
        implements UserConsentDialog.ConsentCallback,
        DeleteManagedProfileDialog.DeleteManagedProfileCallback,
        PreProvisioningController.Ui {

    protected static final int ENCRYPT_DEVICE_REQUEST_CODE = 1;
    protected static final int PROVISIONING_REQUEST_CODE = 2;
    protected static final int WIFI_REQUEST_CODE = 3;
    protected static final int CHANGE_LAUNCHER_REQUEST_CODE = 4;

    // Note: must match the constant defined in HomeSettings
    private static final String EXTRA_SUPPORT_MANAGED_PROFILES = "support_managed_profiles";

    protected PreProvisioningController mController;

    protected TextView mConsentMessageTextView;
    protected TextView mMdmInfoTextView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mController = new PreProvisioningController(
                this,
                this);

        mController.initiateProvisioning(getIntent(), getCallingPackage());
    }

    @Override
    public void finish() {
        // The user has backed out of provisioning, so we perform the necessary clean up steps.
        LogoUtils.cleanUp(this);
        EncryptionController.getInstance(this).cancelEncryptionReminder();
        super.finish();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == ENCRYPT_DEVICE_REQUEST_CODE) {
            if (resultCode == RESULT_CANCELED) {
                ProvisionLogger.loge("User canceled device encryption.");
                setResult(Activity.RESULT_CANCELED);
                finish();
            }
        } else if (requestCode == PROVISIONING_REQUEST_CODE) {
            setResult(resultCode);
            finish();
        } else if (requestCode == CHANGE_LAUNCHER_REQUEST_CODE) {
            if (!mUtils.currentLauncherSupportsManagedProfiles(this)) {
                showCurrentLauncherInvalid();
            } else {
                startProfileOwnerProvisioning(mController.getParams());
            }
        } else if (requestCode == WIFI_REQUEST_CODE) {
            if (resultCode == RESULT_CANCELED) {
                ProvisionLogger.loge("User canceled wifi picking.");
                setResult(RESULT_CANCELED);
                finish();
            } else if (resultCode == RESULT_OK) {
                ProvisionLogger.logd("Wifi request result is OK");
                if (mUtils.isConnectedToWifi(this)) {
                    mController.askForConsentOrStartDeviceOwnerProvisioning();
                } else {
                    requestWifiPick();
                }
            }
        } else {
            ProvisionLogger.logw("Unknown result code :" + resultCode);
        }
    }

    @Override
    public void showErrorAndClose(int resourceId, String logText) {
        ProvisionLogger.loge(logText);
        new AlertDialog.Builder(this)
                .setTitle(R.string.provisioning_error_title)
                .setMessage(resourceId)
                .setCancelable(false)
                .setPositiveButton(R.string.device_owner_error_ok,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,int id) {
                                // Close activity
                                PreProvisioningActivity.this.setResult(
                                        Activity.RESULT_CANCELED);
                                PreProvisioningActivity.this.finish();
                            }
                        })
                .show();
    }

    @Override
    public void requestEncryption(ProvisioningParams params) {
        Intent encryptIntent = new Intent(this, EncryptDeviceActivity.class);
        encryptIntent.putExtra(ProvisioningParams.EXTRA_PROVISIONING_PARAMS, params);
        startActivityForResult(encryptIntent, ENCRYPT_DEVICE_REQUEST_CODE);
    }

    @Override
    public void requestWifiPick() {
        startActivityForResult(mUtils.getWifiPickIntent(), WIFI_REQUEST_CODE);
    }

    @Override
    public void showCurrentLauncherInvalid() {
        new AlertDialog.Builder(this)
                .setCancelable(false)
                .setMessage(R.string.managed_provisioning_not_supported_by_launcher)
                .setNegativeButton(R.string.cancel_provisioning,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,int id) {
                                dialog.dismiss();
                                setResult(Activity.RESULT_CANCELED);
                                finish();
                            }
                        })
                .setPositiveButton(R.string.pick_launcher,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,int id) {
                                requestLauncherPick();
                            }
                        })
                .show();
    }

    private void requestLauncherPick() {
        Intent changeLauncherIntent = new Intent(Settings.ACTION_HOME_SETTINGS);
        changeLauncherIntent.putExtra(EXTRA_SUPPORT_MANAGED_PROFILES, true);
        startActivityForResult(changeLauncherIntent, CHANGE_LAUNCHER_REQUEST_CODE);
    }

    @Override
    public void startDeviceOwnerProvisioning(int userId, ProvisioningParams params) {
        Intent intent = new Intent(this, DeviceOwnerProvisioningActivity.class);
        intent.putExtra(ProvisioningParams.EXTRA_PROVISIONING_PARAMS, params);
        startActivityForResultAsUser(intent, PROVISIONING_REQUEST_CODE, new UserHandle(userId));
        // Set cross-fade transition animation into the interstitial progress activity.
        overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
    }

    @Override
    public void startProfileOwnerProvisioning(ProvisioningParams params) {
        Intent intent = new Intent(this, ProfileOwnerProvisioningActivity.class);
        intent.putExtra(ProvisioningParams.EXTRA_PROVISIONING_PARAMS, params);
        startActivityForResult(intent, PROVISIONING_REQUEST_CODE);
        // Set cross-fade transition animation into the interstitial progress activity.
        overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
    }

    @Override
    public void initiateUi(int headerRes, int titleRes, int consentRes, int mdmInfoRes,
            ProvisioningParams params) {
        // Setup the UI.
        initializeLayoutParams(R.layout.user_consent, headerRes, false);
        configureNavigationButtons(R.string.next, View.INVISIBLE, View.VISIBLE);

        mConsentMessageTextView = (TextView) findViewById(R.id.user_consent_message);
        mMdmInfoTextView = (TextView) findViewById(R.id.mdm_info_message);

        mConsentMessageTextView.setText(consentRes);
        mMdmInfoTextView.setText(mdmInfoRes);

        setMdmIconAndLabel(params.inferDeviceAdminPackageName());

        maybeSetLogoAndMainColor(params.mainColor);
        mNextButton.setVisibility(View.VISIBLE);

        setTitle(titleRes);
    }

    private void setMdmIconAndLabel(@NonNull String packageName) {
        MdmPackageInfo packageInfo = MdmPackageInfo.createFromPackageName(this, packageName);
        TextView deviceManagerName = (TextView) findViewById(R.id.device_manager_name);
        if (packageInfo != null) {
            String appLabel = packageInfo.appLabel;
            ImageView imageView = (ImageView) findViewById(R.id.device_manager_icon_view);
            imageView.setImageDrawable(packageInfo.packageIcon);
            imageView.setContentDescription(
                    getResources().getString(R.string.mdm_icon_label, appLabel));

            deviceManagerName.setText(appLabel);
        } else {
            // During provisioning from trusted source, the package is not actually on the device
            // yet, so show a default information.
            deviceManagerName.setText(packageName);
        }
    }

    @Override
    public void showUserConsentDialog(ProvisioningParams params,
            boolean isProfileOwnerProvisioning) {
        UserConsentDialog dialog;
        if (isProfileOwnerProvisioning) {
            dialog = UserConsentDialog.newProfileOwnerInstance();
        } else {
            dialog = UserConsentDialog.newDeviceOwnerInstance(!params.startedByTrustedSource);
        }
        dialog.show(getFragmentManager(), "UserConsentDialogFragment");
    }

    /**
     * Callback for successful user consent request.
     */
    @Override
    public void onDialogConsent() {
        // Right after user consent, provisioning will be started. To avoid talkback reading out
        // the activity title in the time this activity briefly comes back to the foreground, we
        // remove the title.
        setTitle("");

        mController.continueProvisioningAfterUserConsent();
    }

    /**
     * Callback for cancelled user consent request.
     */
    @Override
    public void onDialogCancel() {
        // only show special UI for device owner provisioning.
        if (mController.isProfileOwnerProvisioning()) {
            return;
        }

        // For Nfc provisioning, we automatically show the user consent dialog if applicable.
        // If the user then decides to cancel, we should finish the entire activity and exit.
        // For other cases, dismissing the consent dialog will lead back to PreProvisioningActivity,
        // where we show another dialog asking for user confirmation to cancel the setup and
        // factory reset the device.
        if (mController.getParams().startedByTrustedSource) {
            setResult(RESULT_CANCELED);
            finish();
        } else {
            new AlertDialog.Builder(this)
                    .setTitle(R.string.cancel_setup_and_factory_reset_dialog_title)
                    .setMessage(R.string.cancel_setup_and_factory_reset_dialog_msg)
                    .setNegativeButton(R.string.cancel_setup_and_factory_reset_dialog_cancel, null)
                    .setPositiveButton(R.string.cancel_setup_and_factory_reset_dialog_ok,
                            new AlertDialog.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int id) {
                                    mUtils.sendFactoryResetBroadcast(
                                            PreProvisioningActivity.this,
                                            "Device owner setup cancelled");
                                }
                            })
                    .show();
        }
    }

    @Override
    public void showDeleteManagedProfileDialog(ComponentName mdmPackageName, String domainName,
            int userId) {
        DeleteManagedProfileDialog.newInstance(userId, mdmPackageName, domainName)
                .show(getFragmentManager(), "DeleteManagedProfileDialogFragment");
    }

    /**
     * Callback for user agreeing to remove existing managed profile.
     */
    @Override
    public void onRemoveProfileApproval(int existingManagedProfileUserId) {
        UserManager userManager = (UserManager) getSystemService(Context.USER_SERVICE);
        userManager.removeUser(existingManagedProfileUserId);
    }

    /**
     * Callback for cancelled deletion of existing managed profile.
     */
    @Override
    public void onRemoveProfileCancel() {
        setResult(Activity.RESULT_CANCELED);
        finish();
    }

    /**
     * When the user backs out of creating a managed profile, show a dialog to double check.
     */
    @Override
    public void onBackPressed() {
        if (!mController.isProfileOwnerProvisioning()) {
            super.onBackPressed();
            return;
        }
        // TODO: Update strings for managed user case
        new AlertDialog.Builder(this)
                .setTitle(R.string.work_profile_setup_later_title)
                .setMessage(R.string.work_profile_setup_later_message)
                .setCancelable(false)
                .setPositiveButton(R.string.work_profile_setup_stop,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,int id) {
                                PreProvisioningActivity.this.setResult(
                                        Activity.RESULT_CANCELED);
                                PreProvisioningActivity.this.finish();
                            }
                        })
                .setNegativeButton(R.string.work_profile_setup_continue,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int id) {
                              // user chose to continue. Do nothing
                            }
                        })
                .show();
    }

    @Override
    public void onNavigateNext() {
        mController.afterNavigateNext();
    }
}
