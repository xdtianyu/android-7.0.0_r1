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
package com.android.phone.vvm.omtp.sync;

import android.app.AlarmManager;
import android.app.IntentService;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.Network;
import android.net.NetworkInfo;
import android.net.Uri;
import android.provider.VoicemailContract;
import android.provider.VoicemailContract.Status;
import android.telecom.PhoneAccountHandle;
import android.telecom.Voicemail;
import android.text.TextUtils;
import android.util.Log;

import com.android.phone.PhoneUtils;
import com.android.phone.VoicemailUtils;
import com.android.phone.settings.VisualVoicemailSettingsUtil;
import com.android.phone.vvm.omtp.LocalLogHelper;
import com.android.phone.vvm.omtp.OmtpVvmCarrierConfigHelper;
import com.android.phone.vvm.omtp.fetch.VoicemailFetchedCallback;
import com.android.phone.vvm.omtp.imap.ImapHelper;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Sync OMTP visual voicemail.
 */
public class OmtpVvmSyncService extends IntentService {

    private static final String TAG = OmtpVvmSyncService.class.getSimpleName();

    // Number of retries
    private static final int NETWORK_RETRY_COUNT = 3;

    /**
     * Signifies a sync with both uploading to the server and downloading from the server.
     */
    public static final String SYNC_FULL_SYNC = "full_sync";
    /**
     * Only upload to the server.
     */
    public static final String SYNC_UPLOAD_ONLY = "upload_only";
    /**
     * Only download from the server.
     */
    public static final String SYNC_DOWNLOAD_ONLY = "download_only";
    /**
     * Only download single voicemail transcription.
     */
    public static final String SYNC_DOWNLOAD_ONE_TRANSCRIPTION =
            "download_one_transcription";
    /**
     * The account to sync.
     */
    public static final String EXTRA_PHONE_ACCOUNT = "phone_account";
    /**
     * The voicemail to fetch.
     */
    public static final String EXTRA_VOICEMAIL = "voicemail";
    /**
     * The sync request is initiated by the user, should allow shorter sync interval.
     */
    public static final String EXTRA_IS_MANUAL_SYNC = "is_manual_sync";
    // Minimum time allowed between full syncs
    private static final int MINIMUM_FULL_SYNC_INTERVAL_MILLIS = 60 * 1000;

    // Minimum time allowed between manual syncs
    private static final int MINIMUM_MANUAL_SYNC_INTERVAL_MILLIS = 3 * 1000;

    private VoicemailsQueryHelper mQueryHelper;

    public OmtpVvmSyncService() {
        super("OmtpVvmSyncService");
    }

    public static Intent getSyncIntent(Context context, String action,
            PhoneAccountHandle phoneAccount, boolean firstAttempt) {
        return getSyncIntent(context, action, phoneAccount, null, firstAttempt);
    }

    public static Intent getSyncIntent(Context context, String action,
            PhoneAccountHandle phoneAccount, Voicemail voicemail, boolean firstAttempt) {
        if (firstAttempt) {
            if (phoneAccount != null) {
                VisualVoicemailSettingsUtil.resetVisualVoicemailRetryInterval(context,
                        phoneAccount);
            } else {
                OmtpVvmSourceManager vvmSourceManager =
                        OmtpVvmSourceManager.getInstance(context);
                Set<PhoneAccountHandle> sources = vvmSourceManager.getOmtpVvmSources();
                for (PhoneAccountHandle source : sources) {
                    VisualVoicemailSettingsUtil.resetVisualVoicemailRetryInterval(context, source);
                }
            }
        }

        Intent serviceIntent = new Intent(context, OmtpVvmSyncService.class);
        serviceIntent.setAction(action);
        if (phoneAccount != null) {
            serviceIntent.putExtra(EXTRA_PHONE_ACCOUNT, phoneAccount);
        }
        if (voicemail != null) {
            serviceIntent.putExtra(EXTRA_VOICEMAIL, voicemail);
        }

        cancelRetriesForIntent(context, serviceIntent);
        return serviceIntent;
    }

    /**
     * Cancel all retry syncs for an account.
     *
     * @param context The context the service runs in.
     * @param phoneAccount The phone account for which to cancel syncs.
     */
    public static void cancelAllRetries(Context context, PhoneAccountHandle phoneAccount) {
        cancelRetriesForIntent(context, getSyncIntent(context, SYNC_FULL_SYNC, phoneAccount,
                false));
    }

