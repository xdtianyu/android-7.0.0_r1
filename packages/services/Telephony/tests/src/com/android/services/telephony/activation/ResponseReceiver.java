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

package com.android.services.telephony.activation;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import com.android.services.telephony.Log;

public class ResponseReceiver extends BroadcastReceiver {
    volatile public static boolean responseReceived = false;
    public static final String ACTION_ACTIVATION_RESPONSE =
            "com.android.services.telephony.ACTIVATION_RESPONSE";

    private final Object mLock;
    private Context mContext;

    ResponseReceiver(Object lock) {
        mLock = lock;
    }

    /** ${inheritDoc} */
    @Override
    public void onReceive(Context context, Intent intent) {
        if (!ACTION_ACTIVATION_RESPONSE.equals(intent.getAction())) {
            Log.e(this, null, "Unexpected intent: " + intent.getAction());
            return;
        }

        responseReceived = true;
        Log.i(this, "received intent");

        if (mLock != null) {
            synchronized(mLock) {
                Log.i(this, "notifying");
                mLock.notify();
            }
        }
    }

    void register(Context context) {
        context.registerReceiver(this, new IntentFilter(ACTION_ACTIVATION_RESPONSE));
        mContext = context;
    }

    void unregister() {
        mContext.unregisterReceiver(this);
    }
}
