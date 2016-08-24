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

import android.car.VehicleWindow;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.car.vehiclenetwork.VehicleNetworkConsts;

@SmallTest
public class VehicleWindowTest extends AndroidTestCase {

    public void testMatchWithVehicleHal() {
        assertEquals(VehicleNetworkConsts.VehicleWindow.VEHICLE_WINDOW_FRONT_WINDSHIELD,
                VehicleWindow.WINDOW_FRONT_WINDSHIELD);
        assertEquals(VehicleNetworkConsts.VehicleWindow.VEHICLE_WINDOW_REAR_WINDSHIELD,
                VehicleWindow.WINDOW_REAR_WINDSHIELD);
        assertEquals(VehicleNetworkConsts.VehicleWindow.VEHICLE_WINDOW_ROOF_TOP,
                VehicleWindow.WINDOW_ROOF_TOP);
        assertEquals(VehicleNetworkConsts.VehicleWindow.VEHICLE_WINDOW_ROW_1_LEFT,
                VehicleWindow.WINDOW_ROW_1_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleWindow.VEHICLE_WINDOW_ROW_1_RIGHT,
                VehicleWindow.WINDOW_ROW_1_RIGHT);
        assertEquals(VehicleNetworkConsts.VehicleWindow.VEHICLE_WINDOW_ROW_2_LEFT,
                VehicleWindow.WINDOW_ROW_2_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleWindow.VEHICLE_WINDOW_ROW_2_RIGHT,
                VehicleWindow.WINDOW_ROW_2_RIGHT);
        assertEquals(VehicleNetworkConsts.VehicleWindow.VEHICLE_WINDOW_ROW_3_LEFT,
                VehicleWindow.WINDOW_ROW_3_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleWindow.VEHICLE_WINDOW_ROW_3_RIGHT,
                VehicleWindow.WINDOW_ROW_3_RIGHT);
    }
}
