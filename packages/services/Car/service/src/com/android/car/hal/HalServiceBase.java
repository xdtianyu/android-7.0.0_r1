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

package com.android.car.hal;

import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;

import java.io.PrintWriter;
import java.util.LinkedList;
import java.util.List;

/**
 * Common interface for all HAL service like sensor HAL.
 * Each HAL service is connected with XyzService supporting XyzManager,
 * and will translate HAL data into car api specific format.
 */
public abstract class HalServiceBase {
    /** For dispatching events. Kept here to avoid alloc every time */
    private final LinkedList<VehiclePropValue> mDispatchList = new LinkedList<VehiclePropValue>();

    public List<VehiclePropValue> getDispatchList() {
        return mDispatchList;
    }

    /** initialize */
    public abstract void init();

    /** release and stop operation */
    public abstract void release();

    /**
     * return supported properties among all properties.
     * @return null if no properties are supported
     */
    /**
     * Take supported properties from given allProperties and return List of supported properties.
     * @param allProperties
     * @return null if no properties are supported.
     */
    public abstract List<VehiclePropConfig> takeSupportedProperties(
            List<VehiclePropConfig> allProperties);

    public abstract void handleHalEvents(List<VehiclePropValue> values);

    public abstract void dump(PrintWriter writer);
}
