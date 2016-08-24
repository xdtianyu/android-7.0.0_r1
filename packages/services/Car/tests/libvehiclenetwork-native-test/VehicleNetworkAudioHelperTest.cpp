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
#define LOG_TAG "VehicleNetworkAudioHelperTest"

#include <unistd.h>

#include <gtest/gtest.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/threads.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <utils/SystemClock.h>
#include <vehicle-internal.h>
#include <VehicleNetwork.h>
#include <VehicleNetworkAudioHelper.h>

#include "TestProperties.h"
#include "VehicleHalMock.h"
#include "VehicleNetworkTestListener.h"

namespace android {

extern "C" {
vehicle_prop_config_t const * getTestPropertiesForAudio();
int getNumTestPropertiesForAudio();
};

class VehicleHalMockForAudioFocus : public VehicleHalMock {
public:
    VehicleHalMockForAudioFocus(sp<VehicleNetwork>& vn)
        : mVN(vn) {
        mAudioProperties = new VehiclePropertiesHolder(false /* deleteConfigsInDestructor */);
        vehicle_prop_config_t const * properties = getTestPropertiesForAudio();
        for (int i = 0; i < getNumTestPropertiesForAudio(); i++) {
            mAudioProperties->getList().push_back(properties + i);
        }
        mValueToGet.prop = VEHICLE_PROPERTY_AUDIO_FOCUS;
        mValueToGet.value_type = VEHICLE_VALUE_TYPE_INT32_VEC4;
        mValueToGet.value.int32_array[0] = 0;
        mValueToGet.value.int32_array[1] = 0;
        mValueToGet.value.int32_array[2] = 0;
        mValueToGet.value.int32_array[3] = 0;
    }
    virtual ~VehicleHalMockForAudioFocus() {};

    virtual sp<VehiclePropertiesHolder> onListProperties() {
        ALOGI("onListProperties");
        Mutex::Autolock autoLock(mLock);
        return mAudioProperties;
    };

    virtual status_t onPropertySet(const vehicle_prop_value_t& value) {
        ALOGI("onPropertySet 0x%x", value.prop);
        return NO_ERROR;
    };

    virtual status_t onPropertyGet(vehicle_prop_value_t* value) {
        ALOGI("onPropertyGet 0x%x", value->prop);
        Mutex::Autolock autoLock(mLock);
        if (value->prop == VEHICLE_PROPERTY_AUDIO_FOCUS) {
            memcpy(value, &mValueToGet, sizeof(vehicle_prop_value_t));
        }
        return NO_ERROR;
    };

    virtual status_t onPropertySubscribe(int32_t property, float /*sampleRate*/,
            int32_t /*zones*/) {
        ALOGI("onPropertySubscribe 0x%x", property);
        return NO_ERROR;
    };

    virtual void onPropertyUnsubscribe(int32_t property) {
        ALOGI("onPropertySubscribe 0x%x", property);
    };

    void setFocusState(int32_t state, int32_t streams, int32_t extState) {
        Mutex::Autolock autoLock(mLock);
        mValueToGet.value.int32_array[VEHICLE_AUDIO_FOCUS_INDEX_FOCUS] = state;
        mValueToGet.value.int32_array[VEHICLE_AUDIO_FOCUS_INDEX_STREAMS] = streams;
        mValueToGet.value.int32_array[VEHICLE_AUDIO_FOCUS_INDEX_EXTERNAL_FOCUS_STATE] = extState;
        mValueToGet.value.int32_array[VEHICLE_AUDIO_FOCUS_INDEX_AUDIO_CONTEXTS] = 0;
        mValueToGet.timestamp = elapsedRealtimeNano();
        mVN->injectEvent(mValueToGet);
    }

    const vehicle_prop_value_t& getCurrentFocus() {
        Mutex::Autolock autoLock(mLock);
        return mValueToGet;
    }

private:
    sp<VehicleNetwork> mVN;
    mutable Mutex mLock;
    sp<VehiclePropertiesHolder> mAudioProperties;
    vehicle_prop_value_t mValueToGet;
};

class VehicleNetworkAudioHelperTest : public testing::Test {
public:
    VehicleNetworkAudioHelperTest() :
        mHalMock(NULL),
        mVN(NULL),
        mListener(new VehicleNetworkTestListener()) { }

    ~VehicleNetworkAudioHelperTest() {}

    const nsecs_t WAIT_NS = 100000000;

protected:
    virtual void SetUp() {
        ASSERT_TRUE(mListener.get() != NULL);
        sp<VehicleNetworkListener> listener(mListener.get());
        mVN = VehicleNetwork::createVehicleNetwork(listener);
        ASSERT_TRUE(mVN.get() != NULL);
        mHalMock = new VehicleHalMockForAudioFocus(mVN);
        sp<VehicleHalMock> halMock = mHalMock;
        mVN->startMocking(halMock);
        mAudioHelper = new VehicleNetworkAudioHelper();
        ASSERT_EQ(NO_ERROR, mAudioHelper->init());
    }

    virtual void TearDown() {
        mAudioHelper->release();
        sp<VehicleHalMock> halMock = mHalMock;
        mVN->stopMocking(halMock);
    }

