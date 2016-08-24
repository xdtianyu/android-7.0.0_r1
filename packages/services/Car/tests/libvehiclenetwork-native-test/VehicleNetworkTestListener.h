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

#ifndef ANDROID_VEHICLENETWORK_TEST_LISTER_H
#define ANDROID_VEHICLENETWORK_TEST_LISTER_H

#include <IVehicleNetworkListener.h>
#include <VehicleNetworkDataTypes.h>

namespace android {

class VehicleNetworkTestListener : public VehicleNetworkListener {
public:
    VehicleNetworkTestListener()
    : mEvents(new VehiclePropValueListHolder(new List<vehicle_prop_value_t* >())) {
        String8 msg;
        msg.appendFormat("Creating VehicleNetworkTestListener 0x%p\n", this);
        std::cout<<msg.string();
    }

    virtual ~VehicleNetworkTestListener() {
        std::cout<<"destroying VehicleNetworkTestListener\n";
        for (auto& e : mEvents->getList()) {
            VehiclePropValueUtil::deleteMembers(e);
            delete e;
        }
    }

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
            vehicle_prop_value_t* copy = VehiclePropValueUtil::allocVehicleProp(*e);
            mEvents->getList().push_back(copy);
        }
        msg.append("\n");
        std::cout<<msg.string();
        mCondition.signal();
    }

    virtual void onHalError(int32_t /*errorCode*/, int32_t /*property*/, int32_t /*operation*/) {
        //TODO
    }

    virtual void onHalRestart(bool /*inMocking*/) {
        //TODO cannot test this in native world without plumbing mocking
    }

    void waitForEvents(nsecs_t reltime) {
        Mutex::Autolock autolock(mLock);
        mCondition.waitRelative(mLock, reltime);
    }

    bool waitForEvent(int32_t property, int initialEventCount, nsecs_t reltime) {
        Mutex::Autolock autolock(mLock);
        int currentCount = initialEventCount;
        int64_t now = android::elapsedRealtimeNano();
        int64_t endTime = now + reltime;
        while ((initialEventCount == currentCount) && (now < endTime)) {
            mCondition.waitRelative(mLock, endTime - now);
            currentCount = getEventCountLocked(property);
            now = android::elapsedRealtimeNano();
        }
        return (initialEventCount != currentCount);
    }

    int getEventCount(int32_t property) {
        Mutex::Autolock autolock(mLock);
        return getEventCountLocked(property);
    }

    sp<VehiclePropValueListHolder>& getEvents() {
        // this is nothing more than memory barrier. Just for testing.
        Mutex::Autolock autolock(mLock);
        return mEvents;
    }

    const vehicle_prop_value& getLastValue() {
        auto itr = getEvents()->getList().end();
        itr--;
        return **itr;
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
    sp<VehiclePropValueListHolder> mEvents;
};

}; // namespace android
#endif // ANDROID_VEHICLENETWORK_TEST_LISTER_H
