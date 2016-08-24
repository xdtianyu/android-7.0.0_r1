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

package com.android.cts.managedprofile;

import android.content.Context;
import android.content.Intent;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.SystemClock;
import android.test.AndroidTestCase;

import java.util.concurrent.TimeUnit;

import static com.android.compatibility.common.util.WifiConfigCreator.ACTION_CREATE_WIFI_CONFIG;
import static com.android.compatibility.common.util.WifiConfigCreator.ACTION_REMOVE_WIFI_CONFIG;
import static com.android.compatibility.common.util.WifiConfigCreator.EXTRA_NETID;
import static com.android.compatibility.common.util.WifiConfigCreator.EXTRA_SSID;

/**
 * Driven by the host-side test: com.android.cts.devicepolicy.ManagedProfileTest
 *
 * Each of these tests can run independently but have side-effects. The side-effects are used as
 * building blocks to test various cleanup routines, for example that networks belonging to one
 * user are deleted
 */
public class WifiTest extends AndroidTestCase {
    private static final String TAG = WifiTest.class.getSimpleName();

    // Unique SSID to use for this test (max SSID length is 32)
    private static final String NETWORK_SSID = "com.android.cts.xwde7ktvh8rmjuhr";

    // Time duration to allow before assuming that a WiFi operation failed and ceasing to wait.
    private static final long UPDATE_TIMEOUT_MS = TimeUnit.MINUTES.toMillis(5);
    private static final long UPDATE_INTERVAL_MS = TimeUnit.SECONDS.toMillis(1);

    // Shared WifiManager instance.
    private WifiManager mWifiManager;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mWifiManager = (WifiManager) getContext().getSystemService(Context.WIFI_SERVICE);
    }

    /**
     * Add a network through the WifiManager API. Verifies that the network was actually added.
     *
     * <p>Side effects:
     * <ul>
     *   <li>Network with SSID {@link WifiTest#NETWORK_SSID} is created.</li>
     * </ul>
     */
    public void testAddWifiNetwork() throws Exception {
        Intent intent = new Intent(ACTION_CREATE_WIFI_CONFIG);
        intent.putExtra(EXTRA_SSID, NETWORK_SSID);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getContext().startActivity(intent);

        // Wait for configuration to appear in networks list.
        assertTrue(awaitNetworkState(NETWORK_SSID, /* exists */ true));
    }

    /**
     * Remove any network through the WifiManager API with a certain SSID. Verifies that the network
     * was actually removed.
     *
     * <p>Side effects:
     * <ul>
     *   <li>If a network with SSID {@link WifiTest#NETWORK_SSID} exists, it will be deleted.</li>
     * </ul>
     */
    public void testRemoveWifiNetworkIfExists() throws Exception {
        WifiConfiguration config = getNetworkForSsid(NETWORK_SSID);

        if (config != null && config.networkId != -1) {
            Intent intent = new Intent(ACTION_REMOVE_WIFI_CONFIG);
            intent.putExtra(EXTRA_NETID, config.networkId);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            getContext().startActivity(intent);
        }

        assertTrue(awaitNetworkState(NETWORK_SSID, /* exists */ false));
    }

    /**
     * Verify that no network exists with a certain SSID.
     *
     * <p>The SSID that will be checked for is {@link WifiTest#NETWORK_SSID}.
     */
    public void testWifiNetworkDoesNotExist() throws Exception {
        assertTrue(awaitNetworkState(NETWORK_SSID, /* exists */ false));
    }

    /**
     * Block until a network configuration with a certain SSID either exists or ceases to.
     * Wait for up to {@link WifiTest#UPDATE_TIMEOUT_MS} milliseconds, in increments of
     * {@link WifiTest#UPDATE_INTERVAL_MS}.
     */
    private boolean awaitNetworkState(String ssid, boolean exists) {
        for (int probes = 0; probes * UPDATE_INTERVAL_MS <= UPDATE_TIMEOUT_MS; probes++) {
            if (probes != 0) {
                SystemClock.sleep(UPDATE_INTERVAL_MS);
            }
            if ((getNetworkForSsid(ssid) != null) == exists) {
                return true;
            }
        }
        return false;
    }

    /**
     * Internal method to find an existing {@link WifiConfiguration} with the given SSID.
     *
     * @return A {@link WifiConfiguration} matching the specification, or {@code null} if no such
     *         configuration exists.
     */
    private WifiConfiguration getNetworkForSsid(String ssid) {
        if (!ssid.startsWith("\"")) {
            ssid = '"' + ssid + '"';
        }
        for (WifiConfiguration config : mWifiManager.getConfiguredNetworks()) {
            if (ssid.equals(config.SSID)) {
                return config;
            }
        }
        return null;
    }
}
