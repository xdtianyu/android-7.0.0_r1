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

package com.android.server.wifi.scanner;

import static com.android.server.wifi.ScanTestUtil.NativeScanSettingsBuilder;
import static com.android.server.wifi.ScanTestUtil.assertScanDataEquals;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.content.Context;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiScanner;
import android.os.SystemClock;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.internal.R;
import com.android.server.wifi.Clock;
import com.android.server.wifi.MockAlarmManager;
import com.android.server.wifi.MockLooper;
import com.android.server.wifi.MockResources;
import com.android.server.wifi.MockWifiMonitor;
import com.android.server.wifi.ScanResults;
import com.android.server.wifi.WifiMonitor;
import com.android.server.wifi.WifiNative;
import com.android.server.wifi.scanner.ChannelHelper.ChannelCollection;

import org.junit.Before;
import org.junit.Test;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.HashSet;
import java.util.Set;


/**
 * Unit tests for {@link com.android.server.wifi.scanner.SupplicantWifiScannerImpl.setPnoList}.
 */
@SmallTest
public class SupplicantPnoScannerTest {

    @Mock Context mContext;
    MockAlarmManager mAlarmManager;
    MockWifiMonitor mWifiMonitor;
    MockLooper mLooper;
    @Mock WifiNative mWifiNative;
    MockResources mResources;
    @Mock Clock mClock;
    SupplicantWifiScannerImpl mScanner;

    @Before
    public void setup() throws Exception {
        MockitoAnnotations.initMocks(this);

        mLooper = new MockLooper();
        mAlarmManager = new MockAlarmManager();
        mWifiMonitor = new MockWifiMonitor();
        mResources = new MockResources();

        when(mWifiNative.getInterfaceName()).thenReturn("a_test_interface_name");
        when(mContext.getSystemService(Context.ALARM_SERVICE))
                .thenReturn(mAlarmManager.getAlarmManager());
        when(mContext.getResources()).thenReturn(mResources);
        when(mClock.elapsedRealtime()).thenReturn(SystemClock.elapsedRealtime());
    }

    /**
     * Verify that the HW disconnected PNO scan triggers a supplicant PNO scan and invokes the
     * OnPnoNetworkFound callback when the scan results are received.
     */
    @Test
    public void startHwDisconnectedPnoScan() {
        createScannerWithHwPnoScanSupport();

        WifiNative.PnoEventHandler pnoEventHandler = mock(WifiNative.PnoEventHandler.class);
        WifiNative.PnoSettings pnoSettings = createDummyPnoSettings(false);
        ScanResults scanResults = createDummyScanResults();

        InOrder order = inOrder(pnoEventHandler, mWifiNative);
        // Start PNO scan
        startSuccessfulPnoScan(null, pnoSettings, null, pnoEventHandler);
        expectSuccessfulHwDisconnectedPnoScan(order, pnoSettings, pnoEventHandler, scanResults);
        verifyNoMoreInteractions(pnoEventHandler);
    }

    /**
     * Verify that we pause & resume HW PNO scan when a single scan is scheduled and invokes the
     * OnPnoNetworkFound callback when the scan results are received.
     */
    @Test
    public void pauseResumeHwDisconnectedPnoScanForSingleScan() {
        createScannerWithHwPnoScanSupport();

        WifiNative.PnoEventHandler pnoEventHandler = mock(WifiNative.PnoEventHandler.class);
        WifiNative.PnoSettings pnoSettings = createDummyPnoSettings(false);
        WifiNative.ScanEventHandler eventHandler = mock(WifiNative.ScanEventHandler.class);
        WifiNative.ScanSettings settings = createDummyScanSettings();
        ScanResults scanResults = createDummyScanResults();

        InOrder order = inOrder(eventHandler, mWifiNative);
        // Start PNO scan
        startSuccessfulPnoScan(null, pnoSettings, null, pnoEventHandler);
        // Start single scan
        assertTrue(mScanner.startSingleScan(settings, eventHandler));
        // Verify that the PNO scan was paused and single scan runs successfully
        expectSuccessfulSingleScanWithHwPnoEnabled(order, eventHandler,
                expectedBandScanFreqs(WifiScanner.WIFI_BAND_24_GHZ), new HashSet<Integer>(),
                scanResults);
        verifyNoMoreInteractions(eventHandler);

        order = inOrder(pnoEventHandler, mWifiNative);
        // Resume PNO scan after the single scan results are received and PNO monitor debounce
        // alarm fires.
        assertTrue("dispatch pno monitor alarm",
                mAlarmManager.dispatch(
                        SupplicantWifiScannerImpl.HwPnoDebouncer.PNO_DEBOUNCER_ALARM_TAG));
        assertEquals("dispatch message after alarm", 1, mLooper.dispatchAll());
        // Now verify that PNO scan is resumed successfully
        expectSuccessfulHwDisconnectedPnoScan(order, pnoSettings, pnoEventHandler, scanResults);
        verifyNoMoreInteractions(pnoEventHandler);
    }

