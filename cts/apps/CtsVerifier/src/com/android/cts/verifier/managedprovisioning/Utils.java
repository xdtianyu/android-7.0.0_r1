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
 * limitations under the License.
 */

package com.android.cts.verifier.managedprovisioning;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.widget.Toast;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.android.cts.verifier.IntentDrivenTestActivity;
import com.android.cts.verifier.IntentDrivenTestActivity.ButtonInfo;
import com.android.cts.verifier.managedprovisioning.ByodHelperActivity;
import com.android.cts.verifier.R;
import com.android.cts.verifier.TestListAdapter.TestListItem;

public class Utils {

    private static final String TAG = "CtsVerifierByodUtils";
    static final int BUGREPORT_NOTIFICATION_ID = 12345;

    static TestListItem createInteractiveTestItem(Activity activity, String id, int titleRes,
            int infoRes, ButtonInfo[] buttonInfos) {
        return TestListItem.newTest(activity, titleRes,
                id, new Intent(activity, IntentDrivenTestActivity.class)
                .putExtra(IntentDrivenTestActivity.EXTRA_ID, id)
                .putExtra(IntentDrivenTestActivity.EXTRA_TITLE, titleRes)
                .putExtra(IntentDrivenTestActivity.EXTRA_INFO, infoRes)
                .putExtra(IntentDrivenTestActivity.EXTRA_BUTTONS, buttonInfos),
                null);
    }

    static TestListItem createInteractiveTestItem(Activity activity, String id, int titleRes,
            int infoRes, ButtonInfo buttonInfo) {
        return createInteractiveTestItem(activity, id, titleRes, infoRes,
                new ButtonInfo[] { buttonInfo });
    }

    static void requestDeleteManagedProfile(Context context) {
        try {
            Intent intent = new Intent(ByodHelperActivity.ACTION_REMOVE_MANAGED_PROFILE);
            context.startActivity(intent);
            String message = context.getString(R.string.provisioning_byod_delete_profile);
            Toast.makeText(context, message, Toast.LENGTH_SHORT).show();
        }
        catch (ActivityNotFoundException e) {
            Log.d(TAG, "requestDeleteProfileOwner: ActivityNotFoundException", e);
        }
    }

    static void showBugreportNotification(Context context, String msg, int notificationId) {
        NotificationManager mNotificationManager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        Notification notification = new Notification.Builder(context)
                .setSmallIcon(R.drawable.icon)
                .setContentTitle(context.getString(
                        R.string.device_owner_requesting_bugreport_tests))
                .setContentText(msg)
                .setStyle(new Notification.BigTextStyle().bigText(msg))
                .build();
        mNotificationManager.notify(notificationId, notification);
    }
}
