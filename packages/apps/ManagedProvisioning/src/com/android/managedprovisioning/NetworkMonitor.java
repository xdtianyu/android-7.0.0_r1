/*
 * Copyright 2014, The Android Open Source Project
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

package com.android.managedprovisioning;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.NetworkInfo.DetailedState;

import com.android.managedprovisioning.common.Utils;

/**
 * Monitor the state of the data network and the checkin service. Invoke a callback when the network
 * is connected and checkin has succeeded. Callbacks are made on the thread that created this
 * object.
 */
public class NetworkMonitor {
    /** State notification callback. Expect some duplicate notifications. */
    public interface Callback {

        void onNetworkConnected();

        void onNetworkDisconnected();
    }

    private Context mContext = null;
    private Callback mCallback = null;

    private boolean mNetworkConnected = false;

    private boolean mReceiverRegistered;

    private final Utils mUtils = new Utils();

    /**
     * Start watching the network and monitoring the checkin service. Immediately invokes one of the
     * callback methods to report the current state, and then invokes callback methods over time as
     * the state changes.
     *
     * @param context to use for intent observers and such
     * @param callback to invoke when the network status changes
     */
    public NetworkMonitor(Context context, Callback callback) {
        mContext = context;
        mCallback = callback;

        IntentFilter filter = new IntentFilter();
        filter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);

        // Listen to immediate connectivity changes which are 3 seconds
        // earlier than CONNECTIVITY_ACTION and may not have IPv6 routes
        // setup. However, this may allow us to start up services like
        // the CheckinService a bit earlier.
        filter.addAction(ConnectivityManager.INET_CONDITION_ACTION);

        context.registerReceiver(mBroadcastReceiver, filter);
        mReceiverRegistered = true;

    }

    /**
     * Stop watching the network and checkin service.
     */
    public synchronized void close() {
        if (mCallback == null) {
            return;
        }
        mCallback = null;

        if (mReceiverRegistered) {
            mContext.unregisterReceiver(mBroadcastReceiver);
            mReceiverRegistered = false;
        }
    }

    public final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            ProvisionLogger.logd("onReceive " + intent.toString());

            mNetworkConnected = mUtils.isConnectedToWifi(context);

            if (intent.getAction().equals(ConnectivityManager.CONNECTIVITY_ACTION) ||
                    intent.getAction().equals(ConnectivityManager.INET_CONDITION_ACTION)) {
                if (mNetworkConnected) {
                    mCallback.onNetworkConnected();
                } else {
                    mCallback.onNetworkDisconnected();
                }
            }
        }
    };

    public boolean isNetworkConnected() {
        return mNetworkConnected;
    }
}
