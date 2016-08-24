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

package android.car.test;

import com.android.car.vehiclenetwork.IVehicleNetworkHalMock;
import com.android.car.vehiclenetwork.VehiclePropValueParcelable;

/** @hide */
interface ICarTest {
    /** For testing only. inject events. */
    void injectEvent(in VehiclePropValueParcelable value)         = 1;
    /** For testing only. Start in mocking mode. */
    void startMocking(in IVehicleNetworkHalMock mock, int flags)  = 2;
    /** Finish mocking mode. */
    void stopMocking(in IVehicleNetworkHalMock mock)              = 3;
    /** If given property is supported or not */
    boolean isPropertySupported(int property)                     = 4;
}