    void changeFocusState(int32_t state, int32_t streams, int32_t extState) {
        mHalMock->setFocusState(state, streams, extState);
        mVN->injectEvent(mHalMock->getCurrentFocus());
    }

protected:
    sp<VehicleHalMockForAudioFocus> mHalMock;
    sp<VehicleNetwork> mVN;
    sp<VehicleNetworkTestListener> mListener;
    sp<VehicleNetworkAudioHelper> mAudioHelper;
};

TEST_F(VehicleNetworkAudioHelperTest, streamStartStop) {
    ASSERT_EQ(NO_ERROR, mVN->subscribe(VEHICLE_PROPERTY_INTERNAL_AUDIO_STREAM_STATE, 0));
    int initialCount = mListener->getEventCount(VEHICLE_PROPERTY_INTERNAL_AUDIO_STREAM_STATE);
    mAudioHelper->notifyStreamStarted(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_0);
    ASSERT_TRUE(mListener->waitForEvent(VEHICLE_PROPERTY_INTERNAL_AUDIO_STREAM_STATE, initialCount,
            WAIT_NS));
    const vehicle_prop_value& lastValue = mListener->getLastValue();
    ASSERT_EQ(VEHICLE_AUDIO_STREAM_STATE_STARTED,
            lastValue.value.int32_array[VEHICLE_AUDIO_STREAM_STATE_INDEX_STATE]);
    ASSERT_EQ(0, lastValue.value.int32_array[VEHICLE_AUDIO_STREAM_STATE_INDEX_STREAM]);

    initialCount = mListener->getEventCount(VEHICLE_PROPERTY_INTERNAL_AUDIO_STREAM_STATE);
    mAudioHelper->notifyStreamStarted(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_1);
    ASSERT_TRUE(mListener->waitForEvent(VEHICLE_PROPERTY_INTERNAL_AUDIO_STREAM_STATE, initialCount,
            WAIT_NS));
    const vehicle_prop_value& lastValue1 = mListener->getLastValue();
    ASSERT_EQ(VEHICLE_AUDIO_STREAM_STATE_STARTED,
            lastValue1.value.int32_array[VEHICLE_AUDIO_STREAM_STATE_INDEX_STATE]);
    ASSERT_EQ(1, lastValue1.value.int32_array[VEHICLE_AUDIO_STREAM_STATE_INDEX_STREAM]);

    initialCount = mListener->getEventCount(VEHICLE_PROPERTY_INTERNAL_AUDIO_STREAM_STATE);
    mAudioHelper->notifyStreamStopped(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_0);
    ASSERT_TRUE(mListener->waitForEvent(VEHICLE_PROPERTY_INTERNAL_AUDIO_STREAM_STATE, initialCount,
            WAIT_NS));
    const vehicle_prop_value& lastValue2 = mListener->getLastValue();
    ASSERT_EQ(VEHICLE_AUDIO_STREAM_STATE_STOPPED,
            lastValue2.value.int32_array[VEHICLE_AUDIO_STREAM_STATE_INDEX_STATE]);
    ASSERT_EQ(0, lastValue2.value.int32_array[VEHICLE_AUDIO_STREAM_STATE_INDEX_STREAM]);
}

TEST_F(VehicleNetworkAudioHelperTest, testFocus) {
    ASSERT_EQ(VEHICLE_NETWORK_AUDIO_HELPER_FOCUS_STATE_NO_FOCUS,
            mAudioHelper->getStreamFocusState(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_0));
    ASSERT_EQ(VEHICLE_NETWORK_AUDIO_HELPER_FOCUS_STATE_NO_FOCUS,
            mAudioHelper->getStreamFocusState(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_1));
    mHalMock->setFocusState(VEHICLE_AUDIO_FOCUS_REQUEST_GAIN, 0x1, 0);
    // should wait for event first. Otherwise polling will fail as change is not delivered yet.
    ASSERT_TRUE(mAudioHelper->waitForStreamFocus(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_0,
            WAIT_NS));
    ASSERT_FALSE(mAudioHelper->waitForStreamFocus(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_1,
            WAIT_NS));
    ASSERT_EQ(VEHICLE_NETWORK_AUDIO_HELPER_FOCUS_STATE_FOCUS,
            mAudioHelper->getStreamFocusState(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_0));
    ASSERT_EQ(VEHICLE_NETWORK_AUDIO_HELPER_FOCUS_STATE_NO_FOCUS,
            mAudioHelper->getStreamFocusState(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_1));
    mHalMock->setFocusState(VEHICLE_AUDIO_FOCUS_REQUEST_GAIN, 0x3, 0);
    ASSERT_TRUE(mAudioHelper->waitForStreamFocus(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_0,
            WAIT_NS));
    ASSERT_TRUE(mAudioHelper->waitForStreamFocus(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_1,
            WAIT_NS));
    ASSERT_EQ(VEHICLE_NETWORK_AUDIO_HELPER_FOCUS_STATE_FOCUS,
            mAudioHelper->getStreamFocusState(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_0));
    ASSERT_EQ(VEHICLE_NETWORK_AUDIO_HELPER_FOCUS_STATE_FOCUS,
            mAudioHelper->getStreamFocusState(VEHICLE_NETWORK_AUDIO_HELPER_STREAM_1));
}

}; // namespace android
