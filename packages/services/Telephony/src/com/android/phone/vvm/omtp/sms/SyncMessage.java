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

import com.android.phone.vvm.omtp.OmtpConstants;

/**
 * Structured data representation of an OMTP SYNC message.
 *
 * Getters will return null if the field was not set in the message body or it could not be parsed.
 */
public class SyncMessage {
    // Sync event that triggered this message.
    private final String mSyncTriggerEvent;
    // Total number of new messages on the server.
    private final Integer mNewMessageCount;
    // UID of the new message.
    private final String mMessageId;
    // Length of the message.
    private final Integer mMessageLength;
    // Content type (voice, video, fax...) of the new message.
    private final String mContentType;
    // Sender of the new message.
    private final String mSender;
    // Timestamp (in millis) of the new message.
    private final Long mMsgTimeMillis;

    @Override
    public String toString() {
        return "SyncMessage [mSyncTriggerEvent=" + mSyncTriggerEvent
                + ", mNewMessageCount=" + mNewMessageCount
                + ", mMessageId=" + mMessageId
                + ", mMessageLength=" + mMessageLength
                + ", mContentType=" + mContentType
                + ", mSender=" + mSender
                + ", mMsgTimeMillis=" + mMsgTimeMillis + "]";
    }

    public SyncMessage(WrappedMessageData wrappedData) {
        mSyncTriggerEvent = wrappedData.extractString(OmtpConstants.SYNC_TRIGGER_EVENT);
        mMessageId = wrappedData.extractString(OmtpConstants.MESSAGE_UID);
        mMessageLength = wrappedData.extractInteger(OmtpConstants.MESSAGE_LENGTH);
        mContentType = wrappedData.extractString(OmtpConstants.CONTENT_TYPE);
        mSender = wrappedData.extractString(OmtpConstants.SENDER);
        mNewMessageCount = wrappedData.extractInteger(OmtpConstants.NUM_MESSAGE_COUNT);
        mMsgTimeMillis = wrappedData.extractTime(OmtpConstants.TIME);
    }

    /**
     * @return the event that triggered the sync message. This is a mandatory field and must always
     * be set.
     */
    public String getSyncTriggerEvent() {
        return mSyncTriggerEvent;
    }

    /**
     * @return the number of new messages stored on the voicemail server.
     */
    public int getNewMessageCount() {
        return mNewMessageCount != null ? mNewMessageCount : 0;
    }

    /**
     * @return the message ID of the new message.
     * <p>
     * Expected to be set only for
     * {@link com.android.phone.vvm.omtp.OmtpConstants#NEW_MESSAGE}
     */
    public String getId() {
        return mMessageId;
    }

    /**
     * @return the content type of the new message.
     * <p>
     * Expected to be set only for
     * {@link com.android.phone.vvm.omtp.OmtpConstants#NEW_MESSAGE}
     */
    public String getContentType() {
        return mContentType;
    }

    /**
     * @return the message length of the new message.
     * <p>
     * Expected to be set only for
     * {@link com.android.phone.vvm.omtp.OmtpConstants#NEW_MESSAGE}
     */
    public int getLength() {
        return mMessageLength != null ? mMessageLength : 0;
    }

    /**
     * @return the sender's phone number of the new message specified as MSISDN.
     * <p>
     * Expected to be set only for
     * {@link com.android.phone.vvm.omtp.OmtpConstants#NEW_MESSAGE}
     */
    public String getSender() {
        return mSender;
    }

    /**
     * @return the timestamp as milliseconds for the new message.
     * <p>
     * Expected to be set only for
     * {@link com.android.phone.vvm.omtp.OmtpConstants#NEW_MESSAGE}
     */
    public long getTimestampMillis() {
        return mMsgTimeMillis != null ? mMsgTimeMillis : 0;
    }
}