//
// Copyright (C) 2014 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "shill/device_claimer.h"

#include <string>

#include <gtest/gtest.h>

#include "shill/error.h"
#include "shill/mock_control.h"
#include "shill/mock_device_info.h"

using std::string;
using ::testing::_;
using ::testing::Mock;
using ::testing::Return;

namespace shill {

const char kServiceName[] = "org.chromium.TestService";
const char kTestDevice1Name[] = "test_device1";
const char kTestDevice2Name[] = "test_device2";

class DeviceClaimerTest : public testing::Test {
 public:
  DeviceClaimerTest()
     : device_info_(nullptr, nullptr, nullptr, nullptr),
       device_claimer_(kServiceName, &device_info_, false) {}

 protected:
  MockDeviceInfo device_info_;
  DeviceClaimer device_claimer_;
};

TEST_F(DeviceClaimerTest, ClaimAndReleaseDevices) {
  // Should not have any device claimed initially.
  EXPECT_FALSE(device_claimer_.DevicesClaimed());

  // Claim device 1.
  Error error;
  EXPECT_CALL(device_info_, AddDeviceToBlackList(kTestDevice1Name)).Times(1);
  EXPECT_TRUE(device_claimer_.Claim(kTestDevice1Name, &error));
  EXPECT_EQ(Error::kSuccess, error.type());
  EXPECT_TRUE(device_claimer_.DevicesClaimed());
  Mock::VerifyAndClearExpectations(&device_info_);

  // Claim device 2.
  error.Reset();
  EXPECT_CALL(device_info_, AddDeviceToBlackList(kTestDevice2Name)).Times(1);
  EXPECT_TRUE(device_claimer_.Claim(kTestDevice2Name, &error));
  EXPECT_EQ(Error::kSuccess, error.type());
  EXPECT_TRUE(device_claimer_.DevicesClaimed());
  Mock::VerifyAndClearExpectations(&device_info_);

  // Claim device 1 again, should fail since it is already claimed.
  const char kDuplicateDevice1Error[] =
      "Device test_device1 had already been claimed";
  error.Reset();
  EXPECT_CALL(device_info_, AddDeviceToBlackList(_)).Times(0);
  EXPECT_FALSE(device_claimer_.Claim(kTestDevice1Name, &error));
  EXPECT_EQ(string(kDuplicateDevice1Error), error.message());
  Mock::VerifyAndClearExpectations(&device_info_);

  // Release device 1.
  error.Reset();
  EXPECT_CALL(device_info_,
              RemoveDeviceFromBlackList(kTestDevice1Name)).Times(1);
  EXPECT_TRUE(device_claimer_.Release(kTestDevice1Name, &error));
  EXPECT_EQ(Error::kSuccess, error.type());
  // Should still have one device claimed.
  EXPECT_TRUE(device_claimer_.DevicesClaimed());
  Mock::VerifyAndClearExpectations(&device_info_);

  // Release device 1 again, should fail since device 1 is not currently
  // claimed.
  const char kDevice1NotClaimedError[] =
      "Device test_device1 have not been claimed";
  error.Reset();
  EXPECT_CALL(device_info_, RemoveDeviceFromBlackList(_)).Times(0);
  EXPECT_FALSE(device_claimer_.Release(kTestDevice1Name, &error));
  EXPECT_EQ(string(kDevice1NotClaimedError), error.message());
  // Should still have one device claimed.
  EXPECT_TRUE(device_claimer_.DevicesClaimed());
  Mock::VerifyAndClearExpectations(&device_info_);

  // Release device 2
  error.Reset();
  EXPECT_CALL(device_info_,
              RemoveDeviceFromBlackList(kTestDevice2Name)).Times(1);
  EXPECT_TRUE(device_claimer_.Release(kTestDevice2Name, &error));
  EXPECT_EQ(Error::kSuccess, error.type());
  Mock::VerifyAndClearExpectations(&device_info_);

  // Should not have any claimed devices.
  EXPECT_FALSE(device_claimer_.DevicesClaimed());
}

}  // namespace shill
