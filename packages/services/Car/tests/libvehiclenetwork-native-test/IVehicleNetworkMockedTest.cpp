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

#include <unistd.h>

#include <gtest/gtest.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/threads.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <utils/SystemClock.h>
#include <IVehicleNetwork.h>

#include "TestProperties.h"
#include "VehicleHalMock.h"
#include "IVehicleNetworkTestListener.h"

namespace android {

class IVehicleNetworkMockedTest : public testing::Test {
public:
    IVehicleNetworkMockedTest() {}

    ~IVehicleNetworkMockedTest() {}

    sp<IVehicleNetwork> connectToService() {
        sp<IBinder> binder = defaultServiceManager()->getService(
                String16(IVehicleNetwork::SERVICE_NAME));
        if (binder != NULL) {
            sp<IVehicleNetwork> vn(interface_cast<IVehicleNetwork>(binder));
            return vn;
        }
        sp<IVehicleNetwork> dummy;
        return dummy;
    }

protected:
    virtual void SetUp() {
        ProcessState::self()->startThreadPool();
        mVN = connectToService();
        ASSERT_TRUE(mVN.get() != NULL);
        mHalMock = new VehicleHalMock();
    }

    virtual void TearDown() {
        mVN->stopMocking(mHalMock);
    }
protected:
    sp<VehicleHalMock> mHalMock;
    sp<IVehicleNetwork> mVN;
};

const nsecs_t WAIT_TIMEOUT_NS = 1000000000;

TEST_F(IVehicleNetworkMockedTest, connect) {
    sp<IVehicleNetwork> vn = connectToService();
    ASSERT_TRUE(vn.get() != NULL);
}

TEST_F(IVehicleNetworkMockedTest, listProperties) {
    mVN->startMocking(mHalMock);
    sp<VehiclePropertiesHolder> properties = mVN->listProperties();
    ASSERT_TRUE(properties.get() != NULL);
    ASSERT_TRUE(mHalMock->isTheSameProperties(properties));
}

TEST_F(IVehicleNetworkMockedTest, halRestart) {
    sp<IVehicleNetworkTestListener> listener(new IVehicleNetworkTestListener());
    int originalCount = listener->getHalRestartCount();
    ASSERT_EQ(NO_ERROR, mVN->startHalRestartMonitoring(listener));
    ASSERT_EQ(NO_ERROR, mVN->startMocking(mHalMock));
    listener->waitForHalRestart(WAIT_TIMEOUT_NS);
    ASSERT_EQ(originalCount + 1, listener->getHalRestartCount());
    mVN->stopMocking(mHalMock);
    listener->waitForHalRestart(WAIT_TIMEOUT_NS);
    ASSERT_EQ(originalCount + 2, listener->getHalRestartCount());
}

TEST_F(IVehicleNetworkMockedTest, halGlobalError) {
    sp<IVehicleNetworkTestListener> listener(new IVehicleNetworkTestListener());
    ASSERT_EQ(NO_ERROR, mVN->startErrorListening(listener));
    ASSERT_EQ(NO_ERROR, mVN->startMocking(mHalMock));
    const int ERROR_CODE = -123;
    const int OPERATION_CODE = 4567;
    ASSERT_EQ(NO_ERROR, mVN->injectHalError(ERROR_CODE, 0, OPERATION_CODE));
    listener->waitForHalError(WAIT_TIMEOUT_NS);
    ASSERT_TRUE(listener->isErrorMatching(ERROR_CODE, 0, OPERATION_CODE));
    mVN->stopErrorListening(listener);
}

TEST_F(IVehicleNetworkMockedTest, halPropertyError) {
    sp<IVehicleNetworkTestListener> listener(new IVehicleNetworkTestListener());
    ASSERT_EQ(NO_ERROR, mVN->startMocking(mHalMock));
    ASSERT_EQ(NO_ERROR, mVN->subscribe(listener, TEST_PROPERTY_INT32, 0, 0));
    const int ERROR_CODE = -123;
    const int OPERATION_CODE = 4567;
    ASSERT_EQ(NO_ERROR, mVN->injectHalError(ERROR_CODE, TEST_PROPERTY_INT32, OPERATION_CODE));
    listener->waitForHalError(WAIT_TIMEOUT_NS);
    ASSERT_TRUE(listener->isErrorMatching(ERROR_CODE, TEST_PROPERTY_INT32, OPERATION_CODE));
    mVN->unsubscribe(listener, TEST_PROPERTY_INT32);
}

}; // namespace android
