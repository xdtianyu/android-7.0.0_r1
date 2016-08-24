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

#ifndef CAR_VEHICLE_HAL_PROPERTY_UTIL_H_
#define CAR_VEHICLE_HAL_PROPERTY_UTIL_H_

#include <stdint.h>
#include <sys/types.h>
#include <inttypes.h>

#include <hardware/hardware.h>
#include <hardware/vehicle.h>

#include <utils/String8.h>

#include <IVehicleNetwork.h>

namespace android {

class VechilePropertyUtil {
public:
    static void dumpProperty(String8& msg, const vehicle_prop_config_t& config) {
        msg.appendFormat("property 0x%x, access:0x%x, change_mode:0x%x, value_type:0x%x",
                config.prop, config.access, config.change_mode, config.value_type);
        msg.appendFormat(",permission:0x%x, zones:0x%x, conflg_flag:0x%x, fsmin:%f, fsmax:%f",
                config.permission_model, config.vehicle_zone_flags, config.config_flags,
                config.min_sample_rate, config.max_sample_rate);
        switch (config.value_type) {
            case VEHICLE_VALUE_TYPE_FLOAT:
            case VEHICLE_VALUE_TYPE_FLOAT_VEC2:
            case VEHICLE_VALUE_TYPE_FLOAT_VEC3:
            case VEHICLE_VALUE_TYPE_FLOAT_VEC4: {
                msg.appendFormat(",v min:%f, v max:%f\n", config.float_min_value,
                        config.float_max_value);
            } break;
            case VEHICLE_VALUE_TYPE_ZONED_FLOAT:
            case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC2:
            case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC3:
            case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC4: {
                if (config.float_min_values == NULL) {
                    if (config.float_max_values == NULL) {
                        msg.appendFormat(",v min:%f, v max:%f\n", config.float_min_value,
                                config.float_max_value);
                    } else {
                        msg.appendFormat(", ERROR: float_max_values not NULL while min is NULL");

                    }
                } else {
                    if (config.float_max_values == NULL) {
                        msg.appendFormat(", ERROR: float_min_values not NULL while max is NULL");
                    } else {
                        int n = VehicleNetworkUtil::countNumberOfZones(
                                config.vehicle_zone_flags);
                        msg.appendFormat(", v min:");
                        for (int i = 0; i < n; i++) {
                            msg.appendFormat("%f,", config.float_min_values[i]);
                        }
                        msg.appendFormat(", v max:");
                        for (int i = 0; i < n; i++) {
                            msg.appendFormat("%f,", config.float_max_values[i]);
                        }
                    }
                }
            } break;
            case VEHICLE_VALUE_TYPE_INT64: {
                msg.appendFormat(",v min:%" PRId64 " max:%" PRId64 "\n", config.int64_min_value,
                        config.int64_max_value);
            } break;
            case VEHICLE_VALUE_TYPE_INT32:
            case VEHICLE_VALUE_TYPE_INT32_VEC2:
            case VEHICLE_VALUE_TYPE_INT32_VEC3:
            case VEHICLE_VALUE_TYPE_INT32_VEC4: {
                msg.appendFormat(",v min:%d, v max:%d\n", config.int32_min_value,
                        config.int32_max_value);
            } break;
            case VEHICLE_VALUE_TYPE_ZONED_INT32:
            case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC2:
            case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC3:
            case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC4: {
                if (config.int32_min_values == NULL) {
                    if (config.int32_max_values == NULL) {
                        msg.appendFormat(",v min:%d, v max:%d\n", config.int32_min_value,
                                config.int32_max_value);
                    } else {
                        msg.appendFormat(", ERROR: int32_max_values not NULL while min is NULL");

                    }
                } else {
                    if (config.int32_max_values == NULL) {
                        msg.appendFormat(", ERROR: int32_min_values not NULL while max is NULL");
                    } else {
                        int n = VehicleNetworkUtil::countNumberOfZones(
                                config.vehicle_zone_flags);
                        msg.appendFormat(", v min:");
                        for (int i = 0; i < n; i++) {
                            msg.appendFormat("%d,", config.int32_min_values[i]);
                        }
                        msg.appendFormat(", v max:");
                        for (int i = 0; i < n; i++) {
                            msg.appendFormat("%d,", config.int32_max_values[i]);
                        }
                    }
                }
            } break;
            default:
                msg.appendFormat("\n");
        }
    }
};


};

#endif /* CAR_VEHICLE_HAL_PROPERTY_UTIL_H_ */
