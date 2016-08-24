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
package com.android.phone.vvm.omtp.sms;

import android.content.BroadcastReceiver;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.UserManager;
import android.provider.Telephony;
import android.provider.VoicemailContract;
import android.telecom.PhoneAccountHandle;
import android.telecom.Voicemail;
import android.telephony.SmsMessage;
import android.util.Log;

import com.android.internal.telephony.PhoneConstants;
import com.android.phone.PhoneGlobals;
import com.android.phone.PhoneUtils;
import com.android.phone.settings.VisualVoicemailSettingsUtil;
import com.android.phone.vvm.omtp.LocalLogHelper;
import com.android.phone.vvm.omtp.OmtpConstants;
import com.android.phone.vvm.omtp.sync.OmtpVvmSourceManager;
import com.android.phone.vvm.omtp.sync.OmtpVvmSyncService;
import com.android.phone.vvm.omtp.sync.VoicemailsQueryHelper;

/**
 * Receive SMS messages and send for processing by the OMTP visual voicemail source.
 */
public class OmtpMessageReceiver extends BroadcastReceiver {
    private static final String TAG = "OmtpMessageReceiver";

    private Context mContext;
    private PhoneAccountHandle mPhoneAccount;

    @Override
    public void onReceive(Context context, Intent intent) {
        if (!UserManager.get(context).isUserUnlocked()) {
            Log.i(TAG, "Received message on locked device");
            // A full sync will happen after the device is unlocked, so nothing need to be done.
            return;
        }

        mContext = context;
        mPhoneAccount = PhoneUtils.makePstnPhoneAccountHandle(
                intent.getExtras().getInt(PhoneConstants.PHONE_KEY));

        if (mPhoneAccount == null) {
            Log.w(TAG, "Received message for null phone account");
            return;
        }

        if (!VisualVoicemailSettingsUtil.isVisualVoicemailEnabled(mContext, mPhoneAccount)) {
            Log.v(TAG, "Received vvm message for disabled vvm source.");
            return;
        }

        SmsMessage[] messages = Telephony.Sms.Intents.getMessagesFromIntent(intent);

        if (messages == null) {
            Log.w(TAG, "Message does not exist in the intent.");
            return;
        }

        StringBuilder messageBody = new StringBuilder();

        for (int i = 0; i < messages.length; i++) {
            if (messages[i].mWrappedSmsMessage != null) {
                messageBody.append(messages[i].getMessageBody());
            }
        }

        WrappedMessageData messageData = OmtpSmsParser.parse(messageBody.toString());
        if (messageData != null) {
            if (messageData.getPrefix() == OmtpConstants.SYNC_SMS_PREFIX) {
                SyncMessage message = new SyncMessage(messageData);

                Log.v(TAG, "Received SYNC sms for " + mPhoneAccount.getId() +
                        " with event " + message.getSyncTriggerEvent());
                LocalLogHelper.log(TAG, "Received SYNC sms for " + mPhoneAccount.getId() +
                        " with event " + message.getSyncTriggerEvent());
                processSync(message);
            } else if (messageData.getPrefix() == OmtpConstants.STATUS_SMS_PREFIX) {
                Log.v(TAG, "Received STATUS sms for " + mPhoneAccount.getId());
                LocalLogHelper.log(TAG, "Received Status sms for " + mPhoneAccount.getId());
                StatusMessage message = new StatusMessage(messageData);
                updateSource(message);
            } else {
                Log.e(TAG, "This should never have happened");
            }
        }
        // Let this fall through: this is not a message we're interested in.
    }

    /**
     * A sync message has two purposes: to signal a new voicemail message, and to indicate the
     * voicemails on the server have changed remotely (usually through the TUI). Save the new
     * message to the voicemail provider if it is the former case and perform a full sync in the
     * latter case.
     *
     * @param message The sync message to extract data from.
     */
    private void processSync(SyncMessage message) {
        Intent serviceIntent = null;
        switch (message.getSyncTriggerEvent()) {
            case OmtpConstants.NEW_MESSAGE:
                Voicemail.Builder builder = Voicemail.createForInsertion(
                        message.getTimestampMillis(), message.getSender())
                        .setPhoneAccount(mPhoneAccount)
                        .setSourceData(message.getId())
                        .setDuration(message.getLength())
                        .setSourcePackage(mContext.getPackageName());
                Voicemail voicemail = builder.build();

                VoicemailsQueryHelper queryHelper = new VoicemailsQueryHelper(mContext);
                if (queryHelper.isVoicemailUnique(voicemail)) {
                    Uri uri = VoicemailContract.Voicemails.insert(mContext, voicemail);
                    voicemail = builder.setId(ContentUris.parseId(uri)).setUri(uri).build();
                    serviceIntent = OmtpVvmSyncService.getSyncIntent(mContext,
                            OmtpVvmSyncService.SYNC_DOWNLOAD_ONE_TRANSCRIPTION, mPhoneAccount,
                            voicemail, true /* firstAttempt */);
                }
                break;
            case OmtpConstants.MAILBOX_UPDATE:
                serviceIntent = OmtpVvmSyncService.getSyncIntent(
                        mContext, OmtpVvmSyncService.SYNC_DOWNLOAD_ONLY, mPhoneAccount,
                        true /* firstAttempt */);
                break;
            case OmtpConstants.GREETINGS_UPDATE:
                // Not implemented in V1
                break;
            default:
               Log.e(TAG, "Unrecognized sync trigger event: " + message.getSyncTriggerEvent());
               break;
        }

        if (serviceIntent != null) {
            mContext.startService(serviceIntent);
        }
    }

    private void updateSource(StatusMessage message) {
        OmtpVvmSourceManager vvmSourceManager =
                OmtpVvmSourceManager.getInstance(mContext);

        if (OmtpConstants.SUCCESS.equals(message.getReturnCode())) {
            VoicemailContract.Status.setStatus(mContext, mPhoneAccount,
                    VoicemailContract.Status.CONFIGURATION_STATE_OK,
                    VoicemailContract.Status.DATA_CHANNEL_STATE_OK,
                    VoicemailContract.Status.NOTIFICATION_CHANNEL_STATE_OK);

            // Save the IMAP credentials in preferences so they are persistent and can be retrieved.
            VisualVoicemailSettingsUtil.setVisualVoicemailCredentialsFromStatusMessage(
                    mContext,
                    mPhoneAccount,
                    message);

            // Add the source to indicate that it is active.
            vvmSourceManager.addSource(mPhoneAccount);

            Intent serviceIntent = OmtpVvmSyncService.getSyncIntent(
                    mContext, OmtpVvmSyncService.SYNC_FULL_SYNC, mPhoneAccount,
                    true /* firstAttempt */);
            mContext.startService(serviceIntent);

            PhoneGlobals.getInstance().clearMwiIndicator(
                    PhoneUtils.getSubIdForPhoneAccountHandle(mPhoneAccount));
        } else {
            Log.w(TAG, "Visual voicemail not available for subscriber.");
            // Override default isEnabled setting to false since visual voicemail is unable to
            // be accessed for some reason.
            VisualVoicemailSettingsUtil.setVisualVoicemailEnabled(mContext, mPhoneAccount,
                    /* isEnabled */ false, /* isUserSet */ true);
        }
    }
}
