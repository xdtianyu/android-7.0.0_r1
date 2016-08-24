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

#ifndef ANDROID_IVEHICLENETWORK_TEST_LISTER_H
#define ANDROID_IVEHICLENETWORK_TEST_LISTER_H

#include <IVehicleNetworkListener.h>

namespace android {

class IVehicleNetworkTestListener : public BnVehicleNetworkListener {
public:
    IVehicleNetworkTestListener() :
        mHalRestartCount(0) {};

    virtual void onEvents(sp<VehiclePropValueListHolder>& events) {
        String8 msg("events ");
        Mutex::Autolock autolock(mLock);
        for (auto& e : events->getList()) {
            ssize_t index = mEventCounts.indexOfKey(e->prop);
            if (index < 0) {
                mEventCounts.add(e->prop, 1); // 1st event
                msg.appendFormat("0x%x:%d ", e->prop, 1);
            } else {
                int count = mEventCounts.valueAt(index);
                count++;
                mEventCounts.replaceValueAt(index, count);
                msg.appendFormat("0x%x:%d ", e->prop, count);
            }
        }
        msg.append("\n");
        std::cout<<msg.string();
        mCondition.signal();
    }

    virtual void onHalError(int32_t errorCode, int32_t property, int32_t operation) {
        Mutex::Autolock autolock(mHalErrorLock);
        mErrorCode = errorCode;
        mProperty = property;
        mOperation = operation;
        mHalErrorCondition.signal();
    }

    virtual void onHalRestart(bool /*inMocking*/) {
        Mutex::Autolock autolock(mHalRestartLock);
        mHalRestartCount++;
        mHalRestartCondition.signal();
    }

    void waitForEvents(nsecs_t reltime) {
        Mutex::Autolock autolock(mLock);
        mCondition.waitRelative(mLock, reltime);
    }

    bool waitForEvent(int32_t property, nsecs_t reltime) {
        Mutex::Autolock autolock(mLock);
        int startCount = getEventCountLocked(property);
        int currentCount = startCount;
        int64_t now = android::elapsedRealtimeNano();
        int64_t endTime = now + reltime;
        while ((startCount == currentCount) && (now < endTime)) {
            mCondition.waitRelative(mLock, endTime - now);
            currentCount = getEventCountLocked(property);
            now = android::elapsedRealtimeNano();
        }
        return (startCount != currentCount);
    }

    int getEventCount(int32_t property) {
        Mutex::Autolock autolock(mLock);
        return getEventCountLocked(property);
    }

    int getHalRestartCount() {
        Mutex::Autolock autolock(mHalRestartLock);
        return mHalRestartCount;
    }

    void waitForHalRestart(nsecs_t reltime) {
        Mutex::Autolock autolock(mHalRestartLock);
        mHalRestartCondition.waitRelative(mHalRestartLock, reltime);
    }

    void waitForHalError(nsecs_t reltime) {
        Mutex::Autolock autolock(mHalErrorLock);
        mHalErrorCondition.waitRelative(mHalErrorLock, reltime);
    }

    bool isErrorMatching(int32_t errorCode, int32_t property, int32_t operation) {
        Mutex::Autolock autolock(mHalErrorLock);
        return mErrorCode == errorCode && mProperty == property && mOperation == operation;
    }

private:
    int getEventCountLocked(int32_t property) {
        ssize_t index = mEventCounts.indexOfKey(property);
        if (index < 0) {
            return 0;
        } else {
            return mEventCounts.valueAt(index);
        }
    }


private:
    Mutex mLock;
    Condition mCondition;
    KeyedVector<int32_t, int> mEventCounts;

    Mutex mHalRestartLock;
    Condition mHalRestartCondition;
    int mHalRestartCount;

    Mutex mHalErrorLock;
    Condition mHalErrorCondition;
    int32_t mErrorCode;
    int32_t mProperty;
    int32_t mOperation;
};

}; // namespace android
#endif // ANDROID_IVEHICLENETWORK_TEST_LISTER_H
