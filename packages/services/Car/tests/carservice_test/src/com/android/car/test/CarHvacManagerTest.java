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
import android.car.hardware.hvac.CarHvacManager;
import android.car.hardware.hvac.CarHvacManager.CarHvacEventListener;
import android.car.hardware.hvac.CarHvacManager.HvacPropertyId;
import android.car.hardware.CarPropertyValue;
import android.car.test.VehicleHalEmulator;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.Log;

import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropAccess;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropChangeMode;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleWindow;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleZone;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;
import com.android.car.vehiclenetwork.VehiclePropConfigUtil;
import com.android.car.vehiclenetwork.VehiclePropValueUtil;

import java.util.HashMap;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

@MediumTest
public class CarHvacManagerTest extends MockedCarTestBase {
    private static final String TAG = CarHvacManagerTest.class.getSimpleName();

    // Use this semaphore to block until the callback is heard of.
    private Semaphore mAvailable;

    private CarHvacManager mCarHvacManager;
    private boolean mEventBoolVal;
    private float mEventFloatVal;
    private int mEventIntVal;
    private int mEventZoneVal;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mAvailable = new Semaphore(0);
        HvacPropertyHandler handler = new HvacPropertyHandler();
        getVehicleHalEmulator().addProperty(
                VehiclePropConfigUtil.createZonedProperty(
                        VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_DEFROSTER,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_BOOLEAN,
                        VehicleWindow.VEHICLE_WINDOW_FRONT_WINDSHIELD,
                        0), handler);
        getVehicleHalEmulator().addProperty(
                VehiclePropConfigUtil.createZonedProperty(
                        VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_FAN_SPEED,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32,
                        VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                        0), handler);
        getVehicleHalEmulator().addProperty(
                VehiclePropConfigUtil.createZonedProperty(
                        VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_TEMPERATURE_SET,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ_WRITE,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
                        VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT,
                        VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                        0), handler);
        getVehicleHalEmulator().addProperty(
                VehiclePropConfigUtil.createZonedProperty(
                        VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_TEMPERATURE_CURRENT,
                        VehiclePropAccess.VEHICLE_PROP_ACCESS_READ,
                        VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_CONTINUOUS,
                        VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT,
                        VehicleZone.VEHICLE_ZONE_ROW_1_ALL,
                        0), handler);

