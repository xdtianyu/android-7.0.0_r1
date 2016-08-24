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

import static com.android.server.wifi.WifiConfigurationTestUtil.generateWifiConfig;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.content.Context;
import android.content.res.Resources;
import android.net.wifi.ScanResult;
import android.net.wifi.ScanResult.InformationElement;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiScanner;
import android.net.wifi.WifiScanner.PnoScanListener;
import android.net.wifi.WifiScanner.PnoSettings;
import android.net.wifi.WifiScanner.ScanListener;
import android.net.wifi.WifiScanner.ScanSettings;
import android.net.wifi.WifiSsid;
import android.os.SystemClock;
import android.os.WorkSource;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.internal.R;
import com.android.server.wifi.MockAnswerUtil.AnswerWithArguments;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Unit tests for {@link com.android.server.wifi.WifiConnectivityManager}.
 */
@SmallTest
public class WifiConnectivityManagerTest {

    /**
     * Called before each test
     */
    @Before
    public void setUp() throws Exception {
        mWifiInjector = mockWifiInjector();
        mResource = mockResource();
        mAlarmManager = new MockAlarmManager();
        mContext = mockContext();
        mWifiStateMachine = mockWifiStateMachine();
        mWifiConfigManager = mockWifiConfigManager();
        mWifiInfo = getWifiInfo();
        mWifiScanner = mockWifiScanner();
        mWifiQNS = mockWifiQualifiedNetworkSelector();
        mWifiConnectivityManager = new WifiConnectivityManager(mContext, mWifiStateMachine,
                mWifiScanner, mWifiConfigManager, mWifiInfo, mWifiQNS, mWifiInjector,
                mLooper.getLooper());
        mWifiConnectivityManager.setWifiEnabled(true);
        when(mClock.elapsedRealtime()).thenReturn(SystemClock.elapsedRealtime());
    }

    /**
     * Called after each test
     */
    @After
    public void cleanup() {
        validateMockitoUsage();
    }

    private Resources mResource;
    private Context mContext;
    private MockAlarmManager mAlarmManager;
    private MockLooper mLooper = new MockLooper();
    private WifiConnectivityManager mWifiConnectivityManager;
    private WifiQualifiedNetworkSelector mWifiQNS;
    private WifiStateMachine mWifiStateMachine;
    private WifiScanner mWifiScanner;
    private WifiConfigManager mWifiConfigManager;
    private WifiInfo mWifiInfo;
    private Clock mClock = mock(Clock.class);
    private WifiLastResortWatchdog mWifiLastResortWatchdog;
    private WifiMetrics mWifiMetrics;
    private WifiInjector mWifiInjector;

    private static final int CANDIDATE_NETWORK_ID = 0;
    private static final String CANDIDATE_SSID = "\"AnSsid\"";
    private static final String CANDIDATE_BSSID = "6c:f3:7f:ae:8c:f3";
    private static final String TAG = "WifiConnectivityManager Unit Test";
    private static final long CURRENT_SYSTEM_TIME_MS = 1000;

    Resources mockResource() {
        Resources resource = mock(Resources.class);

        when(resource.getInteger(R.integer.config_wifi_framework_SECURITY_AWARD)).thenReturn(80);
        when(resource.getInteger(R.integer.config_wifi_framework_SAME_BSSID_AWARD)).thenReturn(24);

        return resource;
    }

    Context mockContext() {
        Context context = mock(Context.class);

        when(context.getResources()).thenReturn(mResource);
        when(context.getSystemService(Context.ALARM_SERVICE)).thenReturn(
                mAlarmManager.getAlarmManager());

        return context;
    }

