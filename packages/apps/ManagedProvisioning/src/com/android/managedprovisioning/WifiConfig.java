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

import android.net.ProxyInfo;
import android.net.IpConfiguration.ProxySettings;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.text.TextUtils;

import java.util.Locale;

/**
 * Utility class for configuring a new WiFi network.
 */
public class WifiConfig {

    private final WifiManager mWifiManager;

    enum SecurityType {
        NONE,
        WPA,
        WEP;
    }

    public WifiConfig(WifiManager manager) {
        mWifiManager = manager;
    }

    /**
     * Adds a new WiFi network.
     *
     * @return the ID of the newly created network description. Returns -1 on failure.
     */
    public int addNetwork(String ssid, boolean hidden, String type, String password,
            String proxyHost, int proxyPort, String proxyBypassHosts, String pacUrl) {
        if (!mWifiManager.isWifiEnabled()) {
            mWifiManager.setWifiEnabled(true);
        }

        WifiConfiguration wifiConf = new WifiConfiguration();
        SecurityType securityType;
        if (type == null || TextUtils.isEmpty(type)) {
            securityType = SecurityType.NONE;
        } else {
            try {
                securityType = Enum.valueOf(SecurityType.class, type.toUpperCase(Locale.US));
            } catch (IllegalArgumentException e) {
                ProvisionLogger.loge("Invalid Wifi security type: " + type);
                return -1;
            }
        }
        // If we have a password, and no security type, assume WPA.
        // TODO: Remove this when the programmer supports it.
        if (securityType.equals(SecurityType.NONE) && !TextUtils.isEmpty(password)) {
            securityType = SecurityType.WPA;
        }

        wifiConf.SSID = ssid;
        wifiConf.status = WifiConfiguration.Status.ENABLED;
        wifiConf.hiddenSSID = hidden;
        wifiConf.userApproved = WifiConfiguration.USER_APPROVED;
        switch (securityType) {
            case NONE:
                wifiConf.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
                wifiConf.allowedAuthAlgorithms.set(WifiConfiguration.AuthAlgorithm.OPEN);
                break;
            case WPA:
                updateForWPAConfiguration(wifiConf, password);
                break;
            case WEP:
                updateForWEPConfiguration(wifiConf, password);
                break;
        }

        updateForProxy(wifiConf, proxyHost, proxyPort, proxyBypassHosts, pacUrl);

        int netId = mWifiManager.addNetwork(wifiConf);

        if (netId != -1) {
            // Setting disableOthers to 'true' should trigger a connection attempt.
            mWifiManager.enableNetwork(netId, true);
            mWifiManager.saveConfiguration();
        }

        return netId;
    }

    protected void updateForWPAConfiguration(WifiConfiguration wifiConf, String wifiPassword) {
        wifiConf.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);
        wifiConf.allowedAuthAlgorithms.set(WifiConfiguration.AuthAlgorithm.OPEN);
        wifiConf.allowedProtocols.set(WifiConfiguration.Protocol.WPA); // For WPA
        wifiConf.allowedProtocols.set(WifiConfiguration.Protocol.RSN); // For WPA2
        wifiConf.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.TKIP);
        wifiConf.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.CCMP);
        wifiConf.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.TKIP);
        wifiConf.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.CCMP);
        if (!TextUtils.isEmpty(wifiPassword)) {
            wifiConf.preSharedKey = "\"" + wifiPassword + "\"";
        }
    }

    protected void updateForWEPConfiguration(WifiConfiguration wifiConf, String password) {
        wifiConf.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
        wifiConf.allowedAuthAlgorithms.set(WifiConfiguration.AuthAlgorithm.OPEN);
        wifiConf.allowedAuthAlgorithms.set(WifiConfiguration.AuthAlgorithm.SHARED);
        wifiConf.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.WEP40);
        wifiConf.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.WEP104);
        wifiConf.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.TKIP);
        wifiConf.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.CCMP);
        int length = password.length();
        if ((length == 10 || length == 26 || length == 58) && password.matches("[0-9A-Fa-f]*")) {
            wifiConf.wepKeys[0] = password;
        } else {
            wifiConf.wepKeys[0] = '"' + password + '"';
        }
        wifiConf.wepTxKeyIndex = 0;
    }

    private void updateForProxy(WifiConfiguration wifiConf, String proxyHost, int proxyPort,
            String proxyBypassHosts, String pacUrl) {
        if (TextUtils.isEmpty(proxyHost) && TextUtils.isEmpty(pacUrl)) {
            return;
        }
        if (!TextUtils.isEmpty(proxyHost)) {
            ProxyInfo proxy = new ProxyInfo(proxyHost, proxyPort, proxyBypassHosts);
            wifiConf.setProxy(ProxySettings.STATIC, proxy);
        } else {
            ProxyInfo proxy = new ProxyInfo(pacUrl);
            wifiConf.setProxy(ProxySettings.PAC, proxy);
        }
    }
}
