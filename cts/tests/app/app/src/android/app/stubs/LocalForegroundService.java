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

package android.app.stubs;

import android.app.Notification;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.cts.util.IBinderParcelable;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.app.stubs.R;

public class LocalForegroundService extends LocalService {

    private static final String TAG = "LocalForegroundService";
    private static final String EXTRA_COMMAND = "LocalForegroundService.command";

    public static final int COMMAND_START_FOREGROUND = 1;
    public static final int COMMAND_STOP_FOREGROUND_REMOVE_NOTIFICATION = 2;
    public static final int COMMAND_STOP_FOREGROUND_DONT_REMOVE_NOTIFICATION = 3;
    public static final int COMMAND_STOP_FOREGROUND_DETACH_NOTIFICATION = 4;
    public static final int COMMAND_STOP_FOREGROUND_REMOVE_NOTIFICATION_USING_FLAGS = 5;

    private int mNotificationId = 0;

    @Override
    public void onStart(Intent intent, int startId) {
        super.onStart(intent, startId);

        Context context = getApplicationContext();
        int command = intent.getIntExtra(EXTRA_COMMAND, -1);

        switch (command) {
            case COMMAND_START_FOREGROUND:
                mNotificationId ++;
                Log.d(TAG, "Starting foreground using notification " + mNotificationId);
                Notification notification = new Notification.Builder(context)
                        .setContentTitle(getNotificationTitle(mNotificationId))
                        .setSmallIcon(R.drawable.black)
                        .build();
                startForeground(mNotificationId, notification);
                break;
            case COMMAND_STOP_FOREGROUND_REMOVE_NOTIFICATION:
                Log.d(TAG, "Stopping foreground removing notification");
                stopForeground(true);
                break;
            case COMMAND_STOP_FOREGROUND_DONT_REMOVE_NOTIFICATION:
                Log.d(TAG, "Stopping foreground without removing notification");
                stopForeground(false);
                break;
            case COMMAND_STOP_FOREGROUND_REMOVE_NOTIFICATION_USING_FLAGS:
                Log.d(TAG, "Stopping foreground removing notification using flags");
                stopForeground(Service.STOP_FOREGROUND_REMOVE | Service.STOP_FOREGROUND_DETACH);
                break;
            case COMMAND_STOP_FOREGROUND_DETACH_NOTIFICATION:
                Log.d(TAG, "Detaching foreground service notification");
                stopForeground(Service.STOP_FOREGROUND_DETACH);
                break;
            default:
                Log.e(TAG, "Unknown command: " + command);
        }
    }

    public static Bundle newCommand(IBinder stateReceiver, int command) {
        Bundle bundle = new Bundle();
        bundle.putParcelable(LocalService.REPORT_OBJ_NAME, new IBinderParcelable(stateReceiver));
        bundle.putInt(EXTRA_COMMAND, command);
        return bundle;
    }

    public static String getNotificationTitle(int id) {
        return "I AM FOREGROOT #" + id;
    }
}
