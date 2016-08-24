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

import com.google.protobuf.ByteString;

import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;

import java.util.Arrays;
import java.util.List;

/**
 * Utility class to help creating VehiclePropValue.
 */
public final class VehiclePropValueUtil {

    /** To prevent creating of utility class */
    private VehiclePropValueUtil() {}

    public static VehiclePropValue createIntValue(int property, int value, long timestamp) {
        return createBuilder(property, VehicleValueType.VEHICLE_VALUE_TYPE_INT32, timestamp).
                addInt32Values(value).
                build();
    }

    public static VehiclePropValue createIntVectorValue(int property, int[] values,
            long timestamp) {
        VehiclePropValue.Builder builder = createBuilder(property,
                getVectorValueType(VehicleValueType.VEHICLE_VALUE_TYPE_INT32, values.length),
                timestamp);
        for (int v : values) {
            builder.addInt32Values(v);
        }
        return builder.build();
    }

    public static VehiclePropValue createFloatValue(int property, float value, long timestamp) {
        return createBuilder(property, VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT, timestamp).
                addFloatValues(value).
                build();
    }

    public static VehiclePropValue createFloatVectorValue(int property, float[] values,
            long timestamp) {
        VehiclePropValue.Builder builder = createBuilder(property,
                getVectorValueType(VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT, values.length),
                timestamp);
        for (float v : values) {
            builder.addFloatValues(v);
        }
        return builder.build();
    }

    public static VehiclePropValue createLongValue(int property, long value, long timestamp) {
        return createBuilder(property, VehicleValueType.VEHICLE_VALUE_TYPE_INT64, timestamp).
                setInt64Value(value).
                build();
    }

    public static VehiclePropValue createStringValue(int property, String value, long timestamp) {
        return createBuilder(property, VehicleValueType.VEHICLE_VALUE_TYPE_STRING, timestamp).
                setStringValue(value).
                build();
    }

    public static VehiclePropValue createBooleanValue(int property, boolean value, long timestamp) {
        return createBuilder(property, VehicleValueType.VEHICLE_VALUE_TYPE_BOOLEAN, timestamp).
                addInt32Values(value ? 1 : 0).
                build();
    }

    public static VehiclePropValue createBytesValue(int property, byte[] value, long timestamp) {
        return createBuilder(property, VehicleValueType.VEHICLE_VALUE_TYPE_BYTES, timestamp).
                setBytesValue(ByteString.copyFrom(value)).
                build();
    }

    public static VehiclePropValue createZonedIntValue(int property, int zone, int value,
            long timestamp) {
        return createBuilder(property, VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32, timestamp).
                setZone(zone).
                addInt32Values(value).
                build();
    }

    public static VehiclePropValue createZonedIntVectorValue(int property, int zone, int[] values,
            long timestamp) {
        int valueType = getVectorValueType(
                VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32, values.length);
        VehiclePropValue.Builder builder = createBuilder(property, valueType, timestamp).
                setZone(zone);
        for (int value : values) {
            builder.addInt32Values(value);
        }
        return builder.build();
    }

    public static VehiclePropValue createZonedFloatVectorValue(int property, int zone,
            float[] values, long timestamp) {
        int valueType = getVectorValueType(
                VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT, values.length);
        VehiclePropValue.Builder builder =  createBuilder(property, valueType, timestamp).
                setZone(zone);
        for (float value : values) {
            builder.addFloatValues(value);
        }
        return builder.build();
    }


    public static VehiclePropValue createZonedBooleanValue(int property, int zone, boolean value,
            long timestamp) {
        return createBuilder(property, VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_BOOLEAN,timestamp).
                setZone(zone).
                addInt32Values(value ? 1 : 0).
                build();
    }

    public static VehiclePropValue createZonedFloatValue(int property, int zone, float value,
            long timestamp) {
        return createBuilder(property, VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT,timestamp).
                setZone(zone).
                addFloatValues(value).
                build();
    }

