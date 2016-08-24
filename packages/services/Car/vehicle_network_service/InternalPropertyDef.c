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

#include <vehicle-internal.h>

static vehicle_prop_config_t INTERNAL_PROPERTIES[] = {
    {
        .prop = VEHICLE_PROPERTY_INTERNAL_AUDIO_STREAM_STATE,
        .access = VEHICLE_PROP_ACCESS_READ_WRITE,
        .change_mode = VEHICLE_PROP_CHANGE_MODE_ON_CHANGE,
        .value_type = VEHICLE_VALUE_TYPE_INT32_VEC2,
        .min_sample_rate = 0,
        .max_sample_rate = 0,
        .hal_data = NULL,
    },
};

vehicle_prop_config_t const * getInternalProperties() {
    return INTERNAL_PROPERTIES;
}

int getNumInternalProperties() {
    return sizeof(INTERNAL_PROPERTIES) / sizeof(vehicle_prop_config_t);
}
