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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.net.wifi.ScanResult;
import android.net.wifi.WifiSsid;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Log;

import com.android.server.wifi.anqp.ANQPElement;
import com.android.server.wifi.anqp.Constants;
import com.android.server.wifi.hotspot2.ANQPData;
import com.android.server.wifi.hotspot2.AnqpCache;
import com.android.server.wifi.hotspot2.NetworkDetail;

import org.junit.Test;
import org.mockito.Mock;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


/**
 * Unit tests for {@link com.android.server.wifi.hotspot2.AnqpCache}.
 */
@SmallTest
public class AnqpCacheTest {

    private static final String TAG = "AnqpCacheTest";

    private static class NetworkDescription {
        ScanDetail[] mScanDetails;
        static int[] sChannels = new int[]{2412, 2437, 2462, 5180, 5220, 5745, 5825};
        static int[] sRSSIs = new int[]{ -50, -80, -60, -80, -55, -90, -75};

        NetworkDescription(String ssid, String bssidPrefix) {
            WifiSsid wifiSsid = WifiSsid.createFromAsciiEncoded(ssid);
            mScanDetails = new ScanDetail[sChannels.length];
            for (int i = 0; i < sChannels.length; i++) {
                String bssid = String.format("%s:%02x", bssidPrefix, i);
                ScanResult.InformationElement[] ie = new ScanResult.InformationElement[1];
                ie[0] = ScanResults.generateSsidIe(ssid);
                List<String> anqpLines = new ArrayList<String>();
                NetworkDetail nd = new NetworkDetail(bssid, ie,
                        new ArrayList<String>(), sChannels[i]);
                mScanDetails[i] = new ScanDetail(nd, wifiSsid,
                        bssid, "", sRSSIs[i], sChannels[i], Long.MAX_VALUE, ie, anqpLines);
            }
        }
    }

    private static final String ATT_SSID         = "att_wifi";
    private static final String ATT_BSSID_PREFIX = "aa:44:bb:55:cc";
    private static final String TWC_SSID         = "TWCWIFI";
    private static final String TWC_BSSID_PREFIX = "11:aa:22:bb:33";

    private static ScanDetail[] getAttWifiNetworkDescription() {
        NetworkDescription network = new NetworkDescription(ATT_SSID, ATT_BSSID_PREFIX);
        return network.mScanDetails;
    }

    private static ScanDetail[] getTwcWifiNetworkDescription() {
        NetworkDescription network = new NetworkDescription(TWC_SSID, TWC_BSSID_PREFIX);
        return network.mScanDetails;
    }

    private static List<Constants.ANQPElementType> buildQueryList() {
        List<Constants.ANQPElementType> list = Arrays.asList(
                Constants.ANQPElementType.class.getEnumConstants());
        return list;
    }

    private static Map<Constants.ANQPElementType, ANQPElement> buildAnqpResult() {
        Map<Constants.ANQPElementType, ANQPElement> elements = new HashMap<>();
        List<Constants.ANQPElementType> list = Arrays.asList(
                Constants.ANQPElementType.class.getEnumConstants());
        for (final Constants.ANQPElementType type : list) {
            ANQPElement element = new ANQPElement(type) {
                @Override
                public Constants.ANQPElementType getID() {
                    return super.getID();
                }
            };
            elements.put(type, element);
        }

        return elements;
    }

    private void advanceTimeAndTrimCache(long howManyMillis) {
        mCurrentTimeMillis += howManyMillis;
        Log.d(TAG, "Time set to " + mCurrentTimeMillis);
        when(mClock.currentTimeMillis()).thenReturn(mCurrentTimeMillis);
        mCache.clear(false, true);
    }

    public AnqpCacheTest() {}

    private static final long SECOND_MS = 1000;
    private static final long MINUTE_MS = 60 * SECOND_MS;

    @Mock Clock mClock;
    long mCurrentTimeMillis = 1000000000;
    AnqpCache mCache;

    /** verify that ANQP data is cached per the (rather abstract) spec */
    @Test
    public void basicAddQueryAndExpiry() {
        initMocks(this);

        AnqpCache cache = mCache = new AnqpCache(mClock);
        advanceTimeAndTrimCache(0);

        List<Constants.ANQPElementType> queryList = buildQueryList();

        ScanDetail[] attScanDetails = getAttWifiNetworkDescription();
        ScanDetail[] twcScanDetails = getTwcWifiNetworkDescription();

        /* query att network at time 0 */
        for (ScanDetail scanDetail : attScanDetails) {
            cache.initiate(scanDetail.getNetworkDetail(), queryList);
        }

        /* verify that no data can be returned */
        for (ScanDetail scanDetail : attScanDetails) {
            ANQPData data = cache.getEntry(scanDetail.getNetworkDetail());
            assertNull(data);
        }

        /* update ANQP results after 1 min */
        advanceTimeAndTrimCache(1 * MINUTE_MS);

        Map<Constants.ANQPElementType, ANQPElement> anqpResults = buildAnqpResult();

        for (ScanDetail scanDetail : attScanDetails) {
            cache.update(scanDetail.getNetworkDetail(), anqpResults);
        }

        /* check ANQP results after another 1 min */
        advanceTimeAndTrimCache(1 * MINUTE_MS);

        for (ScanDetail scanDetail : attScanDetails) {
            ANQPData data = cache.getEntry(scanDetail.getNetworkDetail());
            assertNotNull(data);
            NetworkDetail nd = data.getNetwork();
            Map<Constants.ANQPElementType, ANQPElement> anqp = data.getANQPElements();
            assertEquals(scanDetail.getBSSIDString(), nd.getBSSIDString());
            assertEquals(anqpResults.size(), anqp.size());
        }

        /* query ANQP results for twcwifi after another 10 min */
        advanceTimeAndTrimCache(10 * MINUTE_MS);

        for (ScanDetail scanDetail : twcScanDetails) {
            cache.initiate(scanDetail.getNetworkDetail(), queryList);
        }

        /* update ANQP results for twcwifi after another 10 min */
        advanceTimeAndTrimCache(1 * MINUTE_MS);

        for (ScanDetail scanDetail : twcScanDetails) {
            cache.update(scanDetail.getNetworkDetail(), anqpResults);
        }

        /* check all results after 1 minute */
        advanceTimeAndTrimCache(1 * MINUTE_MS);

        for (ScanDetail scanDetail : attScanDetails) {
            ANQPData data = cache.getEntry(scanDetail.getNetworkDetail());
            assertNull(data);
        }

        for (ScanDetail scanDetail : twcScanDetails) {
            ANQPData data = cache.getEntry(scanDetail.getNetworkDetail());
            assertNotNull(data);
            NetworkDetail nd = data.getNetwork();
            Map<Constants.ANQPElementType, ANQPElement> anqp = data.getANQPElements();
            assertEquals(scanDetail.getBSSIDString(), nd.getBSSIDString());
            assertEquals(anqpResults.size(), anqp.size());
        }
    }
}































