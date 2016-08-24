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

import static android.app.admin.DeviceAdminReceiver.ACTION_PROFILE_PROVISIONING_COMPLETE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE;

import android.app.Activity;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.UserHandle;

import com.android.managedprovisioning.common.IllegalProvisioningArgumentException;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.parser.MessageParser;

/*
 * This class is used to make sure that we start the MDM after we shut the setup wizard down.
 * The shut down of the setup wizard is initiated in the DeviceOwnerProvisioningActivity or
 * ProfileOwnerProvisioningActivity by calling
 * {@link DevicePolicyManager.setUserProvisioningState()}. This will cause the
 * Setup wizard to shut down and send a ACTION_PROVISIONING_FINALIZATION intent. This intent is
 * caught by this receiver instead which will send the
 * ACTION_PROFILE_PROVISIONING_COMPLETE broadcast to the MDM, which can then present it's own
 * activities.
 */
public class FinalizationActivity extends Activity {

    private ProvisioningParams mParams;

    private static final String INTENT_STORE_NAME = "finalization-receiver";

    private final Utils mUtils = new Utils();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        DevicePolicyManager dpm = getSystemService(DevicePolicyManager.class);
        int currentState = dpm.getUserProvisioningState();

        switch (currentState) {
            case DevicePolicyManager.STATE_USER_SETUP_INCOMPLETE:
            case DevicePolicyManager.STATE_USER_SETUP_COMPLETE:
            case DevicePolicyManager.STATE_USER_PROFILE_COMPLETE:
                finalizeProvisioning(dpm);
                break;
            case DevicePolicyManager.STATE_USER_UNMANAGED:
            case DevicePolicyManager.STATE_USER_SETUP_FINALIZED:
                // Nothing to do in these cases.
                ProvisionLogger.logw("Received ACTION_PROVISIONING_FINALIZATION intent, but "
                        + "nothing to do in state: " + currentState);
                break;
        }

        finish();
    }

    public static void storeProvisioningParams(Context context, ProvisioningParams params) {
        Intent intent = new MessageParser().getIntentFromProvisioningParams(params);
        getIntentStore(context).save(intent);
    }

    private void finalizeProvisioning(DevicePolicyManager dpm) {
        mParams = loadProvisioningParamsAndClearIntentStore();
        Intent provisioningCompleteIntent = getProvisioningCompleteIntent(dpm);
        if (provisioningCompleteIntent == null) {
            return;
        }

        // It maybe the case that mParams is null at this point - we expect this to be the case if
        // ManagedProvisioning wasn't invoked to perform setup. We'll simply trigger the normal
        // broadcast so that the installed DPC knows that user-setup completed. Concrete use-case
        // is a user being setup through DevicePolicyManager.createAndInitializeUser() by the
        // device-owner, which sets profile-owner and then launches the user. Setup-wizard on the
        // user runs, and at the end of it's activity flow will trigger the finalization intent,
        // which then allows us to notify the DPC that profile setup is complete.
        if (mParams != null &&
                mParams.provisioningAction.equals(ACTION_PROVISION_MANAGED_PROFILE)) {
            // For the managed profile owner case, we need to send the provisioning complete
            // intent to the mdm. Once it has been received, we'll send
            // ACTION_MANAGED_PROFILE_PROVISIONED in the parent.
            finalizeManagedProfileOwnerProvisioning(provisioningCompleteIntent);
        } else {
            // For managed user and device owner, we just need to send the provisioning complete
            // intent to the mdm.
            sendBroadcast(provisioningCompleteIntent);
        }

        mUtils.markUserProvisioningStateFinalized(this, mParams);
    }

    private void finalizeManagedProfileOwnerProvisioning(Intent provisioningCompleteIntent) {
        UserHandle managedUserHandle = mUtils.getManagedProfile(this);
        if (managedUserHandle == null) {
            ProvisionLogger.loge("Failed to retrieve the userHandle of the managed profile.");
            return;
        }
        BroadcastReceiver mdmReceivedSuccessReceiver = new MdmReceivedSuccessReceiver(
                mParams.accountToMigrate, mParams.deviceAdminComponentName.getPackageName());

        sendOrderedBroadcastAsUser(provisioningCompleteIntent, managedUserHandle, null,
                mdmReceivedSuccessReceiver, null, Activity.RESULT_OK, null, null);
    }

    private Intent getProvisioningCompleteIntent(DevicePolicyManager dpm) {
        Intent intent = new Intent(ACTION_PROFILE_PROVISIONING_COMPLETE);
        try {
            // mParams maybe null for cases where DevicePolicyManager has directly set DO or PO and
            // ManagedProvisioning wasn't involved. In that case, we may still want to use the same
            // mechanism after setup-wizard to invoke the MDM, hence why we fallback to inspecting
            // device and profile-owner.
            if (mParams != null) {
                intent.setComponent(mParams.inferDeviceAdminComponentName(this));
            } else if (dpm.getDeviceOwner() != null) {
                intent.setComponent(mUtils.findDeviceAdmin(dpm.getDeviceOwner(),
                        null /* mdmComponentName */, this));
            } else if (dpm.getProfileOwner() != null) {
                intent.setComponent(mUtils.findDeviceAdmin(dpm.getProfileOwner().getPackageName(),
                        null /* mdmComponentName */, this));
            } else {
                return null;
            }
        } catch (IllegalProvisioningArgumentException e) {
            ProvisionLogger.loge("Failed to infer the device admin component name", e);
            return null;
        }
        intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES | Intent.FLAG_RECEIVER_FOREGROUND);
        if (mParams != null && mParams.adminExtrasBundle != null) {
            intent.putExtra(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE, mParams.adminExtrasBundle);
        }
        return intent;
    }

    private ProvisioningParams loadProvisioningParamsAndClearIntentStore() {
        IntentStore intentStore = getIntentStore(this);
        Intent intent = intentStore.load();
        if (intent == null) {
            ProvisionLogger.loge("Fail to retrieve ProvisioningParams from intent store.");
            return null;
        }
        intentStore.clear();

        try {
            return new MessageParser().parse(intent, this);
        } catch (IllegalProvisioningArgumentException e) {
            ProvisionLogger.loge("Failed to parse provisioning intent", e);
        }
        return null;
    }

    private static IntentStore getIntentStore(Context context) {
        return new IntentStore(context, INTENT_STORE_NAME);
    }
}
