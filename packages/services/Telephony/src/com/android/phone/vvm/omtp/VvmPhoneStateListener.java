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

import android.content.Context;
import android.content.Intent;
import android.provider.VoicemailContract;
import android.telecom.PhoneAccountHandle;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.util.Log;

import com.android.phone.PhoneGlobals;
import com.android.phone.PhoneUtils;
import com.android.phone.vvm.omtp.sync.OmtpVvmSourceManager;
import com.android.phone.vvm.omtp.sync.OmtpVvmSyncService;
import com.android.phone.vvm.omtp.sync.VoicemailStatusQueryHelper;

/**
 * Check if service is lost and indicate this in the voicemail status.
 */
public class VvmPhoneStateListener extends PhoneStateListener {
    private static final String TAG = "VvmPhoneStateListener";

    private PhoneAccountHandle mPhoneAccount;
    private Context mContext;
    private int mPreviousState = -1;

    public VvmPhoneStateListener(Context context, PhoneAccountHandle accountHandle) {
        super(PhoneUtils.getSubIdForPhoneAccountHandle(accountHandle));
        mContext = context;
        mPhoneAccount = accountHandle;
    }

    @Override
    public void onServiceStateChanged(ServiceState serviceState) {
        int state = serviceState.getState();
        if (state == mPreviousState || (state != ServiceState.STATE_IN_SERVICE
                && mPreviousState != ServiceState.STATE_IN_SERVICE)) {
            // Only interested in state changes or transitioning into or out of "in service".
            // Otherwise just quit.
            mPreviousState = state;
            return;
        }

        if (state == ServiceState.STATE_IN_SERVICE) {
            VoicemailStatusQueryHelper voicemailStatusQueryHelper =
                    new VoicemailStatusQueryHelper(mContext);
            if (voicemailStatusQueryHelper.isVoicemailSourceConfigured(mPhoneAccount)) {
                if (!voicemailStatusQueryHelper.isNotificationsChannelActive(mPhoneAccount)) {
                    Log.v(TAG, "Notifications channel is active for " + mPhoneAccount.getId());
                    VoicemailContract.Status.setStatus(mContext, mPhoneAccount,
                            VoicemailContract.Status.CONFIGURATION_STATE_OK,
                            VoicemailContract.Status.DATA_CHANNEL_STATE_OK,
                            VoicemailContract.Status.NOTIFICATION_CHANNEL_STATE_OK);
                    PhoneGlobals.getInstance().clearMwiIndicator(
                            PhoneUtils.getSubIdForPhoneAccountHandle(mPhoneAccount));
                }
            }

            if (OmtpVvmSourceManager.getInstance(mContext).isVvmSourceRegistered(mPhoneAccount)) {
                Log.v(TAG, "Signal returned: requesting resync for " + mPhoneAccount.getId());
                LocalLogHelper.log(TAG,
                        "Signal returned: requesting resync for " + mPhoneAccount.getId());
                // If the source is already registered, run a full sync in case something was missed
                // while signal was down.
                Intent serviceIntent = OmtpVvmSyncService.getSyncIntent(
                        mContext, OmtpVvmSyncService.SYNC_FULL_SYNC, mPhoneAccount,
                        true /* firstAttempt */);
                mContext.startService(serviceIntent);
            } else {
                Log.v(TAG, "Signal returned: reattempting activation for " + mPhoneAccount.getId());
                LocalLogHelper.log(TAG,
                        "Signal returned: reattempting activation for " + mPhoneAccount.getId());
                // Otherwise initiate an activation because this means that an OMTP source was
                // recognized but either the activation text was not successfully sent or a response
                // was not received.
                OmtpVvmCarrierConfigHelper carrierConfigHelper = new OmtpVvmCarrierConfigHelper(
                        mContext, PhoneUtils.getSubIdForPhoneAccountHandle(mPhoneAccount));
                carrierConfigHelper.startActivation();
            }
        } else {
            Log.v(TAG, "Notifications channel is inactive for " + mPhoneAccount.getId());
            mContext.stopService(OmtpVvmSyncService.getSyncIntent(
                    mContext, OmtpVvmSyncService.SYNC_FULL_SYNC, mPhoneAccount,
                    true /* firstAttempt */));

            if (!OmtpVvmSourceManager.getInstance(mContext).isVvmSourceRegistered(mPhoneAccount)) {
                return;
            }

            VoicemailContract.Status.setStatus(mContext, mPhoneAccount,
                    VoicemailContract.Status.CONFIGURATION_STATE_OK,
                    VoicemailContract.Status.DATA_CHANNEL_STATE_NO_CONNECTION,
                    VoicemailContract.Status.NOTIFICATION_CHANNEL_STATE_NO_CONNECTION);
        }
        mPreviousState = state;
    }
}
