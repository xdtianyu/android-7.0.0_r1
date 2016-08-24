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
package com.android.car.vehiclenetwork;

import com.android.car.vehiclenetwork.VehiclePropConfigsParcelable;
import com.android.car.vehiclenetwork.VehiclePropValueParcelable;

/**
 * Listener for vehicle HAL mock. This is used for internal testing only.
 * @hide
 */
interface IVehicleNetworkHalMock {
    VehiclePropConfigsParcelable onListProperties() = 0;
    void onPropertySet(in VehiclePropValueParcelable value) = 1;
    VehiclePropValueParcelable onPropertyGet(in VehiclePropValueParcelable value) = 2;
    void onPropertySubscribe(int property, float sampleRate, int zones) = 3;
    void onPropertyUnsubscribe(int property) = 4;
}
