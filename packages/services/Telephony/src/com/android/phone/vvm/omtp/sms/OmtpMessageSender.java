/*
 * Copyright (C) 2015 Google Inc. All Rights Reserved.
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

import android.annotation.Nullable;
import android.app.PendingIntent;
import android.telephony.SmsManager;

import com.android.phone.vvm.omtp.OmtpConstants;
import com.android.services.telephony.Log;

import java.io.UnsupportedEncodingException;

/**
 * Send client originated OMTP messages to the OMTP server.
 * <p>
 * Uses {@link PendingIntent} instead of a call back to notify when the message is
 * sent. This is primarily to keep the implementation simple and reuse what the underlying
 * {@link SmsManager} interface provides.
 * <p>
 * Provides simple APIs to send different types of mobile originated OMTP SMS to the VVM server.
 */
public abstract class OmtpMessageSender {
    protected static final String TAG = "OmtpMessageSender";
    protected short mApplicationPort;
    protected String mDestinationNumber;
    protected SmsManager mSmsManager;

    public OmtpMessageSender(SmsManager smsManager, short applicationPort,
            String destinationNumber) {
        mSmsManager = smsManager;
        mApplicationPort = applicationPort;
        mDestinationNumber = destinationNumber;
    }

    /**
     * Sends a request to the VVM server to activate VVM for the current subscriber.
     *
     * @param sentIntent If not NULL this PendingIntent is broadcast when the message is
     *            successfully sent, or failed.
     */
    public void requestVvmActivation(@Nullable PendingIntent sentIntent) {}

    /**
     * Sends a request to the VVM server to deactivate VVM for the current subscriber.
     *
     * @param sentIntent If not NULL this PendingIntent is broadcast when the message is
     *            successfully sent, or failed.
     */
    public void requestVvmDeactivation(@Nullable PendingIntent sentIntent) {}

    /**
     * Send a request to the VVM server to get account status of the current subscriber.
     *
     * @param sentIntent If not NULL this PendingIntent is broadcast when the message is
     *            successfully sent, or failed.
     */
    public void requestVvmStatus(@Nullable PendingIntent sentIntent) {}

    protected void sendSms(String text, PendingIntent sentIntent) {
        // If application port is set to 0 then send simple text message, else send data message.
        if (mApplicationPort == 0) {
            Log.v(TAG, String.format("Sending TEXT sms '%s' to %s", text, mDestinationNumber));
            mSmsManager.sendTextMessageWithSelfPermissions(mDestinationNumber, null, text,
                    sentIntent, null);
        } else {
            byte[] data;
            try {
                data = text.getBytes("UTF-8");
            } catch (UnsupportedEncodingException e) {
                throw new IllegalStateException("Failed to encode: " + text);
            }
            Log.v(TAG, String.format("Sending BINARY sms '%s' to %s:%d", text, mDestinationNumber,
                    mApplicationPort));
            mSmsManager.sendDataMessageWithSelfPermissions(mDestinationNumber, null,
                    mApplicationPort, data, sentIntent, null);
        }
    }

    protected void appendField(StringBuilder sb, String field, Object value) {
        sb.append(field).append(OmtpConstants.SMS_KEY_VALUE_SEPARATOR).append(value);
    }
}