    WifiScanner mockWifiScanner() {
        WifiScanner scanner = mock(WifiScanner.class);

        // dummy scan results. QNS PeriodicScanListener bulids scanDetails from
        // the fullScanResult and doesn't really use results
        final WifiScanner.ScanData[] scanDatas = new WifiScanner.ScanData[1];

        // do a synchronous answer for the ScanListener callbacks
        doAnswer(new AnswerWithArguments() {
                public void answer(ScanSettings settings, ScanListener listener,
                        WorkSource workSource) throws Exception {
                    listener.onResults(scanDatas);
                }}).when(scanner).startBackgroundScan(anyObject(), anyObject(), anyObject());

        doAnswer(new AnswerWithArguments() {
                public void answer(ScanSettings settings, ScanListener listener,
                        WorkSource workSource) throws Exception {
                    listener.onResults(scanDatas);
                }}).when(scanner).startScan(anyObject(), anyObject(), anyObject());

        // This unfortunately needs to be a somewhat valid scan result, otherwise
        // |ScanDetailUtil.toScanDetail| raises exceptions.
        final ScanResult[] scanResults = new ScanResult[1];
        scanResults[0] = new ScanResult(WifiSsid.createFromAsciiEncoded(CANDIDATE_SSID),
                CANDIDATE_SSID, CANDIDATE_BSSID, 1245, 0, "some caps",
                -78, 2450, 1025, 22, 33, 20, 0, 0, true);
        scanResults[0].informationElements = new InformationElement[1];
        scanResults[0].informationElements[0] = new InformationElement();
        scanResults[0].informationElements[0].id = InformationElement.EID_SSID;
        scanResults[0].informationElements[0].bytes =
                CANDIDATE_SSID.getBytes(StandardCharsets.UTF_8);

        doAnswer(new AnswerWithArguments() {
            public void answer(ScanSettings settings, PnoSettings pnoSettings,
                    PnoScanListener listener) throws Exception {
                listener.onPnoNetworkFound(scanResults);
            }}).when(scanner).startDisconnectedPnoScan(anyObject(), anyObject(), anyObject());

        doAnswer(new AnswerWithArguments() {
            public void answer(ScanSettings settings, PnoSettings pnoSettings,
                    PnoScanListener listener) throws Exception {
                listener.onPnoNetworkFound(scanResults);
            }}).when(scanner).startConnectedPnoScan(anyObject(), anyObject(), anyObject());

        return scanner;
    }

    WifiStateMachine mockWifiStateMachine() {
        WifiStateMachine stateMachine = mock(WifiStateMachine.class);

        when(stateMachine.getFrequencyBand()).thenReturn(1);
        when(stateMachine.isLinkDebouncing()).thenReturn(false);
        when(stateMachine.isConnected()).thenReturn(false);
        when(stateMachine.isDisconnected()).thenReturn(true);
        when(stateMachine.isSupplicantTransientState()).thenReturn(false);

        return stateMachine;
    }

    WifiQualifiedNetworkSelector mockWifiQualifiedNetworkSelector() {
        WifiQualifiedNetworkSelector qns = mock(WifiQualifiedNetworkSelector.class);

        WifiConfiguration candidate = generateWifiConfig(
                0, CANDIDATE_NETWORK_ID, CANDIDATE_SSID, false, true, null, null);
        candidate.BSSID = CANDIDATE_BSSID;
        ScanResult candidateScanResult = new ScanResult();
        candidateScanResult.SSID = CANDIDATE_SSID;
        candidateScanResult.BSSID = CANDIDATE_BSSID;
        candidate.getNetworkSelectionStatus().setCandidate(candidateScanResult);

        when(qns.selectQualifiedNetwork(anyBoolean(), anyBoolean(), anyObject(),
              anyBoolean(), anyBoolean(), anyBoolean(), anyBoolean())).thenReturn(candidate);
        return qns;
    }

    WifiInfo getWifiInfo() {
        WifiInfo wifiInfo = new WifiInfo();

        wifiInfo.setNetworkId(WifiConfiguration.INVALID_NETWORK_ID);
        wifiInfo.setBSSID(null);
        wifiInfo.setSupplicantState(SupplicantState.DISCONNECTED);

        return wifiInfo;
    }

    WifiConfigManager mockWifiConfigManager() {
        WifiConfigManager wifiConfigManager = mock(WifiConfigManager.class);

        when(wifiConfigManager.getWifiConfiguration(anyInt())).thenReturn(null);
        wifiConfigManager.mThresholdSaturatedRssi24 = new AtomicInteger(
                WifiQualifiedNetworkSelector.RSSI_SATURATION_2G_BAND);
        wifiConfigManager.mCurrentNetworkBoost = new AtomicInteger(
                WifiQualifiedNetworkSelector.SAME_NETWORK_AWARD);

        // Pass dummy pno network list, otherwise Pno scan requests will not be triggered.
        PnoSettings.PnoNetwork pnoNetwork = new PnoSettings.PnoNetwork(CANDIDATE_SSID);
        ArrayList<PnoSettings.PnoNetwork> pnoNetworkList = new ArrayList<>();
        pnoNetworkList.add(pnoNetwork);
        when(wifiConfigManager.retrieveDisconnectedPnoNetworkList()).thenReturn(pnoNetworkList);
        when(wifiConfigManager.retrieveConnectedPnoNetworkList()).thenReturn(pnoNetworkList);

        return wifiConfigManager;
    }

