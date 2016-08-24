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

package com.android.phone;

import android.content.Context;
import android.provider.VoicemailContract;
import android.provider.VoicemailContract.Status;
import android.telecom.PhoneAccountHandle;

public class VoicemailUtils {

    public static void setConfigurationState(Context context, PhoneAccountHandle accountHandle,
            int configurationState) {
        VoicemailContract.Status.setStatus(context, accountHandle,
                configurationState,
                Status.DATA_CHANNEL_STATE_IGNORE,
                Status.NOTIFICATION_CHANNEL_STATE_IGNORE);
    }

    public static void setDataChannelState(Context context, PhoneAccountHandle accountHandle,
            int dataChannelState) {
        VoicemailContract.Status.setStatus(context, accountHandle,
                Status.CONFIGURATION_STATE_IGNORE,
                dataChannelState,
                Status.NOTIFICATION_CHANNEL_STATE_IGNORE);
    }

    public static void setNotificationChannelState(Context context,
            PhoneAccountHandle accountHandle, int notificationChannelState) {
        VoicemailContract.Status.setStatus(context, accountHandle,
                Status.CONFIGURATION_STATE_IGNORE,
                Status.DATA_CHANNEL_STATE_IGNORE,
                notificationChannelState);
    }
}
