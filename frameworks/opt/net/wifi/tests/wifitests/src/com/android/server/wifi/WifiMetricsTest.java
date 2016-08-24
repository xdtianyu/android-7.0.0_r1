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
 * limitations under the License
 */
package com.android.server.wifi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.*;

import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Base64;

import com.android.server.wifi.hotspot2.NetworkDetail;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.io.ByteArrayOutputStream;
import java.io.PrintWriter;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Unit tests for {@link com.android.server.wifi.WifiMetrics}.
 */
@SmallTest
public class WifiMetricsTest {

    WifiMetrics mWifiMetrics;
    WifiMetricsProto.WifiLog mDeserializedWifiMetrics;
    @Mock Clock mClock;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mDeserializedWifiMetrics = null;
        when(mClock.elapsedRealtime()).thenReturn((long) 0);
        mWifiMetrics = new WifiMetrics(mClock);
    }

    /**
     * Test that startConnectionEvent and endConnectionEvent can be called repeatedly and out of
     * order. Only tests no exception occurs. Creates 3 ConnectionEvents.
     */
    @Test
    public void startAndEndConnectionEventSucceeds() throws Exception {
        //Start and end Connection event
        mWifiMetrics.startConnectionEvent(null, "RED",
                WifiMetricsProto.ConnectionEvent.ROAM_ENTERPRISE);
        mWifiMetrics.endConnectionEvent(
                WifiMetrics.ConnectionEvent.FAILURE_AUTHENTICATION_FAILURE,
                WifiMetricsProto.ConnectionEvent.HLF_DHCP);
        //end Connection event without starting one
        mWifiMetrics.endConnectionEvent(
                WifiMetrics.ConnectionEvent.FAILURE_AUTHENTICATION_FAILURE,
                WifiMetricsProto.ConnectionEvent.HLF_DHCP);
        //start two ConnectionEvents in a row
        mWifiMetrics.startConnectionEvent(null, "BLUE",
                WifiMetricsProto.ConnectionEvent.ROAM_ENTERPRISE);
        mWifiMetrics.startConnectionEvent(null, "GREEN",
                WifiMetricsProto.ConnectionEvent.ROAM_ENTERPRISE);
    }

    private static final long TEST_RECORD_DURATION_SEC = 12 * 60 * 60;
    private static final long TEST_RECORD_DURATION_MILLIS = TEST_RECORD_DURATION_SEC * 1000;

    /**
     * Simulate how dumpsys gets the proto from mWifiMetrics, filter the proto bytes out and
     * deserialize them into mDeserializedWifiMetrics
     */
    public void dumpProtoAndDeserialize() throws Exception {
        ByteArrayOutputStream stream = new ByteArrayOutputStream();
        PrintWriter writer = new PrintWriter(stream);
        String[] args = new String[0];

        when(mClock.elapsedRealtime()).thenReturn(TEST_RECORD_DURATION_MILLIS);
        //Test proto dump, by passing in proto arg option
        args = new String[]{WifiMetrics.PROTO_DUMP_ARG};
        mWifiMetrics.dump(null, writer, args);
        writer.flush();
        Pattern pattern = Pattern.compile(
                "(?<=WifiMetrics:\\n)([\\s\\S]*)(?=EndWifiMetrics)");
        Matcher matcher = pattern.matcher(stream.toString());
        assertTrue("Proto Byte string found in WifiMetrics.dump():\n" + stream.toString(),
                matcher.find());
        String protoByteString = matcher.group(1);
        byte[] protoBytes = Base64.decode(protoByteString, Base64.DEFAULT);
        mDeserializedWifiMetrics = WifiMetricsProto.WifiLog.parseFrom(protoBytes);
    }

    @Test
    public void dumpHumanReadable() throws Exception {
        ByteArrayOutputStream stream = new ByteArrayOutputStream();
        PrintWriter writer = new PrintWriter(stream);
        String[] args = new String[0];
        mWifiMetrics.dump(null, writer, args);
        writer.flush();
        assertTrue("stream.toString().contains(\"WifiMetrics\")",
                stream.toString().contains("WifiMetrics"));
    }

    @Test
    public void testDumpProtoAndDeserialize() throws Exception {
        setAndIncrementMetrics();
        dumpProtoAndDeserialize();
        assertDeserializedMetricsCorrect();
    }

    private static final int NUM_SAVED_NETWORKS = 1;
    private static final int NUM_OPEN_NETWORKS = 2;
    private static final int NUM_PERSONAL_NETWORKS = 3;
    private static final int NUM_ENTERPRISE_NETWORKS = 5;
    private static final boolean TEST_VAL_IS_LOCATION_ENABLED = true;
    private static final boolean IS_SCANNING_ALWAYS_ENABLED = true;
    private static final int NUM_NEWTORKS_ADDED_BY_USER = 13;
    private static final int NUM_NEWTORKS_ADDED_BY_APPS = 17;
    private static final int NUM_EMPTY_SCAN_RESULTS = 19;
    private static final int NUM_NON_EMPTY_SCAN_RESULTS = 23;
    private static final int NUM_SCAN_UNKNOWN = 1;
    private static final int NUM_SCAN_SUCCESS = 2;
    private static final int NUM_SCAN_FAILURE_INTERRUPTED = 3;
    private static final int NUM_SCAN_FAILURE_INVALID_CONFIGURATION = 5;
    private static final int NUM_WIFI_UNKNOWN_SCREEN_OFF = 3;
    private static final int NUM_WIFI_UNKNOWN_SCREEN_ON = 5;
    private static final int NUM_WIFI_ASSOCIATED_SCREEN_OFF = 7;
    private static final int NUM_WIFI_ASSOCIATED_SCREEN_ON = 11;
    private static final int NUM_CONNECTIVITY_WATCHDOG_PNO_GOOD = 11;
    private static final int NUM_CONNECTIVITY_WATCHDOG_PNO_BAD = 12;
    private static final int NUM_CONNECTIVITY_WATCHDOG_BACKGROUND_GOOD = 13;
    private static final int NUM_CONNECTIVITY_WATCHDOG_BACKGROUND_BAD = 14;
    private static final int NUM_LAST_RESORT_WATCHDOG_TRIGGERS = 1;
    private static final int NUM_LAST_RESORT_WATCHDOG_BAD_ASSOCIATION_NETWORKS_TOTAL = 2;
    private static final int NUM_LAST_RESORT_WATCHDOG_BAD_AUTHENTICATION_NETWORKS_TOTAL = 3;
    private static final int NUM_LAST_RESORT_WATCHDOG_BAD_DHCP_NETWORKS_TOTAL = 4;
    private static final int NUM_LAST_RESORT_WATCHDOG_BAD_OTHER_NETWORKS_TOTAL = 5;
    private static final int NUM_LAST_RESORT_WATCHDOG_AVAILABLE_NETWORKS_TOTAL = 6;
    private static final int NUM_LAST_RESORT_WATCHDOG_TRIGGERS_WITH_BAD_ASSOCIATION = 7;
    private static final int NUM_LAST_RESORT_WATCHDOG_TRIGGERS_WITH_BAD_AUTHENTICATION = 8;
    private static final int NUM_LAST_RESORT_WATCHDOG_TRIGGERS_WITH_BAD_DHCP = 9;
    private static final int NUM_LAST_RESORT_WATCHDOG_TRIGGERS_WITH_BAD_OTHER = 10;
    /**
     * Set simple metrics, increment others
     */
    public void setAndIncrementMetrics() throws Exception {
        mWifiMetrics.setNumSavedNetworks(NUM_SAVED_NETWORKS);
        mWifiMetrics.setNumOpenNetworks(NUM_OPEN_NETWORKS);
        mWifiMetrics.setNumPersonalNetworks(NUM_PERSONAL_NETWORKS);
        mWifiMetrics.setNumEnterpriseNetworks(NUM_ENTERPRISE_NETWORKS);
        mWifiMetrics.setNumNetworksAddedByUser(NUM_NEWTORKS_ADDED_BY_USER);
        mWifiMetrics.setNumNetworksAddedByApps(NUM_NEWTORKS_ADDED_BY_APPS);
        mWifiMetrics.setIsLocationEnabled(TEST_VAL_IS_LOCATION_ENABLED);
        mWifiMetrics.setIsScanningAlwaysEnabled(IS_SCANNING_ALWAYS_ENABLED);

        for (int i = 0; i < NUM_EMPTY_SCAN_RESULTS; i++) {
            mWifiMetrics.incrementEmptyScanResultCount();
        }
        for (int i = 0; i < NUM_NON_EMPTY_SCAN_RESULTS; i++) {
            mWifiMetrics.incrementNonEmptyScanResultCount();
        }
        mWifiMetrics.incrementScanReturnEntry(WifiMetricsProto.WifiLog.SCAN_UNKNOWN,
                NUM_SCAN_UNKNOWN);
        mWifiMetrics.incrementScanReturnEntry(WifiMetricsProto.WifiLog.SCAN_SUCCESS,
                NUM_SCAN_SUCCESS);
        mWifiMetrics.incrementScanReturnEntry(
                WifiMetricsProto.WifiLog.SCAN_FAILURE_INTERRUPTED,
                NUM_SCAN_FAILURE_INTERRUPTED);
        mWifiMetrics.incrementScanReturnEntry(
                WifiMetricsProto.WifiLog.SCAN_FAILURE_INVALID_CONFIGURATION,
                NUM_SCAN_FAILURE_INVALID_CONFIGURATION);
        for (int i = 0; i < NUM_WIFI_UNKNOWN_SCREEN_OFF; i++) {
            mWifiMetrics.incrementWifiSystemScanStateCount(WifiMetricsProto.WifiLog.WIFI_UNKNOWN,
                    false);
        }
        for (int i = 0; i < NUM_WIFI_UNKNOWN_SCREEN_ON; i++) {
            mWifiMetrics.incrementWifiSystemScanStateCount(WifiMetricsProto.WifiLog.WIFI_UNKNOWN,
                    true);
        }
        for (int i = 0; i < NUM_WIFI_ASSOCIATED_SCREEN_OFF; i++) {
            mWifiMetrics.incrementWifiSystemScanStateCount(WifiMetricsProto.WifiLog.WIFI_ASSOCIATED,
                    false);
        }
        for (int i = 0; i < NUM_WIFI_ASSOCIATED_SCREEN_ON; i++) {
            mWifiMetrics.incrementWifiSystemScanStateCount(WifiMetricsProto.WifiLog.WIFI_ASSOCIATED,
                    true);
        }
        for (int i = 0; i < NUM_CONNECTIVITY_WATCHDOG_PNO_GOOD; i++) {
            mWifiMetrics.incrementNumConnectivityWatchdogPnoGood();
        }
        for (int i = 0; i < NUM_CONNECTIVITY_WATCHDOG_PNO_BAD; i++) {
            mWifiMetrics.incrementNumConnectivityWatchdogPnoBad();
        }
        for (int i = 0; i < NUM_CONNECTIVITY_WATCHDOG_BACKGROUND_GOOD; i++) {
            mWifiMetrics.incrementNumConnectivityWatchdogBackgroundGood();
        }
        for (int i = 0; i < NUM_CONNECTIVITY_WATCHDOG_BACKGROUND_BAD; i++) {
            mWifiMetrics.incrementNumConnectivityWatchdogBackgroundBad();
        }
        for (int i = 0; i < NUM_LAST_RESORT_WATCHDOG_TRIGGERS; i++) {
            mWifiMetrics.incrementNumLastResortWatchdogTriggers();
        }
        mWifiMetrics.addCountToNumLastResortWatchdogBadAssociationNetworksTotal(
                NUM_LAST_RESORT_WATCHDOG_BAD_ASSOCIATION_NETWORKS_TOTAL);
        mWifiMetrics.addCountToNumLastResortWatchdogBadAuthenticationNetworksTotal(
                NUM_LAST_RESORT_WATCHDOG_BAD_AUTHENTICATION_NETWORKS_TOTAL);
        mWifiMetrics.addCountToNumLastResortWatchdogBadDhcpNetworksTotal(
                NUM_LAST_RESORT_WATCHDOG_BAD_DHCP_NETWORKS_TOTAL);
        mWifiMetrics.addCountToNumLastResortWatchdogBadOtherNetworksTotal(
                NUM_LAST_RESORT_WATCHDOG_BAD_OTHER_NETWORKS_TOTAL);
        mWifiMetrics.addCountToNumLastResortWatchdogAvailableNetworksTotal(
                NUM_LAST_RESORT_WATCHDOG_AVAILABLE_NETWORKS_TOTAL);
        for (int i = 0; i < NUM_LAST_RESORT_WATCHDOG_TRIGGERS_WITH_BAD_ASSOCIATION; i++) {
            mWifiMetrics.incrementNumLastResortWatchdogTriggersWithBadAssociation();
        }
        for (int i = 0; i < NUM_LAST_RESORT_WATCHDOG_TRIGGERS_WITH_BAD_AUTHENTICATION; i++) {
            mWifiMetrics.incrementNumLastResortWatchdogTriggersWithBadAuthentication();
        }
        for (int i = 0; i < NUM_LAST_RESORT_WATCHDOG_TRIGGERS_WITH_BAD_DHCP; i++) {
            mWifiMetrics.incrementNumLastResortWatchdogTriggersWithBadDhcp();
        }
        for (int i = 0; i < NUM_LAST_RESORT_WATCHDOG_TRIGGERS_WITH_BAD_OTHER; i++) {
            mWifiMetrics.incrementNumLastResortWatchdogTriggersWithBadOther();
        }
    }

    /**
     * Assert that values in deserializedWifiMetrics match those set in 'setAndIncrementMetrics'
     */
    public void assertDeserializedMetricsCorrect() throws Exception {
        assertEquals("mDeserializedWifiMetrics.numSavedNetworks == NUM_SAVED_NETWORKS",
                mDeserializedWifiMetrics.numSavedNetworks, NUM_SAVED_NETWORKS);
        assertEquals("mDeserializedWifiMetrics.numOpenNetworks == NUM_OPEN_NETWORKS",
                mDeserializedWifiMetrics.numOpenNetworks, NUM_OPEN_NETWORKS);
        assertEquals("mDeserializedWifiMetrics.numPersonalNetworks == NUM_PERSONAL_NETWORKS",
                mDeserializedWifiMetrics.numPersonalNetworks, NUM_PERSONAL_NETWORKS);
        assertEquals("mDeserializedWifiMetrics.numEnterpriseNetworks "
                        + "== NUM_ENTERPRISE_NETWORKS",
                mDeserializedWifiMetrics.numEnterpriseNetworks, NUM_ENTERPRISE_NETWORKS);
        assertEquals("mDeserializedWifiMetrics.numNetworksAddedByUser "
                        + "== NUM_NEWTORKS_ADDED_BY_USER",
                mDeserializedWifiMetrics.numNetworksAddedByUser, NUM_NEWTORKS_ADDED_BY_USER);
        assertEquals("mDeserializedWifiMetrics.numNetworksAddedByApps "
                        + "== NUM_NEWTORKS_ADDED_BY_APPS",
                mDeserializedWifiMetrics.numNetworksAddedByApps, NUM_NEWTORKS_ADDED_BY_APPS);
        assertEquals("mDeserializedWifiMetrics.isLocationEnabled == TEST_VAL_IS_LOCATION_ENABLED",
                mDeserializedWifiMetrics.isLocationEnabled, TEST_VAL_IS_LOCATION_ENABLED);
        assertEquals("mDeserializedWifiMetrics.isScanningAlwaysEnabled "
                        + "== IS_SCANNING_ALWAYS_ENABLED",
                mDeserializedWifiMetrics.isScanningAlwaysEnabled, IS_SCANNING_ALWAYS_ENABLED);
        assertEquals("mDeserializedWifiMetrics.numEmptyScanResults == NUM_EMPTY_SCAN_RESULTS",
                mDeserializedWifiMetrics.numEmptyScanResults, NUM_EMPTY_SCAN_RESULTS);
        assertEquals("mDeserializedWifiMetrics.numNonEmptyScanResults == "
                        + "NUM_NON_EMPTY_SCAN_RESULTS",
                mDeserializedWifiMetrics.numNonEmptyScanResults, NUM_NON_EMPTY_SCAN_RESULTS);
        assertScanReturnEntryEquals(WifiMetricsProto.WifiLog.SCAN_UNKNOWN,
                NUM_SCAN_UNKNOWN);
        assertScanReturnEntryEquals(WifiMetricsProto.WifiLog.SCAN_SUCCESS,
                NUM_SCAN_SUCCESS);
        assertScanReturnEntryEquals(WifiMetricsProto.WifiLog.SCAN_FAILURE_INTERRUPTED,
                NUM_SCAN_FAILURE_INTERRUPTED);
        assertScanReturnEntryEquals(WifiMetricsProto.WifiLog.SCAN_FAILURE_INVALID_CONFIGURATION,
                NUM_SCAN_FAILURE_INVALID_CONFIGURATION);
        assertSystemStateEntryEquals(WifiMetricsProto.WifiLog.WIFI_UNKNOWN, false,
                NUM_WIFI_UNKNOWN_SCREEN_OFF);
        assertSystemStateEntryEquals(WifiMetricsProto.WifiLog.WIFI_UNKNOWN, true,
                NUM_WIFI_UNKNOWN_SCREEN_ON);
        assertSystemStateEntryEquals(
                WifiMetricsProto.WifiLog.WIFI_ASSOCIATED, false, NUM_WIFI_ASSOCIATED_SCREEN_OFF);
        assertSystemStateEntryEquals(WifiMetricsProto.WifiLog.WIFI_ASSOCIATED, true,
                NUM_WIFI_ASSOCIATED_SCREEN_ON);
        assertEquals(mDeserializedWifiMetrics.numConnectivityWatchdogPnoGood,
                NUM_CONNECTIVITY_WATCHDOG_PNO_GOOD);
        assertEquals(mDeserializedWifiMetrics.numConnectivityWatchdogPnoBad,
                NUM_CONNECTIVITY_WATCHDOG_PNO_BAD);
        assertEquals(mDeserializedWifiMetrics.numConnectivityWatchdogBackgroundGood,
                NUM_CONNECTIVITY_WATCHDOG_BACKGROUND_GOOD);
        assertEquals(mDeserializedWifiMetrics.numConnectivityWatchdogBackgroundBad,
                NUM_CONNECTIVITY_WATCHDOG_BACKGROUND_BAD);
        assertEquals(NUM_LAST_RESORT_WATCHDOG_TRIGGERS,
                mDeserializedWifiMetrics.numLastResortWatchdogTriggers);
        assertEquals(NUM_LAST_RESORT_WATCHDOG_BAD_ASSOCIATION_NETWORKS_TOTAL,
                mDeserializedWifiMetrics.numLastResortWatchdogBadAssociationNetworksTotal);
        assertEquals(NUM_LAST_RESORT_WATCHDOG_BAD_AUTHENTICATION_NETWORKS_TOTAL,
                mDeserializedWifiMetrics.numLastResortWatchdogBadAuthenticationNetworksTotal);
        assertEquals(NUM_LAST_RESORT_WATCHDOG_BAD_DHCP_NETWORKS_TOTAL,
                mDeserializedWifiMetrics.numLastResortWatchdogBadDhcpNetworksTotal);
        assertEquals(NUM_LAST_RESORT_WATCHDOG_BAD_OTHER_NETWORKS_TOTAL,
                mDeserializedWifiMetrics.numLastResortWatchdogBadOtherNetworksTotal);
        assertEquals(NUM_LAST_RESORT_WATCHDOG_AVAILABLE_NETWORKS_TOTAL,
                mDeserializedWifiMetrics.numLastResortWatchdogAvailableNetworksTotal);
        assertEquals(NUM_LAST_RESORT_WATCHDOG_TRIGGERS_WITH_BAD_ASSOCIATION,
                mDeserializedWifiMetrics.numLastResortWatchdogTriggersWithBadAssociation);
        assertEquals(NUM_LAST_RESORT_WATCHDOG_TRIGGERS_WITH_BAD_AUTHENTICATION,
                mDeserializedWifiMetrics.numLastResortWatchdogTriggersWithBadAuthentication);
        assertEquals(NUM_LAST_RESORT_WATCHDOG_TRIGGERS_WITH_BAD_DHCP,
                mDeserializedWifiMetrics.numLastResortWatchdogTriggersWithBadDhcp);
        assertEquals(NUM_LAST_RESORT_WATCHDOG_TRIGGERS_WITH_BAD_OTHER,
                mDeserializedWifiMetrics.numLastResortWatchdogTriggersWithBadOther);
        assertEquals(TEST_RECORD_DURATION_SEC,
                mDeserializedWifiMetrics.recordDurationSec);
    }

    /**
     *  Assert deserialized metrics Scan Return Entry equals count
     */
    public void assertScanReturnEntryEquals(int returnCode, int count) {
        for (int i = 0; i < mDeserializedWifiMetrics.scanReturnEntries.length; i++) {
            if (mDeserializedWifiMetrics.scanReturnEntries[i].scanReturnCode == returnCode) {
                assertEquals(mDeserializedWifiMetrics.scanReturnEntries[i].scanResultsCount, count);
                return;
            }
        }
        assertEquals(null, count);
    }

    /**
     *  Assert deserialized metrics SystemState entry equals count
     */
    public void assertSystemStateEntryEquals(int state, boolean screenOn, int count) {
        for (int i = 0; i < mDeserializedWifiMetrics.wifiSystemStateEntries.length; i++) {
            if (mDeserializedWifiMetrics.wifiSystemStateEntries[i].wifiState == state
                    && mDeserializedWifiMetrics.wifiSystemStateEntries[i].isScreenOn == screenOn) {
                assertEquals(mDeserializedWifiMetrics.wifiSystemStateEntries[i].wifiStateCount,
                        count);
                return;
            }
        }
        assertEquals(null, count);
    }
    /**
     * Combination of all other WifiMetrics unit tests, an internal-integration test, or functional
     * test
     */
    @Test
    public void setMetricsSerializeDeserializeAssertMetricsSame() throws Exception {
        setAndIncrementMetrics();
        startAndEndConnectionEventSucceeds();
        dumpProtoAndDeserialize();
        assertDeserializedMetricsCorrect();
        assertEquals("mDeserializedWifiMetrics.connectionEvent.length",
                2, mDeserializedWifiMetrics.connectionEvent.length);
        //<TODO> test individual connectionEvents for correctness,
        // check scanReturnEntries & wifiSystemStateEntries counts and individual elements
        // pending their implementation</TODO>
    }

    private static final String SSID = "red";
    private static final int CONFIG_DTIM = 3;
    private static final int NETWORK_DETAIL_WIFIMODE = 5;
    private static final int NETWORK_DETAIL_DTIM = 7;
    private static final int SCAN_RESULT_LEVEL = -30;
    /**
     * Test that WifiMetrics is correctly getting data from ScanDetail and WifiConfiguration
     */
    @Test
    public void testScanDetailAndWifiConfigurationUsage() throws Exception {
        //Setup mock configs and scan details
        NetworkDetail networkDetail = mock(NetworkDetail.class);
        when(networkDetail.getWifiMode()).thenReturn(NETWORK_DETAIL_WIFIMODE);
        when(networkDetail.getSSID()).thenReturn(SSID);
        when(networkDetail.getDtimInterval()).thenReturn(NETWORK_DETAIL_DTIM);
        ScanResult scanResult = mock(ScanResult.class);
        scanResult.level = SCAN_RESULT_LEVEL;
        WifiConfiguration config = mock(WifiConfiguration.class);
        config.SSID = "\"" + SSID + "\"";
        config.dtimInterval = CONFIG_DTIM;
        WifiConfiguration.NetworkSelectionStatus networkSelectionStat =
                mock(WifiConfiguration.NetworkSelectionStatus.class);
        when(networkSelectionStat.getCandidate()).thenReturn(scanResult);
        when(config.getNetworkSelectionStatus()).thenReturn(networkSelectionStat);
        ScanDetail scanDetail = mock(ScanDetail.class);
        when(scanDetail.getNetworkDetail()).thenReturn(networkDetail);
        when(scanDetail.getScanResult()).thenReturn(scanResult);

        //Create a connection event using only the config
        mWifiMetrics.startConnectionEvent(config, "Red",
                WifiMetricsProto.ConnectionEvent.ROAM_NONE);
        mWifiMetrics.endConnectionEvent(
                WifiMetrics.ConnectionEvent.FAILURE_NONE,
                WifiMetricsProto.ConnectionEvent.HLF_NONE);

        //Create a connection event using the config and a scan detail
        mWifiMetrics.startConnectionEvent(config, "Green",
                WifiMetricsProto.ConnectionEvent.ROAM_NONE);
        mWifiMetrics.setConnectionScanDetail(scanDetail);
        mWifiMetrics.endConnectionEvent(
                WifiMetrics.ConnectionEvent.FAILURE_NONE,
                WifiMetricsProto.ConnectionEvent.HLF_NONE);

        //Dump proto from mWifiMetrics and deserialize it to mDeserializedWifiMetrics
        dumpProtoAndDeserialize();

        //Check that the correct values are being flowed through
        assertEquals(mDeserializedWifiMetrics.connectionEvent.length, 2);
        assertEquals(mDeserializedWifiMetrics.connectionEvent[0].routerFingerprint.dtim,
                CONFIG_DTIM);
        assertEquals(mDeserializedWifiMetrics.connectionEvent[0].signalStrength, SCAN_RESULT_LEVEL);
        assertEquals(mDeserializedWifiMetrics.connectionEvent[1].routerFingerprint.dtim,
                NETWORK_DETAIL_DTIM);
        assertEquals(mDeserializedWifiMetrics.connectionEvent[1].signalStrength,
                SCAN_RESULT_LEVEL);
        assertEquals(mDeserializedWifiMetrics.connectionEvent[1].routerFingerprint.routerTechnology,
                NETWORK_DETAIL_WIFIMODE);
    }

    /**
     * Test that WifiMetrics is being cleared after dumping via proto
     */
    @Test
    public void testMetricsClearedAfterProtoRequested() throws Exception {
        // Create 3 ConnectionEvents
        mWifiMetrics.startConnectionEvent(null, "RED",
                WifiMetricsProto.ConnectionEvent.ROAM_ENTERPRISE);
        mWifiMetrics.endConnectionEvent(
                WifiMetrics.ConnectionEvent.FAILURE_NONE,
                WifiMetricsProto.ConnectionEvent.HLF_NONE);
        mWifiMetrics.startConnectionEvent(null, "YELLOW",
                WifiMetricsProto.ConnectionEvent.ROAM_ENTERPRISE);
        mWifiMetrics.endConnectionEvent(
                WifiMetrics.ConnectionEvent.FAILURE_NONE,
                WifiMetricsProto.ConnectionEvent.HLF_NONE);
        mWifiMetrics.startConnectionEvent(null, "GREEN",
                WifiMetricsProto.ConnectionEvent.ROAM_ENTERPRISE);
        mWifiMetrics.endConnectionEvent(
                WifiMetrics.ConnectionEvent.FAILURE_NONE,
                WifiMetricsProto.ConnectionEvent.HLF_NONE);
        mWifiMetrics.startConnectionEvent(null, "ORANGE",
                WifiMetricsProto.ConnectionEvent.ROAM_ENTERPRISE);
        mWifiMetrics.endConnectionEvent(
                WifiMetrics.ConnectionEvent.FAILURE_NONE,
                WifiMetricsProto.ConnectionEvent.HLF_NONE);

        //Dump proto and deserialize
        //This should clear all the metrics in mWifiMetrics,
        dumpProtoAndDeserialize();
        //Check there are only 3 connection events
        assertEquals(mDeserializedWifiMetrics.connectionEvent.length, 4);

        // Create 2 ConnectionEvents
        mWifiMetrics.startConnectionEvent(null,  "BLUE",
                WifiMetricsProto.ConnectionEvent.ROAM_ENTERPRISE);
        mWifiMetrics.endConnectionEvent(
                WifiMetrics.ConnectionEvent.FAILURE_NONE,
                WifiMetricsProto.ConnectionEvent.HLF_NONE);
        mWifiMetrics.startConnectionEvent(null, "RED",
                WifiMetricsProto.ConnectionEvent.ROAM_ENTERPRISE);
        mWifiMetrics.endConnectionEvent(
                WifiMetrics.ConnectionEvent.FAILURE_NONE,
                WifiMetricsProto.ConnectionEvent.HLF_NONE);

        //Dump proto and deserialize
        dumpProtoAndDeserialize();
        //Check there are only 2 connection events
        assertEquals(mDeserializedWifiMetrics.connectionEvent.length, 2);
    }
}
