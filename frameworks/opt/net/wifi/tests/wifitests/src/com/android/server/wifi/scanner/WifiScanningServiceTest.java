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

import static com.android.server.wifi.ScanTestUtil.*;

import static org.junit.Assert.*;
import static org.mockito.Matchers.*;
import static org.mockito.Mockito.*;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiScanner;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.os.WorkSource;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Pair;

import com.android.internal.app.IBatteryStats;
import com.android.internal.util.Protocol;
import com.android.server.wifi.BidirectionalAsyncChannel;
import com.android.server.wifi.Clock;
import com.android.server.wifi.MockAlarmManager;
import com.android.server.wifi.MockAnswerUtil.AnswerWithArguments;
import com.android.server.wifi.MockLooper;
import com.android.server.wifi.ScanResults;
import com.android.server.wifi.TestUtil;
import com.android.server.wifi.WifiInjector;
import com.android.server.wifi.WifiMetrics;
import com.android.server.wifi.WifiMetricsProto;
import com.android.server.wifi.WifiNative;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.internal.matchers.CapturingMatcher;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.regex.Pattern;

/**
 * Unit tests for {@link com.android.server.wifi.scanner.WifiScanningServiceImpl}.
 */
@SmallTest
public class WifiScanningServiceTest {
    public static final String TAG = "WifiScanningServiceTest";

    @Mock Context mContext;
    MockAlarmManager mAlarmManager;
    @Mock WifiScannerImpl mWifiScannerImpl;
    @Mock WifiScannerImpl.WifiScannerImplFactory mWifiScannerImplFactory;
    @Mock IBatteryStats mBatteryStats;
    @Mock WifiInjector mWifiInjector;
    @Mock Clock mClock;
    WifiMetrics mWifiMetrics;
    MockLooper mLooper;
    WifiScanningServiceImpl mWifiScanningServiceImpl;


    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        mAlarmManager = new MockAlarmManager();
        when(mContext.getSystemService(Context.ALARM_SERVICE))
                .thenReturn(mAlarmManager.getAlarmManager());
        mWifiMetrics = new WifiMetrics(mClock);

        ChannelHelper channelHelper = new PresetKnownBandsChannelHelper(
                new int[]{2400, 2450},
                new int[]{5150, 5175},
                new int[]{5600, 5650, 5660});