    /**
     * Verify that the SW disconnected PNO scan triggers a background scan and invokes the
     * background scan callbacks when scan results are received.
     */
    @Test
    public void startSwDisconnectedPnoScan() {
        createScannerWithSwPnoScanSupport();
        doSuccessfulSwPnoScanTest(false);
    }

    /**
     * Verify that the HW connected PNO scan triggers a background scan and invokes the
     * background scan callbacks when scan results are received.
     */
    @Test
    public void startHwConnectedPnoScan() {
        createScannerWithHwPnoScanSupport();
        doSuccessfulSwPnoScanTest(true);
    }

    /**
     * Verify that the SW connected PNO scan triggers a background scan and invokes the
     * background scan callbacks when scan results are received.
     */
    @Test
    public void startSwConnectedPnoScan() {
        createScannerWithSwPnoScanSupport();
        doSuccessfulSwPnoScanTest(true);
    }

    /**
     * Verify that the HW PNO delayed failure cleans up the scan settings cleanly.
     * 1. Start Hw PNO.
     * 2. Start Single Scan which should pause PNO scan.
     * 3. Fail the PNO scan resume and verify that the OnPnoScanFailed callback is invoked.
     * 4. Now restart a new PNO scan to ensure that the failure was cleanly handled.
     */
    @Test
    public void delayedHwDisconnectedPnoScanFailure() {
        createScannerWithHwPnoScanSupport();

        WifiNative.PnoEventHandler pnoEventHandler = mock(WifiNative.PnoEventHandler.class);
        WifiNative.PnoSettings pnoSettings = createDummyPnoSettings(false);
        WifiNative.ScanEventHandler eventHandler = mock(WifiNative.ScanEventHandler.class);
        WifiNative.ScanSettings settings = createDummyScanSettings();
        ScanResults scanResults = createDummyScanResults();

        InOrder order = inOrder(eventHandler, mWifiNative);
        // Start PNO scan
        startSuccessfulPnoScan(null, pnoSettings, null, pnoEventHandler);
        // Start single scan
        assertTrue(mScanner.startSingleScan(settings, eventHandler));
        // Verify that the PNO scan was paused and single scan runs successfully
        expectSuccessfulSingleScanWithHwPnoEnabled(order, eventHandler,
                expectedBandScanFreqs(WifiScanner.WIFI_BAND_24_GHZ), new HashSet<Integer>(),
                scanResults);
        verifyNoMoreInteractions(eventHandler);

        // Fail the PNO resume and check that the OnPnoScanFailed callback is invoked.
        order = inOrder(pnoEventHandler, mWifiNative);
        when(mWifiNative.setPnoScan(true)).thenReturn(false);
        assertTrue("dispatch pno monitor alarm",
                mAlarmManager.dispatch(
                        SupplicantWifiScannerImpl.HwPnoDebouncer.PNO_DEBOUNCER_ALARM_TAG));
        assertEquals("dispatch message after alarm", 1, mLooper.dispatchAll());
        order.verify(pnoEventHandler).onPnoScanFailed();
        verifyNoMoreInteractions(pnoEventHandler);

        // Add a new PNO scan request
        startSuccessfulPnoScan(null, pnoSettings, null, pnoEventHandler);
        assertTrue("dispatch pno monitor alarm",
                mAlarmManager.dispatch(
                        SupplicantWifiScannerImpl.HwPnoDebouncer.PNO_DEBOUNCER_ALARM_TAG));
        assertEquals("dispatch message after alarm", 1, mLooper.dispatchAll());
        expectSuccessfulHwDisconnectedPnoScan(order, pnoSettings, pnoEventHandler, scanResults);
        verifyNoMoreInteractions(pnoEventHandler);
    }

