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
 * limitations under the License.
 */
package com.android.cts.net.hostside;

import android.app.Notification;
import android.app.PendingIntent.CanceledException;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.util.Log;

/**
 * NotificationListenerService implementation that executes the notification actions once they're
 * created.
 */
public class MyNotificationListenerService extends NotificationListenerService {
    private static final String TAG = "MyNotificationListenerService";

    @Override
    public void onListenerConnected() {
        Log.d(TAG, "onListenerConnected()");
    }

    @Override
    public void onNotificationPosted(StatusBarNotification sbn) {
        Log.d(TAG, "onNotificationPosted(): "  + sbn);
        if (!sbn.getPackageName().startsWith(getPackageName())) {
            Log.v(TAG, "ignoring notification from a different package");
            return;
        }
        final Notification notification = sbn.getNotification();
        if (notification.actions == null) {
            Log.w(TAG, "ignoring notification without an action");
        }
        for (Notification.Action action : notification.actions) {
            Log.i(TAG, "Sending pending intent " + action.actionIntent);
            try {
                action.actionIntent.send();
            } catch (CanceledException e) {
                Log.w(TAG, "Pending Intent canceled");
            }
        }
    }

    static String getId() {
        return String.format("%s/%s", MyNotificationListenerService.class.getPackage().getName(),
                MyNotificationListenerService.class.getName());
    }
}