        mLooper = new MockLooper();
        when(mWifiScannerImplFactory
                .create(any(Context.class), any(Looper.class), any(Clock.class)))
                .thenReturn(mWifiScannerImpl);
        when(mWifiScannerImpl.getChannelHelper()).thenReturn(channelHelper);
        when(mWifiInjector.getWifiMetrics()).thenReturn(mWifiMetrics);
        mWifiScanningServiceImpl = new WifiScanningServiceImpl(mContext, mLooper.getLooper(),
                mWifiScannerImplFactory, mBatteryStats, mWifiInjector);
    }

    @After
    public void cleanup() {
        validateMockitoUsage();
    }

    /**
     * Internal BroadcastReceiver that WifiScanningServiceImpl uses to listen for broadcasts
     * this is initialized by calling startServiceAndLoadDriver
     */
    BroadcastReceiver mBroadcastReceiver;

    private WifiScanner.ScanSettings generateValidScanSettings() {
        return createRequest(WifiScanner.WIFI_BAND_BOTH, 30000, 0, 20,
                WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
    }

    private BidirectionalAsyncChannel connectChannel(Handler handler) {
        BidirectionalAsyncChannel controlChannel = new BidirectionalAsyncChannel();
        controlChannel.connect(mLooper.getLooper(), mWifiScanningServiceImpl.getMessenger(),
                handler);
        mLooper.dispatchAll();
        controlChannel.assertConnected();
        return controlChannel;
    }

    private Message verifyHandleMessageAndGetMessage(InOrder order, Handler handler) {
        ArgumentCaptor<Message> messageCaptor = ArgumentCaptor.forClass(Message.class);
        order.verify(handler).handleMessage(messageCaptor.capture());
        return messageCaptor.getValue();
    }

    private Message verifyHandleMessageAndGetMessage(InOrder order, Handler handler,
            final int what) {
        CapturingMatcher<Message> messageMatcher = new CapturingMatcher<Message>() {
            public boolean matches(Object argument) {
                Message message = (Message) argument;
                return message.what == what;
            }
        };
        order.verify(handler).handleMessage(argThat(messageMatcher));
        return messageMatcher.getLastValue();
    }

    private void verifyScanResultsRecieved(InOrder order, Handler handler, int listenerId,
            WifiScanner.ScanData... expected) {
        Message scanResultMessage = verifyHandleMessageAndGetMessage(order, handler,
                WifiScanner.CMD_SCAN_RESULT);
        assertScanResultsMessage(listenerId, expected, scanResultMessage);
    }

    private void assertScanResultsMessage(int listenerId, WifiScanner.ScanData[] expected,
            Message scanResultMessage) {
        assertEquals("what", WifiScanner.CMD_SCAN_RESULT, scanResultMessage.what);
        assertEquals("listenerId", listenerId, scanResultMessage.arg2);
        assertScanDatasEquals(expected,
                ((WifiScanner.ParcelableScanData) scanResultMessage.obj).getResults());
    }

    private void verifySingleScanCompletedRecieved(InOrder order, Handler handler, int listenerId) {
        Message completedMessage = verifyHandleMessageAndGetMessage(order, handler,
                WifiScanner.CMD_SINGLE_SCAN_COMPLETED);
        assertSingleScanCompletedMessage(listenerId, completedMessage);
    }

    private void assertSingleScanCompletedMessage(int listenerId, Message completedMessage) {
        assertEquals("what", WifiScanner.CMD_SINGLE_SCAN_COMPLETED, completedMessage.what);
        assertEquals("listenerId", listenerId, completedMessage.arg2);
    }

    private void sendBackgroundScanRequest(BidirectionalAsyncChannel controlChannel,
            int scanRequestId, WifiScanner.ScanSettings settings, WorkSource workSource) {
        Bundle scanParams = new Bundle();
        scanParams.putParcelable(WifiScanner.SCAN_PARAMS_SCAN_SETTINGS_KEY, settings);
        scanParams.putParcelable(WifiScanner.SCAN_PARAMS_WORK_SOURCE_KEY, workSource);
        controlChannel.sendMessage(Message.obtain(null, WifiScanner.CMD_START_BACKGROUND_SCAN, 0,
                        scanRequestId, scanParams));
    }

    private void sendSingleScanRequest(BidirectionalAsyncChannel controlChannel,
            int scanRequestId, WifiScanner.ScanSettings settings, WorkSource workSource) {
        Bundle scanParams = new Bundle();
        scanParams.putParcelable(WifiScanner.SCAN_PARAMS_SCAN_SETTINGS_KEY, settings);
        scanParams.putParcelable(WifiScanner.SCAN_PARAMS_WORK_SOURCE_KEY, workSource);
        controlChannel.sendMessage(Message.obtain(null, WifiScanner.CMD_START_SINGLE_SCAN, 0,
                        scanRequestId, scanParams));
    }

    private void verifySuccessfulResponse(InOrder order, Handler handler, int arg2) {
        Message response = verifyHandleMessageAndGetMessage(order, handler);
        assertSuccessfulResponse(arg2, response);
    }

    private void assertSuccessfulResponse(int arg2, Message response) {
        if (response.what == WifiScanner.CMD_OP_FAILED) {
            WifiScanner.OperationResult result = (WifiScanner.OperationResult) response.obj;
            fail("response indicates failure, reason=" + result.reason
                    + ", description=" + result.description);
        } else {
            assertEquals("response.what", WifiScanner.CMD_OP_SUCCEEDED, response.what);
            assertEquals("response.arg2", arg2, response.arg2);
        }
    }

    private void verifyFailedResponse(InOrder order, Handler handler, int arg2,
            int expectedErrorReason, String expectedErrorDescription) {
        Message response = verifyHandleMessageAndGetMessage(order, handler);
        assertFailedResponse(arg2, expectedErrorReason, expectedErrorDescription, response);
    }

    private void assertFailedResponse(int arg2, int expectedErrorReason,
            String expectedErrorDescription, Message response) {
        if (response.what == WifiScanner.CMD_OP_SUCCEEDED) {
            fail("response indicates success");
        } else {
            assertEquals("response.what", WifiScanner.CMD_OP_FAILED, response.what);
            assertEquals("response.arg2", arg2, response.arg2);
            WifiScanner.OperationResult result = (WifiScanner.OperationResult) response.obj;
            assertEquals("response.obj.reason",
                    expectedErrorReason, result.reason);
            assertEquals("response.obj.description",
                    expectedErrorDescription, result.description);
        }
    }

    private WifiNative.ScanEventHandler verifyStartSingleScan(InOrder order,
            WifiNative.ScanSettings expected) {
        ArgumentCaptor<WifiNative.ScanSettings> scanSettingsCaptor =
                ArgumentCaptor.forClass(WifiNative.ScanSettings.class);
        ArgumentCaptor<WifiNative.ScanEventHandler> scanEventHandlerCaptor =
                ArgumentCaptor.forClass(WifiNative.ScanEventHandler.class);
        order.verify(mWifiScannerImpl).startSingleScan(scanSettingsCaptor.capture(),
                scanEventHandlerCaptor.capture());
        assertNativeScanSettingsEquals(expected, scanSettingsCaptor.getValue());
        return scanEventHandlerCaptor.getValue();
    }

    private WifiNative.ScanEventHandler verifyStartBackgroundScan(InOrder order,
            WifiNative.ScanSettings expected) {
        ArgumentCaptor<WifiNative.ScanSettings> scanSettingsCaptor =
                ArgumentCaptor.forClass(WifiNative.ScanSettings.class);
        ArgumentCaptor<WifiNative.ScanEventHandler> scanEventHandlerCaptor =
                ArgumentCaptor.forClass(WifiNative.ScanEventHandler.class);
        order.verify(mWifiScannerImpl).startBatchedScan(scanSettingsCaptor.capture(),
                scanEventHandlerCaptor.capture());
        assertNativeScanSettingsEquals(expected, scanSettingsCaptor.getValue());
        return scanEventHandlerCaptor.getValue();
    }

    private static final int MAX_AP_PER_SCAN = 16;
    private void startServiceAndLoadDriver() {
        mWifiScanningServiceImpl.startService();
        setupAndLoadDriver();
    }

    private void setupAndLoadDriver() {
        when(mWifiScannerImpl.getScanCapabilities(any(WifiNative.ScanCapabilities.class)))
                .thenAnswer(new AnswerWithArguments() {
                        public boolean answer(WifiNative.ScanCapabilities capabilities) {
                            capabilities.max_scan_cache_size = Integer.MAX_VALUE;
                            capabilities.max_scan_buckets = 8;
                            capabilities.max_ap_cache_per_scan = MAX_AP_PER_SCAN;
                            capabilities.max_rssi_sample_size = 8;
                            capabilities.max_scan_reporting_threshold = 10;
                            capabilities.max_hotlist_bssids = 0;
                            capabilities.max_significant_wifi_change_aps = 0;
                            return true;
                        }
                    });
        ArgumentCaptor<BroadcastReceiver> broadcastReceiverCaptor =
                ArgumentCaptor.forClass(BroadcastReceiver.class);
        verify(mContext)
                .registerReceiver(broadcastReceiverCaptor.capture(), any(IntentFilter.class));
        mBroadcastReceiver = broadcastReceiverCaptor.getValue();
        TestUtil.sendWifiScanAvailable(broadcastReceiverCaptor.getValue(), mContext,
                WifiManager.WIFI_STATE_ENABLED);
        mLooper.dispatchAll();
    }

    private String dumpService() {
        StringWriter stringWriter = new StringWriter();
        mWifiScanningServiceImpl.dump(new FileDescriptor(), new PrintWriter(stringWriter),
                new String[0]);
        return stringWriter.toString();
    }

    private void assertDumpContainsRequestLog(String type, int id) {
        String serviceDump = dumpService();
        Pattern logLineRegex = Pattern.compile("^.+" + type + ": ClientInfo\\[uid=\\d+\\],Id=" +
                id + ".*$", Pattern.MULTILINE);
        assertTrue("dump did not contain log with type=" + type + ", id=" + id +
                ": " + serviceDump + "\n",
                logLineRegex.matcher(serviceDump).find());
   }

    private void assertDumpContainsCallbackLog(String callback, int id, String extra) {
        String serviceDump = dumpService();
        String extraPattern = extra == null ? "" : "," + extra;
        Pattern logLineRegex = Pattern.compile("^.+" + callback + ": ClientInfo\\[uid=\\d+\\],Id=" +
                id + extraPattern + "$", Pattern.MULTILINE);
        assertTrue("dump did not contain callback log with callback=" + callback + ", id=" + id +
                ", extra=" + extra + ": " + serviceDump + "\n",
                logLineRegex.matcher(serviceDump).find());
   }

    @Test
    public void construct() throws Exception {
        verifyNoMoreInteractions(mWifiScannerImpl, mWifiScannerImpl,
                mWifiScannerImplFactory, mBatteryStats);
        dumpService(); // make sure this succeeds
    }

    @Test
    public void startService() throws Exception {
        mWifiScanningServiceImpl.startService();
        verifyNoMoreInteractions(mWifiScannerImplFactory);

        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder order = inOrder(handler);
        sendBackgroundScanRequest(controlChannel, 122, generateValidScanSettings(), null);
        mLooper.dispatchAll();
        verifyFailedResponse(order, handler, 122, WifiScanner.REASON_UNSPECIFIED, "not available");
    }

    @Test
    public void disconnectClientBeforeWifiEnabled() throws Exception {
        mWifiScanningServiceImpl.startService();

        BidirectionalAsyncChannel controlChannel = connectChannel(mock(Handler.class));
        mLooper.dispatchAll();

        controlChannel.disconnect();
        mLooper.dispatchAll();
    }

    @Test
    public void loadDriver() throws Exception {
        startServiceAndLoadDriver();
        verify(mWifiScannerImplFactory, times(1))
                .create(any(Context.class), any(Looper.class), any(Clock.class));

        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder order = inOrder(handler);
        when(mWifiScannerImpl.startBatchedScan(any(WifiNative.ScanSettings.class),
                        any(WifiNative.ScanEventHandler.class))).thenReturn(true);
        sendBackgroundScanRequest(controlChannel, 192, generateValidScanSettings(), null);
        mLooper.dispatchAll();
        verifySuccessfulResponse(order, handler, 192);
        assertDumpContainsRequestLog("addBackgroundScanRequest", 192);
    }

    @Test
    public void disconnectClientAfterStartingWifi() throws Exception {
        mWifiScanningServiceImpl.startService();

        BidirectionalAsyncChannel controlChannel = connectChannel(mock(Handler.class));
        mLooper.dispatchAll();

        setupAndLoadDriver();

        controlChannel.disconnect();
        mLooper.dispatchAll();
    }

    @Test
    public void connectAndDisconnectClientAfterStartingWifi() throws Exception {
        startServiceAndLoadDriver();

        BidirectionalAsyncChannel controlChannel = connectChannel(mock(Handler.class));
        mLooper.dispatchAll();
        controlChannel.disconnect();
        mLooper.dispatchAll();
    }

    @Test
    public void sendInvalidCommand() throws Exception {
        startServiceAndLoadDriver();

        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder order = inOrder(handler, mWifiScannerImpl);
        controlChannel.sendMessage(Message.obtain(null, Protocol.BASE_WIFI_MANAGER));
        mLooper.dispatchAll();
        verifyFailedResponse(order, handler, 0, WifiScanner.REASON_INVALID_REQUEST,
                "Invalid request");
    }

    private void doSuccessfulSingleScan(WifiScanner.ScanSettings requestSettings,
            WifiNative.ScanSettings nativeSettings, ScanResults results) throws RemoteException {
        int requestId = 12;
        WorkSource workSource = new WorkSource(2292);
        startServiceAndLoadDriver();

        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder order = inOrder(handler, mWifiScannerImpl);

        when(mWifiScannerImpl.startSingleScan(any(WifiNative.ScanSettings.class),
                        any(WifiNative.ScanEventHandler.class))).thenReturn(true);

        sendSingleScanRequest(controlChannel, requestId, requestSettings, workSource);

        mLooper.dispatchAll();
        WifiNative.ScanEventHandler eventHandler = verifyStartSingleScan(order, nativeSettings);
        verifySuccessfulResponse(order, handler, requestId);
        verify(mBatteryStats).noteWifiScanStartedFromSource(eq(workSource));

        when(mWifiScannerImpl.getLatestSingleScanResults())
                .thenReturn(results.getRawScanData());
        eventHandler.onScanStatus(WifiNative.WIFI_SCAN_RESULTS_AVAILABLE);

        mLooper.dispatchAll();
        verifyScanResultsRecieved(order, handler, requestId, results.getScanData());
        verifySingleScanCompletedRecieved(order, handler, requestId);
        verifyNoMoreInteractions(handler);
        verify(mBatteryStats).noteWifiScanStoppedFromSource(eq(workSource));
        assertDumpContainsRequestLog("addSingleScanRequest", requestId);
        assertDumpContainsCallbackLog("singleScanResults", requestId,
                "results=" + results.getScanData().getResults().length);
    }

    /**
     * Do a single scan for a band and verify that it is successful.
     */
    @Test
    public void sendSingleScanBandRequest() throws Exception {
        WifiScanner.ScanSettings requestSettings = createRequest(WifiScanner.WIFI_BAND_BOTH, 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        doSuccessfulSingleScan(requestSettings, computeSingleScanNativeSettings(requestSettings),
                ScanResults.create(0, 2400, 5150, 5175));
    }

    /**
     * Do a single scan for a list of channels and verify that it is successful.
     */
    @Test
    public void sendSingleScanChannelsRequest() throws Exception {
        WifiScanner.ScanSettings requestSettings = createRequest(WifiScanner.WIFI_BAND_BOTH, 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        doSuccessfulSingleScan(requestSettings, computeSingleScanNativeSettings(requestSettings),
                ScanResults.create(0, 2400, 5150, 5175));
    }

    /**
     * Do a single scan with no results and verify that it is successful.
     */
    @Test
    public void sendSingleScanRequestWithNoResults() throws Exception {
        WifiScanner.ScanSettings requestSettings = createRequest(WifiScanner.WIFI_BAND_BOTH, 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        doSuccessfulSingleScan(requestSettings, computeSingleScanNativeSettings(requestSettings),
                ScanResults.create(0, new int[0]));
    }

    /**
     * Do a single scan with results that do not match the requested scan and verify that it is
     * still successful (and returns no results).
     */
    @Test
    public void sendSingleScanRequestWithBadRawResults() throws Exception {
        WifiScanner.ScanSettings requestSettings = createRequest(WifiScanner.WIFI_BAND_24_GHZ, 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        // Create a set of scan results that has results not matching the request settings, but is
        // limited to zero results for the expected results.
        ScanResults results = ScanResults.createOverflowing(0, 0,
                ScanResults.generateNativeResults(0, 5150, 5171));
        doSuccessfulSingleScan(requestSettings, computeSingleScanNativeSettings(requestSettings),
                results);
    }

    /**
     * Do a single scan, which the hardware fails to start, and verify that a failure response is
     * delivered.
     */
    @Test
    public void sendSingleScanRequestWhichFailsToStart() throws Exception {
        WifiScanner.ScanSettings requestSettings = createRequest(WifiScanner.WIFI_BAND_BOTH, 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        int requestId = 33;

        startServiceAndLoadDriver();

        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder order = inOrder(handler, mWifiScannerImpl);

        // scan fails
        when(mWifiScannerImpl.startSingleScan(any(WifiNative.ScanSettings.class),
                        any(WifiNative.ScanEventHandler.class))).thenReturn(false);

        sendSingleScanRequest(controlChannel, requestId, requestSettings, null);

        mLooper.dispatchAll();
        // Scan is successfully queue, but then fails to execute
        ArgumentCaptor<Message> messageCaptor = ArgumentCaptor.forClass(Message.class);
        order.verify(handler, times(2)).handleMessage(messageCaptor.capture());
        assertSuccessfulResponse(requestId, messageCaptor.getAllValues().get(0));
        assertFailedResponse(requestId, WifiScanner.REASON_UNSPECIFIED,
                "Failed to start single scan", messageCaptor.getAllValues().get(1));
        verifyNoMoreInteractions(mBatteryStats);

        assertEquals(mWifiMetrics.getOneshotScanCount(), 1);
        assertEquals(mWifiMetrics.getScanReturnEntry(WifiMetricsProto.WifiLog.SCAN_UNKNOWN), 1);
        assertDumpContainsRequestLog("addSingleScanRequest", requestId);
    }

    /**
     * Do a single scan, which successfully starts, but fails partway through and verify that a
     * failure response is delivered.
     */
    @Test
    public void sendSingleScanRequestWhichFailsAfterStart() throws Exception {
        WifiScanner.ScanSettings requestSettings = createRequest(WifiScanner.WIFI_BAND_BOTH, 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        int requestId = 33;
        WorkSource workSource = new WorkSource(Binder.getCallingUid()); // don't explicitly set

        startServiceAndLoadDriver();

        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder order = inOrder(handler, mWifiScannerImpl);

        // successful start
        when(mWifiScannerImpl.startSingleScan(any(WifiNative.ScanSettings.class),
                        any(WifiNative.ScanEventHandler.class))).thenReturn(true);

        sendSingleScanRequest(controlChannel, requestId, requestSettings, null);

        // Scan is successfully queue
        mLooper.dispatchAll();
        WifiNative.ScanEventHandler eventHandler =
                verifyStartSingleScan(order, computeSingleScanNativeSettings(requestSettings));
        verifySuccessfulResponse(order, handler, requestId);
        verify(mBatteryStats).noteWifiScanStartedFromSource(eq(workSource));

        // but then fails to execute
        eventHandler.onScanStatus(WifiNative.WIFI_SCAN_FAILED);
        mLooper.dispatchAll();
        verifyFailedResponse(order, handler, requestId,
                WifiScanner.REASON_UNSPECIFIED, "Scan failed");
        assertDumpContainsCallbackLog("singleScanFailed", requestId,
                "reason=" + WifiScanner.REASON_UNSPECIFIED + ", Scan failed");
        assertEquals(mWifiMetrics.getOneshotScanCount(), 1);
        assertEquals(mWifiMetrics.getScanReturnEntry(WifiMetricsProto.WifiLog.SCAN_UNKNOWN), 1);
        verify(mBatteryStats).noteWifiScanStoppedFromSource(eq(workSource));
    }

    // TODO Add more single scan tests
    // * disable wifi while scanning
    // * disable wifi while scanning with pending scan

    /**
     * Send a single scan request and then a second one after the first completes.
     */
    @Test
    public void sendSingleScanRequestAfterPreviousCompletes() {
        WifiScanner.ScanSettings requestSettings1 = createRequest(channelsToSpec(2400), 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        int requestId1 = 12;
        ScanResults results1 = ScanResults.create(0, 2400);


        WifiScanner.ScanSettings requestSettings2 = createRequest(channelsToSpec(2450, 5175), 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        int requestId2 = 13;
        ScanResults results2 = ScanResults.create(0, 2450);


        startServiceAndLoadDriver();

        when(mWifiScannerImpl.startSingleScan(any(WifiNative.ScanSettings.class),
                        any(WifiNative.ScanEventHandler.class))).thenReturn(true);

        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder order = inOrder(handler, mWifiScannerImpl);

        // Run scan 1
        sendSingleScanRequest(controlChannel, requestId1, requestSettings1, null);

        mLooper.dispatchAll();
        WifiNative.ScanEventHandler eventHandler1 = verifyStartSingleScan(order,
                computeSingleScanNativeSettings(requestSettings1));
        verifySuccessfulResponse(order, handler, requestId1);

        // dispatch scan 1 results
        when(mWifiScannerImpl.getLatestSingleScanResults())
                .thenReturn(results1.getScanData());
        eventHandler1.onScanStatus(WifiNative.WIFI_SCAN_RESULTS_AVAILABLE);

        mLooper.dispatchAll();
        verifyScanResultsRecieved(order, handler, requestId1, results1.getScanData());
        verifySingleScanCompletedRecieved(order, handler, requestId1);

        // Run scan 2
        sendSingleScanRequest(controlChannel, requestId2, requestSettings2, null);

        mLooper.dispatchAll();
        WifiNative.ScanEventHandler eventHandler2 = verifyStartSingleScan(order,
                computeSingleScanNativeSettings(requestSettings2));
        verifySuccessfulResponse(order, handler, requestId2);

        // dispatch scan 2 results
        when(mWifiScannerImpl.getLatestSingleScanResults())
                .thenReturn(results2.getScanData());
        eventHandler2.onScanStatus(WifiNative.WIFI_SCAN_RESULTS_AVAILABLE);

        mLooper.dispatchAll();
        verifyScanResultsRecieved(order, handler, requestId2, results2.getScanData());
        verifySingleScanCompletedRecieved(order, handler, requestId2);
    }

    /**
     * Send a single scan request and then a second one before the first completes.
     * Verify that both are scheduled and succeed.
     */
    @Test
    public void sendSingleScanRequestWhilePreviousScanRunning() {
        WifiScanner.ScanSettings requestSettings1 = createRequest(channelsToSpec(2400), 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        int requestId1 = 12;
        ScanResults results1 = ScanResults.create(0, 2400);

        WifiScanner.ScanSettings requestSettings2 = createRequest(channelsToSpec(2450, 5175), 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        int requestId2 = 13;
        ScanResults results2 = ScanResults.create(0, 2450);


        startServiceAndLoadDriver();

        when(mWifiScannerImpl.startSingleScan(any(WifiNative.ScanSettings.class),
                        any(WifiNative.ScanEventHandler.class))).thenReturn(true);

        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder handlerOrder = inOrder(handler);
        InOrder nativeOrder = inOrder(mWifiScannerImpl);

        // Run scan 1
        sendSingleScanRequest(controlChannel, requestId1, requestSettings1, null);

        mLooper.dispatchAll();
        WifiNative.ScanEventHandler eventHandler1 = verifyStartSingleScan(nativeOrder,
                computeSingleScanNativeSettings(requestSettings1));
        verifySuccessfulResponse(handlerOrder, handler, requestId1);

        // Queue scan 2 (will not run because previous is in progress)
        sendSingleScanRequest(controlChannel, requestId2, requestSettings2, null);
        mLooper.dispatchAll();
        verifySuccessfulResponse(handlerOrder, handler, requestId2);

        // dispatch scan 1 results
        when(mWifiScannerImpl.getLatestSingleScanResults())
                .thenReturn(results1.getScanData());
        eventHandler1.onScanStatus(WifiNative.WIFI_SCAN_RESULTS_AVAILABLE);

        mLooper.dispatchAll();
        verifyScanResultsRecieved(handlerOrder, handler, requestId1, results1.getScanData());
        verifySingleScanCompletedRecieved(handlerOrder, handler, requestId1);

        // now that the first scan completed we expect the second one to start
        WifiNative.ScanEventHandler eventHandler2 = verifyStartSingleScan(nativeOrder,
                computeSingleScanNativeSettings(requestSettings2));

        // dispatch scan 2 results
        when(mWifiScannerImpl.getLatestSingleScanResults())
                .thenReturn(results2.getScanData());
        eventHandler2.onScanStatus(WifiNative.WIFI_SCAN_RESULTS_AVAILABLE);

        mLooper.dispatchAll();
        verifyScanResultsRecieved(handlerOrder, handler, requestId2, results2.getScanData());
        verifySingleScanCompletedRecieved(handlerOrder, handler, requestId2);
        assertEquals(mWifiMetrics.getOneshotScanCount(), 2);
        assertEquals(mWifiMetrics.getScanReturnEntry(WifiMetricsProto.WifiLog.SCAN_SUCCESS), 2);
    }


    /**
     * Send a single scan request and then two more before the first completes.
     * Verify that the first completes and the second two are merged.
     */
    @Test
    public void sendMultipleSingleScanRequestWhilePreviousScanRunning() throws RemoteException {
        WifiScanner.ScanSettings requestSettings1 = createRequest(channelsToSpec(2400), 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        int requestId1 = 12;
        WorkSource workSource1 = new WorkSource(1121);
        ScanResults results1 = ScanResults.create(0, 2400);

        WifiScanner.ScanSettings requestSettings2 = createRequest(channelsToSpec(2450, 5175), 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        int requestId2 = 13;
        WorkSource workSource2 = new WorkSource(Binder.getCallingUid()); // don't explicitly set
        ScanResults results2 = ScanResults.create(0, 2450, 5175, 2450);

        WifiScanner.ScanSettings requestSettings3 = createRequest(channelsToSpec(5150), 0,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        int requestId3 = 15;
        WorkSource workSource3 = new WorkSource(2292);
        ScanResults results3 = ScanResults.create(0, 5150, 5150, 5150, 5150);

        WifiNative.ScanSettings nativeSettings2and3 = createSingleScanNativeSettingsForChannels(
                WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN, channelsToSpec(2450, 5175, 5150));
        ScanResults results2and3 = ScanResults.merge(results2, results3);
        WorkSource workSource2and3 = new WorkSource();
        workSource2and3.add(workSource2);
        workSource2and3.add(workSource3);


        startServiceAndLoadDriver();

        when(mWifiScannerImpl.startSingleScan(any(WifiNative.ScanSettings.class),
                        any(WifiNative.ScanEventHandler.class))).thenReturn(true);

        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder handlerOrder = inOrder(handler);
        InOrder nativeOrder = inOrder(mWifiScannerImpl);

        // Run scan 1
        sendSingleScanRequest(controlChannel, requestId1, requestSettings1, workSource1);

        mLooper.dispatchAll();
        WifiNative.ScanEventHandler eventHandler1 = verifyStartSingleScan(nativeOrder,
                computeSingleScanNativeSettings(requestSettings1));
        verifySuccessfulResponse(handlerOrder, handler, requestId1);
        verify(mBatteryStats).noteWifiScanStartedFromSource(eq(workSource1));


        // Queue scan 2 (will not run because previous is in progress)
        // uses uid of calling process
        sendSingleScanRequest(controlChannel, requestId2, requestSettings2, null);
        mLooper.dispatchAll();
        verifySuccessfulResponse(handlerOrder, handler, requestId2);

        // Queue scan 3 (will not run because previous is in progress)
        sendSingleScanRequest(controlChannel, requestId3, requestSettings3, workSource3);
        mLooper.dispatchAll();
        verifySuccessfulResponse(handlerOrder, handler, requestId3);

        // dispatch scan 1 results
        when(mWifiScannerImpl.getLatestSingleScanResults())
                .thenReturn(results1.getScanData());
        eventHandler1.onScanStatus(WifiNative.WIFI_SCAN_RESULTS_AVAILABLE);

        mLooper.dispatchAll();
        verifyScanResultsRecieved(handlerOrder, handler, requestId1, results1.getScanData());
        verifySingleScanCompletedRecieved(handlerOrder, handler, requestId1);
        verify(mBatteryStats).noteWifiScanStoppedFromSource(eq(workSource1));
        verify(mBatteryStats).noteWifiScanStartedFromSource(eq(workSource2and3));

        // now that the first scan completed we expect the second and third ones to start
        WifiNative.ScanEventHandler eventHandler2and3 = verifyStartSingleScan(nativeOrder,
                nativeSettings2and3);

        // dispatch scan 2 and 3 results
        when(mWifiScannerImpl.getLatestSingleScanResults())
                .thenReturn(results2and3.getScanData());
        eventHandler2and3.onScanStatus(WifiNative.WIFI_SCAN_RESULTS_AVAILABLE);

        mLooper.dispatchAll();

        // unfortunatally the order that these events are dispatched is dependant on the order which
        // they are iterated through internally
        ArgumentCaptor<Message> messageCaptor = ArgumentCaptor.forClass(Message.class);
        handlerOrder.verify(handler, times(4)).handleMessage(messageCaptor.capture());
        int firstListenerId = messageCaptor.getAllValues().get(0).arg2;
        assertTrue(firstListenerId + " was neither " + requestId2 + " nor " + requestId3,
                firstListenerId == requestId2 || firstListenerId == requestId3);
        if (firstListenerId == requestId2) {
            assertScanResultsMessage(requestId2,
                    new WifiScanner.ScanData[] {results2.getScanData()},
                    messageCaptor.getAllValues().get(0));
            assertSingleScanCompletedMessage(requestId2, messageCaptor.getAllValues().get(1));
            assertScanResultsMessage(requestId3,
                    new WifiScanner.ScanData[] {results3.getScanData()},
                    messageCaptor.getAllValues().get(2));
            assertSingleScanCompletedMessage(requestId3, messageCaptor.getAllValues().get(3));
        } else {
            assertScanResultsMessage(requestId3,
                    new WifiScanner.ScanData[] {results3.getScanData()},
                    messageCaptor.getAllValues().get(0));
            assertSingleScanCompletedMessage(requestId3, messageCaptor.getAllValues().get(1));
            assertScanResultsMessage(requestId2,
                    new WifiScanner.ScanData[] {results2.getScanData()},
                    messageCaptor.getAllValues().get(2));
            assertSingleScanCompletedMessage(requestId2, messageCaptor.getAllValues().get(3));
        }
        assertEquals(mWifiMetrics.getOneshotScanCount(), 3);
        assertEquals(mWifiMetrics.getScanReturnEntry(WifiMetricsProto.WifiLog.SCAN_SUCCESS), 3);

        verify(mBatteryStats).noteWifiScanStoppedFromSource(eq(workSource2and3));

        assertDumpContainsRequestLog("addSingleScanRequest", requestId1);
        assertDumpContainsRequestLog("addSingleScanRequest", requestId2);
        assertDumpContainsRequestLog("addSingleScanRequest", requestId3);
        assertDumpContainsCallbackLog("singleScanResults", requestId1,
                "results=" + results1.getRawScanResults().length);
        assertDumpContainsCallbackLog("singleScanResults", requestId2,
                "results=" + results2.getRawScanResults().length);
        assertDumpContainsCallbackLog("singleScanResults", requestId3,
                "results=" + results3.getRawScanResults().length);
    }

    private void doSuccessfulBackgroundScan(WifiScanner.ScanSettings requestSettings,
            WifiNative.ScanSettings nativeSettings) {
        startServiceAndLoadDriver();

        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder order = inOrder(handler, mWifiScannerImpl);

        when(mWifiScannerImpl.startBatchedScan(any(WifiNative.ScanSettings.class),
                        any(WifiNative.ScanEventHandler.class))).thenReturn(true);

        sendBackgroundScanRequest(controlChannel, 12, requestSettings, null);
        mLooper.dispatchAll();
        verifyStartBackgroundScan(order, nativeSettings);
        verifySuccessfulResponse(order, handler, 12);
        verifyNoMoreInteractions(handler);
        assertDumpContainsRequestLog("addBackgroundScanRequest", 12);
    }

    /**
     * Do a background scan for a band and verify that it is successful.
     */
    @Test
    public void sendBackgroundScanBandRequest() throws Exception {
        WifiScanner.ScanSettings requestSettings = createRequest(WifiScanner.WIFI_BAND_BOTH, 30000,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        WifiNative.ScanSettings nativeSettings = new NativeScanSettingsBuilder()
                .withBasePeriod(30000)
                .withMaxApPerScan(MAX_AP_PER_SCAN)
                .withMaxScansToCache(BackgroundScanScheduler.DEFAULT_MAX_SCANS_TO_BATCH)
                .addBucketWithBand(30000, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN,
                        WifiScanner.WIFI_BAND_BOTH)
                .build();
        doSuccessfulBackgroundScan(requestSettings, nativeSettings);
        assertEquals(mWifiMetrics.getBackgroundScanCount(), 1);
    }

    /**
     * Do a background scan for a list of channels and verify that it is successful.
     */
    @Test
    public void sendBackgroundScanChannelsRequest() throws Exception {
        WifiScanner.ScanSettings requestSettings = createRequest(channelsToSpec(5150), 30000,
                0, 20, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        WifiNative.ScanSettings nativeSettings = new NativeScanSettingsBuilder()
                .withBasePeriod(30000)
                .withMaxApPerScan(MAX_AP_PER_SCAN)
                .withMaxScansToCache(BackgroundScanScheduler.DEFAULT_MAX_SCANS_TO_BATCH)
                .addBucketWithChannels(30000, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN, 5150)
                .build();
        doSuccessfulBackgroundScan(requestSettings, nativeSettings);
    }

    private Pair<WifiScanner.ScanSettings, WifiNative.ScanSettings> createScanSettingsForHwPno()
            throws Exception {
        WifiScanner.ScanSettings requestSettings = createRequest(
                channelsToSpec(0, 2400, 5150, 5175), 30000, 0, 20,
                WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN);
        WifiNative.ScanSettings nativeSettings = new NativeScanSettingsBuilder()
                .withBasePeriod(30000)
                .withMaxApPerScan(MAX_AP_PER_SCAN)
                .withMaxScansToCache(BackgroundScanScheduler.DEFAULT_MAX_SCANS_TO_BATCH)
                .addBucketWithChannels(30000, WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN,
                        0, 2400, 5150, 5175)
                .build();
        return Pair.create(requestSettings, nativeSettings);
    }

    private Pair<WifiScanner.ScanSettings, WifiNative.ScanSettings> createScanSettingsForSwPno()
            throws Exception {
        Pair<WifiScanner.ScanSettings, WifiNative.ScanSettings> settingsPair =
                createScanSettingsForHwPno();

        WifiScanner.ScanSettings requestSettings = settingsPair.first;
        WifiNative.ScanSettings nativeSettings = settingsPair.second;
        // reportEvents field is overridden for SW PNO
        for (int i = 0; i < nativeSettings.buckets.length; i++) {
            nativeSettings.buckets[i].report_events = WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN
                    | WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT;
        }
        return Pair.create(requestSettings, nativeSettings);
    }

    private Pair<WifiScanner.PnoSettings, WifiNative.PnoSettings> createPnoSettings(
            ScanResults results)
            throws Exception {
        WifiScanner.PnoSettings requestPnoSettings = new WifiScanner.PnoSettings();
        requestPnoSettings.networkList =
                new WifiScanner.PnoSettings.PnoNetwork[results.getRawScanResults().length];
        int i = 0;
        for (ScanResult scanResult : results.getRawScanResults()) {
            requestPnoSettings.networkList[i++] =
                    new WifiScanner.PnoSettings.PnoNetwork(scanResult.SSID);
        }

        WifiNative.PnoSettings nativePnoSettings = new WifiNative.PnoSettings();
        nativePnoSettings.min5GHzRssi = requestPnoSettings.min5GHzRssi;
        nativePnoSettings.min24GHzRssi = requestPnoSettings.min24GHzRssi;
        nativePnoSettings.initialScoreMax = requestPnoSettings.initialScoreMax;
        nativePnoSettings.currentConnectionBonus = requestPnoSettings.currentConnectionBonus;
        nativePnoSettings.sameNetworkBonus = requestPnoSettings.sameNetworkBonus;
        nativePnoSettings.secureBonus = requestPnoSettings.secureBonus;
        nativePnoSettings.band5GHzBonus = requestPnoSettings.band5GHzBonus;
        nativePnoSettings.isConnected = requestPnoSettings.isConnected;
        nativePnoSettings.networkList =
                new WifiNative.PnoNetwork[requestPnoSettings.networkList.length];
        for (i = 0; i < requestPnoSettings.networkList.length; i++) {
            nativePnoSettings.networkList[i] = new WifiNative.PnoNetwork();
            nativePnoSettings.networkList[i].ssid = requestPnoSettings.networkList[i].ssid;
            nativePnoSettings.networkList[i].networkId =
                    requestPnoSettings.networkList[i].networkId;
            nativePnoSettings.networkList[i].priority = requestPnoSettings.networkList[i].priority;
            nativePnoSettings.networkList[i].flags = requestPnoSettings.networkList[i].flags;
            nativePnoSettings.networkList[i].auth_bit_field =
                    requestPnoSettings.networkList[i].authBitField;
        }
        return Pair.create(requestPnoSettings, nativePnoSettings);
    }

    private ScanResults createScanResultsForPno() {
        return ScanResults.create(0, 2400, 5150, 5175);
    }

    private ScanResults createScanResultsForPnoWithNoIE() {
        return ScanResults.createWithNoIE(0, 2400, 5150, 5175);
    }

    private WifiNative.PnoEventHandler verifyHwPno(InOrder order,
            WifiNative.PnoSettings expected) {
        ArgumentCaptor<WifiNative.PnoSettings> pnoSettingsCaptor =
                ArgumentCaptor.forClass(WifiNative.PnoSettings.class);
        ArgumentCaptor<WifiNative.PnoEventHandler> pnoEventHandlerCaptor =
                ArgumentCaptor.forClass(WifiNative.PnoEventHandler.class);
        order.verify(mWifiScannerImpl).setHwPnoList(pnoSettingsCaptor.capture(),
                pnoEventHandlerCaptor.capture());
        assertNativePnoSettingsEquals(expected, pnoSettingsCaptor.getValue());
        return pnoEventHandlerCaptor.getValue();
    }

    private void sendPnoScanRequest(BidirectionalAsyncChannel controlChannel,
            int scanRequestId, WifiScanner.ScanSettings scanSettings,
            WifiScanner.PnoSettings pnoSettings) {
        Bundle pnoParams = new Bundle();
        scanSettings.isPnoScan = true;
        pnoParams.putParcelable(WifiScanner.PNO_PARAMS_SCAN_SETTINGS_KEY, scanSettings);
        pnoParams.putParcelable(WifiScanner.PNO_PARAMS_PNO_SETTINGS_KEY, pnoSettings);
        controlChannel.sendMessage(Message.obtain(null, WifiScanner.CMD_START_PNO_SCAN, 0,
                scanRequestId, pnoParams));
    }

    private void assertPnoNetworkFoundMessage(int listenerId, ScanResult[] expected,
            Message networkFoundMessage) {
        assertEquals("what", WifiScanner.CMD_PNO_NETWORK_FOUND, networkFoundMessage.what);
        assertEquals("listenerId", listenerId, networkFoundMessage.arg2);
        assertScanResultsEquals(expected,
                ((WifiScanner.ParcelableScanResults) networkFoundMessage.obj).getResults());
    }

    private void verifyPnoNetworkFoundRecieved(InOrder order, Handler handler, int listenerId,
            ScanResult[] expected) {
        Message scanResultMessage = verifyHandleMessageAndGetMessage(order, handler,
                WifiScanner.CMD_PNO_NETWORK_FOUND);
        assertPnoNetworkFoundMessage(listenerId, expected, scanResultMessage);
    }

    private void expectSuccessfulBackgroundScan(InOrder order,
            WifiNative.ScanSettings nativeSettings, ScanResults results) {
        when(mWifiScannerImpl.startBatchedScan(any(WifiNative.ScanSettings.class),
                any(WifiNative.ScanEventHandler.class))).thenReturn(true);
        mLooper.dispatchAll();
        WifiNative.ScanEventHandler eventHandler = verifyStartBackgroundScan(order, nativeSettings);
        WifiScanner.ScanData[] scanDatas = new WifiScanner.ScanData[1];
        scanDatas[0] = results.getScanData();
        for (ScanResult fullScanResult : results.getRawScanResults()) {
            eventHandler.onFullScanResult(fullScanResult, 0);
        }
        when(mWifiScannerImpl.getLatestBatchedScanResults(anyBoolean())).thenReturn(scanDatas);
        eventHandler.onScanStatus(WifiNative.WIFI_SCAN_RESULTS_AVAILABLE);
        mLooper.dispatchAll();
    }

    private void expectHwPnoScanWithNoBackgroundScan(InOrder order, Handler handler, int requestId,
            WifiNative.PnoSettings nativeSettings, ScanResults results) {
        when(mWifiScannerImpl.isHwPnoSupported(anyBoolean())).thenReturn(true);
        when(mWifiScannerImpl.shouldScheduleBackgroundScanForHwPno()).thenReturn(false);

        when(mWifiScannerImpl.setHwPnoList(any(WifiNative.PnoSettings.class),
                any(WifiNative.PnoEventHandler.class))).thenReturn(true);
        mLooper.dispatchAll();
        WifiNative.PnoEventHandler eventHandler = verifyHwPno(order, nativeSettings);
        verifySuccessfulResponse(order, handler, requestId);
        eventHandler.onPnoNetworkFound(results.getRawScanResults());
        mLooper.dispatchAll();
    }

    private void expectHwPnoScanWithBackgroundScan(InOrder order, Handler handler, int requestId,
            WifiNative.ScanSettings nativeScanSettings,
            WifiNative.PnoSettings nativePnoSettings, ScanResults results) {
        when(mWifiScannerImpl.isHwPnoSupported(anyBoolean())).thenReturn(true);
        when(mWifiScannerImpl.shouldScheduleBackgroundScanForHwPno()).thenReturn(true);

        when(mWifiScannerImpl.setHwPnoList(any(WifiNative.PnoSettings.class),
                any(WifiNative.PnoEventHandler.class))).thenReturn(true);
        when(mWifiScannerImpl.startBatchedScan(any(WifiNative.ScanSettings.class),
                any(WifiNative.ScanEventHandler.class))).thenReturn(true);
        mLooper.dispatchAll();
        WifiNative.PnoEventHandler eventHandler = verifyHwPno(order, nativePnoSettings);
        verifySuccessfulResponse(order, handler, requestId);
        verifyStartBackgroundScan(order, nativeScanSettings);
        eventHandler.onPnoNetworkFound(results.getRawScanResults());
        mLooper.dispatchAll();
    }

    private void expectHwPnoScanWithBackgroundScanWithNoIE(InOrder order, Handler handler,
            int requestId, WifiNative.ScanSettings nativeBackgroundScanSettings,
            WifiNative.ScanSettings nativeSingleScanSettings,
            WifiNative.PnoSettings nativePnoSettings, ScanResults results) {
        when(mWifiScannerImpl.startSingleScan(any(WifiNative.ScanSettings.class),
                any(WifiNative.ScanEventHandler.class))).thenReturn(true);

        expectHwPnoScanWithBackgroundScan(order, handler, requestId, nativeBackgroundScanSettings,
                nativePnoSettings, results);
        WifiNative.ScanEventHandler eventHandler =
                verifyStartSingleScan(order, nativeSingleScanSettings);
        when(mWifiScannerImpl.getLatestSingleScanResults()).thenReturn(results.getScanData());
        eventHandler.onScanStatus(WifiNative.WIFI_SCAN_RESULTS_AVAILABLE);
        mLooper.dispatchAll();
    }
    private void expectSwPnoScan(InOrder order, WifiNative.ScanSettings nativeScanSettings,
            ScanResults results) {
        when(mWifiScannerImpl.isHwPnoSupported(anyBoolean())).thenReturn(false);
        when(mWifiScannerImpl.shouldScheduleBackgroundScanForHwPno()).thenReturn(true);

        expectSuccessfulBackgroundScan(order, nativeScanSettings, results);
    }

    /**
     * Tests Supplicant PNO scan when the PNO scan results contain IE info. This ensures that the
     * PNO scan results are plumbed back to the client as a PNO network found event.
     */
    @Test
    public void testSuccessfulHwPnoScanWithNoBackgroundScan() throws Exception {
        startServiceAndLoadDriver();
        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder order = inOrder(handler, mWifiScannerImpl);
        int requestId = 12;

        ScanResults scanResults = createScanResultsForPno();
        Pair<WifiScanner.ScanSettings, WifiNative.ScanSettings> scanSettings =
                createScanSettingsForHwPno();
        Pair<WifiScanner.PnoSettings, WifiNative.PnoSettings> pnoSettings =
                createPnoSettings(scanResults);

        sendPnoScanRequest(controlChannel, requestId, scanSettings.first, pnoSettings.first);
        expectHwPnoScanWithNoBackgroundScan(order, handler, requestId, pnoSettings.second,
                scanResults);
        verifyPnoNetworkFoundRecieved(order, handler, requestId, scanResults.getRawScanResults());
    }

    /**
     * Tests Hal ePNO scan when the PNO scan results contain IE info. This ensures that the
     * PNO scan results are plumbed back to the client as a PNO network found event.
     */
    @Test
    public void testSuccessfulHwPnoScanWithBackgroundScan() throws Exception {
        startServiceAndLoadDriver();
        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder order = inOrder(handler, mWifiScannerImpl);
        int requestId = 12;

        ScanResults scanResults = createScanResultsForPno();
        Pair<WifiScanner.ScanSettings, WifiNative.ScanSettings> scanSettings =
                createScanSettingsForHwPno();
        Pair<WifiScanner.PnoSettings, WifiNative.PnoSettings> pnoSettings =
                createPnoSettings(scanResults);

        sendPnoScanRequest(controlChannel, requestId, scanSettings.first, pnoSettings.first);
        expectHwPnoScanWithBackgroundScan(order, handler, requestId, scanSettings.second,
                pnoSettings.second, scanResults);
        verifyPnoNetworkFoundRecieved(order, handler, requestId, scanResults.getRawScanResults());
    }

    /**
     * Tests Hal ePNO scan when the PNO scan results don't contain IE info. This ensures that the
     * single scan results are plumbed back to the client as a PNO network found event.
     */
    @Test
    public void testSuccessfulHwPnoScanWithBackgroundScanWithNoIE() throws Exception {
        startServiceAndLoadDriver();
        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder order = inOrder(handler, mWifiScannerImpl);
        int requestId = 12;

        ScanResults scanResults = createScanResultsForPnoWithNoIE();
        Pair<WifiScanner.ScanSettings, WifiNative.ScanSettings> scanSettings =
                createScanSettingsForHwPno();
        Pair<WifiScanner.PnoSettings, WifiNative.PnoSettings> pnoSettings =
                createPnoSettings(scanResults);

        sendPnoScanRequest(controlChannel, requestId, scanSettings.first, pnoSettings.first);
        expectHwPnoScanWithBackgroundScanWithNoIE(order, handler, requestId, scanSettings.second,
                computeSingleScanNativeSettings(scanSettings.first), pnoSettings.second,
                scanResults);

        ArrayList<ScanResult> sortScanList =
                new ArrayList<ScanResult>(Arrays.asList(scanResults.getRawScanResults()));
        Collections.sort(sortScanList, WifiScannerImpl.SCAN_RESULT_SORT_COMPARATOR);
        verifyPnoNetworkFoundRecieved(order, handler, requestId,
                sortScanList.toArray(new ScanResult[sortScanList.size()]));
    }

    /**
     * Tests SW PNO scan. This ensures that the background scan results are plumbed back to the
     * client as a PNO network found event.
     */
    @Test
    public void testSuccessfulSwPnoScan() throws Exception {
        startServiceAndLoadDriver();
        Handler handler = mock(Handler.class);
        BidirectionalAsyncChannel controlChannel = connectChannel(handler);
        InOrder order = inOrder(handler, mWifiScannerImpl);
        int requestId = 12;

        ScanResults scanResults = createScanResultsForPno();
        Pair<WifiScanner.ScanSettings, WifiNative.ScanSettings> scanSettings =
                createScanSettingsForSwPno();
        Pair<WifiScanner.PnoSettings, WifiNative.PnoSettings> pnoSettings =
                createPnoSettings(scanResults);

        sendPnoScanRequest(controlChannel, requestId, scanSettings.first, pnoSettings.first);
        expectSwPnoScan(order, scanSettings.second, scanResults);
        verifyPnoNetworkFoundRecieved(order, handler, requestId, scanResults.getRawScanResults());
    }
}
