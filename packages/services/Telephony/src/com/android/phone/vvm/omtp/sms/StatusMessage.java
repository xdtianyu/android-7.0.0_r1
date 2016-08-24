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

import android.telecom.Log;

import com.android.phone.vvm.omtp.OmtpConstants;

/**
 * Structured data representation of OMTP STATUS message.
 *
 * The getters will return null if the field was not set in the message body or it could not be
 * parsed.
 */
public class StatusMessage {
    // NOTE: Following Status SMS fields are not yet parsed, as they do not seem
    // to be useful for initial omtp source implementation.
    // lang, g_len, vs_len, pw_len, pm, gm, vtc, vt

    private final String mProvisioningStatus;
    private final String mStatusReturnCode;
    private final String mSubscriptionUrl;
    private final String mServerAddress;
    private final String mTuiAccessNumber;
    private final String mClientSmsDestinationNumber;
    private final String mImapPort;
    private final String mImapUserName;
    private final String mImapPassword;
    private final String mSmtpPort;
    private final String mSmtpUserName;
    private final String mSmtpPassword;

    @Override
    public String toString() {
        return "StatusMessage [mProvisioningStatus=" + mProvisioningStatus
                + ", mStatusReturnCode=" + mStatusReturnCode
                + ", mSubscriptionUrl=" + mSubscriptionUrl
                + ", mServerAddress=" + mServerAddress
                + ", mTuiAccessNumber=" + mTuiAccessNumber
                + ", mClientSmsDestinationNumber=" + mClientSmsDestinationNumber
                + ", mImapPort=" + mImapPort
                + ", mImapUserName=" + mImapUserName
                + ", mImapPassword=" + Log.pii(mImapPassword)
                + ", mSmtpPort=" + mSmtpPort
                + ", mSmtpUserName=" + mSmtpUserName
                + ", mSmtpPassword=" + Log.pii(mSmtpPassword) + "]";
    }

    public StatusMessage(WrappedMessageData wrappedData) {
        mProvisioningStatus = wrappedData.extractString(OmtpConstants.PROVISIONING_STATUS);
        mStatusReturnCode = wrappedData.extractString(OmtpConstants.RETURN_CODE);
        mSubscriptionUrl = wrappedData.extractString(OmtpConstants.SUBSCRIPTION_URL);
        mServerAddress = wrappedData.extractString(OmtpConstants.SERVER_ADDRESS);
        mTuiAccessNumber = wrappedData.extractString(OmtpConstants.TUI_ACCESS_NUMBER);
        mClientSmsDestinationNumber = wrappedData.extractString(
                OmtpConstants.CLIENT_SMS_DESTINATION_NUMBER);
        mImapPort = wrappedData.extractString(OmtpConstants.IMAP_PORT);
        mImapUserName = wrappedData.extractString(OmtpConstants.IMAP_USER_NAME);
        mImapPassword = wrappedData.extractString(OmtpConstants.IMAP_PASSWORD);
        mSmtpPort = wrappedData.extractString(OmtpConstants.SMTP_PORT);
        mSmtpUserName = wrappedData.extractString(OmtpConstants.SMTP_USER_NAME);
        mSmtpPassword = wrappedData.extractString(OmtpConstants.SMTP_PASSWORD);
    }

    /**
     * @return the subscriber's VVM provisioning status.
     */
    public String getProvisioningStatus() {
        return mProvisioningStatus;
    }

    /**
     * @return the return-code of the status SMS.
     */
    public String getReturnCode() {
        return mStatusReturnCode;
    }

    /**
     * @return the URL of the voicemail server. This is the URL to send the users to for subscribing
     * to the visual voicemail service.
     */
    public String getSubscriptionUrl() {
        return mSubscriptionUrl;
    }

    /**
     * @return the voicemail server address. Either server IP address or fully qualified domain
     * name.
     */
    public String getServerAddress() {
        return mServerAddress;
    }

    /**
     * @return the Telephony User Interface number to call to access voicemails directly from the
     * IVR.
     */
    public String getTuiAccessNumber() {
        return mTuiAccessNumber;
    }

    /**
     * @return the number to which client originated SMSes should be sent to.
     */
    public String getClientSmsDestinationNumber() {
        return mClientSmsDestinationNumber;
    }

    /**
     * @return the IMAP server port to talk to.
     */
    public String getImapPort() {
        return mImapPort;
    }

    /**
     * @return the IMAP user name to be used for authentication.
     */
    public String getImapUserName() {
        return mImapUserName;
    }

    /**
     * @return the IMAP password to be used for authentication.
     */
    public String getImapPassword() {
        return mImapPassword;
    }

    /**
     * @return the SMTP server port to talk to.
     */
    public String getSmtpPort() {
        return mSmtpPort;
    }

    /**
     * @return the SMTP user name to be used for SMTP authentication.
     */
    public String getSmtpUserName() {
        return mSmtpUserName;
    }

    /**
     * @return the SMTP password to be used for SMTP authentication.
     */
    public String getSmtpPassword() {
        return mSmtpPassword;
    }
}