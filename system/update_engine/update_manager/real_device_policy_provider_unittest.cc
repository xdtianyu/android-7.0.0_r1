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

#include "update_engine/update_manager/real_device_policy_provider.h"

#include <memory>

#include <brillo/message_loops/fake_message_loop.h>
#include <brillo/message_loops/message_loop.h>
#include <brillo/message_loops/message_loop_utils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <policy/mock_device_policy.h>
#include <policy/mock_libpolicy.h>
#include <session_manager/dbus-proxies.h>
#include <session_manager/dbus-proxy-mocks.h>

#include "update_engine/common/test_utils.h"
#include "update_engine/dbus_test_utils.h"
#include "update_engine/update_manager/umtest_utils.h"

using base::TimeDelta;
using brillo::MessageLoop;
using chromeos_update_engine::dbus_test_utils::MockSignalHandler;
using std::set;
using std::string;
using std::unique_ptr;
using testing::DoAll;
using testing::Mock;
using testing::Return;
using testing::ReturnRef;
using testing::SetArgPointee;
using testing::_;

namespace chromeos_update_manager {

class UmRealDevicePolicyProviderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    loop_.SetAsCurrent();
    provider_.reset(new RealDevicePolicyProvider(&session_manager_proxy_mock_,
                                                 &mock_policy_provider_));
    // By default, we have a device policy loaded. Tests can call
    // SetUpNonExistentDevicePolicy() to override this.
    SetUpExistentDevicePolicy();

    // Setup the session manager_proxy such that it will accept the signal
    // handler and store it in the |property_change_complete_| once registered.
    MOCK_SIGNAL_HANDLER_EXPECT_SIGNAL_HANDLER(property_change_complete_,
                                              session_manager_proxy_mock_,
                                              PropertyChangeComplete);
  }

  void TearDown() override {
    provider_.reset();
    // Check for leaked callbacks on the main loop.
    EXPECT_FALSE(loop_.PendingTasks());
  }

  void SetUpNonExistentDevicePolicy() {
    ON_CALL(mock_policy_provider_, Reload())
        .WillByDefault(Return(false));
    ON_CALL(mock_policy_provider_, device_policy_is_loaded())
        .WillByDefault(Return(false));
    EXPECT_CALL(mock_policy_provider_, GetDevicePolicy()).Times(0);
  }

  void SetUpExistentDevicePolicy() {
    // Setup the default behavior of the mocked PolicyProvider.
    ON_CALL(mock_policy_provider_, Reload())
        .WillByDefault(Return(true));
    ON_CALL(mock_policy_provider_, device_policy_is_loaded())
        .WillByDefault(Return(true));
    ON_CALL(mock_policy_provider_, GetDevicePolicy())
        .WillByDefault(ReturnRef(mock_device_policy_));
  }

  brillo::FakeMessageLoop loop_{nullptr};
  org::chromium::SessionManagerInterfaceProxyMock session_manager_proxy_mock_;
  testing::NiceMock<policy::MockDevicePolicy> mock_device_policy_;
  testing::NiceMock<policy::MockPolicyProvider> mock_policy_provider_;
  unique_ptr<RealDevicePolicyProvider> provider_;

  // The registered signal handler for the signal.
  MockSignalHandler<void(const string&)> property_change_complete_;
};

TEST_F(UmRealDevicePolicyProviderTest, RefreshScheduledTest) {
  // Check that the RefreshPolicy gets scheduled by checking the TaskId.
  EXPECT_TRUE(provider_->Init());
  EXPECT_NE(MessageLoop::kTaskIdNull, provider_->scheduled_refresh_);
  loop_.RunOnce(false);
}

TEST_F(UmRealDevicePolicyProviderTest, FirstReload) {
  // Checks that the policy is reloaded and the DevicePolicy is consulted twice:
  // once on Init() and once again when the signal is connected.
  EXPECT_CALL(mock_policy_provider_, Reload());
  EXPECT_TRUE(provider_->Init());
  Mock::VerifyAndClearExpectations(&mock_policy_provider_);

  EXPECT_CALL(mock_policy_provider_, Reload());
  loop_.RunOnce(false);
}

TEST_F(UmRealDevicePolicyProviderTest, NonExistentDevicePolicyReloaded) {
  // Checks that the policy is reloaded by RefreshDevicePolicy().
  SetUpNonExistentDevicePolicy();
  EXPECT_CALL(mock_policy_provider_, Reload()).Times(3);
  EXPECT_TRUE(provider_->Init());
  loop_.RunOnce(false);
  // Force the policy refresh.
  provider_->RefreshDevicePolicy();
}

