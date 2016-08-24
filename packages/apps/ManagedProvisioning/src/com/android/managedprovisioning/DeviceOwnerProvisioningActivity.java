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

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.UserHandle;
import android.support.v4.content.LocalBroadcastManager;
import android.view.accessibility.AccessibilityEvent;
import android.view.View;
import android.widget.TextView;

import com.android.managedprovisioning.model.ProvisioningParams;

import java.util.ArrayList;

/**
 * This activity starts device owner provisioning:
 * It downloads a mobile device management application(mdm) from a given url and installs it,
 * or a given mdm is already present on the device. The mdm is set as the owner of the device so
 * that it has full control over the device:
 * TODO: put link here with documentation on how a device owner has control over the device
 * The mdm can then execute further setup steps.
 *
 * <p>
 * An example use case might be when a company wants to set up a device for a single use case
 * (such as giving instructions).
 * </p>
 *
 * <p>
 * Provisioning is triggered by a programmer device that sends required provisioning parameters via
 * nfc. For an example of a programmer app see:
 * com.example.android.apis.app.DeviceProvisioningProgrammerSample.
 * </p>
 *
 * <p>
 * In the unlikely case that this activity is killed the whole provisioning process so far is
 * repeated. We made sure that all tasks can be done twice without causing any problems.
 * </p>
 */
public class DeviceOwnerProvisioningActivity extends SetupLayoutActivity {
    private static final boolean DEBUG = false; // To control logging.

    private static final String KEY_CANCEL_DIALOG_SHOWN = "cancel_dialog_shown";
    private static final String KEY_PENDING_INTENTS = "pending_intents";

    private BroadcastReceiver mServiceMessageReceiver;
    private TextView mProgressTextView;

    private ProvisioningParams mParams;

    // Indicates that the cancel dialog is shown.
    private boolean mCancelDialogShown = false;

    // List of intents received while cancel dialog is shown.
    private ArrayList<Intent> mPendingProvisioningIntents = new ArrayList<Intent>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (DEBUG) ProvisionLogger.logd("Device owner provisioning activity ONCREATE");

        if (savedInstanceState != null) {
            mCancelDialogShown = savedInstanceState.getBoolean(KEY_CANCEL_DIALOG_SHOWN, false);
            mPendingProvisioningIntents = savedInstanceState
                    .getParcelableArrayList(KEY_PENDING_INTENTS);
        }

        // Setup the UI.
        initializeLayoutParams(R.layout.progress, R.string.setup_work_device, true);
        configureNavigationButtons(NEXT_BUTTON_EMPTY_LABEL, View.INVISIBLE, View.VISIBLE);
        setTitle(R.string.setup_device_progress);

        mProgressTextView = (TextView) findViewById(R.id.prog_text);
        if (mCancelDialogShown) showCancelResetDialog();

        // Setup broadcast receiver for feedback from service.
        mServiceMessageReceiver = new ServiceMessageReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(DeviceOwnerProvisioningService.ACTION_PROVISIONING_SUCCESS);
        filter.addAction(DeviceOwnerProvisioningService.ACTION_PROVISIONING_ERROR);
        filter.addAction(DeviceOwnerProvisioningService.ACTION_PROGRESS_UPDATE);
        LocalBroadcastManager.getInstance(this).registerReceiver(mServiceMessageReceiver, filter);

