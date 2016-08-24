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

import android.car.Car;
import android.car.test.VehicleHalEmulator;
import android.car.hardware.radio.CarRadioEvent;
import android.car.hardware.radio.CarRadioManager;
import android.car.hardware.radio.CarRadioManager.CarRadioEventListener;
import android.car.hardware.radio.CarRadioPreset;
import android.hardware.radio.RadioManager;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.Log;

import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropAccess;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropChangeMode;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;
import com.android.car.vehiclenetwork.VehiclePropConfigUtil;
import com.android.car.vehiclenetwork.VehiclePropValueUtil;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.HashMap;

@MediumTest
public class CarRadioManagerTest extends MockedCarTestBase {

    private static final String TAG = CarRadioManagerTest.class.getSimpleName();

    // Use this semaphore to block until the callback is heard of.
    private Semaphore mAvailable;

    private static final int NUM_PRESETS = 2;
    private final HashMap<Integer, CarRadioPreset> mRadioPresets =
        new HashMap<Integer, CarRadioPreset>();

    private CarRadioManager mCarRadioManager;

    private class RadioPresetPropertyHandler
            implements VehicleHalEmulator.VehicleHalPropertyHandler {
        public RadioPresetPropertyHandler() { }

        @Override
        public synchronized void onPropertySet(VehiclePropValue value) {
            assertEquals(value.getProp(), VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET);
            assertEquals(value.getValueType(), VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC4);

            Integer[] valueList = new Integer[4];
            value.getInt32ValuesList().toArray(valueList);
            assertFalse(
                "Index out of range: " + valueList[0] + " (0, " + NUM_PRESETS + ")",
                valueList[0] < 1);
            assertFalse(
                "Index out of range: " + valueList[0] + " (0, " + NUM_PRESETS + ")",
                valueList[0] > NUM_PRESETS);

            CarRadioPreset preset =
                new CarRadioPreset(valueList[0], valueList[1], valueList[2], valueList[3]);
            mRadioPresets.put(valueList[0], preset);

            // The test case must be waiting for the semaphore, if not we should throw exception.
            if (mAvailable.availablePermits() != 0) {
                Log.d(TAG, "Lock was free, should have been locked.");
            }
            mAvailable.release();
        }

        @Override
        public synchronized VehiclePropValue onPropertyGet(VehiclePropValue value) {
            assertEquals(value.getProp(), VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET);
            assertEquals(value.getValueType(), VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC4);

            Integer[] valueList = new Integer[4];
            value.getInt32ValuesList().toArray(valueList);

            // Get the actual preset.
            if (valueList[0] < 1 || valueList[0] > NUM_PRESETS) return null;
            CarRadioPreset preset = mRadioPresets.get(valueList[0]);
            VehiclePropValue v =
                VehiclePropValueUtil.createIntVectorValue(
                    VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET,
                    new int[] {
                        preset.getPresetNumber(),
                        preset.getBand(),
                        preset.getChannel(),
                        preset.getSubChannel()}, 0);
            return v;
        }

        @Override
        public synchronized void onPropertySubscribe(int property, float sampleRate, int zones) {
            Log.d(TAG, "onPropertySubscribe property: " + property + " rate: " + sampleRate);
            if (mAvailable.availablePermits() != 0) {
                Log.d(TAG, "Lock was free, should have been locked.");
                return;
            }
            mAvailable.release();
        }

        @Override
        public synchronized void onPropertyUnsubscribe(int property) {
        }
    }

    private class EventListener implements CarRadioEventListener {
        public EventListener() { }

        @Override
        public void onEvent(CarRadioEvent event) {
            // Print the event and release the lock.
            Log.d(TAG, event.toString());
            if (mAvailable.availablePermits() != 0) {
                Log.e(TAG, "Lock should be taken.");
                // Let the timeout fail the test here.
                return;
            }
            mAvailable.release();
        }
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mAvailable = new Semaphore(0);
        getVehicleHalEmulator().addProperty(
                VehiclePropConfigUtil.createProperty(
                        VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC4,
                        NUM_PRESETS),
                new RadioPresetPropertyHandler());
        getVehicleHalEmulator().start();
        mCarRadioManager =
                (CarRadioManager) getCar().getCarManager(Car.RADIO_SERVICE);
    }

    public void testPresetCount() throws Exception {
        int presetCount = mCarRadioManager.getPresetCount();
        assertEquals("Preset count not same.", NUM_PRESETS, presetCount);
    }

    public void testSetAndGetPreset() throws Exception {
        // Create a preset.
        CarRadioPreset preset = new CarRadioPreset(1, RadioManager.BAND_FM, 1234, -1);
        assertEquals("Lock should be freed by now.", 0, mAvailable.availablePermits());
        // mAvailable.acquire(1);
        mCarRadioManager.setPreset(preset);

        // Wait for acquire to be available again, fail if timeout.
        boolean success = mAvailable.tryAcquire(5L, TimeUnit.SECONDS);
        assertEquals("Could not finish setting, timeout!", true, success);

        // Test that get preset gives you the same element.
        assertEquals(preset, mCarRadioManager.getPreset(1));
    }

    public void testSubscribe() throws Exception {
        EventListener l = new EventListener();
        assertEquals("Lock should be freed by now.", 0, mAvailable.availablePermits());
        mCarRadioManager.registerListener(l);

        // Wait for acquire to be available again, fail if timeout.
        boolean success = mAvailable.tryAcquire(5L, TimeUnit.SECONDS);
        assertEquals("registerListener timeout", true, success);

        // Inject an event and wait for its callback in onPropertySet.
        CarRadioPreset preset = new CarRadioPreset(2, RadioManager.BAND_AM, 4321, -1);
        VehiclePropValue v = VehiclePropValueUtil.createIntVectorValue(
                    VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET,
                    new int[] {
                        preset.getPresetNumber(),
                        preset.getBand(),
                        preset.getChannel(),
                        preset.getSubChannel()}, 0);
        getVehicleHalEmulator().injectEvent(v);

        success = mAvailable.tryAcquire(5L, TimeUnit.SECONDS);
        assertEquals("injectEvent, onEvent timeout!", true, success);
    }
}
