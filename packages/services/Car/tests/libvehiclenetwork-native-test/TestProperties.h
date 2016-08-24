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
#ifndef ANDROID_VECHILE_TEST_PROPERTIES_H
#define ANDROID_VECHILE_TEST_PROPERTIES_H

#include <stdint.h>
#include <sys/types.h>

#include <hardware/hardware.h>
#include <hardware/vehicle.h>

#define TEST_PROPERTY_STRING (VEHICLE_PROPERTY_CUSTOM_START + 1)
#define TEST_PROPERTY_BYTES (VEHICLE_PROPERTY_CUSTOM_START + 2)
#define TEST_PROPERTY_BOOLEAN (VEHICLE_PROPERTY_CUSTOM_START + 3)
#define TEST_PROPERTY_ZONED_INT32 (VEHICLE_PROPERTY_CUSTOM_START + 4)
#define TEST_PROPERTY_ZONED_FLOAT (VEHICLE_PROPERTY_CUSTOM_START + 5)
#define TEST_PROPERTY_ZONED_BOOLEAN (VEHICLE_PROPERTY_CUSTOM_START + 6)
#define TEST_PROPERTY_INT64 (VEHICLE_PROPERTY_CUSTOM_START + 7)
#define TEST_PROPERTY_FLOAT (VEHICLE_PROPERTY_CUSTOM_START + 8)
#define TEST_PROPERTY_FLOAT_VEC2 (VEHICLE_PROPERTY_CUSTOM_START + 9)
#define TEST_PROPERTY_FLOAT_VEC3 (VEHICLE_PROPERTY_CUSTOM_START + 10)
#define TEST_PROPERTY_FLOAT_VEC4 (VEHICLE_PROPERTY_CUSTOM_START + 11)
#define TEST_PROPERTY_INT32 (VEHICLE_PROPERTY_CUSTOM_START + 12)
#define TEST_PROPERTY_INT32_VEC2 (VEHICLE_PROPERTY_CUSTOM_START + 13)
#define TEST_PROPERTY_INT32_VEC3 (VEHICLE_PROPERTY_CUSTOM_START + 14)
#define TEST_PROPERTY_INT32_VEC4 (VEHICLE_PROPERTY_CUSTOM_START + 15)

#endif /* ANDROID_VECHILE_TEST_PROPERTIES_H */
