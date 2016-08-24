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

import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.Log;

import com.android.phone.vvm.omtp.OmtpConstants;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Locale;
import java.util.Map;

/**
 * Class wrapping the raw OMTP message data, internally represented as as map of all key-value pairs
 * found in the SMS body.
 * <p>
 * Provides convenience methods to extract parse fields of different types.
 * <p>
 * All the methods return null if either the field was not present or it could not be parsed.
 */
public class WrappedMessageData {
    private final String TAG = "WrappedMessageData";
    private final String mPrefix;
    private final Map<String, String> mFields;

    @Override
    public String toString() {
        return "WrappedMessageData [mFields=" + mFields + "]";
    }

    WrappedMessageData(String prefix, Map<String, String> keyValues) {
        mPrefix = prefix;
        mFields = new ArrayMap<String, String>();
        mFields.putAll(keyValues);
    }

    /**
     * @return The String prefix of the message, designating whether this is the message data of a
     * STATUS or SYNC sms.
     */
    String getPrefix() {
        return mPrefix;
    }

    /**
     * Extracts the requested field from underlying data and returns the String value as is.
     *
     * @param field The requested field.
     * @return the parsed string value, or null if the field was not present or not valid.
     */
    String extractString(final String field) {
        String value = mFields.get(field);
        if (value == null) {
            return null;
        }

        String[] possibleValues = OmtpConstants.possibleValuesMap.get(field);
        if (possibleValues == null) {
            return value;
        }
        for (int i = 0; i < possibleValues.length; i++) {
            if (TextUtils.equals(value, possibleValues[i])) {
                return value;
            }
        }
        Log.e(TAG, "extractString - value \"" + value +
                "\" of field \"" + field + "\" is not allowed.");
        return null;
    }

    /**
     * Extracts the requested field from underlying data and parses it as an {@link Integer}.
     *
     * @param field The requested field.
     * @return the parsed integer value, or null if the field was not present.
     */
    Integer extractInteger(final String field) {
        String value = mFields.get(field);
        if (value == null) {
            return null;
        }

        try {
            return Integer.decode(value);
        } catch (NumberFormatException e) {
            Log.e(TAG, "extractInteger - could not parse integer: " + value);
            return null;
        }
    }

    /**
     * Extracts the requested field from underlying data and parses it as a date/time represented in
     * {@link OmtpConstants#DATE_TIME_FORMAT} format.
     *
     * @param field The requested field.
     * @return the parsed string value, or null if the field was not present.
     */
    Long extractTime(final String field) {
        String value = mFields.get(field);
        if (value == null) {
            return null;
        }

        try {
            return new SimpleDateFormat(
                    OmtpConstants.DATE_TIME_FORMAT, Locale.US).parse(value).getTime();
        } catch (ParseException e) {
            Log.e(TAG, "extractTime - could not parse time: " + value);
            return null;
        }
    }
}