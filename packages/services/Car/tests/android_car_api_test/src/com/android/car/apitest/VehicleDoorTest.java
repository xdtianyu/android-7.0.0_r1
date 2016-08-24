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

import android.car.VehicleDoor;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.car.vehiclenetwork.VehicleNetworkConsts;

@SmallTest
public class VehicleDoorTest extends AndroidTestCase {

    public void testMatchWithVehicleHal() {
        assertEquals(VehicleNetworkConsts.VehicleDoor.VEHICLE_DOOR_HOOD, VehicleDoor.DOOR_HOOD);
        assertEquals(VehicleNetworkConsts.VehicleDoor.VEHICLE_DOOR_REAR, VehicleDoor.DOOR_REAR);
        assertEquals(VehicleNetworkConsts.VehicleDoor.VEHICLE_DOOR_ROW_1_LEFT,
                VehicleDoor.DOOR_ROW_1_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleDoor.VEHICLE_DOOR_ROW_1_RIGHT,
                VehicleDoor.DOOR_ROW_1_RIGHT);
        assertEquals(VehicleNetworkConsts.VehicleDoor.VEHICLE_DOOR_ROW_2_LEFT,
                VehicleDoor.DOOR_ROW_2_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleDoor.VEHICLE_DOOR_ROW_2_RIGHT,
                VehicleDoor.DOOR_ROW_2_RIGHT);
        assertEquals(VehicleNetworkConsts.VehicleDoor.VEHICLE_DOOR_ROW_3_LEFT,
                VehicleDoor.DOOR_ROW_3_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleDoor.VEHICLE_DOOR_ROW_3_RIGHT,
                VehicleDoor.DOOR_ROW_3_RIGHT);
    }
}
