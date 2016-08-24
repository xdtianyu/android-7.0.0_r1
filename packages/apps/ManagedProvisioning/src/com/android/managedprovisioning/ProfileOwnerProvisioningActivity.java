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

import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerFuture;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.support.v4.content.LocalBroadcastManager;
import android.view.View;
import android.widget.TextView;

import com.android.managedprovisioning.model.ProvisioningParams;

import java.io.IOException;
import java.util.concurrent.ExecutionException;

/**
 * Profile owner provisioning sets up a separate profile on a device whose primary user is already
 * set up or being set up.
 *
 * <p>
 * The typical example is setting up a corporate profile that is controlled by their employer on a
 * users personal device to keep personal and work data separate.
 *
 * <p>
 * The activity handles the UI for managed profile provisioning and starts the
 * {@link ProfileOwnerProvisioningService}, which runs through the setup steps in an
 * async task.
 */
public class ProfileOwnerProvisioningActivity extends SetupLayoutActivity {
    protected static final String ACTION_CANCEL_PROVISIONING =
            "com.android.managedprovisioning.CANCEL_PROVISIONING";

    private BroadcastReceiver mServiceMessageReceiver;

    private static final int BROADCAST_TIMEOUT = 2 * 60 * 1000;

    // Provisioning service started
    private static final int STATUS_PROVISIONING = 1;
    // Back button pressed during provisioning, confirm dialog showing.
    private static final int STATUS_CANCEL_CONFIRMING = 2;
    // Cancel confirmed, waiting for the provisioning service to complete.
    private static final int STATUS_CANCELLING = 3;
    // Cancelling not possible anymore, provisioning already finished successfully.
    private static final int STATUS_FINALIZING = 4;

    private static final String KEY_STATUS= "status";
    private static final String KEY_PENDING_INTENT = "pending_intent";

    private int mCancelStatus = STATUS_PROVISIONING;
    private Intent mPendingProvisioningResult = null;
    private ProgressDialog mCancelProgressDialog = null;
    private AccountManager mAccountManager;

    private ProvisioningParams mParams;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ProvisionLogger.logd("Profile owner provisioning activity ONCREATE");
        mAccountManager = (AccountManager) getSystemService(Context.ACCOUNT_SERVICE);

        if (savedInstanceState != null) {
            mCancelStatus = savedInstanceState.getInt(KEY_STATUS, STATUS_PROVISIONING);
            mPendingProvisioningResult = savedInstanceState.getParcelable(KEY_PENDING_INTENT);
        }

        initializeLayoutParams(R.layout.progress, R.string.setup_work_profile, true);
        configureNavigationButtons(NEXT_BUTTON_EMPTY_LABEL, View.INVISIBLE, View.VISIBLE);
        setTitle(R.string.setup_profile_progress);

        TextView textView = (TextView) findViewById(R.id.prog_text);
        if (textView != null) textView.setText(R.string.setting_up_workspace);

        if (mCancelStatus == STATUS_CANCEL_CONFIRMING) {
            showCancelProvisioningDialog();
        } else if (mCancelStatus == STATUS_CANCELLING) {
            showCancelProgressDialog();
        }
        mParams = (ProvisioningParams) getIntent().getParcelableExtra(
                ProvisioningParams.EXTRA_PROVISIONING_PARAMS);
        if (mParams != null) {
            maybeSetLogoAndMainColor(mParams.mainColor);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        // Setup broadcast receiver for feedback from service.
        mServiceMessageReceiver = new ServiceMessageReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(ProfileOwnerProvisioningService.ACTION_PROVISIONING_SUCCESS);
        filter.addAction(ProfileOwnerProvisioningService.ACTION_PROVISIONING_ERROR);
        filter.addAction(ProfileOwnerProvisioningService.ACTION_PROVISIONING_CANCELLED);
        LocalBroadcastManager.getInstance(this).registerReceiver(mServiceMessageReceiver, filter);

        // Start service async to make sure the UI is loaded first.
        final Handler handler = new Handler(getMainLooper());
        handler.post(new Runnable() {
            @Override
            public void run() {
                Intent intent = new Intent(ProfileOwnerProvisioningActivity.this,
                        ProfileOwnerProvisioningService.class);
                intent.putExtras(getIntent());
                startService(intent);
            }
        });
    }

