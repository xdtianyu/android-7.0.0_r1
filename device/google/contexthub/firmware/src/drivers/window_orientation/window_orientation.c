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

#include <stdlib.h>
#include <string.h>
#include <timer.h>
#include <heap.h>
#include <plat/inc/rtc.h>
#include <plat/inc/syscfg.h>
#include <hostIntf.h>
#include <nanohubPacket.h>
#include <floatRt.h>

#include <seos.h>

#include <nanohub_math.h>
#include <sensors.h>
#include <limits.h>

#define ACCEL_MIN_RATE_HZ                  SENSOR_HZ(15) // 15 HZ
#define ACCEL_MAX_LATENCY_NS               40000000ull   // 40 ms in nsec

// all time units in usec, angles in degrees
#define RADIANS_TO_DEGREES                              (180.0f / M_PI)

#define NS2US(x) (x >> 10)   // convert nsec to approx usec

#define PROPOSAL_SETTLE_TIME                            NS2US(40000000ull)       // 40 ms
#define PROPOSAL_MIN_TIME_SINCE_FLAT_ENDED              NS2US(500000000ull)      // 500 ms
#define PROPOSAL_MIN_TIME_SINCE_SWING_ENDED             NS2US(300000000ull)      // 300 ms
#define PROPOSAL_MIN_TIME_SINCE_ACCELERATION_ENDED      NS2US(500000000ull)      // 500 ms

#define FLAT_ANGLE                      80
#define FLAT_TIME                       NS2US(1000000000ull)     // 1 sec

#define SWING_AWAY_ANGLE_DELTA          20
#define SWING_TIME                      NS2US(300000000ull)      // 300 ms

#define MAX_FILTER_DELTA_TIME           NS2US(1000000000ull)     // 1 sec
#define FILTER_TIME_CONSTANT            NS2US(200000000ull)      // 200 ms

#define NEAR_ZERO_MAGNITUDE             1.0f        // m/s^2
#define ACCELERATION_TOLERANCE          4.0f
#define STANDARD_GRAVITY                9.8f
#define MIN_ACCELERATION_MAGNITUDE  (STANDARD_GRAVITY - ACCELERATION_TOLERANCE)
#define MAX_ACCELERATION_MAGNITUDE  (STANDARD_GRAVITY + ACCELERATION_TOLERANCE)

#define MAX_TILT                        80
#define TILT_OVERHEAD_ENTER             -40
#define TILT_OVERHEAD_EXIT              -15

#define ADJACENT_ORIENTATION_ANGLE_GAP  45

#define TILT_HISTORY_SIZE               200
#define TILT_REFERENCE_PERIOD           NS2US(1800000000000ull)  // 30 min
#define TILT_REFERENCE_BACKOFF          NS2US(300000000000ull)   // 5 min

#define MIN_ACCEL_INTERVAL              NS2US(33333333ull)       // 33.3 ms for 30 Hz

#define EVT_SENSOR_ACC_DATA_RDY sensorGetMyEventType(SENS_TYPE_ACCEL)
#define EVT_SENSOR_WIN_ORIENTATION_DATA_RDY sensorGetMyEventType(SENS_TYPE_WIN_ORIENTATION)

static int8_t Tilt_Tolerance[4][2] = {
    /* ROTATION_0   */ { -25, 70 },
    /* ROTATION_90  */ { -25, 65 },
    /* ROTATION_180 */ { -25, 60 },
    /* ROTATION_270 */ { -25, 65 }
};

struct WindowOrientationTask {
    uint32_t tid;
    uint32_t handle;
    uint32_t accelHandle;

    uint64_t last_filtered_time;
    struct TripleAxisDataPoint last_filtered_sample;

    uint64_t tilt_reference_time;
    uint64_t accelerating_time;
    uint64_t predicted_rotation_time;
    uint64_t flat_time;
    uint64_t swinging_time;

    uint32_t tilt_history_time[TILT_HISTORY_SIZE];
    int tilt_history_index;
    int8_t tilt_history[TILT_HISTORY_SIZE];

    int8_t current_rotation;
    int8_t prev_valid_rotation;
    int8_t proposed_rotation;
    int8_t predicted_rotation;

