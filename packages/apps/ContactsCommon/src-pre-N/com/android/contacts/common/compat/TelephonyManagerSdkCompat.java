/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.contacts.common.compat;

import android.net.Uri;
import android.telecom.PhoneAccountHandle;
import android.telephony.TelephonyManager;
import android.util.Log;

/**
 * Placeholder class for pre-N.
 */
public class TelephonyManagerSdkCompat {

    private static final String TAG = "TelephonyManagerSdk";

    public static Uri getVoicemailRingtoneUri(TelephonyManager telephonyManager,
            PhoneAccountHandle accountHandle) {
        Log.wtf(TAG, "Not implemented.");
        return null;
    }

    public static boolean isVoicemailVibrationEnabled(TelephonyManager telephonyManager,
            PhoneAccountHandle accountHandle) {
        Log.wtf(TAG, "Not implemented");
        return false;
    }
}
