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
package com.android.phone.vvm.omtp.imap;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkInfo;
import android.preference.PreferenceManager;
import android.provider.VoicemailContract;
import android.provider.VoicemailContract.Status;
import android.telecom.PhoneAccountHandle;
import android.telecom.Voicemail;
import android.telephony.TelephonyManager;
import android.util.Base64;
import android.util.Log;

import com.android.phone.PhoneUtils;
import com.android.phone.VoicemailUtils;
import com.android.phone.common.mail.Address;
import com.android.phone.common.mail.Body;
import com.android.phone.common.mail.BodyPart;
import com.android.phone.common.mail.FetchProfile;
import com.android.phone.common.mail.Flag;
import com.android.phone.common.mail.Message;
import com.android.phone.common.mail.MessagingException;
import com.android.phone.common.mail.Multipart;
import com.android.phone.common.mail.TempDirectory;
import com.android.phone.common.mail.internet.MimeMessage;
import com.android.phone.common.mail.store.ImapFolder;
import com.android.phone.common.mail.store.ImapStore;
import com.android.phone.common.mail.store.imap.ImapConstants;
import com.android.phone.common.mail.utils.LogUtils;
import com.android.phone.settings.VisualVoicemailSettingsUtil;
import com.android.phone.vvm.omtp.OmtpConstants;
import com.android.phone.vvm.omtp.OmtpVvmCarrierConfigHelper;
import com.android.phone.vvm.omtp.fetch.VoicemailFetchedCallback;
import com.android.phone.vvm.omtp.sync.OmtpVvmSyncService.TranscriptionFetchedCallback;

import libcore.io.IoUtils;

import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * A helper interface to abstract commands sent across IMAP interface for a given account.
 */
public class ImapHelper {
    private final String TAG = "ImapHelper";

    private ImapFolder mFolder;
    private ImapStore mImapStore;

    private final Context mContext;
    private final PhoneAccountHandle mPhoneAccount;
    private final Network mNetwork;

    SharedPreferences mPrefs;
    private static final String PREF_KEY_QUOTA_OCCUPIED = "quota_occupied_";
    private static final String PREF_KEY_QUOTA_TOTAL = "quota_total_";

    private int mQuotaOccupied;
    private int mQuotaTotal;

    public ImapHelper(Context context, PhoneAccountHandle phoneAccount, Network network) {
        mContext = context;
        mPhoneAccount = phoneAccount;
        mNetwork = network;
        try {
            TempDirectory.setTempDirectory(context);

            String username = VisualVoicemailSettingsUtil.getVisualVoicemailCredentials(context,
                    OmtpConstants.IMAP_USER_NAME, phoneAccount);
            String password = VisualVoicemailSettingsUtil.getVisualVoicemailCredentials(context,
                    OmtpConstants.IMAP_PASSWORD, phoneAccount);
            String serverName = VisualVoicemailSettingsUtil.getVisualVoicemailCredentials(context,
                    OmtpConstants.SERVER_ADDRESS, phoneAccount);
            int port = Integer.parseInt(
                    VisualVoicemailSettingsUtil.getVisualVoicemailCredentials(context,
                            OmtpConstants.IMAP_PORT, phoneAccount));
            int auth = ImapStore.FLAG_NONE;

            OmtpVvmCarrierConfigHelper carrierConfigHelper = new OmtpVvmCarrierConfigHelper(context,
                    PhoneUtils.getSubIdForPhoneAccountHandle(phoneAccount));
            if (TelephonyManager.VVM_TYPE_CVVM.equals(carrierConfigHelper.getVvmType())) {
                // TODO: move these into the carrier config app
                port = 993;
                auth = ImapStore.FLAG_SSL;
            }

            mImapStore = new ImapStore(
                    context, this, username, password, port, serverName, auth, network);
        } catch (NumberFormatException e) {
            VoicemailUtils.setDataChannelState(
                    mContext, mPhoneAccount, Status.DATA_CHANNEL_STATE_BAD_CONFIGURATION);
            LogUtils.w(TAG, "Could not parse port number");
        }

        mPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        mQuotaOccupied = mPrefs.getInt(getSharedPrefsKey(PREF_KEY_QUOTA_OCCUPIED),
                VoicemailContract.Status.QUOTA_UNAVAILABLE);
        mQuotaTotal = mPrefs.getInt(getSharedPrefsKey(PREF_KEY_QUOTA_TOTAL),
                VoicemailContract.Status.QUOTA_UNAVAILABLE);

        Log.v(TAG, "Quota:" + mQuotaOccupied + "/" + mQuotaTotal);
    }

