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

import android.support.car.Car;
import android.support.car.CarInfoManager;
import android.util.Log;

import com.android.car.test.MockedCarTestBase;
import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehiclePropConfigUtil;
import com.android.car.vehiclenetwork.VehiclePropValueUtil;

public class CarInfoManagerTest extends MockedCarTestBase {

    private static final String TAG = CarInfoManagerTest.class.getSimpleName();

    private static final String MAKE_NAME = "ANDROID";

    private CarInfoManager mCarInfoManager;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        getVehicleHalEmulator().addStaticProperty(
                VehiclePropConfigUtil.createStaticStringProperty(
                        VehicleNetworkConsts.VEHICLE_PROPERTY_INFO_MAKE),
                VehiclePropValueUtil.createStringValue(
                        VehicleNetworkConsts.VEHICLE_PROPERTY_INFO_MAKE, MAKE_NAME, 0));
        getVehicleHalEmulator().start();
        mCarInfoManager =
                (CarInfoManager) getSupportCar().getCarManager(Car.INFO_SERVICE);
    }

    public void testManufactuter() throws Exception {
        String name = mCarInfoManager.getString(CarInfoManager.KEY_MANUFACTURER);
        assertEquals(MAKE_NAME, name);
        Log.i(TAG, CarInfoManager.KEY_MANUFACTURER + ":" + name);
        try {
            Float v = mCarInfoManager.getFloat(CarInfoManager.KEY_MANUFACTURER);
            fail("type check failed");
        } catch (IllegalArgumentException e) {
            // expected
        }
        try {
            Integer v = mCarInfoManager.getInt(CarInfoManager.KEY_MANUFACTURER);
            fail("type check failed");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    public void testNoSuchInfo() throws Exception {
        final String NO_SUCH_NAME = "no-such-information-available";
        try {
            String name = mCarInfoManager.getString(NO_SUCH_NAME);
            fail("wrong param check");
        } catch (IllegalArgumentException e) {
            // expected
        }
        try {
            Integer intValue = mCarInfoManager.getInt(NO_SUCH_NAME);
            fail("wrong param check");
        } catch (IllegalArgumentException e) {
            //expected
        }
        try {
            Float floatValue = mCarInfoManager.getFloat(NO_SUCH_NAME);
            fail("wrong param check");
        } catch (IllegalArgumentException e) {
            //expected
        }
    }
}