    private void doSuccessfulSwPnoScanTest(boolean isConnectedPno) {
        WifiNative.PnoEventHandler pnoEventHandler = mock(WifiNative.PnoEventHandler.class);
        WifiNative.PnoSettings pnoSettings = createDummyPnoSettings(isConnectedPno);
        WifiNative.ScanEventHandler scanEventHandler = mock(WifiNative.ScanEventHandler.class);
        WifiNative.ScanSettings scanSettings = createDummyScanSettings();
        ScanResults scanResults = createDummyScanResults();

        InOrder order = inOrder(scanEventHandler, mWifiNative);

        // Start PNO scan
        startSuccessfulPnoScan(scanSettings, pnoSettings, scanEventHandler, pnoEventHandler);

        expectSuccessfulSwPnoScan(order, scanEventHandler, scanResults);

        verifyNoMoreInteractions(pnoEventHandler);
    }

    private void createScannerWithHwPnoScanSupport() {
        mResources.setBoolean(R.bool.config_wifi_background_scan_support, true);
        mScanner =
                new SupplicantWifiScannerImpl(mContext, mWifiNative, mLooper.getLooper(), mClock);
    }

    private void createScannerWithSwPnoScanSupport() {
        mResources.setBoolean(R.bool.config_wifi_background_scan_support, false);
        mScanner =
                new SupplicantWifiScannerImpl(mContext, mWifiNative, mLooper.getLooper(), mClock);
    }

    private WifiNative.PnoSettings createDummyPnoSettings(boolean isConnected) {
        WifiNative.PnoSettings pnoSettings = new WifiNative.PnoSettings();
        pnoSettings.isConnected = isConnected;
        pnoSettings.networkList = new WifiNative.PnoNetwork[2];
        pnoSettings.networkList[0] = new WifiNative.PnoNetwork();
        pnoSettings.networkList[0].ssid = "ssid_pno_1";
        pnoSettings.networkList[0].networkId = 1;
        pnoSettings.networkList[0].priority = 1;
        pnoSettings.networkList[1] = new WifiNative.PnoNetwork();
        pnoSettings.networkList[1].ssid = "ssid_pno_2";
        pnoSettings.networkList[1].networkId = 2;
        pnoSettings.networkList[1].priority = 2;
        return pnoSettings;
    }

    private WifiNative.ScanSettings createDummyScanSettings() {
        WifiNative.ScanSettings settings = new NativeScanSettingsBuilder()
                .withBasePeriod(10000)
                .withMaxApPerScan(10)
                .addBucketWithBand(10000, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN,
                        WifiScanner.WIFI_BAND_24_GHZ)
                .build();
        return settings;
    }

    private ScanResults createDummyScanResults() {
        return ScanResults.create(0, 2400, 2450, 2450, 2400, 2450, 2450, 2400, 2450, 2450);
    }

    private void startSuccessfulPnoScan(WifiNative.ScanSettings scanSettings,
            WifiNative.PnoSettings pnoSettings, WifiNative.ScanEventHandler scanEventHandler,
            WifiNative.PnoEventHandler pnoEventHandler) {
        when(mWifiNative.setNetworkVariable(anyInt(), anyString(), anyString())).thenReturn(true);
        when(mWifiNative.enableNetworkWithoutConnect(anyInt())).thenReturn(true);
        // Scans succeed
        when(mWifiNative.scan(any(Set.class), any(Set.class))).thenReturn(true);
        when(mWifiNative.setPnoScan(anyBoolean())).thenReturn(true);

        if (mScanner.isHwPnoSupported(pnoSettings.isConnected)) {
            // This should happen only for HW PNO scan
            assertTrue(mScanner.setHwPnoList(pnoSettings, pnoEventHandler));
        } else {
            // This should happen only for SW PNO scan
            assertTrue(mScanner.startBatchedScan(scanSettings, scanEventHandler));

        }
    }

