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

import com.android.car.vehiclenetwork.IVehicleNetworkHalMock;
import com.android.car.vehiclenetwork.IVehicleNetworkListener;
import com.android.car.vehiclenetwork.VehiclePropConfigsParcelable;
import com.android.car.vehiclenetwork.VehiclePropValueParcelable;
import com.android.car.vehiclenetwork.VehiclePropValuesParcelable;

/**
  * Binder API to access vehicle network service.
  * @hide
  */
interface IVehicleNetwork {
    VehiclePropConfigsParcelable listProperties(int property)                            = 0;
    /** For error case, exception will be thrown. */
    void setProperty(in VehiclePropValueParcelable value)                                = 1;
    VehiclePropValueParcelable getProperty(in VehiclePropValueParcelable value)          = 2;
    /** For error case, exception will be thrown. */
    void subscribe(in IVehicleNetworkListener listener, int property, float sampleRate,
                                                                             int zones)  = 3;
    void unsubscribe(in IVehicleNetworkListener listener, int property)                  = 4;
    /** For testing only. inject events. */
    void injectEvent(in VehiclePropValueParcelable value)                                = 5;
    /** For testing only. Start in mocking mode. */
    void startMocking(in IVehicleNetworkHalMock mock)                                    = 6;
    /** Finish mocking mode. */
    void stopMocking(in IVehicleNetworkHalMock mock)                                     = 7;
    void injectHalError(int errorCode, int property, int operation)                      = 8;
    void startErrorListening(in IVehicleNetworkListener listener)                        = 9;
    void stopErrorListening(in IVehicleNetworkListener listener)                         = 10;
    void startHalRestartMonitoring(in IVehicleNetworkListener listener)                  = 11;
    void stopHalRestartMonitoring(in IVehicleNetworkListener listener)                   = 12;

    //TODO add specialized set for byte array for efficiency
}