TEST_F(UmRealDevicePolicyProviderTest, SessionManagerSignalForcesReload) {
  // Checks that a signal from the SessionManager forces a reload.
  SetUpNonExistentDevicePolicy();
  EXPECT_CALL(mock_policy_provider_, Reload()).Times(2);
  EXPECT_TRUE(provider_->Init());
  loop_.RunOnce(false);
  Mock::VerifyAndClearExpectations(&mock_policy_provider_);

  EXPECT_CALL(mock_policy_provider_, Reload());
  ASSERT_TRUE(property_change_complete_.IsHandlerRegistered());
  property_change_complete_.signal_callback().Run("success");
}

TEST_F(UmRealDevicePolicyProviderTest, NonExistentDevicePolicyEmptyVariables) {
  SetUpNonExistentDevicePolicy();
  EXPECT_CALL(mock_policy_provider_, GetDevicePolicy()).Times(0);
  EXPECT_TRUE(provider_->Init());
  loop_.RunOnce(false);

  UmTestUtils::ExpectVariableHasValue(false,
                                      provider_->var_device_policy_is_loaded());

  UmTestUtils::ExpectVariableNotSet(provider_->var_release_channel());
  UmTestUtils::ExpectVariableNotSet(provider_->var_release_channel_delegated());
  UmTestUtils::ExpectVariableNotSet(provider_->var_update_disabled());
  UmTestUtils::ExpectVariableNotSet(provider_->var_target_version_prefix());
  UmTestUtils::ExpectVariableNotSet(provider_->var_scatter_factor());
  UmTestUtils::ExpectVariableNotSet(
      provider_->var_allowed_connection_types_for_update());
  UmTestUtils::ExpectVariableNotSet(provider_->var_owner());
  UmTestUtils::ExpectVariableNotSet(provider_->var_http_downloads_enabled());
  UmTestUtils::ExpectVariableNotSet(provider_->var_au_p2p_enabled());
}

TEST_F(UmRealDevicePolicyProviderTest, ValuesUpdated) {
  SetUpNonExistentDevicePolicy();
  EXPECT_TRUE(provider_->Init());
  loop_.RunOnce(false);
  Mock::VerifyAndClearExpectations(&mock_policy_provider_);

  // Reload the policy with a good one and set some values as present. The
  // remaining values are false.
  SetUpExistentDevicePolicy();
  EXPECT_CALL(mock_device_policy_, GetReleaseChannel(_))
      .WillOnce(DoAll(SetArgPointee<0>(string("mychannel")), Return(true)));
  EXPECT_CALL(mock_device_policy_, GetAllowedConnectionTypesForUpdate(_))
      .WillOnce(Return(false));

  provider_->RefreshDevicePolicy();

  UmTestUtils::ExpectVariableHasValue(true,
                                      provider_->var_device_policy_is_loaded());

  // Test that at least one variable is set, to ensure the refresh occurred.
  UmTestUtils::ExpectVariableHasValue(string("mychannel"),
                                      provider_->var_release_channel());
  UmTestUtils::ExpectVariableNotSet(
      provider_->var_allowed_connection_types_for_update());
}

TEST_F(UmRealDevicePolicyProviderTest, ScatterFactorConverted) {
  SetUpExistentDevicePolicy();
  EXPECT_CALL(mock_device_policy_, GetScatterFactorInSeconds(_))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<0>(1234), Return(true)));
  EXPECT_TRUE(provider_->Init());
  loop_.RunOnce(false);

  UmTestUtils::ExpectVariableHasValue(TimeDelta::FromSeconds(1234),
                                      provider_->var_scatter_factor());
}

TEST_F(UmRealDevicePolicyProviderTest, NegativeScatterFactorIgnored) {
  SetUpExistentDevicePolicy();
  EXPECT_CALL(mock_device_policy_, GetScatterFactorInSeconds(_))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<0>(-1), Return(true)));
  EXPECT_TRUE(provider_->Init());
  loop_.RunOnce(false);

  UmTestUtils::ExpectVariableNotSet(provider_->var_scatter_factor());
}

TEST_F(UmRealDevicePolicyProviderTest, AllowedTypesConverted) {
  SetUpExistentDevicePolicy();
  EXPECT_CALL(mock_device_policy_, GetAllowedConnectionTypesForUpdate(_))
      .Times(2)
      .WillRepeatedly(DoAll(
          SetArgPointee<0>(set<string>{"bluetooth", "wifi", "not-a-type"}),
          Return(true)));
  EXPECT_TRUE(provider_->Init());
  loop_.RunOnce(false);

  UmTestUtils::ExpectVariableHasValue(
      set<ConnectionType>{ConnectionType::kWifi, ConnectionType::kBluetooth},
      provider_->var_allowed_connection_types_for_update());
}

}  // namespace chromeos_update_manager