    bool flat;
    bool swinging;
    bool accelerating;
    bool overhead;
};

static struct WindowOrientationTask mTask;

static const struct SensorInfo mSi =
{
    .sensorName = "Window Orientation",
    .sensorType = SENS_TYPE_WIN_ORIENTATION,
    .numAxis = NUM_AXIS_EMBEDDED,
    .interrupt = NANOHUB_INT_NONWAKEUP,
    .minSamples = 20
};

static bool isTiltAngleAcceptable(int rotation, int8_t tilt_angle)
{
    return ((tilt_angle >= Tilt_Tolerance[rotation][0])
                && (tilt_angle <= Tilt_Tolerance[rotation][1]));
}

static bool isOrientationAngleAcceptable(int current_rotation, int rotation,
                                            int orientation_angle)
{
    // If there is no current rotation, then there is no gap.
    // The gap is used only to introduce hysteresis among advertised orientation
    // changes to avoid flapping.
    int lower_bound, upper_bound;

    if (current_rotation >= 0) {
        // If the specified rotation is the same or is counter-clockwise
        // adjacent to the current rotation, then we set a lower bound on the
        // orientation angle.
        // For example, if currentRotation is ROTATION_0 and proposed is
        // ROTATION_90, then we want to check orientationAngle > 45 + GAP / 2.
        if ((rotation == current_rotation)
                || (rotation == (current_rotation + 1) % 4)) {
            lower_bound = rotation * 90 - 45
                    + ADJACENT_ORIENTATION_ANGLE_GAP / 2;
            if (rotation == 0) {
                if ((orientation_angle >= 315)
                        && (orientation_angle < lower_bound + 360)) {
                    return false;
                }
            } else {
                if (orientation_angle < lower_bound) {
                    return false;
                }
            }
        }

        // If the specified rotation is the same or is clockwise adjacent,
        // then we set an upper bound on the orientation angle.
        // For example, if currentRotation is ROTATION_0 and rotation is
        // ROTATION_270, then we want to check orientationAngle < 315 - GAP / 2.
        if ((rotation == current_rotation)
                || (rotation == (current_rotation + 3) % 4)) {
            upper_bound = rotation * 90 + 45
                    - ADJACENT_ORIENTATION_ANGLE_GAP / 2;
            if (rotation == 0) {
                if ((orientation_angle <= 45)
                        && (orientation_angle > upper_bound)) {
                    return false;
                }
            } else {
                if (orientation_angle > upper_bound) {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool isPredictedRotationAcceptable(uint64_t now)
{
    // The predicted rotation must have settled long enough.
    if (now < mTask.predicted_rotation_time + PROPOSAL_SETTLE_TIME) {
        return false;
    }

    // The last flat state (time since picked up) must have been sufficiently
    // long ago.
    if (now < mTask.flat_time + PROPOSAL_MIN_TIME_SINCE_FLAT_ENDED) {
        return false;
    }

    // The last swing state (time since last movement to put down) must have
    // been sufficiently long ago.
    if (now < mTask.swinging_time + PROPOSAL_MIN_TIME_SINCE_SWING_ENDED) {
        return false;
    }

    // The last acceleration state must have been sufficiently long ago.
    if (now < mTask.accelerating_time
            + PROPOSAL_MIN_TIME_SINCE_ACCELERATION_ENDED) {
        return false;
    }

    // Looks good!
    return true;
}

static void clearPredictedRotation()
{
    mTask.predicted_rotation = -1;
    mTask.predicted_rotation_time = 0;
}

static void clearTiltHistory()
{
    mTask.tilt_history_time[0] = 0;
    mTask.tilt_history_index = 1;
    mTask.tilt_reference_time = 0;
}

static void reset()
{
    mTask.last_filtered_time = 0;
    mTask.proposed_rotation = -1;

    mTask.flat_time = 0;
    mTask.flat = false;

    mTask.swinging_time = 0;
    mTask.swinging = false;

    mTask.accelerating_time = 0;
    mTask.accelerating = false;

    mTask.overhead = false;

    clearPredictedRotation();
    clearTiltHistory();
}

static void updatePredictedRotation(uint64_t now, int rotation)
{
    if (mTask.predicted_rotation != rotation) {
        mTask.predicted_rotation = rotation;
        mTask.predicted_rotation_time = now;
    }
}

static bool isAccelerating(float magnitude)
{
    return ((magnitude < MIN_ACCELERATION_MAGNITUDE)
                || (magnitude > MAX_ACCELERATION_MAGNITUDE));
}

static void addTiltHistoryEntry(uint64_t now, int8_t tilt)
{
    uint64_t old_reference_time, delta;
    size_t i;
    int index;

    if (mTask.tilt_reference_time == 0) {
        // set reference_time after reset()

        mTask.tilt_reference_time = now - 1;
    } else if (mTask.tilt_reference_time + TILT_REFERENCE_PERIOD < now) {
        // uint32_t tilt_history_time[] is good up to 71 min (2^32 * 1e-6 sec).
        // proactively shift reference_time every 30 min,
        // all history entries are within 5 min interval (15Hz x 200 samples)

        old_reference_time = mTask.tilt_reference_time;
        mTask.tilt_reference_time = now - TILT_REFERENCE_BACKOFF;

        delta = mTask.tilt_reference_time - old_reference_time;
        for (i = 0; i < TILT_HISTORY_SIZE; ++i) {
            mTask.tilt_history_time[i] = (mTask.tilt_history_time[i] > delta)
                ? (mTask.tilt_history_time[i] - delta) : 0;
        }
    }

    index = mTask.tilt_history_index;
    mTask.tilt_history[index] = tilt;
    mTask.tilt_history_time[index] = now - mTask.tilt_reference_time;

    index = ((index + 1) == TILT_HISTORY_SIZE) ? 0 : (index + 1);
    mTask.tilt_history_index = index;
    mTask.tilt_history_time[index] = 0;
}

static int nextTiltHistoryIndex(int index)
{
    int next = (index == 0) ? (TILT_HISTORY_SIZE - 1): (index - 1);
    return ((mTask.tilt_history_time[next] != 0) ? next : -1);
}

static bool isFlat(uint64_t now)
{
    int i = mTask.tilt_history_index;
    for (; (i = nextTiltHistoryIndex(i)) >= 0;) {
        if (mTask.tilt_history[i] < FLAT_ANGLE) {
            break;
        }
        if (mTask.tilt_reference_time + mTask.tilt_history_time[i] + FLAT_TIME <= now) {
            // Tilt has remained greater than FLAT_ANGLE for FLAT_TIME.
            return true;
        }
    }
    return false;
}

static bool isSwinging(uint64_t now, int8_t tilt)
{
    int i = mTask.tilt_history_index;
    for (; (i = nextTiltHistoryIndex(i)) >= 0;) {
        if (mTask.tilt_reference_time + mTask.tilt_history_time[i] + SWING_TIME
                < now) {
            break;
        }
        if (mTask.tilt_history[i] + SWING_AWAY_ANGLE_DELTA <= tilt) {
            // Tilted away by SWING_AWAY_ANGLE_DELTA within SWING_TIME.
            return true;
        }
    }
    return false;
}

static bool add_samples(struct TripleAxisDataEvent *ev)
{
    int i, tilt_tmp;
    int orientation_angle, nearest_rotation;
    float x, y, z, alpha, magnitude;
    uint64_t now_nsec = ev->referenceTime, now;
    uint64_t then, time_delta;
    struct TripleAxisDataPoint *last_sample;
    size_t sampleCnt = ev->samples[0].firstSample.numSamples;
    bool skip_sample;
    bool accelerating, flat, swinging;
    bool change_detected;
    int8_t old_proposed_rotation, proposed_rotation;
    int8_t tilt_angle;

    for (i = 0; i < sampleCnt; i++) {

        x = ev->samples[i].x;
        y = ev->samples[i].y;
        z = ev->samples[i].z;

        // Apply a low-pass filter to the acceleration up vector in cartesian space.
        // Reset the orientation listener state if the samples are too far apart in time.

        now_nsec += i > 0 ? ev->samples[i].deltaTime : 0;
        now = NS2US(now_nsec); // convert to ~usec

        last_sample = &mTask.last_filtered_sample;
        then = mTask.last_filtered_time;
        time_delta = now - then;

        if ((now < then) || (now > then + MAX_FILTER_DELTA_TIME)) {
            reset();
            skip_sample = true;
        } else {
            // alpha is the weight on the new sample
            alpha = floatFromUint64(time_delta) / floatFromUint64(FILTER_TIME_CONSTANT + time_delta);
            x = alpha * (x - last_sample->x) + last_sample->x;
            y = alpha * (y - last_sample->y) + last_sample->y;
            z = alpha * (z - last_sample->z) + last_sample->z;

            skip_sample = false;
        }

        // drop samples when input sampling rate is 2x higher than requested
        if (!skip_sample && (time_delta < MIN_ACCEL_INTERVAL)) {
            skip_sample = true;
        } else {
            mTask.last_filtered_time = now;
            mTask.last_filtered_sample.x = x;
            mTask.last_filtered_sample.y = y;
            mTask.last_filtered_sample.z = z;
        }

        accelerating = false;
        flat = false;
        swinging = false;

        if (!skip_sample) {
            // Calculate the magnitude of the acceleration vector.
            magnitude = sqrtf(x * x + y * y + z * z);

            if (magnitude < NEAR_ZERO_MAGNITUDE) {
                clearPredictedRotation();
            } else {
                // Determine whether the device appears to be undergoing
                // external acceleration.
                if (isAccelerating(magnitude)) {
                    accelerating = true;
                    mTask.accelerating_time = now;
                }

                // Calculate the tilt angle.
                // This is the angle between the up vector and the x-y plane
                // (the plane of the screen) in a range of [-90, 90] degrees.
                //  -90 degrees: screen horizontal and facing the ground (overhead)
                //    0 degrees: screen vertical
                //   90 degrees: screen horizontal and facing the sky (on table)
                tilt_tmp = (int)(asinf(z / magnitude) * RADIANS_TO_DEGREES);
                tilt_tmp = (tilt_tmp > 127) ? 127 : tilt_tmp;
                tilt_tmp = (tilt_tmp < -128) ? -128 : tilt_tmp;
                tilt_angle = tilt_tmp;
                addTiltHistoryEntry(now, tilt_angle);

                // Determine whether the device appears to be flat or swinging.
                if (isFlat(now)) {
                    flat = true;
                    mTask.flat_time = now;
                }
                if (isSwinging(now, tilt_angle)) {
                    swinging = true;
                    mTask.swinging_time = now;
                }

                // If the tilt angle is too close to horizontal then we cannot
                // determine the orientation angle of the screen.
                if (tilt_angle <= TILT_OVERHEAD_ENTER) {
                    mTask.overhead = true;
                } else if (tilt_angle >= TILT_OVERHEAD_EXIT) {
                    mTask.overhead = false;
                }

                if (mTask.overhead) {
                    clearPredictedRotation();
                } else if (fabsf(tilt_angle) > MAX_TILT) {
                    clearPredictedRotation();
                } else {
                    // Calculate the orientation angle.
                    // This is the angle between the x-y projection of the up
                    // vector onto the +y-axis, increasing clockwise in a range
                    // of [0, 360] degrees.
                    orientation_angle = (int)(-atan2f(-x, y) * RADIANS_TO_DEGREES);
                    if (orientation_angle < 0) {
                        // atan2 returns [-180, 180]; normalize to [0, 360]
                        orientation_angle += 360;
                    }

                    // Find the nearest rotation.
                    nearest_rotation = (orientation_angle + 45) / 90;
                    if (nearest_rotation == 4) {
                        nearest_rotation = 0;
                    }
                    // Determine the predicted orientation.
                    if (isTiltAngleAcceptable(nearest_rotation, tilt_angle)
                        && isOrientationAngleAcceptable(mTask.current_rotation,
                                                           nearest_rotation,
                                                           orientation_angle)) {
                        updatePredictedRotation(now, nearest_rotation);
                    } else {
                        clearPredictedRotation();
                    }
                }
            }
        }

        mTask.flat = flat;
        mTask.swinging = swinging;
        mTask.accelerating = accelerating;

        // Determine new proposed rotation.
        old_proposed_rotation = mTask.proposed_rotation;
        if ((mTask.predicted_rotation < 0)
                || isPredictedRotationAcceptable(now)) {

            mTask.proposed_rotation = mTask.predicted_rotation;
        }
        proposed_rotation = mTask.proposed_rotation;

        if ((proposed_rotation != old_proposed_rotation)
                && (proposed_rotation >= 0)) {
            mTask.current_rotation = proposed_rotation;

            change_detected = (proposed_rotation != mTask.prev_valid_rotation);
            mTask.prev_valid_rotation = proposed_rotation;

            if (change_detected) {
                return true;
            }
        }
    }

    return false;
}


static bool windowOrientationPower(bool on, void *cookie)
{
    if (on == false && mTask.accelHandle != 0) {
        sensorRelease(mTask.tid, mTask.accelHandle);
        mTask.accelHandle = 0;
        osEventUnsubscribe(mTask.tid, EVT_SENSOR_ACC_DATA_RDY);
    }

    sensorSignalInternalEvt(mTask.handle, SENSOR_INTERNAL_EVT_POWER_STATE_CHG, on, 0);

    return true;
}

static bool windowOrientationSetRate(uint32_t rate, uint64_t latency, void *cookie)
{
    int i;

    if (mTask.accelHandle == 0) {
        for (i = 0; sensorFind(SENS_TYPE_ACCEL, i, &mTask.accelHandle) != NULL; i++) {
            if (sensorRequest(mTask.tid, mTask.accelHandle, ACCEL_MIN_RATE_HZ, ACCEL_MAX_LATENCY_NS)) {
                // clear hysteresis
                mTask.current_rotation = -1;
                mTask.prev_valid_rotation = -1;
                reset();
                osEventSubscribe(mTask.tid, EVT_SENSOR_ACC_DATA_RDY);
                break;
            }
        }
    }

    if (mTask.accelHandle != 0)
        sensorSignalInternalEvt(mTask.handle, SENSOR_INTERNAL_EVT_RATE_CHG, rate, latency);

    return true;
}

static bool windowOrientationFirmwareUpload(void *cookie)
{
    sensorSignalInternalEvt(mTask.handle, SENSOR_INTERNAL_EVT_FW_STATE_CHG,
            1, 0);
    return true;
}

static bool windowOrientationFlush(void *cookie)
{
    return osEnqueueEvt(sensorGetMyEventType(SENS_TYPE_WIN_ORIENTATION), SENSOR_DATA_EVENT_FLUSH, NULL);
}

static void windowOrientationHandleEvent(uint32_t evtType, const void* evtData)
{
    struct TripleAxisDataEvent *ev;
    union EmbeddedDataPoint sample;
    bool rotation_changed;

    if (evtData == SENSOR_DATA_EVENT_FLUSH)
        return;

    switch (evtType) {
    case EVT_SENSOR_ACC_DATA_RDY:
        ev = (struct TripleAxisDataEvent *)evtData;
        rotation_changed = add_samples(ev);

        if (rotation_changed) {
            //osLog(LOG_INFO, "WO:     ********** rotation changed to ********: %d\n", (int)mTask.proposed_rotation);

            // send a single int32 here so no memory alloc/free needed.
            sample.idata = mTask.proposed_rotation;
            osEnqueueEvt(EVT_SENSOR_WIN_ORIENTATION_DATA_RDY, sample.vptr, NULL);
        }
        break;
    }
}

static const struct SensorOps mSops =
{
    .sensorPower = windowOrientationPower,
    .sensorFirmwareUpload = windowOrientationFirmwareUpload,
    .sensorSetRate = windowOrientationSetRate,
    .sensorFlush = windowOrientationFlush,
};

static bool window_orientation_start(uint32_t tid)
{
    osLog(LOG_INFO, "        WINDOW ORIENTATION:  %ld\n", tid);

    mTask.tid = tid;

    mTask.current_rotation = -1;
    mTask.prev_valid_rotation = -1;
    reset();

    mTask.handle = sensorRegister(&mSi, &mSops, NULL, true);

    return true;
}

static void windowOrientationEnd()
{
}

INTERNAL_APP_INIT(
        APP_ID_MAKE(APP_ID_VENDOR_GOOGLE, 3),
        0,
        window_orientation_start,
        windowOrientationEnd,
        windowOrientationHandleEvent);