    WifiInjector mockWifiInjector() {
        WifiInjector wifiInjector = mock(WifiInjector.class);
        mWifiLastResortWatchdog = mock(WifiLastResortWatchdog.class);
        mWifiMetrics = mock(WifiMetrics.class);
        when(wifiInjector.getWifiLastResortWatchdog()).thenReturn(mWifiLastResortWatchdog);
        when(wifiInjector.getWifiMetrics()).thenReturn(mWifiMetrics);
        when(wifiInjector.getClock()).thenReturn(mClock);
        return wifiInjector;
    }

    /**
     *  Wifi enters disconnected state while screen is on.
     *
     * Expected behavior: WifiConnectivityManager calls
     * WifiStateMachine.autoConnectToNetwork() with the
     * expected candidate network ID and BSSID.
     */
    @Test
    public void enterWifiDisconnectedStateWhenScreenOn() {
        // Set screen to on
        mWifiConnectivityManager.handleScreenStateChanged(true);

        // Set WiFi to disconnected state
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_DISCONNECTED);

        verify(mWifiStateMachine).autoConnectToNetwork(
                CANDIDATE_NETWORK_ID, CANDIDATE_BSSID);
    }

    /**
     *  Wifi enters connected state while screen is on.
     *
     * Expected behavior: WifiConnectivityManager calls
     * WifiStateMachine.autoConnectToNetwork() with the
     * expected candidate network ID and BSSID.
     */
    @Test
    public void enterWifiConnectedStateWhenScreenOn() {
        // Set screen to on
        mWifiConnectivityManager.handleScreenStateChanged(true);

        // Set WiFi to connected state
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_CONNECTED);

        verify(mWifiStateMachine).autoConnectToNetwork(
                CANDIDATE_NETWORK_ID, CANDIDATE_BSSID);
    }

    /**
     *  Screen turned on while WiFi in disconnected state.
     *
     * Expected behavior: WifiConnectivityManager calls
     * WifiStateMachine.autoConnectToNetwork() with the
     * expected candidate network ID and BSSID.
     */
    @Test
    public void turnScreenOnWhenWifiInDisconnectedState() {
        // Set WiFi to disconnected state
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_DISCONNECTED);

        // Set screen to on
        mWifiConnectivityManager.handleScreenStateChanged(true);

        verify(mWifiStateMachine, atLeastOnce()).autoConnectToNetwork(
                CANDIDATE_NETWORK_ID, CANDIDATE_BSSID);
    }

    /**
     *  Screen turned on while WiFi in connected state.
     *
     * Expected behavior: WifiConnectivityManager calls
     * WifiStateMachine.autoConnectToNetwork() with the
     * expected candidate network ID and BSSID.
     */
    @Test
    public void turnScreenOnWhenWifiInConnectedState() {
        // Set WiFi to connected state
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_CONNECTED);

        // Set screen to on
        mWifiConnectivityManager.handleScreenStateChanged(true);

        verify(mWifiStateMachine, atLeastOnce()).autoConnectToNetwork(
                CANDIDATE_NETWORK_ID, CANDIDATE_BSSID);
    }

    /**
     * Multiple back to back connection attempts within the rate interval should be rate limited.
     *
     * Expected behavior: WifiConnectivityManager calls WifiStateMachine.autoConnectToNetwork()
     * with the expected candidate network ID and BSSID for only the expected number of times within
     * the given interval.
     */
    @Test
    public void connectionAttemptRateLimitedWhenScreenOff() {
        int maxAttemptRate = WifiConnectivityManager.MAX_CONNECTION_ATTEMPTS_RATE;
        int timeInterval = WifiConnectivityManager.MAX_CONNECTION_ATTEMPTS_TIME_INTERVAL_MS;
        int numAttempts = 0;
        int connectionAttemptIntervals = timeInterval / maxAttemptRate;

        mWifiConnectivityManager.handleScreenStateChanged(false);

        // First attempt the max rate number of connections within the rate interval.
        long currentTimeStamp = 0;
        for (int attempt = 0; attempt < maxAttemptRate; attempt++) {
            currentTimeStamp += connectionAttemptIntervals;
            when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);
            // Set WiFi to disconnected state to trigger PNO scan
            mWifiConnectivityManager.handleConnectionStateChanged(
                    WifiConnectivityManager.WIFI_STATE_DISCONNECTED);
            numAttempts++;
        }
        // Now trigger another connection attempt before the rate interval, this should be
        // skipped because we've crossed rate limit.
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);
        // Set WiFi to disconnected state to trigger PNO scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_DISCONNECTED);

        // Verify that we attempt to connect upto the rate.
        verify(mWifiStateMachine, times(numAttempts)).autoConnectToNetwork(
                CANDIDATE_NETWORK_ID, CANDIDATE_BSSID);
    }

    /**
     * Multiple back to back connection attempts outside the rate interval should not be rate
     * limited.
     *
     * Expected behavior: WifiConnectivityManager calls WifiStateMachine.autoConnectToNetwork()
     * with the expected candidate network ID and BSSID for only the expected number of times within
     * the given interval.
     */
    @Test
    public void connectionAttemptNotRateLimitedWhenScreenOff() {
        int maxAttemptRate = WifiConnectivityManager.MAX_CONNECTION_ATTEMPTS_RATE;
        int timeInterval = WifiConnectivityManager.MAX_CONNECTION_ATTEMPTS_TIME_INTERVAL_MS;
        int numAttempts = 0;
        int connectionAttemptIntervals = timeInterval / maxAttemptRate;

        mWifiConnectivityManager.handleScreenStateChanged(false);

        // First attempt the max rate number of connections within the rate interval.
        long currentTimeStamp = 0;
        for (int attempt = 0; attempt < maxAttemptRate; attempt++) {
            currentTimeStamp += connectionAttemptIntervals;
            when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);
            // Set WiFi to disconnected state to trigger PNO scan
            mWifiConnectivityManager.handleConnectionStateChanged(
                    WifiConnectivityManager.WIFI_STATE_DISCONNECTED);
            numAttempts++;
        }
        // Now trigger another connection attempt after the rate interval, this should not be
        // skipped because we should've evicted the older attempt.
        when(mClock.elapsedRealtime()).thenReturn(
                currentTimeStamp + connectionAttemptIntervals * 2);
        // Set WiFi to disconnected state to trigger PNO scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_DISCONNECTED);
        numAttempts++;

        // Verify that all the connection attempts went through
        verify(mWifiStateMachine, times(numAttempts)).autoConnectToNetwork(
                CANDIDATE_NETWORK_ID, CANDIDATE_BSSID);
    }

    /**
     * Multiple back to back connection attempts after a user selection should not be rate limited.
     *
     * Expected behavior: WifiConnectivityManager calls WifiStateMachine.autoConnectToNetwork()
     * with the expected candidate network ID and BSSID for only the expected number of times within
     * the given interval.
     */
    @Test
    public void connectionAttemptNotRateLimitedWhenScreenOffAfterUserSelection() {
        int maxAttemptRate = WifiConnectivityManager.MAX_CONNECTION_ATTEMPTS_RATE;
        int timeInterval = WifiConnectivityManager.MAX_CONNECTION_ATTEMPTS_TIME_INTERVAL_MS;
        int numAttempts = 0;
        int connectionAttemptIntervals = timeInterval / maxAttemptRate;

        mWifiConnectivityManager.handleScreenStateChanged(false);

        // First attempt the max rate number of connections within the rate interval.
        long currentTimeStamp = 0;
        for (int attempt = 0; attempt < maxAttemptRate; attempt++) {
            currentTimeStamp += connectionAttemptIntervals;
            when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);
            // Set WiFi to disconnected state to trigger PNO scan
            mWifiConnectivityManager.handleConnectionStateChanged(
                    WifiConnectivityManager.WIFI_STATE_DISCONNECTED);
            numAttempts++;
        }

        mWifiConnectivityManager.connectToUserSelectNetwork(CANDIDATE_NETWORK_ID, false);

        for (int attempt = 0; attempt < maxAttemptRate; attempt++) {
            currentTimeStamp += connectionAttemptIntervals;
            when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);
            // Set WiFi to disconnected state to trigger PNO scan
            mWifiConnectivityManager.handleConnectionStateChanged(
                    WifiConnectivityManager.WIFI_STATE_DISCONNECTED);
            numAttempts++;
        }

        // Verify that all the connection attempts went through
        verify(mWifiStateMachine, times(numAttempts)).autoConnectToNetwork(
                CANDIDATE_NETWORK_ID, CANDIDATE_BSSID);
    }

    /**
     *  PNO retry for low RSSI networks.
     *
     * Expected behavior: WifiConnectivityManager doubles the low RSSI
     * network retry delay value after QNS skips the PNO scan results
     * because of their low RSSI values.
     */
    @Test
    public void PnoRetryForLowRssiNetwork() {
        when(mWifiQNS.selectQualifiedNetwork(anyBoolean(), anyBoolean(), anyObject(),
              anyBoolean(), anyBoolean(), anyBoolean(), anyBoolean())).thenReturn(null);

        // Set screen to off
        mWifiConnectivityManager.handleScreenStateChanged(false);

        // Get the current retry delay value
        int lowRssiNetworkRetryDelayStartValue = mWifiConnectivityManager
                .getLowRssiNetworkRetryDelay();

        // Set WiFi to disconnected state to trigger PNO scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_DISCONNECTED);

        // Get the retry delay value after QNS didn't select a
        // network candicate from the PNO scan results.
        int lowRssiNetworkRetryDelayAfterPnoValue = mWifiConnectivityManager
                .getLowRssiNetworkRetryDelay();

        assertEquals(lowRssiNetworkRetryDelayStartValue * 2,
            lowRssiNetworkRetryDelayAfterPnoValue);
    }

    /**
     * Ensure that the watchdog bite increments the "Pno bad" metric.
     *
     * Expected behavior: WifiConnectivityManager detects that the PNO scan failed to find
     * a candidate while watchdog single scan did.
     */
    @Test
    public void watchdogBitePnoBadIncrementsMetrics() {
        // Set screen to off
        mWifiConnectivityManager.handleScreenStateChanged(false);

        // Set WiFi to disconnected state to trigger PNO scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_DISCONNECTED);

        // Now fire the watchdog alarm and verify the metrics were incremented.
        mAlarmManager.dispatch(WifiConnectivityManager.WATCHDOG_TIMER_TAG);
        mLooper.dispatchAll();

        verify(mWifiMetrics).incrementNumConnectivityWatchdogPnoBad();
        verify(mWifiMetrics, never()).incrementNumConnectivityWatchdogPnoGood();
    }

    /**
     * Ensure that the watchdog bite increments the "Pno good" metric.
     *
     * Expected behavior: WifiConnectivityManager detects that the PNO scan failed to find
     * a candidate which was the same with watchdog single scan.
     */
    @Test
    public void watchdogBitePnoGoodIncrementsMetrics() {
        // Qns returns no candidate after watchdog single scan.
        when(mWifiQNS.selectQualifiedNetwork(anyBoolean(), anyBoolean(), anyObject(),
                anyBoolean(), anyBoolean(), anyBoolean(), anyBoolean())).thenReturn(null);

        // Set screen to off
        mWifiConnectivityManager.handleScreenStateChanged(false);

        // Set WiFi to disconnected state to trigger PNO scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_DISCONNECTED);

        // Now fire the watchdog alarm and verify the metrics were incremented.
        mAlarmManager.dispatch(WifiConnectivityManager.WATCHDOG_TIMER_TAG);
        mLooper.dispatchAll();

        verify(mWifiMetrics).incrementNumConnectivityWatchdogPnoGood();
        verify(mWifiMetrics, never()).incrementNumConnectivityWatchdogPnoBad();
    }

    /**
     *  Verify that scan interval for screen on and wifi disconnected scenario
     *  is in the exponential backoff fashion.
     *
     * Expected behavior: WifiConnectivityManager doubles periodic
     * scan interval.
     */
    @Test
    public void checkPeriodicScanIntervalWhenDisconnected() {
        long currentTimeStamp = CURRENT_SYSTEM_TIME_MS;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Set screen to ON
        mWifiConnectivityManager.handleScreenStateChanged(true);

        // Wait for MAX_PERIODIC_SCAN_INTERVAL_MS so that any impact triggered
        // by screen state change can settle
        currentTimeStamp += WifiConnectivityManager.MAX_PERIODIC_SCAN_INTERVAL_MS;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Set WiFi to disconnected state to trigger periodic scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_DISCONNECTED);

        // Get the first periodic scan interval
        long firstIntervalMs = mAlarmManager
                    .getTriggerTimeMillis(WifiConnectivityManager.PERIODIC_SCAN_TIMER_TAG)
                    - currentTimeStamp;
        assertEquals(firstIntervalMs, WifiConnectivityManager.PERIODIC_SCAN_INTERVAL_MS);

        currentTimeStamp += firstIntervalMs;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Now fire the first periodic scan alarm timer
        mAlarmManager.dispatch(WifiConnectivityManager.PERIODIC_SCAN_TIMER_TAG);
        mLooper.dispatchAll();

        // Get the second periodic scan interval
        long secondIntervalMs = mAlarmManager
                    .getTriggerTimeMillis(WifiConnectivityManager.PERIODIC_SCAN_TIMER_TAG)
                    - currentTimeStamp;

        // Verify the intervals are exponential back off
        assertEquals(firstIntervalMs * 2, secondIntervalMs);

        currentTimeStamp += secondIntervalMs;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Make sure we eventually stay at the maximum scan interval.
        long intervalMs = 0;
        for (int i = 0; i < 5; i++) {
            mAlarmManager.dispatch(WifiConnectivityManager.PERIODIC_SCAN_TIMER_TAG);
            mLooper.dispatchAll();
            intervalMs = mAlarmManager
                    .getTriggerTimeMillis(WifiConnectivityManager.PERIODIC_SCAN_TIMER_TAG)
                    - currentTimeStamp;
            currentTimeStamp += intervalMs;
            when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);
        }

        assertEquals(intervalMs, WifiConnectivityManager.MAX_PERIODIC_SCAN_INTERVAL_MS);
    }

    /**
     *  Verify that scan interval for screen on and wifi connected scenario
     *  is in the exponential backoff fashion.
     *
     * Expected behavior: WifiConnectivityManager doubles periodic
     * scan interval.
     */
    @Test
    public void checkPeriodicScanIntervalWhenConnected() {
        long currentTimeStamp = CURRENT_SYSTEM_TIME_MS;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Set screen to ON
        mWifiConnectivityManager.handleScreenStateChanged(true);

        // Wait for MAX_PERIODIC_SCAN_INTERVAL_MS so that any impact triggered
        // by screen state change can settle
        currentTimeStamp += WifiConnectivityManager.MAX_PERIODIC_SCAN_INTERVAL_MS;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Set WiFi to connected state to trigger periodic scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_CONNECTED);

        // Get the first periodic scan interval
        long firstIntervalMs = mAlarmManager
                    .getTriggerTimeMillis(WifiConnectivityManager.PERIODIC_SCAN_TIMER_TAG)
                    - currentTimeStamp;
        assertEquals(firstIntervalMs, WifiConnectivityManager.PERIODIC_SCAN_INTERVAL_MS);

        currentTimeStamp += firstIntervalMs;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Now fire the first periodic scan alarm timer
        mAlarmManager.dispatch(WifiConnectivityManager.PERIODIC_SCAN_TIMER_TAG);
        mLooper.dispatchAll();

        // Get the second periodic scan interval
        long secondIntervalMs = mAlarmManager
                    .getTriggerTimeMillis(WifiConnectivityManager.PERIODIC_SCAN_TIMER_TAG)
                    - currentTimeStamp;

        // Verify the intervals are exponential back off
        assertEquals(firstIntervalMs * 2, secondIntervalMs);

        currentTimeStamp += secondIntervalMs;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Make sure we eventually stay at the maximum scan interval.
        long intervalMs = 0;
        for (int i = 0; i < 5; i++) {
            mAlarmManager.dispatch(WifiConnectivityManager.PERIODIC_SCAN_TIMER_TAG);
            mLooper.dispatchAll();
            intervalMs = mAlarmManager
                    .getTriggerTimeMillis(WifiConnectivityManager.PERIODIC_SCAN_TIMER_TAG)
                    - currentTimeStamp;
            currentTimeStamp += intervalMs;
            when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);
        }

        assertEquals(intervalMs, WifiConnectivityManager.MAX_PERIODIC_SCAN_INTERVAL_MS);
    }

    /**
     *  When screen on trigger two connection state change events back to back to
     *  verify that the minium scan interval is enforced.
     *
     * Expected behavior: WifiConnectivityManager start the second periodic single
     * scan PERIODIC_SCAN_INTERVAL_MS after the first one.
     */
    @Test
    public void checkMinimumPeriodicScanIntervalWhenScreenOn() {
        long currentTimeStamp = CURRENT_SYSTEM_TIME_MS;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Set screen to ON
        mWifiConnectivityManager.handleScreenStateChanged(true);

        // Wait for MAX_PERIODIC_SCAN_INTERVAL_MS so that any impact triggered
        // by screen state change can settle
        currentTimeStamp += WifiConnectivityManager.MAX_PERIODIC_SCAN_INTERVAL_MS;
        long firstScanTimeStamp = currentTimeStamp;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Set WiFi to connected state to trigger the periodic scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_CONNECTED);

        // Set the second scan attempt time stamp.
        currentTimeStamp += 2000;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Set WiFi to disconnected state to trigger another periodic scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_DISCONNECTED);

        // Get the second periodic scan actual time stamp
        long secondScanTimeStamp = mAlarmManager
                    .getTriggerTimeMillis(WifiConnectivityManager.PERIODIC_SCAN_TIMER_TAG);

        // Verify that the second scan is scheduled PERIODIC_SCAN_INTERVAL_MS after the
        // very first scan.
        assertEquals(secondScanTimeStamp, firstScanTimeStamp
                       + WifiConnectivityManager.PERIODIC_SCAN_INTERVAL_MS);

    }

    /**
     *  When screen on trigger a connection state change event and a forced connectivity
     *  scan event back to back to verify that the minimum scan interval is not applied
     *  in this scenario.
     *
     * Expected behavior: WifiConnectivityManager starts the second periodic single
     * scan immediately.
     */
    @Test
    public void checkMinimumPeriodicScanIntervalNotEnforced() {
        long currentTimeStamp = CURRENT_SYSTEM_TIME_MS;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Set screen to ON
        mWifiConnectivityManager.handleScreenStateChanged(true);

        // Wait for MAX_PERIODIC_SCAN_INTERVAL_MS so that any impact triggered
        // by screen state change can settle
        currentTimeStamp += WifiConnectivityManager.MAX_PERIODIC_SCAN_INTERVAL_MS;
        long firstScanTimeStamp = currentTimeStamp;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Set WiFi to connected state to trigger the periodic scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_CONNECTED);

        // Set the second scan attempt time stamp
        currentTimeStamp += 2000;
        when(mClock.elapsedRealtime()).thenReturn(currentTimeStamp);

        // Force a connectivity scan
        mWifiConnectivityManager.forceConnectivityScan();

        // Get the second periodic scan actual time stamp. Note, this scan is not
        // started from the AlarmManager.
        long secondScanTimeStamp = mWifiConnectivityManager.getLastPeriodicSingleScanTimeStamp();

        // Verify that the second scan is fired immediately
        assertEquals(secondScanTimeStamp, currentTimeStamp);
    }

    /**
     * Verify that we perform full band scan when the currently connected network's tx/rx success
     * rate is low.
     *
     * Expected behavior: WifiConnectivityManager does full band scan.
     */
    @Test
    public void checkSingleScanSettingsWhenConnectedWithLowDataRate() {
        mWifiInfo.txSuccessRate = 0;
        mWifiInfo.rxSuccessRate = 0;

        final HashSet<Integer> channelList = new HashSet<>();
        channelList.add(1);
        channelList.add(2);
        channelList.add(3);

        when(mWifiStateMachine.getCurrentWifiConfiguration())
                .thenReturn(new WifiConfiguration());
        when(mWifiStateMachine.getFrequencyBand())
                .thenReturn(WifiManager.WIFI_FREQUENCY_BAND_5GHZ);
        when(mWifiConfigManager.makeChannelList(any(WifiConfiguration.class), anyInt()))
                .thenReturn(channelList);

        doAnswer(new AnswerWithArguments() {
            public void answer(ScanSettings settings, ScanListener listener,
                    WorkSource workSource) throws Exception {
                assertEquals(settings.band, WifiScanner.WIFI_BAND_5_GHZ_WITH_DFS);
                assertNull(settings.channels);
            }}).when(mWifiScanner).startScan(anyObject(), anyObject(), anyObject());

        // Set screen to ON
        mWifiConnectivityManager.handleScreenStateChanged(true);

        // Set WiFi to connected state to trigger periodic scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_CONNECTED);

        verify(mWifiScanner).startScan(anyObject(), anyObject(), anyObject());
    }

    /**
     * Verify that we perform partial scan when the currently connected network's tx/rx success
     * rate is high and when the currently connected network is present in scan
     * cache in WifiConfigManager.
     *
     * Expected behavior: WifiConnectivityManager does full band scan.
     */
    @Test
    public void checkSingleScanSettingsWhenConnectedWithHighDataRate() {
        mWifiInfo.txSuccessRate = WifiConfigManager.MAX_TX_PACKET_FOR_FULL_SCANS * 2;
        mWifiInfo.rxSuccessRate = WifiConfigManager.MAX_RX_PACKET_FOR_FULL_SCANS * 2;

        final HashSet<Integer> channelList = new HashSet<>();
        channelList.add(1);
        channelList.add(2);
        channelList.add(3);

        when(mWifiStateMachine.getCurrentWifiConfiguration())
                .thenReturn(new WifiConfiguration());
        when(mWifiConfigManager.makeChannelList(any(WifiConfiguration.class), anyInt()))
                .thenReturn(channelList);

        doAnswer(new AnswerWithArguments() {
            public void answer(ScanSettings settings, ScanListener listener,
                    WorkSource workSource) throws Exception {
                assertEquals(settings.band, WifiScanner.WIFI_BAND_UNSPECIFIED);
                assertEquals(settings.channels.length, channelList.size());
                for (int chanIdx = 0; chanIdx < settings.channels.length; chanIdx++) {
                    assertTrue(channelList.contains(settings.channels[chanIdx].frequency));
                }
            }}).when(mWifiScanner).startScan(anyObject(), anyObject(), anyObject());

        // Set screen to ON
        mWifiConnectivityManager.handleScreenStateChanged(true);

        // Set WiFi to connected state to trigger periodic scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_CONNECTED);

        verify(mWifiScanner).startScan(anyObject(), anyObject(), anyObject());
    }

    /**
     * Verify that we fall back to full band scan when the currently connected network's tx/rx
     * success rate is high and the currently connected network is not present in scan cache in
     * WifiConfigManager. This is simulated by returning an empty hashset in |makeChannelList|.
     *
     * Expected behavior: WifiConnectivityManager does full band scan.
     */
    @Test
    public void checkSingleScanSettingsWhenConnectedWithHighDataRateNotInCache() {
        mWifiInfo.txSuccessRate = WifiConfigManager.MAX_TX_PACKET_FOR_FULL_SCANS * 2;
        mWifiInfo.rxSuccessRate = WifiConfigManager.MAX_RX_PACKET_FOR_FULL_SCANS * 2;

        final HashSet<Integer> channelList = new HashSet<>();

        when(mWifiStateMachine.getCurrentWifiConfiguration())
                .thenReturn(new WifiConfiguration());
        when(mWifiStateMachine.getFrequencyBand())
                .thenReturn(WifiManager.WIFI_FREQUENCY_BAND_5GHZ);
        when(mWifiConfigManager.makeChannelList(any(WifiConfiguration.class), anyInt()))
                .thenReturn(channelList);

        doAnswer(new AnswerWithArguments() {
            public void answer(ScanSettings settings, ScanListener listener,
                    WorkSource workSource) throws Exception {
                assertEquals(settings.band, WifiScanner.WIFI_BAND_5_GHZ_WITH_DFS);
                assertNull(settings.channels);
            }}).when(mWifiScanner).startScan(anyObject(), anyObject(), anyObject());

        // Set screen to ON
        mWifiConnectivityManager.handleScreenStateChanged(true);

        // Set WiFi to connected state to trigger periodic scan
        mWifiConnectivityManager.handleConnectionStateChanged(
                WifiConnectivityManager.WIFI_STATE_CONNECTED);

        verify(mWifiScanner).startScan(anyObject(), anyObject(), anyObject());
    }
}