    /**
     * If mImapStore is null, this means that there was a missing or badly formatted port number,
     * which means there aren't sufficient credentials for login. If mImapStore is succcessfully
     * initialized, then ImapHelper is ready to go.
     */
    public boolean isSuccessfullyInitialized() {
        return mImapStore != null;
    }

    public boolean isRoaming(){
        ConnectivityManager connectivityManager = (ConnectivityManager) mContext.getSystemService(
                Context.CONNECTIVITY_SERVICE);
        NetworkInfo info = connectivityManager.getNetworkInfo(mNetwork);
        if(info == null){
            return false;
        }
        return info.isRoaming();
    }

    /** The caller thread will block until the method returns. */
    public boolean markMessagesAsRead(List<Voicemail> voicemails) {
        return setFlags(voicemails, Flag.SEEN);
    }

    /** The caller thread will block until the method returns. */
    public boolean markMessagesAsDeleted(List<Voicemail> voicemails) {
        return setFlags(voicemails, Flag.DELETED);
    }

    public void setDataChannelState(int dataChannelState) {
        VoicemailUtils.setDataChannelState(mContext, mPhoneAccount, dataChannelState);
    }

    /**
     * Set flags on the server for a given set of voicemails.
     *
     * @param voicemails The voicemails to set flags for.
     * @param flags The flags to set on the voicemails.
     * @return {@code true} if the operation completes successfully, {@code false} otherwise.
     */
    private boolean setFlags(List<Voicemail> voicemails, String... flags) {
        if (voicemails.size() == 0) {
            return false;
        }
        try {
            mFolder = openImapFolder(ImapFolder.MODE_READ_WRITE);
            if (mFolder != null) {
                mFolder.setFlags(convertToImapMessages(voicemails), flags, true);
                return true;
            }
            return false;
        } catch (MessagingException e) {
            LogUtils.e(TAG, e, "Messaging exception");
            return false;
        } finally {
            closeImapFolder();
        }
    }

    /**
     * Fetch a list of voicemails from the server.
     *
     * @return A list of voicemail objects containing data about voicemails stored on the server.
     */
    public List<Voicemail> fetchAllVoicemails() {
        List<Voicemail> result = new ArrayList<Voicemail>();
        Message[] messages;
        try {
            mFolder = openImapFolder(ImapFolder.MODE_READ_WRITE);
            if (mFolder == null) {
                // This means we were unable to successfully open the folder.
                return null;
            }

            // This method retrieves lightweight messages containing only the uid of the message.
            messages = mFolder.getMessages(null);

            for (Message message : messages) {
                // Get the voicemail details (message structure).
                MessageStructureWrapper messageStructureWrapper = fetchMessageStructure(message);
                if (messageStructureWrapper != null) {
                    result.add(getVoicemailFromMessageStructure(messageStructureWrapper));
                }
            }
            return result;
        } catch (MessagingException e) {
            LogUtils.e(TAG, e, "Messaging Exception");
            return null;
        } finally {
            closeImapFolder();
        }
    }

    /**
     * Extract voicemail details from the message structure. Also fetch transcription if a
     * transcription exists.
     */
    private Voicemail getVoicemailFromMessageStructure(
            MessageStructureWrapper messageStructureWrapper) throws MessagingException{
        Message messageDetails = messageStructureWrapper.messageStructure;

        TranscriptionFetchedListener listener = new TranscriptionFetchedListener();
        if (messageStructureWrapper.transcriptionBodyPart != null) {
            FetchProfile fetchProfile = new FetchProfile();
            fetchProfile.add(messageStructureWrapper.transcriptionBodyPart);

            mFolder.fetch(new Message[] {messageDetails}, fetchProfile, listener);
        }

        // Found an audio attachment, this is a valid voicemail.
        long time = messageDetails.getSentDate().getTime();
        String number = getNumber(messageDetails.getFrom());
        boolean isRead = Arrays.asList(messageDetails.getFlags()).contains(Flag.SEEN);
        return Voicemail.createForInsertion(time, number)
                .setPhoneAccount(mPhoneAccount)
                .setSourcePackage(mContext.getPackageName())
                .setSourceData(messageDetails.getUid())
                .setIsRead(isRead)
                .setTranscription(listener.getVoicemailTranscription())
                .build();
    }

