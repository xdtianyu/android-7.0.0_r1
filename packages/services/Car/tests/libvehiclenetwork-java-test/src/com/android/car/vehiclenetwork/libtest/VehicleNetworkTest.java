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
package com.android.car.vehiclenetwork.libtest;

import android.os.HandlerThread;
import android.os.SystemClock;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.ArraySet;
import android.util.Log;

import com.android.car.vehiclenetwork.VehicleNetwork;
import com.android.car.vehiclenetwork.VehicleNetwork.VehicleNetworkListener;
import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropAccess;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropChangeMode;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfigs;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValues;
import com.android.car.vehiclenetwork.VehicleNetworkProtoUtil;

import java.util.LinkedList;

@MediumTest
public class VehicleNetworkTest extends AndroidTestCase {
    private static final String TAG = VehicleNetworkTest.class.getSimpleName();
    private static final long TIMEOUT_MS = 2000;

    private final HandlerThread mHandlerThread = new HandlerThread(
            VehicleNetworkTest.class.getSimpleName());
    private VehicleNetwork mVehicleNetwork;
    private final TestListener mListener = new TestListener();

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mHandlerThread.start();
        mVehicleNetwork = VehicleNetwork.createVehicleNetwork(mListener,
                mHandlerThread.getLooper());
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        mHandlerThread.quit();
    }

    public void testListProperties() {
        VehiclePropConfigs configs = mVehicleNetwork.listProperties();
        assertNotNull(configs);
        assertTrue(configs.getConfigsCount() > 0);
        Log.i(TAG, "got configs:" + configs.getConfigsCount());
        for (VehiclePropConfig config : configs.getConfigsList()) {
            Log.i(TAG, VehicleNetworkProtoUtil.VehiclePropConfigToString(config));
        }

        int oneProperty = configs.getConfigs(0).getProp();
        VehiclePropConfigs configs2 = mVehicleNetwork.listProperties(oneProperty);
        assertEquals(1, configs2.getConfigsCount());
        assertTrue(VehicleNetworkProtoUtil.VehiclePropConfigEquals(
                configs.getConfigs(0), configs2.getConfigs(0)));

        VehiclePropConfigs configs3 = mVehicleNetwork.listProperties(-1);
        assertNull(configs3);
    }

    public void testGetProperty() {
        try {
            VehiclePropValue value = mVehicleNetwork.getProperty(-1);
            fail();
        } catch (IllegalArgumentException e) {
            // expected
        }
        VehiclePropConfigs configs = mVehicleNetwork.listProperties();
        assertNotNull(configs);
        assertTrue(configs.getConfigsCount() > 0);
        Log.i(TAG, "got configs:" + configs.getConfigsCount());
        for (VehiclePropConfig config : configs.getConfigsList()) {
            if ((config.getAccess() & VehiclePropAccess.VEHICLE_PROP_ACCESS_READ) != 0) {
                if (config.getProp() == VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET) {
                    continue;
                }
                if (config.getProp() >= VehicleNetworkConsts.VEHICLE_PROPERTY_INTERNAL_START &&
                        config.getProp() <= VehicleNetworkConsts.VEHICLE_PROPERTY_INTERNAL_END) {
                    // internal property requires write to read
                    VehiclePropValue v = VehicleNetworkTestUtil.createDummyValue(config.getProp(),
                            config.getValueType());
                    mVehicleNetwork.setProperty(v);
                }
                VehiclePropValue value = mVehicleNetwork.getProperty(config.getProp());
                assertEquals(config.getProp(), value.getProp());
                assertEquals(config.getValueType(), value.getValueType());
                Log.i(TAG, " got property:" +
                        VehicleNetworkProtoUtil.VehiclePropValueToString(value));
            }
        }
    }

    public void testSetProperty() {
        try {
            VehiclePropValue value = VehiclePropValue.newBuilder().
                setProp(-1).
                setValueType(VehicleValueType.VEHICLE_VALUE_TYPE_INT32).
                addInt32Values(0).
                build();
            mVehicleNetwork.setProperty(value);
            fail();
        } catch (SecurityException e) {
            // expected
        }
        VehiclePropConfigs configs = mVehicleNetwork.listProperties();
        assertNotNull(configs);
        assertTrue(configs.getConfigsCount() > 0);
        Log.i(TAG, "got configs:" + configs.getConfigsCount());
        for (VehiclePropConfig config : configs.getConfigsList()) {
            if ((config.getAccess() & VehiclePropAccess.VEHICLE_PROP_ACCESS_WRITE) != 0) {
                if (config.getValueType() == VehicleValueType.VEHICLE_VALUE_TYPE_INT32) {
                    VehiclePropValue value = VehiclePropValue.newBuilder().
                            setProp(config.getProp()).
                            setValueType(config.getValueType()).
                            addInt32Values(0).
                            build();
                    mVehicleNetwork.setProperty(value);
                }
            } else {
                try {
                    VehiclePropValue value = VehiclePropValue.newBuilder().
                            setProp(config.getProp()).
                            setValueType(config.getValueType()).
                            build();
                    mVehicleNetwork.setProperty(value);
                    fail();
                } catch (IllegalArgumentException e) {
                    // expected
                }
            }
        }
    }

    public void testSubscribe() throws Exception {
        try {
            mVehicleNetwork.subscribe(-1, 0);
            fail();
        } catch (SecurityException e) {
            //expected
        }
        VehiclePropConfigs configs = mVehicleNetwork.listProperties();
        assertNotNull(configs);
        assertTrue(configs.getConfigsCount() > 0);
        Log.i(TAG, "got configs:" + configs.getConfigsCount());
        LinkedList<Integer> subscribedProperties = new LinkedList<Integer>();
        for (VehiclePropConfig config : configs.getConfigsList()) {
            if (config.getChangeMode() == VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_STATIC) {
                // cannot subscribe
                try {
                    mVehicleNetwork.subscribe(config.getProp(), config.getSampleRateMin());
                    fail();
                } catch (IllegalArgumentException e) {
                    //expected
                }
            } else if ((config.getAccess() & VehiclePropAccess.VEHICLE_PROP_ACCESS_READ) != 0){
                mVehicleNetwork.subscribe(config.getProp(), config.getSampleRateMin());
                subscribedProperties.add(config.getProp());
                if (config.getProp() >= VehicleNetworkConsts.VEHICLE_PROPERTY_INTERNAL_START &&
                        config.getProp() <= VehicleNetworkConsts.VEHICLE_PROPERTY_INTERNAL_END) {
                    // internal property requires write to get notification
                    VehiclePropValue v = VehicleNetworkTestUtil.createDummyValue(config.getProp(),
                            config.getValueType());
                    mVehicleNetwork.setProperty(v);
                }
            }
        }
        // now confirm event
        for (Integer prop : subscribedProperties) {
            mListener.waitForEvent(prop, TIMEOUT_MS);
            mVehicleNetwork.unsubscribe(prop);
        }
        // wait for already patched events to go away.
        Thread.sleep(1000);
        mListener.resetEventRecord();
        Thread.sleep(2000);
        assertEquals(0, mListener.getActiveEventsCount());
    }

    private class TestListener implements VehicleNetworkListener {
        private final ArraySet<Integer> mEventRecord = new ArraySet<Integer>();

        @Override
        public void onVehicleNetworkEvents(VehiclePropValues values) {
            for (VehiclePropValue value : values.getValuesList()) {
                Log.i(TAG, "event " + VehicleNetworkProtoUtil.VehiclePropValueToString(value));
                synchronized (this) {
                    mEventRecord.add(value.getProp());
                    notifyAll();
                }
            }
        }

        @Override
        public void onHalError(int errorCode, int property, int operation) {
            // TODO Auto-generated method stub
        }

        @Override
        public void onHalRestart(boolean inMocking) {
            // TODO Auto-generated method stub
        }

        private synchronized boolean waitForEvent(Integer prop, long timeoutMs)
                throws InterruptedException {
            long now = SystemClock.elapsedRealtime();
            long end = now + timeoutMs;
            long timeToWait = end - now;
            while (timeToWait > 0 && !mEventRecord.contains(prop)) {
                wait(timeToWait);
                timeToWait = end - SystemClock.elapsedRealtime();
            }
            return mEventRecord.contains(prop);
        }

        private synchronized void resetEventRecord() {
            mEventRecord.clear();
        }

        private synchronized int getActiveEventsCount() {
            return mEventRecord.size();
        }
    }
}
