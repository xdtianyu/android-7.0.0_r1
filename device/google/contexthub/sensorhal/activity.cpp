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

#define LOG_TAG "ActivityRecognitionHAL"
//#define LOG_NDEBUG  0
#include <utils/Log.h>

#include "activity.h"

#include <media/stagefright/foundation/ADebug.h>

using namespace android;

static const int kVersionMajor = 1;
static const int kVersionMinor = 0;

static const int ACTIVITY_TYPE_TILTING_INDEX = 6;

static const char *const kActivityList[] = {
    ACTIVITY_TYPE_IN_VEHICLE,
    ACTIVITY_TYPE_ON_BICYCLE,
    ACTIVITY_TYPE_WALKING,
    ACTIVITY_TYPE_RUNNING,
    ACTIVITY_TYPE_STILL,
    "com.google.android.contexthub.ar.inconsistent",
    ACTIVITY_TYPE_TILTING
};

ActivityContext::ActivityContext(const struct hw_module_t *module)
    : mHubConnection(HubConnection::getInstance()),
      mHubAlive(true),
      mCallback(NULL),
      mPrevActivity(-1),
      mInitExitDone(false) {
    memset(&device, 0, sizeof(device));

    device.common.tag = HARDWARE_DEVICE_TAG;
    device.common.version = ACTIVITY_RECOGNITION_API_VERSION_0_1;
    device.common.module = const_cast<hw_module_t *>(module);
    device.common.close = CloseWrapper;
    device.register_activity_callback = RegisterActivityCallbackWrapper;
    device.enable_activity_event = EnableActivityEventWrapper;
    device.disable_activity_event = DisableActivityEventWrapper;
    device.flush = FlushWrapper;

    if (mHubConnection->initCheck() != (status_t)OK) {
        mHubAlive = false;
    } else {
        if (mHubConnection->getAliveCheck() != (status_t)OK) {
            mHubAlive = false;
        } else {
            mHubConnection->setActivityCallback(
                    this, &ActivityContext::HubCallbackWrapper);

            mHubConnection->queueActivate(COMMS_SENSOR_ACTIVITY, false /* enable */);
        }
    }
}

ActivityContext::~ActivityContext() {
    mHubConnection->setActivityCallback(NULL, NULL);
}

int ActivityContext::close() {
    ALOGI("close");

    delete this;

    return 0;
}

