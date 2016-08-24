/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */
package com.android.phone.vvm.omtp;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.UserHandle;
import android.os.UserManager;
import android.telecom.PhoneAccountHandle;
import android.telephony.CarrierConfigManager;
import android.telephony.SubscriptionManager;
import android.util.Log;

import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.TelephonyIntents;
import com.android.phone.PhoneUtils;
import com.android.phone.R;
import com.android.phone.settings.VisualVoicemailSettingsUtil;
import com.android.phone.vvm.omtp.sync.OmtpVvmSourceManager;

/**
 * This class listens to the {@link CarrierConfigManager#ACTION_CARRIER_CONFIG_CHANGED} and {@link
 * TelephonyIntents#ACTION_SIM_STATE_CHANGED} to determine when a SIM is added, replaced, or
 * removed.
 *
 * When a SIM is added, send an activate SMS. When a SIM is removed, remove the sync accounts and
 * change the status in the voicemail_status table.
 */
public class SimChangeReceiver extends BroadcastReceiver {

    private static final String TAG = "SimChangeReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (UserHandle.myUserId() != UserHandle.USER_SYSTEM) {
            Log.v(TAG, "Received broadcast for user that is not system.");
            return;
        }

        final String action = intent.getAction();
        if (action == null) {
            Log.w(TAG, "Null action for intent.");
            return;
        }

        switch (action) {
            case TelephonyIntents.ACTION_SIM_STATE_CHANGED:
                if (IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(
                        intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE))) {
                    Log.i(TAG, "Sim removed, removing inactive accounts");
                    OmtpVvmSourceManager.getInstance(context).removeInactiveSources();
                }
                break;
            case CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED:
                int subId = intent.getIntExtra(PhoneConstants.SUBSCRIPTION_KEY,
                        SubscriptionManager.INVALID_SUBSCRIPTION_ID);

                if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
                    Log.i(TAG, "Received SIM change for invalid subscription id.");
                    return;
                }

                if (!UserManager.get(context).isUserUnlocked()) {
                    OmtpBootCompletedReceiver.addDeferredSubId(context, subId);
                } else {
                    processSubId(context, subId);
                }
                break;
        }
    }

    public static void processSubId(Context context, int subId) {
        OmtpVvmCarrierConfigHelper carrierConfigHelper =
                new OmtpVvmCarrierConfigHelper(context, subId);
        if (carrierConfigHelper.isOmtpVvmType()) {
            PhoneAccountHandle phoneAccount = PhoneUtils.makePstnPhoneAccountHandle(
                    SubscriptionManager.getPhoneId(subId));

            boolean isUserSet = VisualVoicemailSettingsUtil.isVisualVoicemailUserSet(
                    context, phoneAccount);
            boolean isEnabledInSettings =
                    VisualVoicemailSettingsUtil.isVisualVoicemailEnabled(context,
                            phoneAccount);
            boolean isSupported =
                    context.getResources().getBoolean(R.bool.allow_visual_voicemail);
            boolean isEnabled = isSupported && (isUserSet ? isEnabledInSettings :
                    carrierConfigHelper.isEnabledByDefault());

            if (!isUserSet) {
                // Preserve the previous setting for "isVisualVoicemailEnabled" if it is
                // set by the user, otherwise, set this value for the first time.
                VisualVoicemailSettingsUtil.setVisualVoicemailEnabled(context, phoneAccount,
                        isEnabled, /** isUserSet */false);
            }

            if (isEnabled) {
                LocalLogHelper.log(TAG, "Sim state or carrier config changed: requesting"
                        + " activation for " + phoneAccount.getId());

                // Add a phone state listener so that changes to the communication channels
                // can be recorded.
                OmtpVvmSourceManager.getInstance(context).addPhoneStateListener(
                        phoneAccount);
                carrierConfigHelper.startActivation();
            } else {
                // It may be that the source was not registered to begin with but we want
                // to run through the steps to remove the source just in case.
                OmtpVvmSourceManager.getInstance(context).removeSource(phoneAccount);
                Log.v(TAG, "Sim change for disabled account.");
            }
        }
    }
}