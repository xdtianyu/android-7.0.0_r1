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

package com.android.tv.settings.connectivity;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.EthernetManager;
import android.net.IpConfiguration;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.net.NetworkInfo;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.text.TextUtils;

import com.android.settingslib.wifi.AccessPoint;
import com.android.settingslib.wifi.WifiTracker;

import java.util.List;

/**
 * Listens for changes to the current connectivity status.
 */
public class ConnectivityListener implements WifiTracker.WifiListener {

    public interface Listener {
        void onConnectivityChange();
    }

    public interface WifiNetworkListener {
        void onWifiListChanged();
    }

    private static final String TAG = "ConnectivityListener";
    private static final boolean DEBUG = false;

    private final Context mContext;
    private final Listener mListener;
    private boolean mStarted;

    private WifiTracker mWifiTracker;

    private final ConnectivityManager mConnectivityManager;
    private final WifiManager mWifiManager;
    private final EthernetManager mEthernetManager;
    private WifiNetworkListener mWifiListener;
    private final BroadcastReceiver mWifiEnabledReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            mListener.onConnectivityChange();
        }
    };
    private final EthernetManager.Listener mEthernetListener = new EthernetManager.Listener() {
        @Override
        public void onAvailabilityChanged(boolean isAvailable) {
            mListener.onConnectivityChange();
        }
    };

    public static class ConnectivityStatus {
        public static final int NETWORK_NONE = 1;
        public static final int NETWORK_WIFI_OPEN = 3;
        public static final int NETWORK_WIFI_SECURE = 5;
        public static final int NETWORK_ETHERNET = 7;

        public int mNetworkType;
        public String mWifiSsid;
        public int mWifiSignalStrength;

        boolean isEthernetConnected() { return mNetworkType == NETWORK_ETHERNET; }
        boolean isWifiConnected() {
            return mNetworkType == NETWORK_WIFI_OPEN ||  mNetworkType == NETWORK_WIFI_SECURE;
        }

        @Override
        public String toString() {
            return
                    "mNetworkType " + mNetworkType +
                    "  miWifiSsid " + mWifiSsid +
                    "  mWifiSignalStrength " + mWifiSignalStrength;
        }
    }

    private final ConnectivityStatus mConnectivityStatus = new ConnectivityStatus();

    public ConnectivityListener(Context context, Listener listener) {
        mContext = context;
        mConnectivityManager = (ConnectivityManager) mContext.getSystemService(
                Context.CONNECTIVITY_SERVICE);
        mWifiManager = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
        mEthernetManager = (EthernetManager) mContext.getSystemService(Context.ETHERNET_SERVICE);
        mListener = listener;
        mWifiTracker = new WifiTracker(context, this, true, true);
    }

    /**
     * Starts {@link ConnectivityListener}.
     * This should be called only from main thread.
     */
    public void start() {
        if (!mStarted) {
            mStarted = true;
            updateConnectivityStatus();
            mWifiTracker.startTracking();
            mContext.registerReceiver(mWifiEnabledReceiver, new IntentFilter(
                    WifiManager.WIFI_STATE_CHANGED_ACTION));
            mEthernetManager.addListener(mEthernetListener);
        }
    }

    /**
     * Stops {@link ConnectivityListener}.
     * This should be called only from main thread.
     */
    public void stop() {
        if (mStarted) {
            mStarted = false;
            mWifiTracker.stopTracking();
            mContext.unregisterReceiver(mWifiEnabledReceiver);
            mWifiListener = null;
            mEthernetManager.removeListener(mEthernetListener);
        }
    }

    public void setWifiListener(WifiNetworkListener wifiListener) {
        mWifiListener = wifiListener;
    }

    public ConnectivityStatus getConnectivityStatus() {
        return mConnectivityStatus;
    }

    public String getWifiIpAddress() {
        if (mConnectivityStatus.isWifiConnected()) {
            WifiInfo wifiInfo = mWifiManager.getConnectionInfo();
            int ip = wifiInfo.getIpAddress();
            return String.format("%d.%d.%d.%d", (ip & 0xff), (ip >> 8 & 0xff),
                    (ip >> 16 & 0xff), (ip >> 24 & 0xff));
        } else {
            return "";
        }
    }

    /**
     * Return the MAC address of the currently connected Wifi AP.
     */
    public String getWifiMacAddress() {
        if (mConnectivityStatus.isWifiConnected()) {
            WifiInfo wifiInfo = mWifiManager.getConnectionInfo();
            return wifiInfo.getMacAddress();
        } else {
            return "";
        }
    }

    /**
     * Return whether Ethernet port is available.
     */
    public boolean isEthernetAvailable() {
        return mConnectivityManager.isNetworkSupported(ConnectivityManager.TYPE_ETHERNET)
                && mEthernetManager.isAvailable();
    }

    private Network getFirstEthernet() {
        final Network[] networks = mConnectivityManager.getAllNetworks();
        for (final Network network : networks) {
            NetworkInfo networkInfo = mConnectivityManager.getNetworkInfo(network);
            if (networkInfo != null && networkInfo.getType() == ConnectivityManager.TYPE_ETHERNET) {
                return network;
            }
        }
        return null;
    }

    public String getEthernetMacAddress() {
        final Network network = getFirstEthernet();
        return network != null ? mConnectivityManager.getNetworkInfo(network).getExtraInfo() : null;
    }

    public String getEthernetIpAddress() {
        final Network network = getFirstEthernet();
        if (network == null) {
            return null;
        }
        final StringBuilder sb = new StringBuilder();
        boolean gotAddress = false;
        final LinkProperties linkProperties = mConnectivityManager.getLinkProperties(network);
        for (LinkAddress linkAddress : linkProperties.getLinkAddresses()) {
            if (gotAddress) {
                sb.append("\n");
            }
            sb.append(linkAddress.getAddress().getHostAddress());
            gotAddress = true;
        }
        if (gotAddress) {
            return sb.toString();
        } else {
            return null;
        }
    }

    public int getWifiSignalStrength(int maxLevel) {
        WifiInfo wifiInfo = mWifiManager.getConnectionInfo();
        return WifiManager.calculateSignalLevel(wifiInfo.getRssi(), maxLevel);
    }

    public void forgetWifiNetwork() {
        int networkId = getWifiNetworkId();
        if (networkId != -1) {
            mWifiManager.forget(networkId, null);
        }
    }

    public int getWifiNetworkId() {
        WifiInfo wifiInfo = mWifiManager.getConnectionInfo();
        if (wifiInfo != null) {
            return wifiInfo.getNetworkId();
        } else {
            return -1;
        }
    }

    public WifiConfiguration getWifiConfiguration() {
        WifiInfo wifiInfo = mWifiManager.getConnectionInfo();
        if (wifiInfo != null) {
            int networkId = wifiInfo.getNetworkId();
            List<WifiConfiguration> configuredNetworks = mWifiManager.getConfiguredNetworks();
            if (configuredNetworks != null) {
                for (WifiConfiguration configuredNetwork : configuredNetworks) {
                    if (configuredNetwork.networkId == networkId) {
                        return configuredNetwork;
                    }
                }
            }
        }
        return null;
    }

    /**
     * Return a list of wifi networks. Ensure that if a wifi network is connected that it appears
     * as the first item on the list.
     */
    public List<AccessPoint> getAvailableNetworks() {
        return mWifiTracker.getAccessPoints();
    }

    public IpConfiguration getIpConfiguration() {
        return mEthernetManager.getConfiguration();
    }

    private boolean isSecureWifi(WifiInfo wifiInfo) {
        if (wifiInfo == null)
            return false;
        int networkId = wifiInfo.getNetworkId();
        List<WifiConfiguration> configuredNetworks = mWifiManager.getConfiguredNetworks();
        if (configuredNetworks != null) {
            for (WifiConfiguration configuredNetwork : configuredNetworks) {
                if (configuredNetwork.networkId == networkId) {
                    return configuredNetwork.allowedKeyManagement.get(KeyMgmt.WPA_PSK) ||
                        configuredNetwork.allowedKeyManagement.get(KeyMgmt.WPA_EAP) ||
                        configuredNetwork.allowedKeyManagement.get(KeyMgmt.IEEE8021X);
                }
            }
        }
        return false;
    }

    public boolean isWifiEnabled() {
        return mWifiManager.isWifiEnabled();
    }

    public void setWifiEnabled(boolean enable) {
        mWifiManager.setWifiEnabled(enable);
    }

    private boolean setNetworkType(int networkType) {
        boolean hasChanged = mConnectivityStatus.mNetworkType != networkType;
        mConnectivityStatus.mNetworkType = networkType;
        return hasChanged;
    }

    private boolean updateConnectivityStatus() {
        NetworkInfo networkInfo = mConnectivityManager.getActiveNetworkInfo();
        if (networkInfo == null) {
            return setNetworkType(ConnectivityStatus.NETWORK_NONE);
        } else {
            switch (networkInfo.getType()) {
                case ConnectivityManager.TYPE_WIFI: {
                    boolean hasChanged;

                    // Determine if this is an open or secure wifi connection.
                    WifiInfo wifiInfo = mWifiManager.getConnectionInfo();
                    if (isSecureWifi(wifiInfo)) {
                        hasChanged = setNetworkType(ConnectivityStatus.NETWORK_WIFI_SECURE);
                    } else {
                        hasChanged = setNetworkType(ConnectivityStatus.NETWORK_WIFI_OPEN);
                    }

                    // Find the SSID of network.
                    String ssid = null;
                    if (wifiInfo != null) {
                        ssid = wifiInfo.getSSID();
                        if (ssid != null) {
                            ssid = WifiInfo.removeDoubleQuotes(ssid);
                        }
                    }
                    if (!TextUtils.equals(mConnectivityStatus.mWifiSsid, ssid)) {
                        hasChanged = true;
                        mConnectivityStatus.mWifiSsid = ssid;
                    }

                    // Calculate the signal strength.
                    int signalStrength;
                    if (wifiInfo != null) {
                        // Calculate the signal strength between 0 and 3.
                        signalStrength = WifiManager.calculateSignalLevel(wifiInfo.getRssi(), 4);
                    } else {
                        signalStrength = 0;
                    }
                    if (mConnectivityStatus.mWifiSignalStrength != signalStrength) {
                        hasChanged = true;
                        mConnectivityStatus.mWifiSignalStrength = signalStrength;
                    }
                    return hasChanged;
                }

                case ConnectivityManager.TYPE_ETHERNET:
                    return setNetworkType(ConnectivityStatus.NETWORK_ETHERNET);

                default:
                    return setNetworkType(ConnectivityStatus.NETWORK_NONE);
            }
        }
    }

    @Override
    public void onWifiStateChanged(int state) {
        mListener.onConnectivityChange();
    }

    @Override
    public void onConnectedChanged() {

    }

    @Override
    public void onAccessPointsChanged() {
        if (mWifiListener != null) {
            mWifiListener.onWifiListChanged();
        }
    }
}
