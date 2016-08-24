/*
 * Copyright (C) 2014 The Android Open Source Project
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
 * limitations under the License.
 */

package com.android.server.telecom;

import android.content.Context;
import android.content.Intent;
import android.os.UserHandle;

public final class TelecomBroadcastIntentProcessor {
    /** The action used to send SMS response for the missed call notification. */
    public static final String ACTION_SEND_SMS_FROM_NOTIFICATION =
            "com.android.server.telecom.ACTION_SEND_SMS_FROM_NOTIFICATION";

    /** The action used to call a handle back for the missed call notification. */
    public static final String ACTION_CALL_BACK_FROM_NOTIFICATION =
            "com.android.server.telecom.ACTION_CALL_BACK_FROM_NOTIFICATION";

    /** The action used to clear missed calls. */
    public static final String ACTION_CLEAR_MISSED_CALLS =
            "com.android.server.telecom.ACTION_CLEAR_MISSED_CALLS";

    public static final String EXTRA_USERHANDLE = "userhandle";

    private final Context mContext;
    private final CallsManager mCallsManager;

    public TelecomBroadcastIntentProcessor(Context context, CallsManager callsManager) {
        mContext = context;
        mCallsManager = callsManager;
    }

    public void processIntent(Intent intent) {
        String action = intent.getAction();

        Log.v(this, "Action received: %s.", action);
        UserHandle userHandle = intent.getParcelableExtra(EXTRA_USERHANDLE);
        if (userHandle == null) {
            Log.d(this, "user handle can't be null, not processing the broadcast");
            return;
        }

        MissedCallNotifier missedCallNotifier = mCallsManager.getMissedCallNotifier();

        // Send an SMS from the missed call notification.
        if (ACTION_SEND_SMS_FROM_NOTIFICATION.equals(action)) {
            // Close the notification shade and the notification itself.
            closeSystemDialogs(mContext);
            missedCallNotifier.clearMissedCalls(userHandle);

            Intent callIntent = new Intent(Intent.ACTION_SENDTO, intent.getData());
            callIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            mContext.startActivityAsUser(callIntent, userHandle);

        // Call back recent caller from the missed call notification.
        } else if (ACTION_CALL_BACK_FROM_NOTIFICATION.equals(action)) {
            // Close the notification shade and the notification itself.
            closeSystemDialogs(mContext);
            missedCallNotifier.clearMissedCalls(userHandle);

            Intent callIntent = new Intent(Intent.ACTION_CALL, intent.getData());
            callIntent.setFlags(
                    Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
            mContext.startActivityAsUser(callIntent, userHandle);

        // Clear the missed call notification and call log entries.
        } else if (ACTION_CLEAR_MISSED_CALLS.equals(action)) {
            missedCallNotifier.clearMissedCalls(userHandle);
        }
    }

    /**
     * Closes open system dialogs and the notification shade.
     */
    private void closeSystemDialogs(Context context) {
        Intent intent = new Intent(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        context.sendBroadcastAsUser(intent, UserHandle.ALL);
    }
}
