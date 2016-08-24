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

#ifndef ANDROID_VEHICLE_INTERNAL_H
#define ANDROID_VEHICLE_INTERNAL_H

#include <hardware/vehicle.h>

__BEGIN_DECLS

/**
 * Define all internal properties used in VNS. This is not shared with vehicle HAL, but
 * used for internal synchronization / testing purpose.
 */

/**
 * Represents state of audio stream. Audio HAL should set this when a steam is starting or ending.
 * Actual streaming of data should be done only after getting focus for the given stream from
 * car audio module. Focus can be already granted when stream is started. Focus state can be
 * monitored by monitoring VEHICLE_PROPERTY_AUDIO_FOCUS. If car does not support
 * VEHICLE_PROPERTY_AUDIO_FOCUS, there is no need to monitor focus as focus is assumed to be
 * granted always.
 * Data has the following format:
 *   int32_array[0] : vehicle_audio_stream_state
 *   int32_array[1] : stream number
 *
 * @value_type VEHICLE_VALUE_TYPE_INT32_VEC2
 * @change_mode VEHICLE_PROP_CHANGE_MODE_ON_CHANGE
 * @access VEHICLE_PROP_ACCESS_READ_WRITE
 * @data_member int32_array
 */
#define VEHICLE_PROPERTY_INTERNAL_AUDIO_STREAM_STATE      (0x74000000)

enum vehicle_audio_stream_state {
    VEHICLE_AUDIO_STREAM_STATE_STOPPED = 0,
    VEHICLE_AUDIO_STREAM_STATE_STARTED = 1,
};

enum vehicle_audio_stream_state_index {
    VEHICLE_AUDIO_STREAM_STATE_INDEX_STATE = 0,
    VEHICLE_AUDIO_STREAM_STATE_INDEX_STREAM = 1,
};

__END_DECLS

#endif /* ANDROID_VEHICLE_INTERNAL_H */
