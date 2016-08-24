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
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.Log;

import com.android.car.vehiclenetwork.VehicleNetwork;
import com.android.car.vehiclenetwork.VehicleNetwork.VehicleNetworkHalMock;
import com.android.car.vehiclenetwork.VehicleNetwork.VehicleNetworkListener;
import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropAccess;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropChangeMode;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleZone;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfigs;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValues;
import com.android.car.vehiclenetwork.VehiclePropConfigUtil;
import com.android.car.vehiclenetwork.VehiclePropValueUtil;

import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

@MediumTest
public class VehicleNetworkMockedTest extends AndroidTestCase {
    private static final String TAG = VehicleNetworkMockedTest.class.getSimpleName();

    private static final long TIMEOUT_MS = 1000;

    private static final int CUSTOM_PROPERTY_INT32 =
            VehicleNetworkConsts.VEHICLE_PROPERTY_CUSTOM_START;
    private static final int CUSTOM_PROPERTY_ZONED_INT32 =
            VehicleNetworkConsts.VEHICLE_PROPERTY_CUSTOM_START + 1;
    private static final int CUSTOM_PROPERTY_ZONED_INT32_VEC2 =
            VehicleNetworkConsts.VEHICLE_PROPERTY_CUSTOM_START + 2;
    private static final int CUSTOM_PROPERTY_ZONED_INT32_VEC3 =
            VehicleNetworkConsts.VEHICLE_PROPERTY_CUSTOM_START + 3;
    private static final int CUSTOM_PROPERTY_ZONED_FLOAT_VEC2 =
            VehicleNetworkConsts.VEHICLE_PROPERTY_CUSTOM_START + 4;
    private static final int CUSTOM_PROPERTY_ZONED_FLOAT_VEC3 =
            VehicleNetworkConsts.VEHICLE_PROPERTY_CUSTOM_START + 5;
    private static final int CUSTOM_PROPERTY_FLOAT_VEC2 =
            VehicleNetworkConsts.VEHICLE_PROPERTY_CUSTOM_START + 6;
    private static final int CUSTOM_PROPERTY_INT32_VEC2 =
            VehicleNetworkConsts.VEHICLE_PROPERTY_CUSTOM_START + 7;