void ActivityContext::onActivityEvent(
        uint64_t when_us, bool is_flush, float x, float, float) {
    Mutex::Autolock autoLock(mLock);

    if (!mCallback) {
        return;
    }

    if (is_flush) {
        activity_event_t ev;
        memset(&ev, 0, sizeof(ev));

        ev.event_type = ACTIVITY_EVENT_FLUSH_COMPLETE;
        ev.activity = 0;
        ev.timestamp = 0ll;

        (*mCallback->activity_callback)(mCallback, &ev, 1);
        return;
    }

    int activityRaw = (int)x;

    ALOGV("activityRaw = %d", activityRaw);

    if (mPrevActivity >= 0 && mPrevActivity == activityRaw) {
        // same old, same old...
        return;
    }

    activity_event_t ev[8];
    memset(&ev, 0, 8*sizeof(activity_event_t));
    int num_events = 0;

    // exit all other activities when first enabled.
    if (!mInitExitDone) {
        mInitExitDone = true;

        int numActivities = sizeof(kActivityList) / sizeof(kActivityList[0]);
        for (int i = 0; i < numActivities; ++i) {
            if ((i == activityRaw) || !isEnabled(i, ACTIVITY_EVENT_EXIT)) {
                continue;
            }

            activity_event_t *curr_ev = &ev[num_events];
            curr_ev->event_type = ACTIVITY_EVENT_EXIT;
            curr_ev->activity = i;
            curr_ev->timestamp = when_us * 1000ll;  // timestamp is in ns.
            curr_ev->reserved[0] = curr_ev->reserved[1] = curr_ev->reserved[2] = curr_ev->reserved[3] = 0;
            num_events++;
        }
    }

    // tilt activities do not change the current activity type, but have a
    // simultaneous enter and exit event type
    if (activityRaw == ACTIVITY_TYPE_TILTING_INDEX) {
        if (isEnabled(activityRaw, ACTIVITY_EVENT_ENTER)) {
            activity_event_t *curr_ev = &ev[num_events];
            curr_ev->event_type = ACTIVITY_EVENT_ENTER;
            curr_ev->activity = activityRaw;
            curr_ev->timestamp = when_us * 1000ll;  // timestamp is in ns.
            curr_ev->reserved[0] = curr_ev->reserved[1] = curr_ev->reserved[2] = curr_ev->reserved[3] = 0;
            num_events++;
        }

        if (isEnabled(activityRaw, ACTIVITY_EVENT_EXIT)) {
            activity_event_t *curr_ev = &ev[num_events];
            curr_ev->event_type = ACTIVITY_EVENT_EXIT;
            curr_ev->activity = activityRaw;
            curr_ev->timestamp = when_us * 1000ll;  // timestamp is in ns.
            curr_ev->reserved[0] = curr_ev->reserved[1] = curr_ev->reserved[2] = curr_ev->reserved[3] = 0;
            num_events++;
        }
    } else {
        if ((mPrevActivity >= 0) &&
            (isEnabled(mPrevActivity, ACTIVITY_EVENT_EXIT))) {
            activity_event_t *curr_ev = &ev[num_events];
            curr_ev->event_type = ACTIVITY_EVENT_EXIT;
            curr_ev->activity = mPrevActivity;
            curr_ev->timestamp = when_us * 1000ll;  // timestamp is in ns.
            curr_ev->reserved[0] = curr_ev->reserved[1] = curr_ev->reserved[2] = curr_ev->reserved[3] = 0;
            num_events++;
        }

        if (isEnabled(activityRaw, ACTIVITY_EVENT_ENTER)) {
            activity_event_t *curr_ev = &ev[num_events];
            curr_ev->event_type = ACTIVITY_EVENT_ENTER;
            curr_ev->activity = activityRaw;
            curr_ev->timestamp = when_us * 1000ll;  // timestamp is in ns.
            curr_ev->reserved[0] = curr_ev->reserved[1] = curr_ev->reserved[2] = curr_ev->reserved[3] = 0;
            num_events++;
        }

        mPrevActivity = activityRaw;
    }

    if (num_events > 0) {
        (*mCallback->activity_callback)(mCallback, ev, num_events);
    }
}

void ActivityContext::registerActivityCallback(
        const activity_recognition_callback_procs_t *callback) {
    ALOGI("registerActivityCallback");

    Mutex::Autolock autoLock(mLock);
    mCallback = callback;
}

int ActivityContext::enableActivityEvent(
        uint32_t activity_handle,
        uint32_t event_type,
        int64_t max_batch_report_latency_ns) {
    ALOGI("enableActivityEvent");

    bool wasEnabled = !mMaxBatchReportLatencyNs.isEmpty();
    int64_t prev_latency = calculateReportLatencyNs();

    ALOGD_IF(DEBUG_ACTIVITY_RECOGNITION, "ACTVT type = %u, latency = %d sec", (unsigned) event_type,
          (int)(max_batch_report_latency_ns/1000000000ull));

    mMaxBatchReportLatencyNs.add(
            ((uint64_t)activity_handle << 32) | event_type,
            max_batch_report_latency_ns);

    if (!wasEnabled) {
        mPrevActivity = -1;
        mInitExitDone = false;

        mHubConnection->queueBatch(
            COMMS_SENSOR_ACTIVITY, SENSOR_FLAG_ON_CHANGE_MODE, 1000000, max_batch_report_latency_ns);
        mHubConnection->queueActivate(COMMS_SENSOR_ACTIVITY, true /* enable */);
    } else if (max_batch_report_latency_ns != prev_latency) {
        mHubConnection->queueBatch(
            COMMS_SENSOR_ACTIVITY, SENSOR_FLAG_ON_CHANGE_MODE, 1000000, max_batch_report_latency_ns);
    }

    return 0;
}

int64_t ActivityContext::calculateReportLatencyNs() {
    int64_t ret = INT64_MAX;

    for (size_t i = 0 ; i < mMaxBatchReportLatencyNs.size(); ++i) {
        if (mMaxBatchReportLatencyNs[i] <ret) {
            ret = mMaxBatchReportLatencyNs[i];
        }
    }
    return ret;
}