    /**
     * The "from" field of a visual voicemail IMAP message is the number of the caller who left
     * the message. Extract this number from the list of "from" addresses.
     *
     * @param fromAddresses A list of addresses that comprise the "from" line.
     * @return The number of the voicemail sender.
     */
    private String getNumber(Address[] fromAddresses) {
        if (fromAddresses != null && fromAddresses.length > 0) {
            if (fromAddresses.length != 1) {
                LogUtils.w(TAG, "More than one from addresses found. Using the first one.");
            }
            String sender = fromAddresses[0].getAddress();
            int atPos = sender.indexOf('@');
            if (atPos != -1) {
                // Strip domain part of the address.
                sender = sender.substring(0, atPos);
            }
            return sender;
        }
        return null;
    }

    /**
     * Fetches the structure of the given message and returns a wrapper containing the message
     * structure and the transcription structure (if applicable).
     *
     * @throws MessagingException if fetching the structure of the message fails
     */
    private MessageStructureWrapper fetchMessageStructure(Message message)
            throws MessagingException {
        LogUtils.d(TAG, "Fetching message structure for " + message.getUid());

        MessageStructureFetchedListener listener = new MessageStructureFetchedListener();

        FetchProfile fetchProfile = new FetchProfile();
        fetchProfile.addAll(Arrays.asList(FetchProfile.Item.FLAGS, FetchProfile.Item.ENVELOPE,
                FetchProfile.Item.STRUCTURE));

        // The IMAP folder fetch method will call "messageRetrieved" on the listener when the
        // message is successfully retrieved.
        mFolder.fetch(new Message[] {message}, fetchProfile, listener);
        return listener.getMessageStructure();
    }

    public boolean fetchVoicemailPayload(VoicemailFetchedCallback callback, final String uid) {
        try {
            mFolder = openImapFolder(ImapFolder.MODE_READ_WRITE);
            if (mFolder == null) {
                // This means we were unable to successfully open the folder.
                return false;
            }
            Message message = mFolder.getMessage(uid);
            if (message == null) {
                return false;
            }
            VoicemailPayload voicemailPayload = fetchVoicemailPayload(message);

            if (voicemailPayload == null) {
                return false;
            }

            callback.setVoicemailContent(voicemailPayload);
            return true;
        } catch (MessagingException e) {
        } finally {
            closeImapFolder();
        }
        return false;
    }

    /**
     * Fetches the body of the given message and returns the parsed voicemail payload.
     *
     * @throws MessagingException if fetching the body of the message fails
     */
    private VoicemailPayload fetchVoicemailPayload(Message message)
            throws MessagingException {
        LogUtils.d(TAG, "Fetching message body for " + message.getUid());

        MessageBodyFetchedListener listener = new MessageBodyFetchedListener();

        FetchProfile fetchProfile = new FetchProfile();
        fetchProfile.add(FetchProfile.Item.BODY);

        mFolder.fetch(new Message[] {message}, fetchProfile, listener);
        return listener.getVoicemailPayload();
    }

    public boolean fetchTranscription(TranscriptionFetchedCallback callback, String uid) {
        try {
            mFolder = openImapFolder(ImapFolder.MODE_READ_WRITE);
            if (mFolder == null) {
                // This means we were unable to successfully open the folder.
                return false;
            }

            Message message = mFolder.getMessage(uid);
            if (message == null) {
                return false;
            }

            MessageStructureWrapper messageStructureWrapper = fetchMessageStructure(message);
            if (messageStructureWrapper != null) {
                TranscriptionFetchedListener listener = new TranscriptionFetchedListener();
                if (messageStructureWrapper.transcriptionBodyPart != null) {
                    FetchProfile fetchProfile = new FetchProfile();
                    fetchProfile.add(messageStructureWrapper.transcriptionBodyPart);

                    // This method is called synchronously so the transcription will be populated
                    // in the listener once the next method is called.
                    mFolder.fetch(new Message[] {message}, fetchProfile, listener);
                    callback.setVoicemailTranscription(listener.getVoicemailTranscription());
                }
            }
            return true;
        } catch (MessagingException e) {
            LogUtils.e(TAG, e, "Messaging Exception");
            return false;
        } finally {
            closeImapFolder();
        }
    }

