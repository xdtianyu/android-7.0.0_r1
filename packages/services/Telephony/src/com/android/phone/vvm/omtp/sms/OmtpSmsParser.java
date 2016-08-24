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

import android.util.ArrayMap;
import android.util.Log;

import com.android.phone.vvm.omtp.OmtpConstants;

import java.util.Map;

/**
 * OMTP SMS parser interface, for parsing SYNC and STATUS SMS sent by OMTP visual voicemail server.
 */
public class OmtpSmsParser {
    private static String TAG = "OmtpSmsParser";
    /**
     * Parses the supplied SMS body and returns back a structured OMTP message.
     * Returns null if unable to parse the SMS body.
     */
    public static WrappedMessageData parse(String smsBody) {
        if (smsBody == null) {
            return null;
        }

        WrappedMessageData messageData = null;
        if (smsBody.startsWith(OmtpConstants.SYNC_SMS_PREFIX)) {
            messageData = new WrappedMessageData(OmtpConstants.SYNC_SMS_PREFIX,
                    parseSmsBody(smsBody.substring(OmtpConstants.SYNC_SMS_PREFIX.length())));
            // Check for a mandatory field.
            String triggerEvent = messageData.extractString(OmtpConstants.SYNC_TRIGGER_EVENT);
            if (triggerEvent == null) {
                Log.e(TAG, "Missing mandatory field: " + OmtpConstants.SYNC_TRIGGER_EVENT);
                return null;
            }
        } else if (smsBody.startsWith(OmtpConstants.STATUS_SMS_PREFIX)) {
            messageData = new WrappedMessageData(OmtpConstants.STATUS_SMS_PREFIX,
                    parseSmsBody(smsBody.substring(OmtpConstants.STATUS_SMS_PREFIX.length())));
        }

        return messageData;
    }

    /**
     * Converts a String of key/value pairs into a Map object. The WrappedMessageData object
     * contains helper functions to retrieve the values.
     *
     * e.g. "//VVM:STATUS:st=R;rc=0;srv=1;dn=1;ipt=1;spt=0;u=eg@example.com;pw=1"
     * => "WrappedMessageData [mFields={st=R, ipt=1, srv=1, dn=1, u=eg@example.com, pw=1, rc=0}]"
     *
     * @param message The sms string with the prefix removed.
     * @return A WrappedMessageData object containing the map.
     */
    private static Map<String, String> parseSmsBody(String message) {
        Map<String, String> keyValues = new ArrayMap<String, String>();
        String[] entries = message.split(OmtpConstants.SMS_FIELD_SEPARATOR);
        for (String entry : entries) {
            String[] keyValue = entry.split(OmtpConstants.SMS_KEY_VALUE_SEPARATOR);
            if (keyValue.length != 2) {
                continue;
            }
            keyValues.put(keyValue[0].trim(), keyValue[1].trim());
        }

        return keyValues;
    }
}