        // Load the ProvisioningParams (from message in Intent).
        mParams = (ProvisioningParams) getIntent().getParcelableExtra(
                ProvisioningParams.EXTRA_PROVISIONING_PARAMS);
        if (mParams != null) {
            maybeSetLogoAndMainColor(mParams.mainColor);
        }
        startDeviceOwnerProvisioningService();
    }

    private void startDeviceOwnerProvisioningService() {
        Intent intent = new Intent(this, DeviceOwnerProvisioningService.class);
        intent.putExtras(getIntent());
        startService(intent);
    }

    class ServiceMessageReceiver extends BroadcastReceiver
    {
        @Override
        public void onReceive(Context context, Intent intent)
        {
            if (mCancelDialogShown) {

                // Postpone handling the intent.
                mPendingProvisioningIntents.add(intent);
                return;
            }
            handleProvisioningIntent(intent);
        }
    }

    private void handleProvisioningIntent(Intent intent) {
        String action = intent.getAction();
        if (action.equals(DeviceOwnerProvisioningService.ACTION_PROVISIONING_SUCCESS)) {
            if (DEBUG) ProvisionLogger.logd("Successfully provisioned");
            onProvisioningSuccess();
        } else if (action.equals(DeviceOwnerProvisioningService.ACTION_PROVISIONING_ERROR)) {
            int errorMessageId = intent.getIntExtra(
                    DeviceOwnerProvisioningService.EXTRA_USER_VISIBLE_ERROR_ID_KEY,
                    R.string.device_owner_error_general);
            boolean factoryResetRequired = intent.getBooleanExtra(
                    DeviceOwnerProvisioningService.EXTRA_FACTORY_RESET_REQUIRED,
                    true);

            if (DEBUG) {
                ProvisionLogger.logd("Error reported with code "
                        + getResources().getString(errorMessageId));
            }
            error(errorMessageId, factoryResetRequired);
        } else if (action.equals(DeviceOwnerProvisioningService.ACTION_PROGRESS_UPDATE)) {
            int progressMessage = intent.getIntExtra(
                    DeviceOwnerProvisioningService.EXTRA_PROGRESS_MESSAGE_ID_KEY, -1);
            if (DEBUG) {
                ProvisionLogger.logd("Progress update reported with code "
                    + getResources().getString(progressMessage));
            }
            if (progressMessage >= 0) {
                progressUpdate(progressMessage);
            }
        }
    }


    private void onProvisioningSuccess() {
        stopService(new Intent(this, DeviceOwnerProvisioningService.class));
        // Note: the DeviceOwnerProvisioningService will stop itself.
        setResult(Activity.RESULT_OK);
        finish();
    }

    @Override
    public void onBackPressed() {
        if (mCancelDialogShown) {
            return;
        }

        mCancelDialogShown = true;
        showCancelResetDialog();
    }

    private void showCancelResetDialog() {
        new AlertDialog.Builder(DeviceOwnerProvisioningActivity.this)
                .setCancelable(false)
                .setMessage(R.string.device_owner_cancel_message)
                .setNegativeButton(R.string.device_owner_cancel_cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int id) {
                                dialog.dismiss();
                                handlePendingIntents();
                                mCancelDialogShown = false;
                            }
                        })
                .setPositiveButton(R.string.device_owner_error_reset,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int id) {
                                dialog.dismiss();

                                // Factory reset the device.
                                mUtils.sendFactoryResetBroadcast(
                                        DeviceOwnerProvisioningActivity.this,
                                        "DeviceOwnerProvisioningActivity.showCancelResetDialog()");
                            }
                        })
                .show();
    }

    private void handlePendingIntents() {
        for (Intent intent : mPendingProvisioningIntents) {
            if (DEBUG) ProvisionLogger.logd("Handling pending intent " + intent.getAction());
            handleProvisioningIntent(intent);
        }
        mPendingProvisioningIntents.clear();
    }

    private void progressUpdate(int progressMessage) {
        mProgressTextView.setText(progressMessage);
        mProgressTextView.sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_FOCUSED);
    }

    private void error(int dialogMessage, boolean resetRequired) {
        AlertDialog.Builder alertBuilder = new AlertDialog.Builder(this)
                .setTitle(R.string.provisioning_error_title)
                .setMessage(dialogMessage)
                .setCancelable(false);
        if (resetRequired) {
            alertBuilder.setPositiveButton(R.string.device_owner_error_reset,
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog,int id) {
                            dialog.dismiss();

                            // Factory reset the device.
                            Intent intent = new Intent(Intent.ACTION_MASTER_CLEAR);
                            intent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);
                            intent.putExtra(Intent.EXTRA_REASON,
                                    "DeviceOwnerProvisioningActivity.error()");
                            sendBroadcast(intent);
                            stopService(new Intent(DeviceOwnerProvisioningActivity.this,
                                            DeviceOwnerProvisioningService.class));
                            setResult(RESULT_CANCELED);
                            finish();
                        }
                    });
        } else {
            alertBuilder.setPositiveButton(R.string.device_owner_error_ok,
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog,int id) {
                            dialog.dismiss();

                            // Close activity.
                            stopService(new Intent(DeviceOwnerProvisioningActivity.this,
                                            DeviceOwnerProvisioningService.class));
                            setResult(RESULT_CANCELED);
                            finish();
                        }
                    });
        }
        alertBuilder.show();
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        outState.putBoolean(KEY_CANCEL_DIALOG_SHOWN, mCancelDialogShown);
        outState.putParcelableArrayList(KEY_PENDING_INTENTS, mPendingProvisioningIntents);
    }

    @Override
    public void onDestroy() {
        if (DEBUG) ProvisionLogger.logd("Device owner provisioning activity ONDESTROY");
        if (mServiceMessageReceiver != null) {
            LocalBroadcastManager.getInstance(this).unregisterReceiver(mServiceMessageReceiver);
            mServiceMessageReceiver = null;
        }
        super.onDestroy();
    }

    @Override
    protected void onRestart() {
        if (DEBUG) ProvisionLogger.logd("Device owner provisioning activity ONRESTART");
        super.onRestart();
    }

    @Override
    protected void onResume() {
        if (DEBUG) ProvisionLogger.logd("Device owner provisioning activity ONRESUME");
        super.onResume();
    }

    @Override
    protected void onPause() {
        if (DEBUG) ProvisionLogger.logd("Device owner provisioning activity ONPAUSE");
        super.onPause();
    }

    @Override
    protected void onStop() {
        if (DEBUG) ProvisionLogger.logd("Device owner provisioning activity ONSTOP");
        super.onStop();
    }
}