int ActivityContext::disableActivityEvent(
        uint32_t activity_handle, uint32_t event_type) {
    ALOGI("disableActivityEvent");

    bool wasEnabled = !mMaxBatchReportLatencyNs.isEmpty();

    mMaxBatchReportLatencyNs.removeItem(
            ((uint64_t)activity_handle << 32) | event_type);

    bool isEnabled = !mMaxBatchReportLatencyNs.isEmpty();

    if (wasEnabled && !isEnabled) {
        mHubConnection->queueActivate(COMMS_SENSOR_ACTIVITY, false /* enable */);
    }

    return 0;
}

bool ActivityContext::isEnabled(
        uint32_t activity_handle, uint32_t event_type) const {
    return mMaxBatchReportLatencyNs.indexOfKey(
            ((uint64_t)activity_handle << 32) | event_type) >= 0;
}

int ActivityContext::flush() {
    mHubConnection->queueFlush(COMMS_SENSOR_ACTIVITY);
    return 0;
}

// static
int ActivityContext::CloseWrapper(struct hw_device_t *dev) {
    return reinterpret_cast<ActivityContext *>(dev)->close();
}

// static
void ActivityContext::RegisterActivityCallbackWrapper(
        const struct activity_recognition_device *dev,
        const activity_recognition_callback_procs_t *callback) {
    const_cast<ActivityContext *>(
            reinterpret_cast<const ActivityContext *>(dev))
        ->registerActivityCallback(callback);
}

// static
int ActivityContext::EnableActivityEventWrapper(
        const struct activity_recognition_device *dev,
        uint32_t activity_handle,
        uint32_t event_type,
        int64_t max_batch_report_latency_ns) {
    return const_cast<ActivityContext *>(
            reinterpret_cast<const ActivityContext *>(dev))
        ->enableActivityEvent(
            activity_handle, event_type, max_batch_report_latency_ns);
}

// static
int ActivityContext::DisableActivityEventWrapper(
        const struct activity_recognition_device *dev,
        uint32_t activity_handle,
        uint32_t event_type) {
    return const_cast<ActivityContext *>(
            reinterpret_cast<const ActivityContext *>(dev))
        ->disableActivityEvent(activity_handle, event_type);
}

// static
int ActivityContext::FlushWrapper(
        const struct activity_recognition_device *dev) {
    return const_cast<ActivityContext *>(
            reinterpret_cast<const ActivityContext *>(dev))->flush();
}

// static
void ActivityContext::HubCallbackWrapper(
        void *me, uint64_t time_ms, bool is_flush, float x, float y, float z) {
    static_cast<ActivityContext *>(me)->onActivityEvent(time_ms, is_flush, x, y, z);
}

bool ActivityContext::getHubAlive() {
    return mHubAlive;
}

////////////////////////////////////////////////////////////////////////////////

static bool gHubAlive = false;

static int open_activity(
        const struct hw_module_t *module,
        const char *,
        struct hw_device_t **dev) {
    ALOGI("open_activity");

    ActivityContext *ctx = new ActivityContext(module);

    gHubAlive = ctx->getHubAlive();
    *dev = &ctx->device.common;

    return 0;
}

static struct hw_module_methods_t activity_module_methods = {
    .open = open_activity
};

static int get_activity_list(
        struct activity_recognition_module *,
        char const* const **activity_list) {
    ALOGI("get_activity_list");

    if (gHubAlive) {
        *activity_list = kActivityList;
        return sizeof(kActivityList) / sizeof(kActivityList[0]);
    } else {
        *activity_list = {};
        return 0;
    }
}

struct activity_recognition_module HAL_MODULE_INFO_SYM = {
        .common = {
                .tag = HARDWARE_MODULE_TAG,
                .version_major = kVersionMajor,
                .version_minor = kVersionMinor,
                .id = ACTIVITY_RECOGNITION_HARDWARE_MODULE_ID,
                .name = "Google Activity Recognition module",
                .author = "Google",
                .methods = &activity_module_methods,
                .dso  = NULL,
                .reserved = {0},
        },
        .get_supported_activities_list = get_activity_list,
};

