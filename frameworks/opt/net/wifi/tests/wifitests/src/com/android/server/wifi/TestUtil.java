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

package com.android.server.wifi;

import static org.mockito.Mockito.when;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;

import java.lang.reflect.Field;
import java.util.ArrayList;

/**
 * Utils for wifi tests.
 */
public class TestUtil {

    /**
     * Override wifi interface using {@code wifiNative}.
     */
    public static void installWlanWifiNative(WifiNative wifiNative) throws Exception {
        Field field = WifiNative.class.getDeclaredField("wlanNativeInterface");
        field.setAccessible(true);
        field.set(null, wifiNative);

        when(wifiNative.getInterfaceName()).thenReturn("mockWlan");
    }

    /**
     * Send {@link WifiManager#NETWORK_STATE_CHANGED_ACTION} broadcast.
     */
    public static void sendNetworkStateChanged(BroadcastReceiver broadcastReceiver,
            Context context, NetworkInfo.DetailedState detailedState) {
        Intent intent = new Intent(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        NetworkInfo networkInfo = new NetworkInfo(0, 0, "", "");
        networkInfo.setDetailedState(detailedState, "", "");
        intent.putExtra(WifiManager.EXTRA_NETWORK_INFO, networkInfo);
        broadcastReceiver.onReceive(context, intent);
    }

    /**
     * Send {@link WifiManager#SCAN_RESULTS_AVAILABLE_ACTION} broadcast.
     */
    public static void sendScanResultsAvailable(BroadcastReceiver broadcastReceiver,
            Context context) {
        Intent intent = new Intent(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
        broadcastReceiver.onReceive(context, intent);
    }

    /**
     * Send {@link WifiManager#WIFI_SCAN_AVAILABLE} broadcast.
     */
    public static void sendWifiScanAvailable(BroadcastReceiver broadcastReceiver,
            Context context, int scanAvailable) {
        Intent intent = new Intent(WifiManager.WIFI_SCAN_AVAILABLE);
        intent.putExtra(WifiManager.EXTRA_SCAN_AVAILABLE, scanAvailable);
        broadcastReceiver.onReceive(context, intent);
    }

    /**
     * Send {@link WifiManager#WIFI_STATE_CHANGED} broadcast.
     */
    public static void sendWifiStateChanged(BroadcastReceiver broadcastReceiver,
            Context context, int wifiState) {
        Intent intent = new Intent(WifiManager.WIFI_STATE_CHANGED_ACTION);
        intent.putExtra(WifiManager.EXTRA_WIFI_STATE, wifiState);
        broadcastReceiver.onReceive(context, intent);
    }

    /**
     * Send {@link ConnectivityManager#ACTION_TETHER_STATE_CHANGED} broadcast.
     */
    public static void sendTetherStateChanged(BroadcastReceiver broadcastReceiver,
            Context context, ArrayList<String> available, ArrayList<String> active) {
        Intent intent = new Intent(ConnectivityManager.ACTION_TETHER_STATE_CHANGED);
        intent.putExtra(ConnectivityManager.EXTRA_AVAILABLE_TETHER, available);
        intent.putExtra(ConnectivityManager.EXTRA_ACTIVE_TETHER, active);
        broadcastReceiver.onReceive(context, intent);
    }
}
