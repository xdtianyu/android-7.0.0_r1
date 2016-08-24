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

import android.car.VehicleZone;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.car.vehiclenetwork.VehicleNetworkConsts;

@SmallTest
public class VehicleZoneTest extends AndroidTestCase {

    public void testMatchWithVehicleHal() {
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ALL, VehicleZone.ZONE_ALL);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_1_ALL,
                VehicleZone.ZONE_ROW_1_ALL);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_1_CENTER,
                VehicleZone.ZONE_ROW_1_CENTER);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_1_LEFT,
                VehicleZone.ZONE_ROW_1_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_1_RIGHT,
                VehicleZone.ZONE_ROW_1_RIGHT);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_2_ALL,
                VehicleZone.ZONE_ROW_2_ALL);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_2_CENTER,
                VehicleZone.ZONE_ROW_2_CENTER);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_2_LEFT,
                VehicleZone.ZONE_ROW_2_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_2_RIGHT,
                VehicleZone.ZONE_ROW_2_RIGHT);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_3_ALL,
                VehicleZone.ZONE_ROW_3_ALL);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_3_CENTER,
                VehicleZone.ZONE_ROW_3_CENTER);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_3_LEFT,
                VehicleZone.ZONE_ROW_3_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_3_RIGHT,
                VehicleZone.ZONE_ROW_3_RIGHT);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_4_ALL,
                VehicleZone.ZONE_ROW_4_ALL);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_4_CENTER,
                VehicleZone.ZONE_ROW_4_CENTER);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_4_LEFT,
                VehicleZone.ZONE_ROW_4_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleZone.VEHICLE_ZONE_ROW_4_RIGHT,
                VehicleZone.ZONE_ROW_4_RIGHT);
    }
}