    class ServiceMessageReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (mCancelStatus == STATUS_CANCEL_CONFIRMING) {
                // Store the incoming intent and only process it after the user has responded to
                // the cancel dialog
                mPendingProvisioningResult = intent;
                return;
            }
            handleProvisioningResult(intent);
        }
    }

    private void handleProvisioningResult(Intent intent) {
        String action = intent.getAction();
        if (ProfileOwnerProvisioningService.ACTION_PROVISIONING_SUCCESS.equals(action)) {
            if (mCancelStatus == STATUS_CANCELLING) {
                return;
            }

            ProvisionLogger.logd("Successfully provisioned."
                    + "Finishing ProfileOwnerProvisioningActivity");

            onProvisioningSuccess();
        } else if (ProfileOwnerProvisioningService.ACTION_PROVISIONING_ERROR.equals(action)) {
            if (mCancelStatus == STATUS_CANCELLING){
                return;
            }
            String errorLogMessage = intent.getStringExtra(
                    ProfileOwnerProvisioningService.EXTRA_LOG_MESSAGE_KEY);
            ProvisionLogger.logd("Error reported: " + errorLogMessage);
            error(R.string.managed_provisioning_error_text, errorLogMessage);
            // Note that this will be reported as a canceled action
            mCancelStatus = STATUS_FINALIZING;
        } else if (ProfileOwnerProvisioningService.ACTION_PROVISIONING_CANCELLED.equals(action)) {
            if (mCancelStatus != STATUS_CANCELLING) {
                return;
            }
            mCancelProgressDialog.dismiss();
            onProvisioningAborted();
        }
    }

    private void onProvisioningAborted() {
        stopService(new Intent(this, ProfileOwnerProvisioningService.class));
        setResult(Activity.RESULT_CANCELED);
        finish();
    }

    @Override
    public void onBackPressed() {
        if (mCancelStatus == STATUS_PROVISIONING) {
            mCancelStatus = STATUS_CANCEL_CONFIRMING;
            showCancelProvisioningDialog();
        } else {
            super.onBackPressed();
        }
    }

    private void showCancelProvisioningDialog() {
        AlertDialog alertDialog = new AlertDialog.Builder(this)
                .setCancelable(false)
                .setMessage(R.string.profile_owner_cancel_message)
                .setNegativeButton(R.string.profile_owner_cancel_cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,int id) {
                                mCancelStatus = STATUS_PROVISIONING;
                                if (mPendingProvisioningResult != null) {
                                    handleProvisioningResult(mPendingProvisioningResult);
                                }
                            }
                        })
                .setPositiveButton(R.string.profile_owner_cancel_ok,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,int id) {
                                confirmCancel();
                            }
                        })
                .create();
        alertDialog.show();
    }

    protected void showCancelProgressDialog() {
        mCancelProgressDialog = new ProgressDialog(this);
        mCancelProgressDialog.setMessage(getText(R.string.profile_owner_cancelling));
        mCancelProgressDialog.setCancelable(false);
        mCancelProgressDialog.setCanceledOnTouchOutside(false);
        mCancelProgressDialog.show();
    }

    public void error(int resourceId, String logText) {
        ProvisionLogger.loge(logText);
        new AlertDialog.Builder(this)
                .setTitle(R.string.provisioning_error_title)
                .setMessage(resourceId)
                .setCancelable(false)
                .setPositiveButton(R.string.device_owner_error_ok,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,int id) {
                                onProvisioningAborted();
                            }
                        })
                .show();
    }

    private void confirmCancel() {
        if (mCancelStatus != STATUS_CANCEL_CONFIRMING) {
            // Can only cancel if provisioning hasn't finished at this point.
            return;
        }
        mCancelStatus = STATUS_CANCELLING;
        Intent intent = new Intent(ProfileOwnerProvisioningActivity.this,
                ProfileOwnerProvisioningService.class);
        intent.setAction(ACTION_CANCEL_PROVISIONING);
        startService(intent);
        showCancelProgressDialog();
    }

    /**
     * Finish activity and stop service.
     */
    private void onProvisioningSuccess() {
        mBackButton.setVisibility(View.INVISIBLE);

        mCancelStatus = STATUS_FINALIZING;
        stopService(new Intent(this, ProfileOwnerProvisioningService.class));
        setResult(Activity.RESULT_OK);
        finish();
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        outState.putInt(KEY_STATUS, mCancelStatus);
        outState.putParcelable(KEY_PENDING_INTENT, mPendingProvisioningResult);
    }

    @Override
    public void onPause() {
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mServiceMessageReceiver);
        super.onPause();
    }
}