    public static VehiclePropValue createDummyValue(int property, int valueType) {
        switch (valueType) {
            case VehicleValueType.VEHICLE_VALUE_TYPE_STRING: {
                return createStringValue(property, "dummy", 0);
            }
            case VehicleValueType.VEHICLE_VALUE_TYPE_BYTES: {
                return createBytesValue(property, new byte[1], 0);
            }
            case VehicleValueType.VEHICLE_VALUE_TYPE_BOOLEAN: {
                return createBooleanValue(property, false, 0);
            }
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32: {
                return createZonedIntValue(property, 0, 0, 0);
            }
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT: {
                return createZonedFloatValue(property, 0, 0, 0);
            }
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_BOOLEAN: {
                return createZonedBooleanValue(property, 0, false, 0);
            }
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT64: {
                return createLongValue(property, 0, 0);
            }
            case VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT: {
                return createFloatValue(property, 0, 0);
            }
            case VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC2:
            case VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC3:
            case VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC4:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC2:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC3:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC4:{
                return createFloatVectorValue(property, new float[getVectorLength(valueType)], 0);
            }
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT32: {
                return createIntValue(property, 0, 0);
            }
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC2:
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC3:
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC4:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32_VEC2:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32_VEC3:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32_VEC4: {
                return createIntVectorValue(property, new int[getVectorLength(valueType)], 0);
            }

        }
        return null;
    }

    public static VehiclePropValue.Builder createBuilder(int property, int valueType,
            long timestamp) {
        return VehiclePropValue.newBuilder().
                setProp(property).
                setValueType(valueType).
                setTimestamp(timestamp);
    }

    public static int getVectorLength(int vehicleValueType) {
        switch (vehicleValueType) {
            case VehicleValueType.VEHICLE_VALUE_TYPE_BOOLEAN:
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT32:
            case VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_BOOLEAN:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT:
                return 1;
            case VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC2:
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC2:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC2:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32_VEC2:
                return 2;
            case VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC3:
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC3:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC3:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32_VEC3:
                return 3;
            case VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC4:
            case VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC4:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC4:
            case VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32_VEC4:
                return 4;
            default:
                throw new IllegalArgumentException("Unknown value type: " + vehicleValueType);
        }
    }

    public static boolean isCustomProperty(int property) {
        return property >= VehicleNetworkConsts.VEHICLE_PROPERTY_CUSTOM_START &&
                property <= VehicleNetworkConsts.VEHICLE_PROPERTY_CUSTOM_END;
    }

    /** Converts {@link VehiclePropValue} to string just for debug purpose. */
    public static String toString(VehiclePropValue value) {
        if (value == null) {
            return String.valueOf(null);
        }
        return new StringBuilder()
                .append("prop: " + value.getProp() + "\n")
                .append("valueType: " + value.getValueType() + "\n")
                .append("timestamp: " + value.getTimestamp() + "\n")
                .append("int32Values: " + Arrays.toString(toIntArray(value.getInt32ValuesList()))
                        + "\n")
                .append("int64Value: " + value.getInt64Value() + "\n")
                .append("floatValues: " + Arrays.toString(toFloatArray(value.getFloatValuesList()))
                        + "\n")
                .append("stringValue: " + value.getStringValue() + "\n")
                .append("byteValue: " + Arrays.toString(value.getBytesValue().toByteArray()) + "\n")
                .append("zone: {" + value.getZone() + "}")
                .toString();
    }

    public static int[] toIntArray(List<Integer> collection) {
        int[] array = new int[collection.size()];
        int i = 0;
        for (int value : collection) {
            array[i++] = value;
        }
        return array;
    }

    public static float[] toFloatArray(List<Float> collection) {
        float[] array = new float[collection.size()];
        int i = 0;
        for (float value : collection) {
            array[i++] = value;
        }
        return array;
    }

    public static int getVectorValueType(int vehicleValueType, int length) {
        return vehicleValueType + length - 1;
    }
}
