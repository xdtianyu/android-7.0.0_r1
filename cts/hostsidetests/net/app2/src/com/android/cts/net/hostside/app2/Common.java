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
package com.android.cts.net.hostside.app2;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;

public final class Common {

    static final String TAG = "CtsNetApp2";

    // Constants below must match values defined on app's
    // AbstractRestrictBackgroundNetworkTestCase.java
    static final String MANIFEST_RECEIVER = "ManifestReceiver";
    static final String DYNAMIC_RECEIVER = "DynamicReceiver";
    static final String ACTION_GET_COUNTERS =
            "com.android.cts.net.hostside.app2.action.GET_COUNTERS";
    static final String ACTION_GET_RESTRICT_BACKGROUND_STATUS =
            "com.android.cts.net.hostside.app2.action.GET_RESTRICT_BACKGROUND_STATUS";
    static final String ACTION_CHECK_NETWORK =
            "com.android.cts.net.hostside.app2.action.CHECK_NETWORK";
    static final String ACTION_RECEIVER_READY =
            "com.android.cts.net.hostside.app2.action.RECEIVER_READY";
    static final String ACTION_FINISH_ACTIVITY =
            "com.android.cts.net.hostside.app2.action.FINISH_ACTIVITY";
    static final String ACTION_SEND_NOTIFICATION =
            "com.android.cts.net.hostside.app2.action.SEND_NOTIFICATION";
    static final String EXTRA_ACTION = "com.android.cts.net.hostside.app2.extra.ACTION";
    static final String EXTRA_RECEIVER_NAME =
            "com.android.cts.net.hostside.app2.extra.RECEIVER_NAME";
    static final String EXTRA_NOTIFICATION_ID =
            "com.android.cts.net.hostside.app2.extra.NOTIFICATION_ID";

    static int getUid(Context context) {
        final String packageName = context.getPackageName();
        try {
            return context.getPackageManager().getPackageUid(packageName, 0);
        } catch (NameNotFoundException e) {
            throw new IllegalStateException("Could not get UID for " + packageName, e);
        }
    }
}
