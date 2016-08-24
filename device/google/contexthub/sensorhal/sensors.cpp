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

#define LOG_TAG "sensors"
// #defined LOG_NDEBUG  1
#include <utils/Log.h>

#include "hubconnection.h"
#include "sensorlist.h"
#include "sensors.h"

#include <errno.h>
#include <math.h>
#include <media/stagefright/foundation/ADebug.h>
#include <string.h>

using namespace android;

////////////////////////////////////////////////////////////////////////////////

SensorContext::SensorContext(const struct hw_module_t *module)
    : mHubConnection(HubConnection::getInstance()), mHubAlive(true) {
    memset(&device, 0, sizeof(device));

    device.common.tag = HARDWARE_DEVICE_TAG;
    device.common.version = SENSORS_DEVICE_API_VERSION_1_3;
    device.common.module = const_cast<hw_module_t *>(module);
    device.common.close = CloseWrapper;
    device.activate = ActivateWrapper;
    device.setDelay = SetDelayWrapper;
    device.poll = PollWrapper;
    device.batch = BatchWrapper;
    device.flush = FlushWrapper;

    mHubAlive = (mHubConnection->initCheck() == OK
        && mHubConnection->getAliveCheck() == OK);
}

int SensorContext::close() {
    ALOGI("close");

    delete this;

    return 0;
}

int SensorContext::activate(int handle, int enabled) {
    ALOGI("activate");

    mHubConnection->queueActivate(handle, enabled);

    return 0;
}

int SensorContext::setDelay(int handle, int64_t delayNs) {
    ALOGI("setDelay");

    // clamp sample rate based on minDelay and maxDelay defined in kSensorList
    int64_t delayNsClamped = delayNs;
    for (size_t i = 0; i < kSensorCount; i++) {
        sensor_t sensor = kSensorList[i];
        if (sensor.handle != handle) {
            continue;
        }

        if ((sensor.flags & REPORTING_MODE_MASK) == SENSOR_FLAG_CONTINUOUS_MODE) {
            if ((delayNs/1000) < sensor.minDelay) {
                delayNsClamped = sensor.minDelay * 1000;
            } else if ((delayNs/1000) > sensor.maxDelay) {
                delayNsClamped = sensor.maxDelay * 1000;
            }
        }

        break;
    }

    mHubConnection->queueSetDelay(handle, delayNsClamped);

    return 0;
}

int SensorContext::poll(sensors_event_t *data, int count) {
    ALOGV("poll");

    ssize_t n = mHubConnection->read(data, count);

    if (n < 0) {
        return -1;
    }

    return n;
}

int SensorContext::batch(
        int handle,
        int flags,
        int64_t sampling_period_ns,
        int64_t max_report_latency_ns) {
    ALOGI("batch");

    // clamp sample rate based on minDelay and maxDelay defined in kSensorList
    int64_t sampling_period_ns_clamped = sampling_period_ns;
    for (size_t i = 0; i < kSensorCount; i++) {
        sensor_t sensor = kSensorList[i];
        if (sensor.handle != handle) {
            continue;
        }

        if ((sensor.flags & REPORTING_MODE_MASK) == SENSOR_FLAG_CONTINUOUS_MODE) {
            if ((sampling_period_ns/1000) < sensor.minDelay) {
                sampling_period_ns_clamped = sensor.minDelay * 1000;
            } else if ((sampling_period_ns/1000) > sensor.maxDelay) {
                sampling_period_ns_clamped = sensor.maxDelay * 1000;
            }
        }

        break;
    }

    mHubConnection->queueBatch(
            handle, flags, sampling_period_ns_clamped, max_report_latency_ns);

    return 0;
}

int SensorContext::flush(int handle) {
    ALOGI("flush");

    mHubConnection->queueFlush(handle);
    return 0;
}

// static
int SensorContext::CloseWrapper(struct hw_device_t *dev) {
    return reinterpret_cast<SensorContext *>(dev)->close();
}

// static
int SensorContext::ActivateWrapper(
        struct sensors_poll_device_t *dev, int handle, int enabled) {
    return reinterpret_cast<SensorContext *>(dev)->activate(handle, enabled);
}

// static
int SensorContext::SetDelayWrapper(
        struct sensors_poll_device_t *dev, int handle, int64_t delayNs) {
    return reinterpret_cast<SensorContext *>(dev)->setDelay(handle, delayNs);
}

// static
int SensorContext::PollWrapper(
        struct sensors_poll_device_t *dev, sensors_event_t *data, int count) {
    return reinterpret_cast<SensorContext *>(dev)->poll(data, count);
}

// static
int SensorContext::BatchWrapper(
        struct sensors_poll_device_1 *dev,
        int handle,
        int flags,
        int64_t sampling_period_ns,
        int64_t max_report_latency_ns) {
    return reinterpret_cast<SensorContext *>(dev)->batch(
            handle, flags, sampling_period_ns, max_report_latency_ns);
}

// static
int SensorContext::FlushWrapper(struct sensors_poll_device_1 *dev, int handle) {
    return reinterpret_cast<SensorContext *>(dev)->flush(handle);
}

bool SensorContext::getHubAlive() {
    return mHubAlive;
}

////////////////////////////////////////////////////////////////////////////////

static bool gHubAlive;

static int open_sensors(
        const struct hw_module_t *module,
        const char *,
        struct hw_device_t **dev) {
    ALOGI("open_sensors");

    SensorContext *ctx = new SensorContext(module);

    gHubAlive = ctx->getHubAlive();
    *dev = &ctx->device.common;

    return 0;
}

static struct hw_module_methods_t sensors_module_methods = {
    .open = open_sensors
};

static int get_sensors_list(
        struct sensors_module_t *,
        struct sensor_t const **list) {
    ALOGI("get_sensors_list");

    if (gHubAlive) {
        *list = kSensorList;
        return kSensorCount;
    } else {
        *list = {};
        return 0;
    }
}

static int set_operation_mode(unsigned int mode) {
    ALOGI("set_operation_mode");
    return (mode) ? -EINVAL : 0;
}

struct sensors_module_t HAL_MODULE_INFO_SYM = {
        .common = {
                .tag = HARDWARE_MODULE_TAG,
                .version_major = 1,
                .version_minor = 0,
                .id = SENSORS_HARDWARE_MODULE_ID,
                .name = "Google Sensor module",
                .author = "Google",
                .methods = &sensors_module_methods,
                .dso  = NULL,
                .reserved = {0},
        },
        .get_sensors_list = get_sensors_list,
        .set_operation_mode = set_operation_mode,
};
