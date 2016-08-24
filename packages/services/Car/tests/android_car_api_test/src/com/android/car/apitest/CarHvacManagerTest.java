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
package com.android.car.apitest;

import android.car.Car;
import android.car.hardware.hvac.CarHvacManager;
import android.car.hardware.hvac.CarHvacManager.HvacPropertyId;
import android.car.hardware.CarPropertyConfig;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.Log;

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

@MediumTest
public class CarHvacManagerTest extends CarApiTestBase {
    private static final String TAG = CarHvacManagerTest.class.getSimpleName();

    private CarHvacManager mHvacManager;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mHvacManager = (CarHvacManager) getCar().getCarManager(Car.HVAC_SERVICE);
        assertNotNull(mHvacManager);
    }

    public void testAllHvacProperties() throws Exception {
        List<CarPropertyConfig> properties = mHvacManager.getPropertyList();
        Set<Class> supportedTypes = new HashSet<>(Arrays.asList(
                new Class[] { Integer.class, Float.class, Boolean.class }));

        for (CarPropertyConfig property : properties) {
            if (supportedTypes.contains(property.getPropertyType())) {
                assertTypeAndZone(property);
            } else {
                fail("Type is not supported for " + property);
            }
        }
    }

    private void assertTypeAndZone(CarPropertyConfig property) {
        switch (property.getPropertyId()) {
            case HvacPropertyId.MIRROR_DEFROSTER_ON: // non-zoned bool
                assertEquals(Boolean.class, property.getPropertyType());
                assertTrue(property.isGlobalProperty());
                break;
            case HvacPropertyId.STEERING_WHEEL_TEMP: // non-zoned int
                assertEquals(Integer.class, property.getPropertyType());
                assertTrue(property.isGlobalProperty());
                checkIntMinMax(property);
                break;
            case HvacPropertyId.ZONED_TEMP_SETPOINT: // zoned float
            case HvacPropertyId.ZONED_TEMP_ACTUAL:
                assertEquals(Float.class, property.getPropertyType());
                assertFalse(property.isGlobalProperty());
                checkFloatMinMax(property);
                break;
            case HvacPropertyId.ZONED_TEMP_IS_FAHRENHEIT: // zoned boolean
            case HvacPropertyId.ZONED_AC_ON:
            case HvacPropertyId.WINDOW_DEFROSTER_ON:
            case HvacPropertyId.ZONED_AUTOMATIC_MODE_ON:
            case HvacPropertyId.ZONED_AIR_RECIRCULATION_ON:
                assertEquals(Boolean.class, property.getPropertyType());
                assertFalse(property.isGlobalProperty());
                break;
            case HvacPropertyId.ZONED_FAN_SPEED_SETPOINT: // zoned int
            case HvacPropertyId.ZONED_FAN_SPEED_RPM:
            case HvacPropertyId.ZONED_FAN_POSITION_AVAILABLE:
            case HvacPropertyId.ZONED_FAN_POSITION:
            case HvacPropertyId.ZONED_SEAT_TEMP:
                assertEquals(Integer.class, property.getPropertyType());
                assertFalse(property.isGlobalProperty());
                checkIntMinMax(property);
                break;
        }
    }

    private void checkIntMinMax(CarPropertyConfig<Integer> property) {
        Log.i(TAG, "checkIntMinMax property:" + property);
        if (!property.isGlobalProperty()) {
            int[] areaIds = property.getAreaIds();
            assertTrue(areaIds.length > 0);
            assertEquals(areaIds.length, property.getAreaCount());

            for (int areId : areaIds) {
                assertTrue(property.hasArea(areId));
                int min = property.getMinValue(areId);
                int max = property.getMaxValue(areId);
                assertTrue(min <= max);
            }
        } else {
            int min = property.getMinValue();
            int max = property.getMaxValue();
            assertTrue(min <= max);
            for (int i = 0; i < 32; i++) {
                assertFalse(property.hasArea(0x1 << i));
                assertNull(property.getMinValue(0x1 << i));
                assertNull(property.getMaxValue(0x1 << i));
            }
        }
    }

    private void checkFloatMinMax(CarPropertyConfig<Float> property) {
        Log.i(TAG, "checkFloatMinMax property:" + property);
        if (!property.isGlobalProperty()) {
            int[] areaIds = property.getAreaIds();
            assertTrue(areaIds.length > 0);
            assertEquals(areaIds.length, property.getAreaCount());

            for (int areId : areaIds) {
                assertTrue(property.hasArea(areId));
                float min = property.getMinValue(areId);
                float max = property.getMaxValue(areId);
                assertTrue(min <= max);
            }
        } else {
            float min = property.getMinValue();
            float max = property.getMaxValue();
            assertTrue(min <= max);
            for (int i = 0; i < 32; i++) {
                assertFalse(property.hasArea(0x1 << i));
                assertNull(property.getMinValue(0x1 << i));
                assertNull(property.getMaxValue(0x1 << i));
            }
        }
    }
}