        getVehicleHalEmulator().start();
        mCarHvacManager = (CarHvacManager) getCar().getCarManager(Car.HVAC_SERVICE);
    }

    // Test a boolean property
    public void testHvacRearDefrosterOn() throws Exception {
        mCarHvacManager.setBooleanProperty(HvacPropertyId.WINDOW_DEFROSTER_ON,
                VehicleWindow.VEHICLE_WINDOW_FRONT_WINDSHIELD, true);
        boolean defrost = mCarHvacManager.getBooleanProperty(HvacPropertyId.WINDOW_DEFROSTER_ON,
                VehicleWindow.VEHICLE_WINDOW_FRONT_WINDSHIELD);
        assertTrue(defrost);

        mCarHvacManager.setBooleanProperty(HvacPropertyId.WINDOW_DEFROSTER_ON,
                VehicleWindow.VEHICLE_WINDOW_FRONT_WINDSHIELD, false);
        defrost = mCarHvacManager.getBooleanProperty(HvacPropertyId.WINDOW_DEFROSTER_ON,
                VehicleWindow.VEHICLE_WINDOW_FRONT_WINDSHIELD);
        assertFalse(defrost);
    }

    // Test an integer property
    public void testHvacFanSpeed() throws Exception {
        mCarHvacManager.setIntProperty(HvacPropertyId.ZONED_FAN_SPEED_SETPOINT,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT, 15);
        int speed = mCarHvacManager.getIntProperty(HvacPropertyId.ZONED_FAN_SPEED_SETPOINT,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT);
        assertEquals(15, speed);

        mCarHvacManager.setIntProperty(HvacPropertyId.ZONED_FAN_SPEED_SETPOINT,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT, 23);
        speed = mCarHvacManager.getIntProperty(HvacPropertyId.ZONED_FAN_SPEED_SETPOINT,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT);
        assertEquals(23, speed);
    }

    // Test an float property
    public void testHvacTempSetpoint() throws Exception {
        mCarHvacManager.setFloatProperty(HvacPropertyId.ZONED_TEMP_SETPOINT,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT, 70);
        float temp = mCarHvacManager.getFloatProperty(HvacPropertyId.ZONED_TEMP_SETPOINT,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT);
        assertEquals(70.0, temp, 0);

        mCarHvacManager.setFloatProperty(HvacPropertyId.ZONED_TEMP_SETPOINT,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT, (float) 65.5);
        temp = mCarHvacManager.getFloatProperty(HvacPropertyId.ZONED_TEMP_SETPOINT,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT);
        assertEquals(65.5, temp, 0);
    }

    // Test an event
    public void testEvent() throws Exception {
        mCarHvacManager.registerListener(new EventListener());

        // Inject a boolean event and wait for its callback in onPropertySet.
        VehiclePropValue v = VehiclePropValueUtil.createZonedBooleanValue(
                VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_DEFROSTER,
                VehicleWindow.VEHICLE_WINDOW_FRONT_WINDSHIELD, true, 0);
        assertEquals(0, mAvailable.availablePermits());
        getVehicleHalEmulator().injectEvent(v);

        assertTrue(mAvailable.tryAcquire(2L, TimeUnit.SECONDS));
        assertTrue(mEventBoolVal);
        assertEquals(mEventZoneVal, VehicleWindow.VEHICLE_WINDOW_FRONT_WINDSHIELD);

        // Inject a float event and wait for its callback in onPropertySet.
        v = VehiclePropValueUtil.createZonedFloatValue(
                VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_TEMPERATURE_CURRENT,
                VehicleZone.VEHICLE_ZONE_ROW_1_ALL, 67, 0);
        assertEquals(0, mAvailable.availablePermits());
        getVehicleHalEmulator().injectEvent(v);

        assertTrue(mAvailable.tryAcquire(2L, TimeUnit.SECONDS));
        assertEquals(mEventFloatVal, 67, 0);
        assertEquals(mEventZoneVal, VehicleZone.VEHICLE_ZONE_ROW_1_ALL);

        // Inject an integer event and wait for its callback in onPropertySet.
        v = VehiclePropValueUtil.createZonedIntValue(
                VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_FAN_SPEED,
                VehicleZone.VEHICLE_ZONE_ROW_1_LEFT, 4, 0);
        assertEquals(0, mAvailable.availablePermits());
        getVehicleHalEmulator().injectEvent(v);

        assertTrue(mAvailable.tryAcquire(2L, TimeUnit.SECONDS));
        assertEquals(mEventIntVal, 4);
        assertEquals(mEventZoneVal, VehicleZone.VEHICLE_ZONE_ROW_1_LEFT);
    }


    private class HvacPropertyHandler
            implements VehicleHalEmulator.VehicleHalPropertyHandler {
        HashMap<Integer, VehiclePropValue> mMap = new HashMap<>();

        @Override
        public synchronized void onPropertySet(VehiclePropValue value) {
            mMap.put(value.getProp(), value);
        }

        @Override
        public synchronized VehiclePropValue onPropertyGet(VehiclePropValue value) {
            return mMap.get(value.getProp());
        }

        @Override
        public synchronized void onPropertySubscribe(int property, float sampleRate, int zones) {
            Log.d(TAG, "onPropertySubscribe property " + property + " sampleRate " + sampleRate);
        }

        @Override
        public synchronized void onPropertyUnsubscribe(int property) {
            Log.d(TAG, "onPropertyUnSubscribe property " + property);
        }
    }

    private class EventListener implements CarHvacEventListener {
        public EventListener() { }

        @Override
        public void onChangeEvent(final CarPropertyValue value) {
            Log.d(TAG, "onChangeEvent: "  + value);
            Object o = value.getValue();
            mEventZoneVal = value.getAreaId();

            if (o instanceof Integer) {
                mEventIntVal = (Integer) o;
            } else if (o instanceof Float) {
                mEventFloatVal = (Float) o;
            } else if (o instanceof Boolean) {
                mEventBoolVal = (Boolean) o;
            }
            mAvailable.release();
        }

        @Override
        public void onErrorEvent(final int propertyId, final int zone) {
            Log.d(TAG, "Error:  propertyId=" + propertyId + "  zone=" + zone);
        }
    }
}
