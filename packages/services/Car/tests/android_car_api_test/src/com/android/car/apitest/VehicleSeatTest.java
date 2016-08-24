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

import android.car.VehicleSeat;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.car.vehiclenetwork.VehicleNetworkConsts;

@SmallTest
public class VehicleSeatTest extends AndroidTestCase {

    public void testMatchWithVehicleHal() {
        assertEquals(VehicleNetworkConsts.VehicleSeat.VEHICLE_SEAT_DRIVER_LHD,
                VehicleSeat.SEAT_DRIVER_LHD);
        assertEquals(VehicleNetworkConsts.VehicleSeat.VEHICLE_SEAT_DRIVER_RHD,
                VehicleSeat.SEAT_DRIVER_RHD);
        assertEquals(VehicleNetworkConsts.VehicleSeat.VEHICLE_SEAT_ROW_1_PASSENGER_LEFT,
                VehicleSeat.SEAT_ROW_1_PASSENGER_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleSeat.VEHICLE_SEAT_ROW_1_PASSENGER_CENTER,
                VehicleSeat.SEAT_ROW_1_PASSENGER_CENTER);
        assertEquals(VehicleNetworkConsts.VehicleSeat.VEHICLE_SEAT_ROW_1_PASSENGER_RIGHT,
                VehicleSeat.SEAT_ROW_1_PASSENGER_RIGHT);
        assertEquals(VehicleNetworkConsts.VehicleSeat.VEHICLE_SEAT_ROW_2_PASSENGER_LEFT,
                VehicleSeat.SEAT_ROW_2_PASSENGER_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleSeat.VEHICLE_SEAT_ROW_2_PASSENGER_CENTER,
                VehicleSeat.SEAT_ROW_2_PASSENGER_CENTER);
        assertEquals(VehicleNetworkConsts.VehicleSeat.VEHICLE_SEAT_ROW_2_PASSENGER_RIGHT,
                VehicleSeat.SEAT_ROW_2_PASSENGER_RIGHT);
        assertEquals(VehicleNetworkConsts.VehicleSeat.VEHICLE_SEAT_ROW_3_PASSENGER_LEFT,
                VehicleSeat.SEAT_ROW_3_PASSENGER_LEFT);
        assertEquals(VehicleNetworkConsts.VehicleSeat.VEHICLE_SEAT_ROW_3_PASSENGER_CENTER,
                VehicleSeat.SEAT_ROW_3_PASSENGER_CENTER);
        assertEquals(VehicleNetworkConsts.VehicleSeat.VEHICLE_SEAT_ROW_3_PASSENGER_RIGHT,
                VehicleSeat.SEAT_ROW_3_PASSENGER_RIGHT);
    }
}


