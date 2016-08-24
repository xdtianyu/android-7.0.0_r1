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
package com.android.support.car.test;

import android.car.test.VehicleHalEmulator.VehicleHalPropertyHandler;
import android.content.Context;
import android.media.AudioManager;
import android.support.car.Car;
import android.support.car.CarAppContextManager;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.Log;

import com.android.car.test.MockedCarTestBase;
import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleAudioContextFlag;
import com.android.car.vehiclenetwork.VehiclePropConfigUtil;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePermissionModel;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropAccess;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropChangeMode;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

@MediumTest
public class AppContextTest extends MockedCarTestBase {
    private static final String TAG = AppContextTest.class.getSimpleName();
    private static final long DEFAULT_WAIT_TIMEOUT_MS = 1000;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        getVehicleHalEmulator().start();
    }

    public void testContextChange() throws Exception {
        CarAppContextManager manager = (CarAppContextManager) getSupportCar().getCarManager(
                Car.APP_CONTEXT_SERVICE);
        ContextChangeListener listener = new ContextChangeListener();
        ContextOwnershipChangeListerner ownershipListener = new ContextOwnershipChangeListerner();
        manager.registerContextListener(listener, CarAppContextManager.APP_CONTEXT_NAVIGATION |
                CarAppContextManager.APP_CONTEXT_VOICE_COMMAND);
        manager.setActiveContexts(ownershipListener, CarAppContextManager.APP_CONTEXT_NAVIGATION);
        manager.setActiveContexts(ownershipListener, CarAppContextManager.APP_CONTEXT_VOICE_COMMAND);
        manager.resetActiveContexts(CarAppContextManager.APP_CONTEXT_NAVIGATION);
        manager.resetActiveContexts(CarAppContextManager.APP_CONTEXT_VOICE_COMMAND);
        manager.unregisterContextListener();
    }

    private class ContextChangeListener implements CarAppContextManager.AppContextChangeListener {
        private int mLastChangeEvent;
        private final Semaphore mChangeWait = new Semaphore(0);

        public boolean waitForContextChangeAndAssert(long timeoutMs, int expectedContexts)
                throws Exception {
            if (!mChangeWait.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
                return false;
            }
            assertEquals(expectedContexts, mLastChangeEvent);
            return true;
        }

        @Override
        public void onAppContextChange(int activeContexts) {
            Log.i(TAG, "onAppContextChange " + Integer.toHexString(activeContexts));
            mLastChangeEvent = activeContexts;
            mChangeWait.release();
        }
    }

    private class ContextOwnershipChangeListerner
            implements CarAppContextManager.AppContextOwnershipChangeListener {
        private int mLastLossEvent;
        private final Semaphore mLossEventWait = new Semaphore(0);

        public boolean waitForOwnershipLossAndAssert(long timeoutMs, int expectedContexts)
                throws Exception {
            if (!mLossEventWait.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
                return false;
            }
            assertEquals(expectedContexts, mLastLossEvent);
            return true;
        }

        @Override
        public void onAppContextOwnershipLoss(int context) {
            Log.i(TAG, "onAppContextOwnershipLoss " + Integer.toHexString(context));
            mLastLossEvent = context;
            mLossEventWait.release();
        }
    }
}
