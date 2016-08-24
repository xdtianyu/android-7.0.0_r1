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

import android.car.test.VehicleHalEmulator;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.support.car.Car;
import android.support.car.ServiceConnectionListener;
import android.test.AndroidTestCase;
import android.util.Log;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * Base class for testing with mocked vehicle HAL (=car).
 * It is up to each app to start emulation by getVehicleHalEmulator().start() as there will be
 * per test set up that should be done before starting.
 */
public class MockedCarTestBase extends AndroidTestCase {
    private static final String TAG = MockedCarTestBase.class.getSimpleName();
    private static final long DEFAULT_WAIT_TIMEOUT_MS = 3000;

    private Car mSupportCar;
    private android.car.Car mCar;
    private VehicleHalEmulator mVehicleHalEmulator;

    private final Semaphore mConnectionWaitForSupportCar = new Semaphore(0);
    private final Semaphore mConnectionWaitForCar = new Semaphore(0);
    private final Semaphore mWaitForMain = new Semaphore(0);
    private final Handler mMainHalder = new Handler(Looper.getMainLooper());

    private final ServiceConnectionListener mConnectionListener = new ServiceConnectionListener() {

        @Override
        public void onServiceSuspended(int cause) {
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
        }

        @Override
        public void onServiceConnectionFailed(int cause) {
        }

        @Override
        public void onServiceConnected(ComponentName name) {
            Log.i(TAG, "onServiceConnected, component" + name);
            mConnectionWaitForSupportCar.release();
        }
    };

    @Override
    protected synchronized void setUp() throws Exception {
        super.setUp();
        mSupportCar = Car.createCar(getContext(), mConnectionListener);
        mSupportCar.connect();
        mCar = android.car.Car.createCar(getContext(), new ServiceConnection() {

            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                mConnectionWaitForCar.release();
            }

            @Override
            public void onServiceDisconnected(ComponentName name) {
            }
        });
        mCar.connect();
        assertTrue(waitForConnection(DEFAULT_WAIT_TIMEOUT_MS));
        assertTrue(mConnectionWaitForCar.tryAcquire(DEFAULT_WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS)
                );
        mVehicleHalEmulator = new VehicleHalEmulator(mCar);
    }

    @Override
    protected synchronized void tearDown() throws Exception {
        super.tearDown();
        if (mVehicleHalEmulator.isStarted()) {
            mVehicleHalEmulator.stop();
        }
        mSupportCar.disconnect();
        mCar.disconnect();
    }

    protected synchronized Car getSupportCar() {
        return mSupportCar;
    }

    protected synchronized android.car.Car getCar() {
        return mCar;
    }

    protected void runOnMain(final Runnable r) {
        mMainHalder.post(r);
    }

    protected void runOnMainSync(final Runnable r) throws Exception {
        mMainHalder.post(new Runnable() {
            @Override
            public void run() {
                r.run();
                mWaitForMain.release();
            }
        });
        mWaitForMain.acquire();
    }

    protected synchronized VehicleHalEmulator getVehicleHalEmulator() {
        return mVehicleHalEmulator;
    }

    private boolean waitForConnection(long timeoutMs) throws InterruptedException {
        return mConnectionWaitForSupportCar.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS);
    }
}
