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

#ifndef HUB_DEFS_H_
#define HUB_DEFS_H_

#include <hardware/sensors.h>

#define MAX_SPI_PAYLOAD_SIZE            256

namespace android {

#define CONTEXTHUB_SETTINGS_PATH        "/persist/sensorcal.json"
#define CONTEXTHUB_SAVED_SETTINGS_PATH  "/data/misc/sensorcal_saved.json"
#define MAG_BIAS_FILE_PATH              "/sys/class/power_supply/battery/compass_compensation"

static const uint32_t kMinClockRateHz = 960000;
static const uint32_t kClockRateHz = kMinClockRateHz * 5;  // 4.8MHz

enum comms_sensor_t {
    COMMS_SENSOR_INVALID                = 0,
    COMMS_SENSOR_ACCEL                  = 1,
    COMMS_SENSOR_GYRO                   = 2,
    COMMS_SENSOR_MAG                    = 3,
    COMMS_SENSOR_PRESSURE               = 4,
    COMMS_SENSOR_TEMPERATURE            = 5,
    COMMS_SENSOR_PROXIMITY              = 6,
    COMMS_SENSOR_LIGHT                  = 7,
    COMMS_SENSOR_ORIENTATION            = 8,
    COMMS_SENSOR_STEP_DETECTOR          = 9,
    COMMS_SENSOR_ANY_MOTION             = 10,
    COMMS_SENSOR_NO_MOTION              = 11,
    COMMS_SENSOR_SIGNIFICANT_MOTION     = 12,
    COMMS_SENSOR_FLAT                   = 13,
    COMMS_SENSOR_ACTIVITY               = 14,
    COMMS_SENSOR_GRAVITY                = 15,
    COMMS_SENSOR_LINEAR_ACCEL           = 16,
    COMMS_SENSOR_ROTATION_VECTOR        = 17,
    COMMS_SENSOR_HALL                   = 18,
    COMMS_SENSOR_GEO_MAG                = 19,
    COMMS_SENSOR_GAME_ROTATION_VECTOR   = 20,
    COMMS_SENSOR_GESTURE                = 21,
    COMMS_SENSOR_TILT                   = 22,
    COMMS_SENSOR_MAG_BIAS               = 23,
    COMMS_SENSOR_STEP_COUNTER           = 24,
    COMMS_SENSOR_MAG_UNCALIBRATED       = 25,
    COMMS_SENSOR_GYRO_UNCALIBRATED      = 26,
    COMMS_SENSOR_GYRO_BIAS              = 27,
    COMMS_SENSOR_SYNC                   = 28,
    COMMS_SENSOR_DOUBLE_TWIST           = 29,
    COMMS_SENSOR_DOUBLE_TAP             = 30,
    COMMS_SENSOR_WINDOW_ORIENTATION     = 31,

    NUM_COMMS_SENSORS_PLUS_1,

    COMMS_SENSOR_DEBUG                  = 0x99,
};

enum {
    SPI_COMMS_CMD_SYNC                  = 0,
    SPI_COMMS_CMD_SWITCH_SENSOR         = 1,
    SPI_COMMS_CMD_ABSOLUTE_TIME         = 2,
    SPI_COMMS_SENSOR_DATA_SCALAR        = 3,
    SPI_COMMS_SENSOR_DATA_VEC3          = 4,
    SPI_COMMS_SENSOR_DATA_VEC4          = 5,
    SPI_COMMS_SENSOR_DATA_FLUSH         = 6,
    SPI_COMMS_CMD_UPDATE_MAG_BIAS       = 7,
    SPI_COMMS_CMD_UPDATE_MAG_ACCURACY   = 8,
    SPI_COMMS_CMD_UPDATE_GYRO_BIAS      = 9,
    SPI_COMMS_CMD_ACK_SUSPEND_STATE     = 10,
    SPI_COMMS_DEBUG_OUTPUT              = 0xff,
};

// Please keep existing values unchanged when adding or removing SENSOR_TYPE
enum {
    SENSOR_TYPE_INTERNAL_TEMPERATURE    = SENSOR_TYPE_DEVICE_PRIVATE_BASE + 0,
    SENSOR_TYPE_SYNC                    = SENSOR_TYPE_DEVICE_PRIVATE_BASE + 1,
    SENSOR_TYPE_DOUBLE_TWIST            = SENSOR_TYPE_DEVICE_PRIVATE_BASE + 2,
    SENSOR_TYPE_DOUBLE_TAP              = SENSOR_TYPE_DEVICE_PRIVATE_BASE + 3,
};

}  // namespace android

#endif  // HUB_DEFS_H_
