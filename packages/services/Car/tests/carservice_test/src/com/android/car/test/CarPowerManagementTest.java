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
package com.android.car.test;

import android.car.test.VehicleHalEmulator.VehicleHalPropertyHandler;
import android.content.Context;
import android.hardware.display.DisplayManager;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.Log;
import android.view.Display;

import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleApPowerBootupReason;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleApPowerSetState;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleApPowerState;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleApPowerStateConfigFlag;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleApPowerStateIndex;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleApPowerStateShutdownParam;
import com.android.car.vehiclenetwork.VehiclePropConfigUtil;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePermissionModel;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropAccess;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropChangeMode;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;
import com.android.car.vehiclenetwork.VehiclePropValueUtil;

import java.util.LinkedList;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

@MediumTest
public class CarPowerManagementTest extends MockedCarTestBase {

    private static final long TIMEOUT_MS = 3000;
    private static final String TAG = CarPowerManagementTest.class.getSimpleName();

    private final PowerStatePropertyHandler mPowerStateHandler = new PowerStatePropertyHandler();
    private final Handler mMainHandler = new Handler(Looper.getMainLooper());
    private final DisplayStateListener mDisplayListener = new DisplayStateListener();
    private DisplayManager mDisplayManager;

    @Override
    protected synchronized void setUp() throws Exception {
        super.setUp();
        mDisplayManager = (DisplayManager) getContext().getSystemService(Context.DISPLAY_SERVICE);
        mDisplayManager.registerDisplayListener(mDisplayListener, mMainHandler);
    }

    @Override
    protected synchronized void tearDown() throws Exception {
        super.tearDown();
        mDisplayManager.unregisterDisplayListener(mDisplayListener);
    }

    private void setupPowerPropertyAndStart(boolean allowSleep) {
        getVehicleHalEmulator().addProperty(
                VehiclePropConfigUtil.getBuilder(
                        VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC2,
                        VehiclePermissionModel.VEHICLE_PERMISSION_SYSTEM_APP_ONLY,
                        allowSleep ?
                                VehicleApPowerStateConfigFlag.VEHICLE_AP_POWER_STATE_CONFIG_ENABLE_DEEP_SLEEP_FLAG
                                : 0
                            /*configFlags*/,
                        0 /*sampleRateMax*/, 0 /*sampleRateMin*/).build(),
                mPowerStateHandler);
        getVehicleHalEmulator().addStaticProperty(
                VehiclePropConfigUtil.getBuilder(
                        VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_BOOTUP_REASON,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_STATIC,
                        VehicleValueType.VEHICLE_VALUE_TYPE_INT32,
                        VehiclePermissionModel.VEHICLE_PERMISSION_SYSTEM_APP_ONLY,
                        0 /*configFlags*/, 0 /*sampleRateMax*/, 0 /*sampleRateMin*/).build(),
                VehiclePropValueUtil.createIntValue(
                        VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_BOOTUP_REASON,
                        VehicleApPowerBootupReason.VEHICLE_AP_POWER_BOOTUP_REASON_USER_POWER_ON,
                        0));
        getVehicleHalEmulator().start();
    }

    public void testImmediateShutdown() throws Exception {
        setupPowerPropertyAndStart(true);
        assertBootComplete();
        mPowerStateHandler.sendPowerState(
                VehicleApPowerState.VEHICLE_AP_POWER_STATE_SHUTDOWN_PREPARE,
                VehicleApPowerStateShutdownParam.VEHICLE_AP_POWER_SHUTDOWN_PARAM_SHUTDOWN_IMMEDIATELY);
        mPowerStateHandler.waitForStateSetAndGetAll(TIMEOUT_MS,
                VehicleApPowerSetState.VEHICLE_AP_POWER_SET_SHUTDOWN_START);
        mPowerStateHandler.sendPowerState(
                VehicleApPowerState.VEHICLE_AP_POWER_STATE_ON_FULL,
                0);
    }

    public void testDisplayOnOff() throws Exception {
        setupPowerPropertyAndStart(true);
        assertBootComplete();
        for (int i = 0; i < 2; i++) {
            mPowerStateHandler.sendPowerState(
                    VehicleApPowerState.VEHICLE_AP_POWER_STATE_ON_DISP_OFF,
                    0);
            mDisplayListener.waitForDisplayState(false, TIMEOUT_MS);
            mPowerStateHandler.sendPowerState(
                    VehicleApPowerState.VEHICLE_AP_POWER_STATE_ON_FULL,
                    0);
            mDisplayListener.waitForDisplayState(true, TIMEOUT_MS);
        }
    }

    /* TODO make deep sleep work to test this
    public void testSleepEntry() throws Exception {
        assertBootComplete();
        mPowerStateHandler.sendPowerState(
                VehicleApPowerState.VEHICLE_AP_POWER_STATE_SHUTDOWN_PREPARE,
                VehicleApPowerStateShutdownParam.VEHICLE_AP_POWER_SHUTDOWN_PARAM_CAN_SLEEP);
        assertResponse(VehicleApPowerSetState.VEHICLE_AP_POWER_SET_DEEP_SLEEP_ENTRY, 0);
        assertResponse(VehicleApPowerSetState.VEHICLE_AP_POWER_SET_DEEP_SLEEP_EXIT, 0);
        mPowerStateHandler.sendPowerState(
                VehicleApPowerState.VEHICLE_AP_POWER_STATE_ON_FULL,
                0);
    }*/

