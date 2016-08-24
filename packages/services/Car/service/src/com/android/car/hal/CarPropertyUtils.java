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
package com.android.car.hal;

import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_BOOLEAN;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC2;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC3;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT_VEC4;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_INT32;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC2;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC3;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC4;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_STRING;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_BOOLEAN;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC2;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC3;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC4;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32_VEC2;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32_VEC3;
import static com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32_VEC4;
import static com.android.car.vehiclenetwork.VehiclePropValueUtil.getVectorValueType;
import static java.lang.Integer.toHexString;

import android.car.VehicleZoneUtil;
import android.car.VehicleAreaType;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;

import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;

import java.util.List;

/**
 * Utility functions to work with {@link CarPropertyConfig} and {@link CarPropertyValue}
 */
/*package*/ final class CarPropertyUtils {

    /* Utility class has no public constructor */
    private CarPropertyUtils() {}

    /** Converts {@link VehiclePropValue} to {@link CarPropertyValue} */
    static CarPropertyValue<?> toCarPropertyValue(
            VehiclePropValue halValue, int propertyId) {
        Class<?> clazz = getJavaClass(halValue.getValueType());
        int areaId = halValue.getZone();
        if (Boolean.class == clazz) {
            return new CarPropertyValue<>(propertyId, areaId, halValue.getInt32Values(0) == 1);
        } else if (String.class == clazz) {
            return new CarPropertyValue<>(propertyId, areaId, halValue.getStringValue());
        } else if (Long.class == clazz) {
            return new CarPropertyValue<>(propertyId, areaId, halValue.getInt64Value());
        } else /* All list properties */ {
            Object[] values = getRawValueList(clazz, halValue).toArray();
            return new CarPropertyValue<>(propertyId, areaId,
                    values.length == 1 ? values[0] : values);
        }
    }

    /** Converts {@link CarPropertyValue} to {@link VehiclePropValue} */
    static VehiclePropValue toVehiclePropValue(CarPropertyValue hvacProp, int halPropId) {
        VehiclePropValue.Builder builder = VehiclePropValue.newBuilder();
        builder.setProp(halPropId);

        if (hvacProp.getAreaId() != 0) {
            builder.setZone(hvacProp.getAreaId());
        }

        Object o = hvacProp.getValue();

        boolean hasArea = hvacProp.getAreaId() != 0;
        int vectorLength = (o instanceof Object[] ? ((Object[]) o).length : 0);
        int halType;
        if (o instanceof Boolean) {
            halType = hasArea ? VEHICLE_VALUE_TYPE_ZONED_BOOLEAN : VEHICLE_VALUE_TYPE_BOOLEAN;
            builder.addInt32Values(((Boolean )o) ? 1 : 0);
        } else if (o instanceof Integer) {
            halType = hasArea ? VEHICLE_VALUE_TYPE_ZONED_INT32 : VEHICLE_VALUE_TYPE_INT32;
            builder.addInt32Values((Integer) o);
        } else if (o instanceof Float) {
            halType = hasArea ? VEHICLE_VALUE_TYPE_ZONED_FLOAT : VEHICLE_VALUE_TYPE_FLOAT;
            builder.addFloatValues((Float) o);
        } else if (o instanceof Integer[]) {
            halType = getVectorValueType(
                    hasArea ? VEHICLE_VALUE_TYPE_ZONED_INT32 : VEHICLE_VALUE_TYPE_ZONED_INT32,
                    vectorLength);
            for (Integer i : (Integer[]) o) {
                builder.addInt32Values(i);
            }
        } else if (o instanceof Float[]) {
            halType = getVectorValueType(
                    hasArea ? VEHICLE_VALUE_TYPE_ZONED_FLOAT : VEHICLE_VALUE_TYPE_ZONED_FLOAT,
                    vectorLength);
            for (Float f : (Float[]) o) {
                builder.addFloatValues(f);
            }
        } else if (o instanceof String) {
            halType = VEHICLE_VALUE_TYPE_STRING;
            builder.setStringValue((String) o);
        } else {
            throw new IllegalArgumentException("Unexpected type in: " + hvacProp);
        }
        builder.setValueType(halType);
        return builder.build();
    }

    /**
     * Converts {@link VehiclePropConfig} to {@link CarPropertyConfig}.
     */
    static CarPropertyConfig<?> toCarPropertyConfig(VehiclePropConfig p, int propertyId) {
        int[] areas = VehicleZoneUtil.listAllZones(p.getZones());

        // TODO: handle other vehicle area types.
        int areaType = areas.length == 0
                ? VehicleAreaType.VEHICLE_AREA_TYPE_NONE : VehicleAreaType.VEHICLE_AREA_TYPE_ZONE;

        Class<?> clazz = getJavaClass(p.getValueType());
        if (clazz == Boolean.class) {
            return CarPropertyConfig
                    .newBuilder(clazz, propertyId, areaType, /* capacity */ 1)
                    .addAreas(areas)
                    .build();
        } else {
            List mins;
            List maxs;
            if (classMatched(Integer.class, clazz)) {
                mins = p.getInt32MinsList();
                maxs = p.getInt32MaxsList();
            } else if (classMatched(Float.class, clazz)) {
                mins = p.getFloatMinsList();
                maxs = p.getFloatMaxsList();
            } else {
                throw new IllegalArgumentException("Unexpected type: " + clazz);
            }
            CarPropertyConfig.Builder builder = CarPropertyConfig
                    .newBuilder(clazz, propertyId, areaType, /* capacity */ mins.size());
            for (int i = 0; i < mins.size(); i++) {
                int areaId = areas.length == 0 ? 0 : areas[i];
                builder.addAreaConfig(areaId, mins.get(i), maxs.get(i));
            }
            return builder.build();
        }
    }

    private static Class<?> getJavaClass(int halType) {
        switch (halType) {
            case VEHICLE_VALUE_TYPE_BOOLEAN:
            case VEHICLE_VALUE_TYPE_ZONED_BOOLEAN:
                return Boolean.class;
            case VEHICLE_VALUE_TYPE_FLOAT:
            case VEHICLE_VALUE_TYPE_ZONED_FLOAT:
                return Float.class;
            case VEHICLE_VALUE_TYPE_INT32:
            case VEHICLE_VALUE_TYPE_ZONED_INT32:
                return Integer.class;
            case VEHICLE_VALUE_TYPE_INT32_VEC2:
            case VEHICLE_VALUE_TYPE_INT32_VEC3:
            case VEHICLE_VALUE_TYPE_INT32_VEC4:
            case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC2:
            case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC3:
            case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC4:
                return Integer[].class;
            case VEHICLE_VALUE_TYPE_FLOAT_VEC2:
            case VEHICLE_VALUE_TYPE_FLOAT_VEC3:
            case VEHICLE_VALUE_TYPE_FLOAT_VEC4:
            case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC2:
            case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC3:
            case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC4:
                return Float[].class;
            case VEHICLE_VALUE_TYPE_STRING:
                return String.class;
            default:
                throw new IllegalArgumentException("Unexpected type: " + toHexString(halType));
        }
    }

    private static List getRawValueList(Class<?> clazz, VehiclePropValue vehiclePropValue) {
        if (classMatched(Float.class, clazz)) {
            return vehiclePropValue.getFloatValuesList();
        } else if (classMatched(Integer.class, clazz)) {
            return vehiclePropValue.getInt32ValuesList();
        } else {
            throw new IllegalArgumentException("Unexpected type: " + clazz);
        }
    }

    private static boolean classMatched(Class<?> class1, Class<?> class2) {
        return class1 == class2 || class1.getComponentType() == class2;
    }
}