    public void updateQuota() {
        try {
            mFolder = openImapFolder(ImapFolder.MODE_READ_WRITE);
            if (mFolder == null) {
                // This means we were unable to successfully open the folder.
                return;
            }
            updateQuota(mFolder);
        } catch (MessagingException e) {
            LogUtils.e(TAG, e, "Messaging Exception");
        } finally {
            closeImapFolder();
        }
    }

    private void updateQuota(ImapFolder folder) throws MessagingException {
        setQuota(folder.getQuota());
    }

    private void setQuota(ImapFolder.Quota quota) {
        if (quota == null) {
            return;
        }
        if (quota.occupied == mQuotaOccupied && quota.total == mQuotaTotal) {
            Log.v(TAG, "Quota hasn't changed");
            return;
        }
        mQuotaOccupied = quota.occupied;
        mQuotaTotal = quota.total;
        VoicemailContract.Status
                .setQuota(mContext, mPhoneAccount, mQuotaOccupied, mQuotaTotal);
        mPrefs.edit()
                .putInt(getSharedPrefsKey(PREF_KEY_QUOTA_OCCUPIED), mQuotaOccupied)
                .putInt(getSharedPrefsKey(PREF_KEY_QUOTA_TOTAL), mQuotaTotal)
                .apply();
        Log.v(TAG, "Quota changed to " + mQuotaOccupied + "/" + mQuotaTotal);
    }
    /**
     * A wrapper to hold a message with its header details and the structure for transcriptions
     * (so they can be fetched in the future).
     */
    public class MessageStructureWrapper {
        public Message messageStructure;
        public BodyPart transcriptionBodyPart;

        public MessageStructureWrapper() { }
    }

    /**
     * Listener for the message structure being fetched.
     */
    private final class MessageStructureFetchedListener
            implements ImapFolder.MessageRetrievalListener {
        private MessageStructureWrapper mMessageStructure;

        public MessageStructureFetchedListener() {
        }

        public MessageStructureWrapper getMessageStructure() {
            return mMessageStructure;
        }

        @Override
        public void messageRetrieved(Message message) {
            LogUtils.d(TAG, "Fetched message structure for " + message.getUid());
            LogUtils.d(TAG, "Message retrieved: " + message);
            try {
                mMessageStructure = getMessageOrNull(message);
                if (mMessageStructure == null) {
                    LogUtils.d(TAG, "This voicemail does not have an attachment...");
                    return;
                }
            } catch (MessagingException e) {
                LogUtils.e(TAG, e, "Messaging Exception");
                closeImapFolder();
            }
        }

        /**
         * Check if this IMAP message is a valid voicemail and whether it contains a transcription.
         *
         * @param message The IMAP message.
         * @return The MessageStructureWrapper object corresponding to an IMAP message and
         * transcription.
         * @throws MessagingException
         */
        private MessageStructureWrapper getMessageOrNull(Message message)
                throws MessagingException {
            if (!message.getMimeType().startsWith("multipart/")) {
                LogUtils.w(TAG, "Ignored non multi-part message");
                return null;
            }

            MessageStructureWrapper messageStructureWrapper = new MessageStructureWrapper();

            Multipart multipart = (Multipart) message.getBody();
            for (int i = 0; i < multipart.getCount(); ++i) {
                BodyPart bodyPart = multipart.getBodyPart(i);
                String bodyPartMimeType = bodyPart.getMimeType().toLowerCase();
                LogUtils.d(TAG, "bodyPart mime type: " + bodyPartMimeType);

                if (bodyPartMimeType.startsWith("audio/")) {
                    messageStructureWrapper.messageStructure = message;
                } else if (bodyPartMimeType.startsWith("text/")) {
                    messageStructureWrapper.transcriptionBodyPart = bodyPart;
                }
            }

            if (messageStructureWrapper.messageStructure != null) {
                return messageStructureWrapper;
            }

            // No attachment found, this is not a voicemail.
            return null;
        }
    }