    private Set<Integer> expectedBandScanFreqs(int band) {
        ChannelCollection collection = mScanner.getChannelHelper().createChannelCollection();
        collection.addBand(band);
        return collection.getSupplicantScanFreqs();
    }

    /**
     * Verify that the PNO scan was successfully started.
     */
    private void expectSuccessfulHwDisconnectedPnoScan(InOrder order,
            WifiNative.PnoSettings pnoSettings, WifiNative.PnoEventHandler eventHandler,
            ScanResults scanResults) {
        for (int i = 0; i < pnoSettings.networkList.length; i++) {
            WifiNative.PnoNetwork network = pnoSettings.networkList[i];
            order.verify(mWifiNative).setNetworkVariable(network.networkId,
                    WifiConfiguration.priorityVarName, Integer.toString(network.priority));
            order.verify(mWifiNative).enableNetworkWithoutConnect(network.networkId);
        }
        // Verify  HW PNO scan started
        order.verify(mWifiNative).setPnoScan(true);

        // Setup scan results
        when(mWifiNative.getScanResults()).thenReturn(scanResults.getScanDetailArrayList());

        // Notify scan has finished
        mWifiMonitor.sendMessage(mWifiNative.getInterfaceName(), WifiMonitor.SCAN_RESULTS_EVENT);
        assertEquals("dispatch message after results event", 1, mLooper.dispatchAll());

        order.verify(eventHandler).onPnoNetworkFound(scanResults.getRawScanResults());
    }

    /**
     * Verify that the single scan results were delivered and that the PNO scan was paused and
     * resumed either side of it.
     */
    private void expectSuccessfulSingleScanWithHwPnoEnabled(InOrder order,
            WifiNative.ScanEventHandler eventHandler, Set<Integer> expectedScanFreqs,
            Set<Integer> expectedHiddenNetIds, ScanResults scanResults) {
        // Pause PNO scan first
        order.verify(mWifiNative).setPnoScan(false);

        order.verify(mWifiNative).scan(eq(expectedScanFreqs), eq(expectedHiddenNetIds));

        when(mWifiNative.getScanResults()).thenReturn(scanResults.getScanDetailArrayList());

        // Notify scan has finished
        mWifiMonitor.sendMessage(mWifiNative.getInterfaceName(), WifiMonitor.SCAN_RESULTS_EVENT);
        assertEquals("dispatch message after results event", 1, mLooper.dispatchAll());

        order.verify(eventHandler).onScanStatus(WifiNative.WIFI_SCAN_RESULTS_AVAILABLE);
        assertScanDataEquals(scanResults.getScanData(), mScanner.getLatestSingleScanResults());
    }

    /**
     * Verify that the SW PNO scan was successfully started. This could either be disconnected
     * or connected PNO.
     * This is basically ensuring that the background scan runs successfully and returns the
     * expected result.
     */
    private void expectSuccessfulSwPnoScan(InOrder order,
            WifiNative.ScanEventHandler eventHandler, ScanResults scanResults) {

        // Verify scan started
        order.verify(mWifiNative).scan(any(Set.class), any(Set.class));

        // Make sure that HW PNO scan was not started
        verify(mWifiNative, never()).setPnoScan(anyBoolean());

        // Setup scan results
        when(mWifiNative.getScanResults()).thenReturn(scanResults.getScanDetailArrayList());

        // Notify scan has finished
        mWifiMonitor.sendMessage(mWifiNative.getInterfaceName(), WifiMonitor.SCAN_RESULTS_EVENT);
        assertEquals("dispatch message after results event", 1, mLooper.dispatchAll());

        // Verify background scan results delivered
        order.verify(eventHandler).onScanStatus(WifiNative.WIFI_SCAN_RESULTS_AVAILABLE);
        WifiScanner.ScanData[] scanData = mScanner.getLatestBatchedScanResults(true);
        WifiScanner.ScanData lastScanData = scanData[scanData.length -1];
        assertScanDataEquals(scanResults.getScanData(), lastScanData);
    }
}