    /**
     * A helper method to cancel all pending alarms for intents that would be identical to the given
     * intent.
     *
     * @param context The context the service runs in.
     * @param intent The intent to search and cancel.
     */
    private static void cancelRetriesForIntent(Context context, Intent intent) {
        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        alarmManager.cancel(PendingIntent.getService(context, 0, intent, 0));

        Intent copyIntent = new Intent(intent);
        if (SYNC_FULL_SYNC.equals(copyIntent.getAction())) {
            // A full sync action should also cancel both of the other types of syncs
            copyIntent.setAction(SYNC_DOWNLOAD_ONLY);
            alarmManager.cancel(PendingIntent.getService(context, 0, copyIntent, 0));
            copyIntent.setAction(SYNC_UPLOAD_ONLY);
            alarmManager.cancel(PendingIntent.getService(context, 0, copyIntent, 0));
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mQueryHelper = new VoicemailsQueryHelper(this);
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        if (intent == null) {
            Log.d(TAG, "onHandleIntent: could not handle null intent");
            return;
        }
        String action = intent.getAction();
        PhoneAccountHandle phoneAccount = intent.getParcelableExtra(EXTRA_PHONE_ACCOUNT);
        LocalLogHelper.log(TAG, "Sync requested: " + action +
                " for all accounts: " + String.valueOf(phoneAccount == null));

        boolean isManualSync = intent.getBooleanExtra(EXTRA_IS_MANUAL_SYNC, false);
        Voicemail voicemail = intent.getParcelableExtra(EXTRA_VOICEMAIL);
        if (phoneAccount != null) {
            Log.v(TAG, "Sync requested: " + action + " - for account: " + phoneAccount);
            setupAndSendRequest(phoneAccount, voicemail, action, isManualSync);
        } else {
            Log.v(TAG, "Sync requested: " + action + " - for all accounts");
            OmtpVvmSourceManager vvmSourceManager =
                    OmtpVvmSourceManager.getInstance(this);
            Set<PhoneAccountHandle> sources = vvmSourceManager.getOmtpVvmSources();
            for (PhoneAccountHandle source : sources) {
                setupAndSendRequest(source, null, action, isManualSync);
            }
        }
    }

    private void setupAndSendRequest(PhoneAccountHandle phoneAccount, Voicemail voicemail,
            String action, boolean isManualSync) {
        if (!VisualVoicemailSettingsUtil.isVisualVoicemailEnabled(this, phoneAccount)) {
            Log.v(TAG, "Sync requested for disabled account");
            return;
        }

        if (SYNC_FULL_SYNC.equals(action)) {
            long lastSyncTime = VisualVoicemailSettingsUtil.getVisualVoicemailLastFullSyncTime(
                    this, phoneAccount);
            long currentTime = System.currentTimeMillis();
            int minimumInterval = isManualSync ? MINIMUM_MANUAL_SYNC_INTERVAL_MILLIS
                    : MINIMUM_MANUAL_SYNC_INTERVAL_MILLIS;
            if (currentTime - lastSyncTime < minimumInterval) {
                // If it's been less than a minute since the last sync, bail.
                Log.v(TAG, "Avoiding duplicate full sync: synced recently for "
                        + phoneAccount.getId());

                /**
                 *  Perform a NOOP change to the database so the sender can observe the sync is
                 *  completed.
                 *  TODO: Instead of this hack, refactor the sync to be synchronous so the sender
                 *  can use sendOrderedBroadcast() to register a callback once all syncs are
                 *  finished
                 *  b/26937720
                 */
                Status.setStatus(this, phoneAccount,
                        Status.CONFIGURATION_STATE_IGNORE,
                        Status.DATA_CHANNEL_STATE_IGNORE,
                        Status.NOTIFICATION_CHANNEL_STATE_IGNORE);
                return;
            }
            VisualVoicemailSettingsUtil.setVisualVoicemailLastFullSyncTime(
                    this, phoneAccount, currentTime);
        }

        VvmNetworkRequestCallback networkCallback = new SyncNetworkRequestCallback(this,
                phoneAccount, voicemail, action);
        networkCallback.requestNetwork();
    }

    private void doSync(Network network, VvmNetworkRequestCallback callback,
            PhoneAccountHandle phoneAccount, Voicemail voicemail, String action) {
        int retryCount = NETWORK_RETRY_COUNT;
        try {
            while (retryCount > 0) {
                ImapHelper imapHelper = new ImapHelper(this, phoneAccount, network);
                if (!imapHelper.isSuccessfullyInitialized()) {
                    Log.w(TAG, "Can't retrieve Imap credentials.");
                    VisualVoicemailSettingsUtil.resetVisualVoicemailRetryInterval(this,
                            phoneAccount);
                    return;
                }

                boolean success = true;
                if (voicemail == null) {
                    success = syncAll(action, imapHelper, phoneAccount);
                } else {
                    success = syncOne(imapHelper, voicemail, phoneAccount);
                }
                imapHelper.updateQuota();

                // Need to check again for whether visual voicemail is enabled because it could have
                // been disabled while waiting for the response from the network.
                if (VisualVoicemailSettingsUtil.isVisualVoicemailEnabled(this, phoneAccount) &&
                        !success) {
                    retryCount--;
                    Log.v(TAG, "Retrying " + action);
                } else {
                    // Nothing more to do here, just exit.
                    VisualVoicemailSettingsUtil.resetVisualVoicemailRetryInterval(this,
                            phoneAccount);
                    VoicemailUtils.setDataChannelState(
                            this, phoneAccount, Status.DATA_CHANNEL_STATE_OK);
                    return;
                }
            }
        } finally {
            if (callback != null) {
                callback.releaseNetwork();
            }
        }
    }

    private boolean syncAll(String action, ImapHelper imapHelper, PhoneAccountHandle account) {
        boolean uploadSuccess = true;
        boolean downloadSuccess = true;

        if (SYNC_FULL_SYNC.equals(action) || SYNC_UPLOAD_ONLY.equals(action)) {
            uploadSuccess = upload(imapHelper);
        }
        if (SYNC_FULL_SYNC.equals(action) || SYNC_DOWNLOAD_ONLY.equals(action)) {
            downloadSuccess = download(imapHelper, account);
        }

        Log.v(TAG, "upload succeeded: [" + String.valueOf(uploadSuccess)
                + "] download succeeded: [" + String.valueOf(downloadSuccess) + "]");

        boolean success = uploadSuccess && downloadSuccess;
        if (!uploadSuccess || !downloadSuccess) {
            if (uploadSuccess) {
                action = SYNC_DOWNLOAD_ONLY;
            } else if (downloadSuccess) {
                action = SYNC_UPLOAD_ONLY;
            }
        }

        return success;
    }

    private boolean syncOne(ImapHelper imapHelper, Voicemail voicemail,
            PhoneAccountHandle account) {
        if (shouldPerformPrefetch(account, imapHelper)) {
            VoicemailFetchedCallback callback = new VoicemailFetchedCallback(this,
                    voicemail.getUri());
            imapHelper.fetchVoicemailPayload(callback, voicemail.getSourceData());
        }

        return imapHelper.fetchTranscription(
                new TranscriptionFetchedCallback(this, voicemail),
                voicemail.getSourceData());
    }

    private class SyncNetworkRequestCallback extends VvmNetworkRequestCallback {

        Voicemail mVoicemail;
        private String mAction;

        public SyncNetworkRequestCallback(Context context, PhoneAccountHandle phoneAccount,
                Voicemail voicemail, String action) {
            super(context, phoneAccount);
            mAction = action;
            mVoicemail = voicemail;
        }

        @Override
        public void onAvailable(Network network) {
            super.onAvailable(network);
            NetworkInfo info = getConnectivityManager().getNetworkInfo(network);
            if (info == null) {
                Log.d(TAG, "Network Type: Unknown");
            } else {
                Log.d(TAG, "Network Type: " + info.getTypeName());
            }

            doSync(network, this, mPhoneAccount, mVoicemail, mAction);
        }

    }

    private boolean upload(ImapHelper imapHelper) {
        List<Voicemail> readVoicemails = mQueryHelper.getReadVoicemails();
        List<Voicemail> deletedVoicemails = mQueryHelper.getDeletedVoicemails();

        boolean success = true;

        if (deletedVoicemails.size() > 0) {
            if (imapHelper.markMessagesAsDeleted(deletedVoicemails)) {
                // We want to delete selectively instead of all the voicemails for this provider
                // in case the state changed since the IMAP query was completed.
                mQueryHelper.deleteFromDatabase(deletedVoicemails);
            } else {
                success = false;
            }
        }

        if (readVoicemails.size() > 0) {
            if (imapHelper.markMessagesAsRead(readVoicemails)) {
                mQueryHelper.markReadInDatabase(readVoicemails);
            } else {
                success = false;
            }
        }

        return success;
    }

    private boolean download(ImapHelper imapHelper, PhoneAccountHandle account) {
        List<Voicemail> serverVoicemails = imapHelper.fetchAllVoicemails();
        List<Voicemail> localVoicemails = mQueryHelper.getAllVoicemails();

        if (localVoicemails == null || serverVoicemails == null) {
            // Null value means the query failed.
            return false;
        }

        Map<String, Voicemail> remoteMap = buildMap(serverVoicemails);

        // Go through all the local voicemails and check if they are on the server.
        // They may be read or deleted on the server but not locally. Perform the
        // appropriate local operation if the status differs from the server. Remove
        // the messages that exist both locally and on the server to know which server
        // messages to insert locally.
        for (int i = 0; i < localVoicemails.size(); i++) {
            Voicemail localVoicemail = localVoicemails.get(i);
            Voicemail remoteVoicemail = remoteMap.remove(localVoicemail.getSourceData());
            if (remoteVoicemail == null) {
                mQueryHelper.deleteFromDatabase(localVoicemail);
            } else {
                if (remoteVoicemail.isRead() != localVoicemail.isRead()) {
                    mQueryHelper.markReadInDatabase(localVoicemail);
                }

                if (!TextUtils.isEmpty(remoteVoicemail.getTranscription()) &&
                        TextUtils.isEmpty(localVoicemail.getTranscription())) {
                    mQueryHelper.updateWithTranscription(localVoicemail,
                            remoteVoicemail.getTranscription());
                }
            }
        }

        // The leftover messages are messages that exist on the server but not locally.
        boolean prefetchEnabled = shouldPerformPrefetch(account, imapHelper);
        for (Voicemail remoteVoicemail : remoteMap.values()) {
            Uri uri = VoicemailContract.Voicemails.insert(this, remoteVoicemail);
            if (prefetchEnabled) {
                VoicemailFetchedCallback fetchedCallback = new VoicemailFetchedCallback(this, uri);
                imapHelper.fetchVoicemailPayload(fetchedCallback, remoteVoicemail.getSourceData());
            }
        }

        return true;
    }

    private boolean shouldPerformPrefetch(PhoneAccountHandle account, ImapHelper imapHelper) {
        OmtpVvmCarrierConfigHelper carrierConfigHelper = new OmtpVvmCarrierConfigHelper(
                this, PhoneUtils.getSubIdForPhoneAccountHandle(account));
        return carrierConfigHelper.isPrefetchEnabled() && !imapHelper.isRoaming();
    }

    protected void setRetryAlarm(PhoneAccountHandle phoneAccount, String action) {
        Intent serviceIntent = new Intent(this, OmtpVvmSyncService.class);
        serviceIntent.setAction(action);
        serviceIntent.putExtra(OmtpVvmSyncService.EXTRA_PHONE_ACCOUNT, phoneAccount);
        PendingIntent pendingIntent = PendingIntent.getService(this, 0, serviceIntent, 0);
        long retryInterval = VisualVoicemailSettingsUtil.getVisualVoicemailRetryInterval(this,
                phoneAccount);

        Log.v(TAG, "Retrying " + action + " in " + retryInterval + "ms");

        AlarmManager alarmManager = (AlarmManager) this.getSystemService(Context.ALARM_SERVICE);
        alarmManager.set(AlarmManager.RTC, System.currentTimeMillis() + retryInterval,
                pendingIntent);

        VisualVoicemailSettingsUtil.setVisualVoicemailRetryInterval(this, phoneAccount,
                retryInterval * 2);
    }

    /**
     * Builds a map from provider data to message for the given collection of voicemails.
     */
    private Map<String, Voicemail> buildMap(List<Voicemail> messages) {
        Map<String, Voicemail> map = new HashMap<String, Voicemail>();
        for (Voicemail message : messages) {
            map.put(message.getSourceData(), message);
        }
        return map;
    }

    public class TranscriptionFetchedCallback {

        private Context mContext;
        private Voicemail mVoicemail;

        public TranscriptionFetchedCallback(Context context, Voicemail voicemail) {
            mContext = context;
            mVoicemail = voicemail;
        }

        public void setVoicemailTranscription(String transcription) {
            VoicemailsQueryHelper queryHelper = new VoicemailsQueryHelper(mContext);
            queryHelper.updateWithTranscription(mVoicemail, transcription);
        }
    }
}
