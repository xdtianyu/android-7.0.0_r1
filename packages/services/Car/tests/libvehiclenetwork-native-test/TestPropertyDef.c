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

#include <stdint.h>
#include <sys/types.h>

#include "TestProperties.h"

static vehicle_prop_config_t TEST_PROPERTIES[] = {
    {
        .prop = TEST_PROPERTY_STRING,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_STRING,
        .config_flags = 0x1234, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_BYTES,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_BYTES,
        .config_flags = 0x12345, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_BOOLEAN,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_BOOLEAN,
        .config_flags = 0x123456, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_ZONED_INT32,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_ZONED_INT32,
        .config_flags = 0x1234567, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_ZONED_FLOAT,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_ZONED_FLOAT,
        .config_flags = 0x12345678, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_ZONED_BOOLEAN,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_ZONED_BOOLEAN,
        .config_flags = 0x10, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_INT64,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_INT64,
        .config_flags = 0x11, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_FLOAT,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_FLOAT,
        .config_flags = 0x12, // just random
        .float_min_value = 0.1f,
        .float_max_value = 10.0f,
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_FLOAT_VEC2,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_FLOAT_VEC2,
        .config_flags = 0x13, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_FLOAT_VEC3,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_FLOAT_VEC3,
        .config_flags = 0x14, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_FLOAT_VEC4,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_FLOAT_VEC4,
        .config_flags = 0x15, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_INT32,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_INT32,
        .config_flags = 0x16, // just random
        .int32_min_value = 10,
        .int32_max_value = 100,
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_INT32_VEC2,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_INT32_VEC2,
        .config_flags = 0x17, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_INT32_VEC3,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_INT32_VEC3,
        .config_flags = 0x18, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
    {
        .prop = TEST_PROPERTY_INT32_VEC4,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_INT32_VEC4,
        .config_flags = 0x0, // just random
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
};

vehicle_prop_config_t const * getTestProperties() {
    return TEST_PROPERTIES;
}

int getNumTestProperties() {
    return sizeof(TEST_PROPERTIES) / sizeof(vehicle_prop_config_t);
}

