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

import android.content.ComponentName;
import android.os.IBinder;
import android.os.Looper;
import android.support.car.Car;
import android.support.car.CarConnectionListener;
import android.support.car.ServiceConnectionListener;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.Log;

import java.util.LinkedList;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import com.android.car.test.MockedCarTestBase;

@MediumTest
public class CarConnectionListenerInMockingTest extends MockedCarTestBase {

    private static final long DEFAULT_WAIT_TIMEOUT_MS = 1000;
    private static final String TAG = CarConnectionListenerInMockingTest.class.getSimpleName();

    private final Semaphore mConnectionWait = new Semaphore(0);

    private Car mCar;

    private final ServiceConnectionListener mConnectionListener = new ServiceConnectionListener() {

        @Override
        public void onServiceSuspended(int cause) {
            assertMainThread();
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            assertMainThread();
        }

        @Override
        public void onServiceConnectionFailed(int cause) {
            assertMainThread();
        }

        @Override
        public void onServiceConnected(ComponentName name) {
            assertMainThread();
            mConnectionWait.release();
        }
    };

    private void assertMainThread() {
        assertTrue(Looper.getMainLooper().isCurrentThread());
    }
    private void waitForConnection(long timeoutMs) throws InterruptedException {
        mConnectionWait.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mCar = Car.createCar(getContext(), mConnectionListener);
        mCar.connect();
        waitForConnection(DEFAULT_WAIT_TIMEOUT_MS);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        mCar.disconnect();
    }

    public void testConnectDisconnectWithMocking() throws Exception {
        CarConnectionListerImpl listener1 = new CarConnectionListerImpl();
        mCar.registerCarConnectionListener(listener1);
        assertTrue(listener1.waitForConnection(DEFAULT_WAIT_TIMEOUT_MS));
        getVehicleHalEmulator().start();
        assertTrue(listener1.waitForDisconnect(DEFAULT_WAIT_TIMEOUT_MS));
        assertTrue(listener1.waitForConnection(DEFAULT_WAIT_TIMEOUT_MS));
        getVehicleHalEmulator().stop();
        assertTrue(listener1.waitForDisconnect(DEFAULT_WAIT_TIMEOUT_MS));
        assertTrue(listener1.waitForConnection(DEFAULT_WAIT_TIMEOUT_MS));
    }

    private class CarConnectionListerImpl implements CarConnectionListener {
        private static final int CONNECTION_INVALID = -1;
        private Semaphore mWaitSemaphore = new Semaphore(0);
        private final LinkedList<Integer> mEvents = new LinkedList<>();

        @Override
        public void onConnected(int connectionType) {
            Log.i(TAG, "onConnected " + connectionType);
            mEvents.add(connectionType);
            mWaitSemaphore.release();
        }

        @Override
        public void onDisconnected() {
            Log.i(TAG, "onDisconnected");
            mEvents.add(CONNECTION_INVALID);
            mWaitSemaphore.release();
        }

        public boolean waitForConnection(long timeoutMs) throws Exception {
            if (!mWaitSemaphore.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
                return false;
            }
            Integer event = mEvents.poll();
            assertNotNull(event);
            assertEquals(Car.CONNECTION_TYPE_EMBEDDED, event.intValue());
            return true;
        }

        public boolean waitForDisconnect(long timeoutMs) throws Exception {
            if (!mWaitSemaphore.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
                return false;
            }
            Integer event = mEvents.poll();
            assertNotNull(event);
            assertEquals(CONNECTION_INVALID, event.intValue());
            return true;
        }
    }
}
