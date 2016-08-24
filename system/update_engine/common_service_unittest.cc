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

#include "update_engine/common_service.h"

#include <gtest/gtest.h>
#include <string>

#include <brillo/errors/error.h>
#include <policy/libpolicy.h>
#include <policy/mock_device_policy.h>

#include "update_engine/fake_system_state.h"

using std::string;
using testing::Return;
using testing::SetArgumentPointee;
using testing::_;

namespace chromeos_update_engine {

class UpdateEngineServiceTest : public ::testing::Test {
 protected:
  UpdateEngineServiceTest()
      : mock_update_attempter_(fake_system_state_.mock_update_attempter()),
        common_service_(&fake_system_state_) {}

  void SetUp() override {
    fake_system_state_.set_device_policy(nullptr);
  }

  // Fake/mock infrastructure.
  FakeSystemState fake_system_state_;
  policy::MockDevicePolicy mock_device_policy_;

  // Shortcut for fake_system_state_.mock_update_attempter().
  MockUpdateAttempter* mock_update_attempter_;

  brillo::ErrorPtr error_;
  UpdateEngineService common_service_;
};

TEST_F(UpdateEngineServiceTest, AttemptUpdate) {
  EXPECT_CALL(*mock_update_attempter_, CheckForUpdate(
      "app_ver", "url", false /* interactive */));
  // The update is non-interactive when we pass the non-interactive flag.
  EXPECT_TRUE(common_service_.AttemptUpdate(
      &error_, "app_ver", "url",
      UpdateEngineService::kAttemptUpdateFlagNonInteractive));
  EXPECT_EQ(nullptr, error_);
}

// SetChannel is allowed when there's no device policy (the device is not
// enterprise enrolled).
TEST_F(UpdateEngineServiceTest, SetChannelWithNoPolicy) {
  EXPECT_CALL(*mock_update_attempter_, RefreshDevicePolicy());
  // If SetTargetChannel is called it means the policy check passed.
  EXPECT_CALL(*fake_system_state_.mock_request_params(),
              SetTargetChannel("stable-channel", true, _))
      .WillOnce(Return(true));
  EXPECT_TRUE(common_service_.SetChannel(&error_, "stable-channel", true));
  ASSERT_EQ(nullptr, error_);
}

// When the policy is present, the delegated value should be checked.
TEST_F(UpdateEngineServiceTest, SetChannelWithDelegatedPolicy) {
  policy::MockDevicePolicy mock_device_policy;
  fake_system_state_.set_device_policy(&mock_device_policy);
  EXPECT_CALL(mock_device_policy, GetReleaseChannelDelegated(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(true), Return(true)));
  EXPECT_CALL(*fake_system_state_.mock_request_params(),
              SetTargetChannel("beta-channel", true, _))
      .WillOnce(Return(true));

  EXPECT_TRUE(common_service_.SetChannel(&error_, "beta-channel", true));
  ASSERT_EQ(nullptr, error_);
}

// When passing an invalid value (SetTargetChannel fails) an error should be
// raised.
TEST_F(UpdateEngineServiceTest, SetChannelWithInvalidChannel) {
  EXPECT_CALL(*mock_update_attempter_, RefreshDevicePolicy());
  EXPECT_CALL(*fake_system_state_.mock_request_params(),
              SetTargetChannel("foo-channel", true, _)).WillOnce(Return(false));

  EXPECT_FALSE(common_service_.SetChannel(&error_, "foo-channel", true));
  ASSERT_NE(nullptr, error_);
  EXPECT_TRUE(error_->HasError(UpdateEngineService::kErrorDomain,
                               UpdateEngineService::kErrorFailed));
}

TEST_F(UpdateEngineServiceTest, GetChannel) {
  fake_system_state_.mock_request_params()->set_current_channel("current");
  fake_system_state_.mock_request_params()->set_target_channel("target");
  string channel;
  EXPECT_TRUE(common_service_.GetChannel(
      &error_, true /* get_current_channel */, &channel));
  EXPECT_EQ(nullptr, error_);
  EXPECT_EQ("current", channel);

  EXPECT_TRUE(common_service_.GetChannel(
      &error_, false /* get_current_channel */, &channel));
  EXPECT_EQ(nullptr, error_);
  EXPECT_EQ("target", channel);
}

TEST_F(UpdateEngineServiceTest, ResetStatusSucceeds) {
  EXPECT_CALL(*mock_update_attempter_, ResetStatus()).WillOnce(Return(true));
  EXPECT_TRUE(common_service_.ResetStatus(&error_));
  EXPECT_EQ(nullptr, error_);
}

TEST_F(UpdateEngineServiceTest, ResetStatusFails) {
  EXPECT_CALL(*mock_update_attempter_, ResetStatus()).WillOnce(Return(false));
  EXPECT_FALSE(common_service_.ResetStatus(&error_));
  ASSERT_NE(nullptr, error_);
  EXPECT_TRUE(error_->HasError(UpdateEngineService::kErrorDomain,
                               UpdateEngineService::kErrorFailed));
}

}  // namespace chromeos_update_engine
