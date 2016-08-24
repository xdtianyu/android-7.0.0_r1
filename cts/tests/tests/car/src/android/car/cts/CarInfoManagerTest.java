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
package android.car.cts;

import android.car.Car;
import android.car.CarInfoManager;

public class CarInfoManagerTest extends CarApiTestBase {

    private CarInfoManager mCarInfoManager;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mCarInfoManager = (CarInfoManager) getCar().getCarManager(Car.INFO_SERVICE);
    }

    public void testManufacturer() throws Exception {
        // The values are not guaranteed, so just checking data types here.
        mCarInfoManager.getString(CarInfoManager.KEY_MANUFACTURER);
        mCarInfoManager.getInt(CarInfoManager.KEY_MODEL_YEAR);
        mCarInfoManager.getString(CarInfoManager.KEY_VEHICLE_ID);
        mCarInfoManager.getString(CarInfoManager.KEY_MODEL);
        try {
            mCarInfoManager.getFloat(CarInfoManager.KEY_MANUFACTURER);
            fail("type check failed");
        } catch (IllegalArgumentException e) {
            // Expected.
        }
        try {
            mCarInfoManager.getInt(CarInfoManager.KEY_MANUFACTURER);
            fail("type check failed");
        } catch (IllegalArgumentException e) {
            // Expected.
        }
    }

    public void testNoSuchInfo() throws Exception {
        final String NO_SUCH_NAME = "no-such-information-available";
        try {
            mCarInfoManager.getString(NO_SUCH_NAME);
            fail("wrong param check");
        } catch (IllegalArgumentException e) {
            // Expected.
        }
        try {
            mCarInfoManager.getInt(NO_SUCH_NAME);
            fail("wrong param check");
        } catch (IllegalArgumentException e) {
            // Expected.
        }
        try {
            mCarInfoManager.getFloat(NO_SUCH_NAME);
            fail("wrong param check");
        } catch (IllegalArgumentException e) {
            // Expected.
        }
    }
}
