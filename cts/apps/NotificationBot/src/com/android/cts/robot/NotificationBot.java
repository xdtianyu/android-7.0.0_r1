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
package com.android.cts.robot;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;


public class NotificationBot extends BroadcastReceiver {
    private static final String TAG = "NotificationBot";
    private static final String EXTRA_ID = "ID";
    private static final String EXTRA_NOTIFICATION = "NOTIFICATION";
    private static final String ACTION_POST = "com.android.cts.robot.ACTION_POST";
    private static final String ACTION_CANCEL = "com.android.cts.robot.ACTION_CANCEL";

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.i(TAG, "received intent: " + intent);
        if (ACTION_POST.equals(intent.getAction())) {
            Log.i(TAG, ACTION_POST);
            if (!intent.hasExtra(EXTRA_NOTIFICATION) || !intent.hasExtra(EXTRA_ID)) {
                Log.e(TAG, "received post action with missing content");
                return;
            }
            int id = intent.getIntExtra(EXTRA_ID, -1);
            Log.i(TAG, "id: " + id);
            Notification n = (Notification) intent.getParcelableExtra(EXTRA_NOTIFICATION);
            Log.i(TAG, "n: " + n);
            NotificationManager noMa =
                    (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
            noMa.notify(id, n);

        } else if (ACTION_CANCEL.equals(intent.getAction())) {
            Log.i(TAG, ACTION_CANCEL);
            int id = intent.getIntExtra(EXTRA_ID, -1);
            Log.i(TAG, "id: " + id);
            if (id < 0) {
                Log.e(TAG, "received cancel action with no ID");
                return;
            }
            NotificationManager noMa =
                    (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
            noMa.cancel(id);

        } else {
            Log.i(TAG, "received unexpected action: " + intent.getAction());
        }
    }
}
