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

#include "android/net/UidRange.h"

#define LOG_TAG "UidRange"

#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <log/log.h>
#include <utils/Errors.h>

using android::BAD_VALUE;
using android::NO_ERROR;
using android::Parcel;
using android::status_t;

namespace android {

namespace net {

UidRange::UidRange(int32_t start, int32_t stop) {
    ALOG_ASSERT(start <= stop, "start UID must be less than or equal to stop UID");
    mStart = start;
    mStop = stop;
}

status_t UidRange::writeToParcel(Parcel* parcel) const {
    /*
     * Keep implementation in sync with writeToParcel() in
     * frameworks/base/core/java/android/net/UidRange.java.
     */
    if (status_t err = parcel->writeInt32(mStart)) {
        return err;
    }
    if (status_t err = parcel->writeInt32(mStop)) {
        return err;
    }
    return NO_ERROR;
}

status_t UidRange::readFromParcel(const Parcel* parcel) {
    /*
     * Keep implementation in sync with readFromParcel() in
     * frameworks/base/core/java/android/net/UidRange.java.
     */
    if (status_t err = parcel->readInt32(&mStart)) {
        return err;
    }
    if (status_t err = parcel->readInt32(&mStop)) {
        return err;
    }
    if (mStart > mStop) return BAD_VALUE;
    return NO_ERROR;
}

void UidRange::setStart(int32_t uid) {
    if (mStop != -1) {
        ALOG_ASSERT(uid <= mStop, "start UID must be less than or equal to stop UID");
    }
    mStart = uid;
}

void UidRange::setStop(int32_t uid) {
    if (mStart != -1) {
        ALOG_ASSERT(uid <= mStop, "stop UID must be greater than or equal to start UID");
    }
    mStop = uid;
}

int32_t UidRange::getStart() const {
    return mStart;
}

int32_t UidRange::getStop() const {
    return mStop;
}

}  // namespace net

}  // namespace android