    private final HandlerThread mHandlerThread = new HandlerThread(
            VehicleNetworkTest.class.getSimpleName());
    private VehicleNetwork mVehicleNetwork;
    private EventListener mListener = new EventListener();
    private final VehicleHalMock mVehicleHalMock = new VehicleHalMock();

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mHandlerThread.start();
        mVehicleNetwork = VehicleNetwork.createVehicleNetwork(mListener,
                mHandlerThread.getLooper());
        mVehicleHalMock.registerProperty(
                VehiclePropConfigUtil.createProperty(
                        CUSTOM_PROPERTY_INT32,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_INT32, 0x0),
                new DefaultVehiclePropertyHandler(VehiclePropValueUtil.createIntValue(
                        CUSTOM_PROPERTY_INT32, 0, 0)));
        mVehicleHalMock.registerProperty(
                VehiclePropConfigUtil.createProperty(
                        CUSTOM_PROPERTY_ZONED_INT32,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32, 0x0),
                new DefaultVehiclePropertyHandler(VehiclePropValueUtil.createZonedIntValue(
                        CUSTOM_PROPERTY_ZONED_INT32, VehicleZone.VEHICLE_ZONE_ROW_2_LEFT, 0, 0)));
        mVehicleHalMock.registerProperty(
                VehiclePropConfigUtil.createProperty(
                        CUSTOM_PROPERTY_ZONED_INT32_VEC2,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32_VEC2, 0x0),
                new DefaultVehiclePropertyHandler(VehiclePropValueUtil.createZonedIntVectorValue(
                        CUSTOM_PROPERTY_ZONED_INT32_VEC2,
                        VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                        new int[2], 0)));
        mVehicleHalMock.registerProperty(
                VehiclePropConfigUtil.createProperty(
                        CUSTOM_PROPERTY_INT32_VEC2,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC2, 0x0),
                new DefaultVehiclePropertyHandler(VehiclePropValueUtil.createIntVectorValue(
                        CUSTOM_PROPERTY_INT32_VEC2, new int[2], 0)));
        mVehicleHalMock.registerProperty(
                VehiclePropConfigUtil.createZonedProperty(
                        CUSTOM_PROPERTY_ZONED_INT32_VEC3,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32_VEC3,
                        VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                        0x0),
                new DefaultVehiclePropertyHandler(VehiclePropValueUtil.createZonedIntVectorValue(
                        CUSTOM_PROPERTY_ZONED_INT32_VEC3,
                        VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                        new int[3], 0)));
        mVehicleHalMock.registerProperty(
                VehiclePropConfigUtil.createZonedProperty(
                        CUSTOM_PROPERTY_ZONED_FLOAT_VEC2,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC2,
                        VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                        0x0),
                new DefaultVehiclePropertyHandler(VehiclePropValueUtil.createZonedFloatVectorValue(
                        CUSTOM_PROPERTY_ZONED_FLOAT_VEC2,
                        VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                        new float[2], 0)));
        mVehicleHalMock.registerProperty(
                VehiclePropConfigUtil.createProperty(
                        CUSTOM_PROPERTY_FLOAT_VEC2,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC2, 0x0),
                new DefaultVehiclePropertyHandler(VehiclePropValueUtil.createFloatVectorValue(
                        CUSTOM_PROPERTY_FLOAT_VEC2, new float[2], 0)));
        mVehicleHalMock.registerProperty(
                VehiclePropConfigUtil.createZonedProperty(
                        CUSTOM_PROPERTY_ZONED_FLOAT_VEC3,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC3,
                        VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                        0x0),
                new DefaultVehiclePropertyHandler(VehiclePropValueUtil.createZonedFloatVectorValue(
                        CUSTOM_PROPERTY_ZONED_FLOAT_VEC3,
                        VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                        new float[3], 0)));
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        mHandlerThread.quit();
        mVehicleNetwork.stopMocking();
    }

    public void testHalRestartListening() throws Exception {
        mVehicleNetwork.startHalRestartMonitoring();
        mVehicleNetwork.startMocking(mVehicleHalMock);
        assertTrue(mListener.waitForHalRestartAndAssert(TIMEOUT_MS, true /*expectedInMocking*/));
        mVehicleNetwork.stopMocking();
        assertTrue(mListener.waitForHalRestartAndAssert(TIMEOUT_MS, false /*expectedInMocking*/));
        mVehicleNetwork.stopHalRestartMonitoring();
    }

    public void testCustomZonedIntProperties() {
        final int INT_VALUE = 10;
        final int INT_VALUE2 = 20;

        mVehicleNetwork.startMocking(mVehicleHalMock);

        mVehicleNetwork.setIntProperty(CUSTOM_PROPERTY_INT32, INT_VALUE);
        assertEquals(INT_VALUE, mVehicleNetwork.getIntProperty(CUSTOM_PROPERTY_INT32));

        mVehicleNetwork.setZonedIntProperty(CUSTOM_PROPERTY_ZONED_INT32,
                VehicleZone.VEHICLE_ZONE_ROW_2_LEFT, INT_VALUE);
        mVehicleNetwork.setZonedIntProperty(CUSTOM_PROPERTY_ZONED_INT32,
                VehicleZone.VEHICLE_ZONE_ROW_2_RIGHT, INT_VALUE2);

        assertEquals(INT_VALUE,
                mVehicleNetwork.getZonedIntProperty(CUSTOM_PROPERTY_ZONED_INT32,
                        VehicleZone.VEHICLE_ZONE_ROW_2_LEFT));
        assertEquals(INT_VALUE2,
                mVehicleNetwork.getZonedIntProperty(CUSTOM_PROPERTY_ZONED_INT32,
                        VehicleZone.VEHICLE_ZONE_ROW_2_RIGHT));
        assertEquals(INT_VALUE,
                mVehicleNetwork.getZonedIntProperty(CUSTOM_PROPERTY_ZONED_INT32,
                        VehicleZone.VEHICLE_ZONE_ROW_2_LEFT));
    }

    public void testCustomZonedIntVecProperties() {
        final int[] ZONED_INT_VALUE_LEFT = new int[] {30, 40};
        final int[] ZONED_INT_VALUE_RIGHT = new int[] {50, 60};
        final int[] ZONED_INT_VALUE_VEC3 = new int[] {30, 40, 50};

        mVehicleNetwork.startMocking(mVehicleHalMock);

        int[] actualValue = mVehicleNetwork.getZonedIntVectorProperty(
                CUSTOM_PROPERTY_ZONED_INT32_VEC2,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT);
        // Verify the default values before calling setProperty.
        assertArrayEquals(new int[2], actualValue);
        mVehicleNetwork.setZonedIntVectorProperty(CUSTOM_PROPERTY_ZONED_INT32_VEC2,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                ZONED_INT_VALUE_LEFT);
        actualValue = mVehicleNetwork.getZonedIntVectorProperty(
                CUSTOM_PROPERTY_ZONED_INT32_VEC2,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT);
        assertArrayEquals(ZONED_INT_VALUE_LEFT, actualValue);

        // Verify different zone for the same property
        mVehicleNetwork.setZonedIntVectorProperty(CUSTOM_PROPERTY_ZONED_INT32_VEC2,
                VehicleZone.VEHICLE_ZONE_ROW_1_RIGHT,
                ZONED_INT_VALUE_RIGHT);
        actualValue = mVehicleNetwork.getZonedIntVectorProperty(
                CUSTOM_PROPERTY_ZONED_INT32_VEC2,
                VehicleZone.VEHICLE_ZONE_ROW_1_RIGHT);
        assertArrayEquals(ZONED_INT_VALUE_RIGHT, actualValue);

        mVehicleNetwork.setZonedIntVectorProperty(CUSTOM_PROPERTY_ZONED_INT32_VEC3,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                ZONED_INT_VALUE_VEC3);
        actualValue = mVehicleNetwork.getZonedIntVectorProperty(
                CUSTOM_PROPERTY_ZONED_INT32_VEC3,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT);
        assertArrayEquals(ZONED_INT_VALUE_VEC3, actualValue);
    }

    public void testCustomZonedFloatVecProperties() {
        final float[] ZONED_FLOAT_VALUE_LEFT = new float[] {30.1f, 40.3f};
        final float[] ZONED_FLOAT_VALUE_RIGHT = new float[] {50.5f, 60};
        final float[] ZONED_FLOAT_VALUE_VEC3 = new float[] {30, 40.3f, 50};

        mVehicleNetwork.startMocking(mVehicleHalMock);

        float[] actualValue = mVehicleNetwork.getZonedFloatVectorProperty(
                CUSTOM_PROPERTY_ZONED_FLOAT_VEC2,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT);
        // Verify the default values before calling setProperty.
        assertArrayEquals(new float[2], actualValue);
        mVehicleNetwork.setZonedFloatVectorProperty(CUSTOM_PROPERTY_ZONED_FLOAT_VEC2,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                ZONED_FLOAT_VALUE_LEFT);
        actualValue = mVehicleNetwork.getZonedFloatVectorProperty(
                CUSTOM_PROPERTY_ZONED_FLOAT_VEC2,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT);
        assertArrayEquals(ZONED_FLOAT_VALUE_LEFT, actualValue);

        // Verify different zone for the same property
        mVehicleNetwork.setZonedFloatVectorProperty(CUSTOM_PROPERTY_ZONED_FLOAT_VEC2,
                VehicleZone.VEHICLE_ZONE_ROW_1_RIGHT,
                ZONED_FLOAT_VALUE_RIGHT);
        actualValue = mVehicleNetwork.getZonedFloatVectorProperty(
                CUSTOM_PROPERTY_ZONED_FLOAT_VEC2,
                VehicleZone.VEHICLE_ZONE_ROW_1_RIGHT);
        assertArrayEquals(ZONED_FLOAT_VALUE_RIGHT, actualValue);

        mVehicleNetwork.setZonedFloatVectorProperty(CUSTOM_PROPERTY_ZONED_FLOAT_VEC3,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                ZONED_FLOAT_VALUE_VEC3);
        actualValue = mVehicleNetwork.getZonedFloatVectorProperty(
                CUSTOM_PROPERTY_ZONED_FLOAT_VEC3,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT);
        assertArrayEquals(ZONED_FLOAT_VALUE_VEC3, actualValue);
    }

    public void testCustomFloatVecProperties() {
        final float[] FLOAT_VALUE = new float[] {30.1f, 40.3f};

        mVehicleNetwork.startMocking(mVehicleHalMock);

        float[] actualValue = mVehicleNetwork.getFloatVectorProperty(
                CUSTOM_PROPERTY_FLOAT_VEC2);
        // Verify the default values before calling setProperty.
        assertArrayEquals(new float[2], actualValue);
        mVehicleNetwork.setFloatVectorProperty(CUSTOM_PROPERTY_FLOAT_VEC2, FLOAT_VALUE);
        actualValue = mVehicleNetwork.getFloatVectorProperty(
                CUSTOM_PROPERTY_FLOAT_VEC2);
        assertArrayEquals(FLOAT_VALUE, actualValue);
    }

    public void testCustomIntVecProperties() {
        final int[] INT32_VALUE = new int[] {30, 40};

        mVehicleNetwork.startMocking(mVehicleHalMock);

        int[] actualValue = mVehicleNetwork.getIntVectorProperty(
                CUSTOM_PROPERTY_INT32_VEC2);
        // Verify the default values before calling setProperty.
        assertArrayEquals(new int[2], actualValue);
        mVehicleNetwork.setIntVectorProperty(CUSTOM_PROPERTY_INT32_VEC2, INT32_VALUE);
        actualValue = mVehicleNetwork.getIntVectorProperty(
                CUSTOM_PROPERTY_INT32_VEC2);
        assertArrayEquals(INT32_VALUE, actualValue);
    }

    public void testGlobalErrorListening() throws Exception {
        mVehicleNetwork.startErrorListening();
        mVehicleNetwork.startMocking(mVehicleHalMock);
        final int ERROR_CODE = 0x1;
        final int ERROR_OPERATION = 0x10;
        mVehicleNetwork.injectHalError(ERROR_CODE, 0, ERROR_OPERATION);
        assertTrue(mListener.waitForHalErrorAndAssert(TIMEOUT_MS, ERROR_CODE, 0, ERROR_OPERATION));
        mVehicleNetwork.injectHalError(ERROR_CODE, CUSTOM_PROPERTY_INT32, ERROR_OPERATION);
        assertTrue(mListener.waitForHalErrorAndAssert(TIMEOUT_MS,
                ERROR_CODE, CUSTOM_PROPERTY_INT32, ERROR_OPERATION));
        mVehicleNetwork.stopMocking();
        mVehicleNetwork.stopErrorListening();
    }

    public void testPropertyErrorListening() throws Exception {
        mVehicleNetwork.startMocking(mVehicleHalMock);
        mVehicleNetwork.subscribe(CUSTOM_PROPERTY_INT32, 0);
        final int ERROR_CODE = 0x1;
        final int ERROR_OPERATION = 0x10;
        mVehicleNetwork.injectHalError(ERROR_CODE, CUSTOM_PROPERTY_INT32, ERROR_OPERATION);
        assertTrue(mListener.waitForHalErrorAndAssert(TIMEOUT_MS,
                ERROR_CODE, CUSTOM_PROPERTY_INT32, ERROR_OPERATION));
        mVehicleNetwork.unsubscribe(CUSTOM_PROPERTY_INT32);
        mVehicleNetwork.stopMocking();
    }

    public void testSubscribe() throws Exception {
        final int PROPERTY = CUSTOM_PROPERTY_ZONED_INT32_VEC3;
        final int ZONE = VehicleZone.VEHICLE_ZONE_ROW_1_LEFT;
        final int[] VALUES = new int[] {10, 20, 30};
        mVehicleNetwork.startMocking(mVehicleHalMock);
        mVehicleNetwork.subscribe(PROPERTY, 0, ZONE);
        VehiclePropValue value = VehiclePropValueUtil.createZonedIntVectorValue(
                PROPERTY, ZONE, VALUES, 0);
        mVehicleNetwork.injectEvent(value);
        assertTrue(mListener.waitForEvent(TIMEOUT_MS, value));
        mVehicleNetwork.unsubscribe(PROPERTY);
    }

    public void testGetPropertyFailsForCustom() {
        try {
            mVehicleNetwork.getProperty(CUSTOM_PROPERTY_INT32);
            fail();
        } catch (IllegalArgumentException expected) {
            // Expected.
        }
    }

    private void assertArrayEquals(int[] expected, int[] actual) {
        assertEquals(Arrays.toString(expected), Arrays.toString(actual));
    }

    private void assertArrayEquals(float[] expected, float[] actual) {
        assertEquals(Arrays.toString(expected), Arrays.toString(actual));
    }

    private class EventListener implements VehicleNetworkListener {
        boolean mInMocking;
        int mErrorCode;
        int mErrorProperty;
        int mErrorOperation;
        VehiclePropValues mValuesReceived;

        private final Semaphore mRestartWait = new Semaphore(0);
        private final Semaphore mErrorWait = new Semaphore(0);
        private final Semaphore mEventWait = new Semaphore(0);

        @Override
        public void onVehicleNetworkEvents(VehiclePropValues values) {
            Log.i(TAG, "onVehicleNetworkEvents");
            mValuesReceived = values;
            mEventWait.release();
        }

        @Override
        public void onHalError(int errorCode, int property, int operation) {
            mErrorCode = errorCode;
            mErrorProperty = property;
            mErrorOperation = operation;
            mErrorWait.release();
        }

        public boolean waitForHalErrorAndAssert(long timeoutMs, int expectedErrorCode,
                int expectedErrorProperty, int expectedErrorOperation) throws Exception {
            if (!mErrorWait.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
                return false;
            }
            assertEquals(expectedErrorCode, mErrorCode);
            assertEquals(expectedErrorProperty, mErrorProperty);
            assertEquals(expectedErrorOperation, mErrorOperation);
            return true;
        }

        @Override
        public void onHalRestart(boolean inMocking) {
            mInMocking = inMocking;
            mRestartWait.release();
        }

        public boolean waitForHalRestartAndAssert(long timeoutMs, boolean expectedInMocking)
                throws Exception {
            if (!mRestartWait.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
                return false;
            }
            assertEquals(expectedInMocking, mInMocking);
            return true;
        }

        public boolean waitForEvent(long timeoutMs, VehiclePropValue expected)
                throws InterruptedException {
            if (!mEventWait.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
                Log.w(TAG, "Timed out waiting for event.");
                return false;
            }
            assertEquals(1, mValuesReceived.getValuesCount());
            assertEquals(VehiclePropValueUtil.toString(expected),
                    VehiclePropValueUtil.toString(mValuesReceived.getValues(0)));
            return true;
        }
    }

    private interface VehiclePropertyHandler {
        void onPropertySet(VehiclePropValue value);
        VehiclePropValue onPropertyGet(VehiclePropValue property);
        void onPropertySubscribe(int property, float sampleRate, int zones);
        void onPropertyUnsubscribe(int property);
    }

    private class VehicleHalMock implements VehicleNetworkHalMock {
        private LinkedList<VehiclePropConfig> mConfigs = new LinkedList<>();
        private HashMap<Integer, VehiclePropertyHandler> mHandlers = new HashMap<>();

        public synchronized void registerProperty(VehiclePropConfig config,
                VehiclePropertyHandler handler) {
            int property = config.getProp();
            mConfigs.add(config);
            mHandlers.put(property, handler);
        }

        @Override
        public synchronized VehiclePropConfigs onListProperties() {
            Log.i(TAG, "onListProperties, num properties:" + mConfigs.size());
            VehiclePropConfigs configs =
                    VehiclePropConfigs.newBuilder().addAllConfigs(mConfigs).build();
            return configs;
        }

        @Override
        public void onPropertySet(VehiclePropValue value) {
            int property = value.getProp();
            VehiclePropertyHandler handler = getPropertyHandler(property);
            if (handler == null) {
                fail("onPropertySet for unknown property " + Integer.toHexString(property));
            }
            handler.onPropertySet(value);
        }

        @Override
        public VehiclePropValue onPropertyGet(VehiclePropValue value) {
            int property = value.getProp();
            VehiclePropertyHandler handler = getPropertyHandler(property);
            if (handler == null) {
                fail("onPropertyGet for unknown property " + Integer.toHexString(property));
            }
            VehiclePropValue propValue = handler.onPropertyGet(value);
            return propValue;
        }

        @Override
        public void onPropertySubscribe(int property, float sampleRate, int zones) {
            VehiclePropertyHandler handler = getPropertyHandler(property);
            if (handler == null) {
                fail("onPropertySubscribe for unknown property " + Integer.toHexString(property));
            }
            handler.onPropertySubscribe(property, sampleRate, zones);
        }

        @Override
        public void onPropertyUnsubscribe(int property) {
            VehiclePropertyHandler handler = getPropertyHandler(property);
            if (handler == null) {
                fail("onPropertyUnsubscribe for unknown property " + Integer.toHexString(property));
            }
            handler.onPropertyUnsubscribe(property);
        }

        public synchronized VehiclePropertyHandler getPropertyHandler(int property) {
            return mHandlers.get(property);
        }
    }

    private class DefaultVehiclePropertyHandler implements VehiclePropertyHandler {
        private Map<Integer, VehiclePropValue> mZoneValueMap = new HashMap<>();

        DefaultVehiclePropertyHandler(VehiclePropValue initialValue) {
            setValue(initialValue);
        }

        @Override
        public void onPropertySet(VehiclePropValue value) {
            setValue(value);
        }

        @Override
        public synchronized VehiclePropValue onPropertyGet(VehiclePropValue property) {
            int zone = property.getZone();
            VehiclePropValue value = mZoneValueMap.get(zone);
            if (value == null) {
                Log.w(TAG, "Property not found: " + property.getProp() + ", zone: " + zone);
            }
            return value;
        }

        @Override
        public void onPropertySubscribe(int property, float sampleRate, int zones) {
            Log.i(TAG, "Property subscribed:0x" + Integer.toHexString(property) +
                    " zones:0x" + Integer.toHexString(zones));
        }

        @Override
        public void onPropertyUnsubscribe(int property) {
            // TODO Auto-generated method stub
        }

        private void setValue(VehiclePropValue value) {
            mZoneValueMap.put(value.getZone(), VehiclePropValue.newBuilder(value).build());
        }
    }
}
