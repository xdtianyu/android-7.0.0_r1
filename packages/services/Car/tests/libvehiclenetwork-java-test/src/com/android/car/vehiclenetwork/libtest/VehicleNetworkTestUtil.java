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
package com.android.car.vehiclenetwork.libtest;

import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;
import com.google.protobuf.ByteString;

public class VehicleNetworkTestUtil {

    public static VehiclePropValue createDummyValue(int prop, int valueType) {
        VehiclePropValue.Builder builder = VehiclePropValue.newBuilder().
                setProp(prop).
                setValueType(valueType);
        switch (valueType) {
            case VehicleValueType.VEHICLE_VALUE_TYPE_STRING:
                builder.setStringValue("");
                break;
            case VehicleValueType.VEHICLE_VALUE_TYPE_BYTES:
                builder.setBytesValue(ByteString.copyFrom(new byte[1]));
                break;
            case VehicleValueType.VEHICLE_VALUE_TYPE_BOOLEAN:
                builder.addInt32Values(0);
                break;
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_BOOLEAN:
                builder.setZone(0).addInt32Values(0);
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT:
                builder.setZone(0).addFloatValues(0f);
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT64:
                builder.setInt64Value(0);
                break;
            case VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT:
            case VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC2:
            case VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC3:
            case VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC4: {
                int n = valueType - VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT + 1;
                for (int i = 0; i < n; i++) {
                    builder.addFloatValues(0f);
                }
            } break;
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT32:
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC2:
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC3:
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC4: {
                int n = valueType - VehicleValueType.VEHICLE_VALUE_TYPE_INT32 + 1;
                for (int i = 0; i < n; i++) {
                    builder.addInt32Values(0);
                }
            } break;
        }
        return builder.build();
    }
}
