/*
 * Copyright 2014, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.managedprovisioning.task;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import java.lang.Thread;

import com.android.managedprovisioning.NetworkMonitor;
import com.android.managedprovisioning.ProvisionLogger;
import com.android.managedprovisioning.WifiConfig;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.WifiInfo;

/**
 * Adds a wifi network to system.
 */
public class AddWifiNetworkTask implements NetworkMonitor.Callback {
    private static final int RETRY_SLEEP_DURATION_BASE_MS = 500;
    private static final int RETRY_SLEEP_MULTIPLIER = 2;
    private static final int MAX_RETRIES = 6;
    private static final int RECONNECT_TIMEOUT_MS = 60000;

    private final Context mContext;
    @Nullable
    private final WifiInfo mWifiInfo;
    private final Callback mCallback;

    private WifiManager mWifiManager;
    private NetworkMonitor mNetworkMonitor;
    private WifiConfig mWifiConfig;

    private Handler mHandler;
    private boolean mTaskDone = false;

    private int mDurationNextSleep = RETRY_SLEEP_DURATION_BASE_MS;
    private int mRetriesLeft = MAX_RETRIES;

    private final Utils mUtils = new Utils();

    /**
     * @throws IllegalArgumentException if the {@code ssid} parameter is empty.
     */
    public AddWifiNetworkTask(Context context, WifiInfo wifiInfo, Callback callback) {
        mCallback = callback;
        mContext = context;
        mWifiInfo = wifiInfo;
        mWifiManager  = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
        mWifiConfig = new WifiConfig(mWifiManager);

        HandlerThread thread = new HandlerThread("Timeout thread",
                android.os.Process.THREAD_PRIORITY_BACKGROUND);
        thread.start();
        Looper looper = thread.getLooper();
        mHandler = new Handler(looper);
    }

    public void run() {
        if (mWifiInfo == null) {
            mCallback.onSuccess();
            return;
        }
        if (!enableWifi()) {
            ProvisionLogger.loge("Failed to enable wifi");
            mCallback.onError();
            return;
        }

        if (isConnectedToSpecifiedWifi()) {
            mCallback.onSuccess();
            return;
        }

        mNetworkMonitor = new NetworkMonitor(mContext, this);
        connectToProvidedNetwork();
    }

    private void connectToProvidedNetwork() {
        int netId = mWifiConfig.addNetwork(mWifiInfo.ssid, mWifiInfo.hidden, mWifiInfo.securityType,
                mWifiInfo.password, mWifiInfo.proxyHost, mWifiInfo.proxyPort,
                mWifiInfo.proxyBypassHosts, mWifiInfo.pacUrl);

        if (netId == -1) {
            ProvisionLogger.loge("Failed to save network.");
            if (mRetriesLeft > 0) {
                ProvisionLogger.loge("Retrying in " + mDurationNextSleep + " ms.");
                try {
                    Thread.sleep(mDurationNextSleep);
                } catch (InterruptedException e) {
                    ProvisionLogger.loge("Retry interrupted.");
                }
                mDurationNextSleep *= RETRY_SLEEP_MULTIPLIER;
                mRetriesLeft--;
                connectToProvidedNetwork();
                return;
            } else {
                ProvisionLogger.loge("Already retried " +  MAX_RETRIES + " times."
                        + " Quit retrying and report error.");
                mCallback.onError();
                return;
            }
        }

        // Network was successfully saved, now connect to it.
        if (!mWifiManager.reconnect()) {
            ProvisionLogger.loge("Unable to connect to wifi");
            mCallback.onError();
            return;
        }

        // NetworkMonitor will call onNetworkConnected when in Wifi mode.
        // Post time out event in case the NetworkMonitor doesn't call back.
        mHandler.postDelayed(new Runnable() {
                public void run(){
                    synchronized(this) {
                        if (mTaskDone) return;
                        mTaskDone = true;
                    }
                    ProvisionLogger.loge("Setting up wifi connection timed out.");
                    mCallback.onError();
                    return;
                }
            }, RECONNECT_TIMEOUT_MS);
    }

    private boolean enableWifi() {
        return mWifiManager != null
                && (mWifiManager.isWifiEnabled() || mWifiManager.setWifiEnabled(true));
    }

    @Override
    public void onNetworkConnected() {
        if (isConnectedToSpecifiedWifi()) {
            synchronized(this) {
                if (mTaskDone) return;
                mTaskDone = true;
            }

            ProvisionLogger.logd("Connected to the correct network");

            // Remove time out callback.
            mHandler.removeCallbacksAndMessages(null);

            cleanUp();
            mCallback.onSuccess();
            return;
        }
    }

    @Override
    public void onNetworkDisconnected() {

    }

    public void cleanUp() {
        if (mNetworkMonitor != null) {
            mNetworkMonitor.close();
            mNetworkMonitor = null;
        }
    }

    private boolean isConnectedToSpecifiedWifi() {
        return mUtils.isConnectedToWifi(mContext)
                && mWifiManager != null
                && mWifiManager.getConnectionInfo() != null
                && mWifiInfo.ssid.equals(mWifiManager.getConnectionInfo().getSSID());
    }

    public abstract static class Callback {
        public abstract void onSuccess();
        public abstract void onError();
    }
}