    /**
     * Listener for the message body being fetched.
     */
    private final class MessageBodyFetchedListener implements ImapFolder.MessageRetrievalListener {
        private VoicemailPayload mVoicemailPayload;

        /** Returns the fetch voicemail payload. */
        public VoicemailPayload getVoicemailPayload() {
            return mVoicemailPayload;
        }

        @Override
        public void messageRetrieved(Message message) {
            LogUtils.d(TAG, "Fetched message body for " + message.getUid());
            LogUtils.d(TAG, "Message retrieved: " + message);
            try {
                mVoicemailPayload = getVoicemailPayloadFromMessage(message);
            } catch (MessagingException e) {
                LogUtils.e(TAG, "Messaging Exception:", e);
            } catch (IOException e) {
                LogUtils.e(TAG, "IO Exception:", e);
            }
        }

        private VoicemailPayload getVoicemailPayloadFromMessage(Message message)
                throws MessagingException, IOException {
            Multipart multipart = (Multipart) message.getBody();
            for (int i = 0; i < multipart.getCount(); ++i) {
                BodyPart bodyPart = multipart.getBodyPart(i);
                String bodyPartMimeType = bodyPart.getMimeType().toLowerCase();
                LogUtils.d(TAG, "bodyPart mime type: " + bodyPartMimeType);

                if (bodyPartMimeType.startsWith("audio/")) {
                    byte[] bytes = getDataFromBody(bodyPart.getBody());
                    LogUtils.d(TAG, String.format("Fetched %s bytes of data", bytes.length));
                    return new VoicemailPayload(bodyPartMimeType, bytes);
                }
            }
            LogUtils.e(TAG, "No audio attachment found on this voicemail");
            return null;
        }
    }

    /**
     * Listener for the transcription being fetched.
     */
    private final class TranscriptionFetchedListener implements
            ImapFolder.MessageRetrievalListener {
        private String mVoicemailTranscription;

        /** Returns the fetched voicemail transcription. */
        public String getVoicemailTranscription() {
            return mVoicemailTranscription;
        }

        @Override
        public void messageRetrieved(Message message) {
            LogUtils.d(TAG, "Fetched transcription for " + message.getUid());
            try {
                mVoicemailTranscription = new String(getDataFromBody(message.getBody()));
            } catch (MessagingException e) {
                LogUtils.e(TAG, "Messaging Exception:", e);
            } catch (IOException e) {
                LogUtils.e(TAG, "IO Exception:", e);
            }
        }
    }

    private ImapFolder openImapFolder(String modeReadWrite) {
        try {
            if (mImapStore == null) {
                return null;
            }
            ImapFolder folder = new ImapFolder(mImapStore, ImapConstants.INBOX);
            folder.open(modeReadWrite);
            return folder;
        } catch (MessagingException e) {
            LogUtils.e(TAG, e, "Messaging Exception");
        }
        return null;
    }

    private Message[] convertToImapMessages(List<Voicemail> voicemails) {
        Message[] messages = new Message[voicemails.size()];
        for (int i = 0; i < voicemails.size(); ++i) {
            messages[i] = new MimeMessage();
            messages[i].setUid(voicemails.get(i).getSourceData());
        }
        return messages;
    }

    private void closeImapFolder() {
        if (mFolder != null) {
            mFolder.close(true);
        }
    }

    private byte[] getDataFromBody(Body body) throws IOException, MessagingException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        BufferedOutputStream bufferedOut = new BufferedOutputStream(out);
        try {
            body.writeTo(bufferedOut);
            return Base64.decode(out.toByteArray(), Base64.DEFAULT);
        } finally {
            IoUtils.closeQuietly(bufferedOut);
            IoUtils.closeQuietly(out);
        }
    }

    private String getSharedPrefsKey(String key) {
        return VisualVoicemailSettingsUtil.getVisualVoicemailSharedPrefsKey(key, mPhoneAccount);
    }
}