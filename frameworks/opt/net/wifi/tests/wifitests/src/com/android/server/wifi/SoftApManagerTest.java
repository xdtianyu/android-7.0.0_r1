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

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.InterfaceConfiguration;
import android.net.LinkAddress;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.INetworkManagementService;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Locale;

/** Unit tests for {@link SoftApManager}. */
@SmallTest
public class SoftApManagerTest {

    private static final String TAG = "SoftApManagerTest";

    private static final String TEST_INTERFACE_NAME = "TestInterface";
    private static final String TEST_COUNTRY_CODE = "TestCountry";
    private static final Integer[] ALLOWED_2G_CHANNELS = {1, 2, 3, 4};
    private static final String[] AVAILABLE_DEVICES = { TEST_INTERFACE_NAME };

    private final ArrayList<Integer> mAllowed2GChannels =
            new ArrayList<Integer>(Arrays.asList(ALLOWED_2G_CHANNELS));

    MockLooper mLooper;
    @Mock Context mContext;
    @Mock WifiNative mWifiNative;
    @Mock INetworkManagementService mNmService;
    @Mock ConnectivityManager mConnectivityManager;
    @Mock SoftApManager.Listener mListener;
    @Mock InterfaceConfiguration mInterfaceConfiguration;

    /**
     * Internal BroadcastReceiver that SoftApManager uses to listen for tethering
     * events from ConnectivityManager.
     */
    BroadcastReceiver mBroadcastReceiver;

    SoftApManager mSoftApManager;

    /** Sets up test. */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mLooper = new MockLooper();

        when(mWifiNative.getInterfaceName()).thenReturn(TEST_INTERFACE_NAME);
        when(mNmService.getInterfaceConfig(TEST_INTERFACE_NAME))
                .thenReturn(mInterfaceConfiguration);
        when(mConnectivityManager.getTetherableWifiRegexs())
                .thenReturn(AVAILABLE_DEVICES);

        mSoftApManager = new SoftApManager(mContext,
                                           mLooper.getLooper(),
                                           mWifiNative,
                                           mNmService,
                                           mConnectivityManager,
                                           TEST_COUNTRY_CODE,
                                           mAllowed2GChannels,
                                           mListener);
        ArgumentCaptor<BroadcastReceiver> broadcastReceiverCaptor =
                ArgumentCaptor.forClass(BroadcastReceiver.class);
        verify(mContext).registerReceiver(
                broadcastReceiverCaptor.capture(), any(IntentFilter.class));
        mBroadcastReceiver = broadcastReceiverCaptor.getValue();

        mLooper.dispatchAll();
    }

    /** Verifies startSoftAp will fail if AP configuration is not provided. */
    @Test
    public void startSoftApWithoutConfig() throws Exception {
        InOrder order = inOrder(mListener);

        mSoftApManager.start(null);
        mLooper.dispatchAll();

        order.verify(mListener).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLING, 0);
        order.verify(mListener).onStateChanged(
                WifiManager.WIFI_AP_STATE_FAILED, WifiManager.SAP_START_FAILURE_GENERAL);
    }

    /** Tests the handling of timeout after tethering is started. */
    @Test
    public void tetheringTimedOut() throws Exception {
        startSoftApAndVerifyEnabled();
        announceAvailableForTethering();
        verifyTetheringRequested();

        InOrder order = inOrder(mListener);

        /* Move the time forward to simulate notification timeout. */
        mLooper.moveTimeForward(5000);
        mLooper.dispatchAll();

        /* Verify soft ap is disabled. */
        verify(mNmService).stopAccessPoint(eq(TEST_INTERFACE_NAME));
        order.verify(mListener).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLING, 0);
        order.verify(mListener).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLED, 0);
    }

    /** Tests the handling of tethered notification after tethering is started. */
    @Test
    public void tetherCompleted() throws Exception {
        startSoftApAndVerifyEnabled();
        announceAvailableForTethering();
        verifyTetheringRequested();
        announceTethered();
        verifySoftApNotDisabled();
    }

    /** Tests the handling of stop command when soft AP is not started. */
    @Test
    public void stopWhenNotStarted() throws Exception {
        mSoftApManager.stop();
        mLooper.dispatchAll();
        /* Verify no state changes. */
        verify(mListener, never()).onStateChanged(anyInt(), anyInt());
    }

    /** Tests the handling of stop command when soft AP is started. */
    @Test
    public void stopWhenStarted() throws Exception {
        startSoftApAndVerifyEnabled();

        InOrder order = inOrder(mListener);

        mSoftApManager.stop();
        mLooper.dispatchAll();

        verify(mNmService).stopAccessPoint(TEST_INTERFACE_NAME);
        order.verify(mListener).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLING, 0);
        order.verify(mListener).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLED, 0);
    }

    /** Starts soft AP and verifies that it is enabled successfully. */
    protected void startSoftApAndVerifyEnabled() throws Exception {
        InOrder order = inOrder(mListener);

        /**
         *  Only test the default configuration. Testing for different configurations
         *  are taken care of by ApConfigUtilTest.
         */
        WifiConfiguration config = new WifiConfiguration();
        config.apBand = WifiConfiguration.AP_BAND_2GHZ;
        when(mWifiNative.isHalStarted()).thenReturn(false);
        when(mWifiNative.setCountryCodeHal(TEST_COUNTRY_CODE.toUpperCase(Locale.ROOT)))
                .thenReturn(true);
        mSoftApManager.start(config);
        mLooper.dispatchAll();
        verify(mNmService).startAccessPoint(
                any(WifiConfiguration.class), eq(TEST_INTERFACE_NAME));
        order.verify(mListener).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLING, 0);
        order.verify(mListener).onStateChanged(WifiManager.WIFI_AP_STATE_ENABLED, 0);
    }

    /** Verifies that soft AP was not disabled. */
    protected void verifySoftApNotDisabled() throws Exception {
        verify(mListener, never()).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLING, 0);
        verify(mListener, never()).onStateChanged(WifiManager.WIFI_AP_STATE_DISABLED, 0);
    }

    /** Sends a broadcast intent indicating that the interface is available for tethering. */
    protected void announceAvailableForTethering() throws Exception {
        when(mConnectivityManager.tether(TEST_INTERFACE_NAME))
                .thenReturn(ConnectivityManager.TETHER_ERROR_NO_ERROR);
        ArrayList<String> availableList =
                new ArrayList<String>(Arrays.asList(AVAILABLE_DEVICES));
        TestUtil.sendTetherStateChanged(
                mBroadcastReceiver, mContext, availableList, new ArrayList<String>());
        mLooper.dispatchAll();
    }

    /** Verifies that tethering was requested. */
    protected void verifyTetheringRequested() throws Exception {
        verify(mInterfaceConfiguration).setLinkAddress(any(LinkAddress.class));
        verify(mInterfaceConfiguration).setInterfaceUp();
        verify(mNmService).setInterfaceConfig(eq(TEST_INTERFACE_NAME), eq(mInterfaceConfiguration));
    }

    /** Sends a broadcast intent indicating that the interface is tethered. */
    protected void announceTethered() throws Exception {
        ArrayList<String> deviceList =
                new ArrayList<String>(Arrays.asList(AVAILABLE_DEVICES));
        TestUtil.sendTetherStateChanged(
                mBroadcastReceiver, mContext, deviceList, deviceList);
        mLooper.dispatchAll();
    }
}
