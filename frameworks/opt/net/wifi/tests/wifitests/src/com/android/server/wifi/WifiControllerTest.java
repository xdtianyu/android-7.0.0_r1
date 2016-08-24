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

import static com.android.server.wifi.WifiController.CMD_AP_STOPPED;
import static com.android.server.wifi.WifiController.CMD_EMERGENCY_CALL_STATE_CHANGED;
import static com.android.server.wifi.WifiController.CMD_EMERGENCY_MODE_CHANGED;
import static com.android.server.wifi.WifiController.CMD_SET_AP;
import static com.android.server.wifi.WifiController.CMD_WIFI_TOGGLED;

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.*;
import static org.mockito.Mockito.*;

import android.content.ContentResolver;
import android.content.Context;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Log;

import com.android.internal.util.IState;
import com.android.internal.util.StateMachine;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.io.ByteArrayOutputStream;
import java.io.PrintWriter;
import java.lang.reflect.Method;

/**
 * Test WifiController for changes in and out of ECM and SoftAP modes.
 */
@SmallTest
public class WifiControllerTest {

    private static final String TAG = "WifiControllerTest";

    private void dumpState() {
        ByteArrayOutputStream stream = new ByteArrayOutputStream();
        PrintWriter writer = new PrintWriter(stream);
        mWifiController.dump(null, writer, null);
        writer.flush();
        Log.d(TAG, "WifiStateMachine state -" + stream.toString());
    }

    private IState getCurrentState() throws Exception {
        Method method = StateMachine.class.getDeclaredMethod("getCurrentState");
        method.setAccessible(true);
        return (IState) method.invoke(mWifiController);
    }

    private void initializeSettingsStore() throws Exception {
        when(mSettingsStore.isAirplaneModeOn()).thenReturn(false);
        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        when(mSettingsStore.isScanAlwaysAvailable()).thenReturn(true);
    }

    MockLooper mLooper;
    @Mock Context mContext;
    @Mock WifiServiceImpl mService;
    @Mock FrameworkFacade mFacade;
    @Mock WifiSettingsStore mSettingsStore;
    @Mock WifiStateMachine mWifiStateMachine;
    @Mock WifiServiceImpl.LockList mLockList;

    WifiController mWifiController;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        mLooper = new MockLooper();

        initializeSettingsStore();

        when(mContext.getContentResolver()).thenReturn(mock(ContentResolver.class));

        mWifiController = new WifiController(mContext, mWifiStateMachine,
                mSettingsStore, mLockList, mLooper.getLooper(), mFacade);

        mWifiController.start();
        mLooper.dispatchAll();
    }

    @After
    public void cleanUp() {
        mLooper.dispatchAll();
    }

    @Test
    public void enableWifi() throws Exception {
        assertEquals("StaDisabledWithScanState", getCurrentState().getName());

        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(true);
        mWifiController.sendMessage(CMD_WIFI_TOGGLED);
        mLooper.dispatchAll();
        assertEquals("DeviceActiveState", getCurrentState().getName());

        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(false);
        mWifiController.sendMessage(CMD_WIFI_TOGGLED);
        mLooper.dispatchAll();
        assertEquals("StaDisabledWithScanState", getCurrentState().getName());

        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(true);
        mWifiController.sendMessage(CMD_WIFI_TOGGLED);
        mLooper.dispatchAll();
        assertEquals("DeviceActiveState", getCurrentState().getName());
    }

    @Test
    public void testEcmOn() throws Exception {
        enableWifi();

        // Test with WifiDisableInECBM turned on:
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(true);
        doTestEcm(true);
    }

    @Test
    public void testEcmOff() throws Exception {
        enableWifi();

        // Test with WifiDisableInECBM turned off
        when(mFacade.getConfigWiFiDisableInECBM(mContext)).thenReturn(false);
        doTestEcm(false);
    }

    private void assertInEcm(boolean ecmEnabled) throws Exception {
        if (ecmEnabled) {
            assertEquals("EcmState", getCurrentState().getName());
        } else {
            assertEquals("DeviceActiveState", getCurrentState().getName());
        }
    }


    private void doTestEcm(boolean ecmEnabled) throws Exception {

        // test ecm changed
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();
        assertEquals("DeviceActiveState", getCurrentState().getName());

        // test call state changed
        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 1);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 0);
        mLooper.dispatchAll();
        assertEquals("DeviceActiveState", getCurrentState().getName());


        // test both changed (variation 1 - the good case)
        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 1);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 0);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();
        assertEquals("DeviceActiveState", getCurrentState().getName());

        // test both changed (variation 2 - emergency call in ecm)
        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 1);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 0);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();
        assertEquals("DeviceActiveState", getCurrentState().getName());

        // test both changed (variation 3 - not so good order of events)
        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 1);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 1);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        mWifiController.sendMessage(CMD_EMERGENCY_MODE_CHANGED, 0);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 0);
        mLooper.dispatchAll();
        assertEquals("DeviceActiveState", getCurrentState().getName());

        // test that Wifi toggle doesn't exit Ecm
        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 1);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        when(mSettingsStore.isWifiToggleEnabled()).thenReturn(true);
        mWifiController.sendMessage(CMD_WIFI_TOGGLED);
        mLooper.dispatchAll();
        assertInEcm(ecmEnabled);

        mWifiController.sendMessage(CMD_EMERGENCY_CALL_STATE_CHANGED, 0);
        mLooper.dispatchAll();
        assertEquals("DeviceActiveState", getCurrentState().getName());
    }

    /**
     * When AP mode is enabled and wifi was previously in AP mode, we should return to
     * DeviceActiveState after the AP is disabled.
     * Enter DeviceActiveState, activate AP mode, disable AP mode.
     * <p>
     * Expected: AP should successfully start and exit, then return to DeviceActiveState.
     */
    @Test
    public void testReturnToDeviceActiveStateAfterAPModeShutdown() throws Exception {
        enableWifi();
        assertEquals("DeviceActiveState", getCurrentState().getName());

        mWifiController.obtainMessage(CMD_SET_AP, 1, 0).sendToTarget();
        mLooper.dispatchAll();
        assertEquals("ApEnabledState", getCurrentState().getName());

        when(mSettingsStore.getWifiSavedState()).thenReturn(1);
        mWifiController.obtainMessage(CMD_AP_STOPPED).sendToTarget();
        mLooper.dispatchAll();

        InOrder inOrder = inOrder(mWifiStateMachine);
        inOrder.verify(mWifiStateMachine).setSupplicantRunning(true);
        inOrder.verify(mWifiStateMachine).setOperationalMode(WifiStateMachine.CONNECT_MODE);
        inOrder.verify(mWifiStateMachine).setDriverStart(true);
        assertEquals("DeviceActiveState", getCurrentState().getName());
    }
}
