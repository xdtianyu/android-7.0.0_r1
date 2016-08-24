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
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Notification;
import android.app.NotificationManager;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.os.UserHandle;
import android.provider.Settings;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for {@link com.android.server.wifi.WifiScanningServiceImpl}.
 */
@SmallTest
public class WifiNotificationControllerTest {
    public static final String TAG = "WifiScanningServiceTest";

    @Mock private Context mContext;
    @Mock private WifiStateMachine mWifiStateMachine;
    @Mock private FrameworkFacade mFrameworkFacade;
    @Mock private NotificationManager mNotificationManager;
    WifiNotificationController mWifiNotificationController;

    /**
     * Internal BroadcastReceiver that WifiNotificationController uses to listen for broadcasts
     * this is initialized by calling startServiceAndLoadDriver
     */
    BroadcastReceiver mBroadcastReceiver;

    /** Initialize objects before each test run. */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        // Needed for the NotificationEnabledSettingObserver.
        when(mContext.getContentResolver()).thenReturn(mock(ContentResolver.class));

        when(mContext.getSystemService(Context.NOTIFICATION_SERVICE))
                .thenReturn(mNotificationManager);

        when(mFrameworkFacade.getIntegerSetting(mContext,
                Settings.Global.WIFI_NETWORKS_AVAILABLE_NOTIFICATION_ON, 1)).thenReturn(1);

        MockLooper mock_looper = new MockLooper();
        mWifiNotificationController = new WifiNotificationController(
                mContext, mock_looper.getLooper(), mWifiStateMachine, mFrameworkFacade,
                mock(Notification.Builder.class));
        ArgumentCaptor<BroadcastReceiver> broadcastReceiverCaptor =
                ArgumentCaptor.forClass(BroadcastReceiver.class);

        verify(mContext)
                .registerReceiver(broadcastReceiverCaptor.capture(), any(IntentFilter.class));
        mBroadcastReceiver = broadcastReceiverCaptor.getValue();
    }

    private void setOpenAccessPoint() {
        List<ScanResult> scanResults = new ArrayList<>();
        ScanResult scanResult = new ScanResult();
        scanResult.capabilities = "[ESS]";
        scanResults.add(scanResult);
        when(mWifiStateMachine.syncGetScanResultsList()).thenReturn(scanResults);
    }

    /** Verifies that a notification is displayed (and retracted) given system events. */
    @Test
    public void verifyNotificationDisplayed() throws Exception {
        TestUtil.sendWifiStateChanged(mBroadcastReceiver, mContext, WifiManager.WIFI_STATE_ENABLED);
        TestUtil.sendNetworkStateChanged(mBroadcastReceiver, mContext,
                NetworkInfo.DetailedState.DISCONNECTED);
        setOpenAccessPoint();

        // The notification should not be displayed after only two scan results.
        TestUtil.sendScanResultsAvailable(mBroadcastReceiver, mContext);
        TestUtil.sendScanResultsAvailable(mBroadcastReceiver, mContext);
        verify(mNotificationManager, never())
                .notifyAsUser(any(String.class), anyInt(), any(Notification.class),
                        any(UserHandle.class));

        // Changing to and from "SCANNING" state should not affect the counter.
        TestUtil.sendNetworkStateChanged(mBroadcastReceiver, mContext,
                NetworkInfo.DetailedState.SCANNING);
        TestUtil.sendNetworkStateChanged(mBroadcastReceiver, mContext,
                NetworkInfo.DetailedState.DISCONNECTED);

        // Needed while WifiNotificationController creates its notification.
        when(mContext.getResources()).thenReturn(mock(Resources.class));

        // The third scan result notification will trigger the notification.
        TestUtil.sendScanResultsAvailable(mBroadcastReceiver, mContext);
        verify(mNotificationManager)
                .notifyAsUser(any(String.class), anyInt(), any(Notification.class),
                        any(UserHandle.class));
        verify(mNotificationManager, never())
                .cancelAsUser(any(String.class), anyInt(), any(UserHandle.class));

        // Changing network state should cause the notification to go away.
        TestUtil.sendNetworkStateChanged(mBroadcastReceiver, mContext,
                NetworkInfo.DetailedState.CONNECTED);
        verify(mNotificationManager)
                .cancelAsUser(any(String.class), anyInt(), any(UserHandle.class));
    }
}