    private void assertResponse(int expectedResponseState, int expectedResponseParam)
            throws Exception {
        LinkedList<int[]> setEvents = mPowerStateHandler.waitForStateSetAndGetAll(TIMEOUT_MS,
                expectedResponseState);
        int[] last = setEvents.getLast();
        assertEquals(expectedResponseState, last[0]);
        assertEquals(expectedResponseParam, last[1]);
    }

    private void assertBootComplete() throws Exception {
        mPowerStateHandler.waitForSubscription(TIMEOUT_MS);
        LinkedList<int[]> setEvents = mPowerStateHandler.waitForStateSetAndGetAll(TIMEOUT_MS,
                VehicleApPowerSetState.VEHICLE_AP_POWER_SET_BOOT_COMPLETE);
        int[] first = setEvents.getFirst();
        assertEquals(VehicleApPowerSetState.VEHICLE_AP_POWER_SET_BOOT_COMPLETE, first[0]);
        assertEquals(0, first[1]);
    }

    private synchronized boolean isMainDisplayOn() {
        Display disp = mDisplayManager.getDisplay(Display.DEFAULT_DISPLAY);
        return disp.getState() == Display.STATE_ON;
    }

    private class PowerStatePropertyHandler implements VehicleHalPropertyHandler {

        private int mPowerState = VehicleApPowerState.VEHICLE_AP_POWER_STATE_ON_FULL;
        private int mPowerParam = 0;

        private final Semaphore mSubscriptionWaitSemaphore = new Semaphore(0);
        private final Semaphore mSetWaitSemaphore = new Semaphore(0);
        private LinkedList<int[]> mSetStates = new LinkedList<>();

        @Override
        public void onPropertySet(VehiclePropValue value) {
            synchronized (this) {
                mSetStates.add(new int[] {
                        value.getInt32Values(
                                VehicleApPowerStateIndex.VEHICLE_AP_POWER_STATE_INDEX_STATE),
                        value.getInt32Values(
                                VehicleApPowerStateIndex.VEHICLE_AP_POWER_STATE_INDEX_ADDITIONAL)
                                });
            }
            mSetWaitSemaphore.release();
        }

        @Override
        public synchronized VehiclePropValue onPropertyGet(VehiclePropValue value) {
            int[] values = { mPowerState, mPowerParam };
            return VehiclePropValueUtil.createIntVectorValue(
                    VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE, values,
                    SystemClock.elapsedRealtimeNanos());
        }

        @Override
        public void onPropertySubscribe(int property, float sampleRate, int zones) {
            mSubscriptionWaitSemaphore.release();
        }

        @Override
        public void onPropertyUnsubscribe(int property) {
            //ignore
        }

        private synchronized void setCurrentState(int state, int param) {
            mPowerState = state;
            mPowerParam = param;
        }

        private void waitForSubscription(long timeoutMs) throws Exception {
            if (!mSubscriptionWaitSemaphore.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
                fail("waitForSubscription timeout");
            }
        }

        private LinkedList<int[]> waitForStateSetAndGetAll(long timeoutMs, int expectedSet)
                throws Exception {
            while (true) {
                if (!mSetWaitSemaphore.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
                    fail("waitForStateSetAndGetAll timeout");
                }
                synchronized (this) {
                    boolean found = false;
                    for (int[] state : mSetStates) {
                        if (state[0] == expectedSet) {
                            found = true;
                        }
                    }
                    if (found) {
                        LinkedList<int[]> res = mSetStates;
                        mSetStates = new LinkedList<>();
                        return res;
                    }
                }
            }
        }

        private void sendPowerState(int state, int param) {
            int[] values = { state, param };
            getVehicleHalEmulator().injectEvent(
                    VehiclePropValueUtil.createIntVectorValue(
                            VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE, values,
                            SystemClock.elapsedRealtimeNanos()));
        }
    }

    private class DisplayStateListener implements DisplayManager.DisplayListener {

        private final Semaphore mDisplayWait = new Semaphore(0);

        private void waitForDisplayState(boolean expectedState, long timeoutMs) throws Exception {
            while (isMainDisplayOn() != expectedState) {
                if (!mDisplayWait.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
                    fail("waitForDisplayState timeout, expected:" + expectedState);
                }
            }
        }

        @Override
        public void onDisplayAdded(int displayId) {
            // not used
        }

        @Override
        public void onDisplayChanged(int displayId) {
            Log.i(TAG, "onDisplayChanged, id:" + displayId + " main display on:" +
                    isMainDisplayOn());
            if (displayId == Display.DEFAULT_DISPLAY) {
                mDisplayWait.release();
            }
        }

        @Override
        public void onDisplayRemoved(int displayId) {
            // not used
        }
    }
}
