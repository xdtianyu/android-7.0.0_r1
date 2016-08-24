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

import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePermissionModel;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropAccess;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropChangeMode;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;

/**
 * Utility class to help creating VehiclePropConfig.
 */
public class VehiclePropConfigUtil {

    public static VehiclePropConfig createStaticStringProperty(int property) {
        return getBuilder(property,
                VehiclePropAccess.VEHICLE_PROP_ACCESS_READ,
                VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_STATIC,
                VehicleValueType.VEHICLE_VALUE_TYPE_STRING,
                VehiclePermissionModel.VEHICLE_PERMISSION_NO_RESTRICTION,
                0, 0f, 0f)
                .build();
    }

    public static VehiclePropConfig createZonedProperty(
            int property, int propAccess, int changeType, int valueType, int zones,
            int configFlags) {
            return VehiclePropConfig.newBuilder().
                    setProp(property).
                    setAccess(propAccess).
                    setChangeMode(changeType).
                    setValueType(valueType).
                    setPermissionModel(VehiclePermissionModel.VEHICLE_PERMISSION_NO_RESTRICTION).
                    setZones(zones).
                    addConfigArray(configFlags).
                    setSampleRateMax(0).
                    setSampleRateMin(0).
                    build();
    }

    public static VehiclePropConfig createProperty(
        int property, int propAccess, int changeType, int valueType, int configFlags) {
        return getBuilder(property,
                propAccess,
                changeType,
                valueType,
                VehiclePermissionModel.VEHICLE_PERMISSION_NO_RESTRICTION,
                configFlags, 0f, 0f)
                .build();
    }

    public static VehiclePropConfig.Builder getBuilder(int property, int access, int changeMode,
            int type, int permissionModel, int configFlags, float sampleRateMax,
            float sampleRateMin) {
        return VehiclePropConfig.newBuilder().
                setProp(property).
                setAccess(access).
                setChangeMode(changeMode).
                setValueType(type).
                setPermissionModel(permissionModel).
                addConfigArray(configFlags).
                setSampleRateMax(sampleRateMax).
                setSampleRateMin(sampleRateMin);
    }
}
