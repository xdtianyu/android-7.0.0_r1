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

#include <base/logging.h>
#include <base/macros.h>
#include <base/time/time.h>
#include <binderwrapper/binder_test_base.h>
#include <binderwrapper/stub_binder_wrapper.h>
#include <nativepower/constants.h>
#include <nativepower/power_manager_client.h>
#include <nativepower/power_manager_stub.h>

namespace android {

class PowerManagerClientTest : public BinderTestBase {
 public:
  PowerManagerClientTest()
      : power_manager_(new PowerManagerStub()),
        power_manager_binder_(power_manager_) {
    binder_wrapper()->SetBinderForService(kPowerManagerServiceName,
                                          power_manager_binder_);
    CHECK(client_.Init());
  }
  ~PowerManagerClientTest() override = default;

 protected:
  PowerManagerStub* power_manager_;  // Owned by |power_manager_binder_|.
  sp<IBinder> power_manager_binder_;
  PowerManagerClient client_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PowerManagerClientTest);
};

TEST_F(PowerManagerClientTest, Suspend) {
  EXPECT_EQ(0, power_manager_->num_suspend_requests());

  const auto kEventTime = base::TimeDelta::FromMilliseconds(123);
  const int kFlags = 0x456;
  EXPECT_TRUE(client_.Suspend(kEventTime, SuspendReason::POWER_BUTTON, kFlags));
  EXPECT_EQ(1, power_manager_->num_suspend_requests());
  EXPECT_EQ(PowerManagerStub::ConstructSuspendRequestString(
                kEventTime.InMilliseconds(),
                static_cast<int>(SuspendReason::POWER_BUTTON), kFlags),
            power_manager_->GetSuspendRequestString(0));
}

TEST_F(PowerManagerClientTest, ShutDown) {
  EXPECT_TRUE(client_.ShutDown(ShutdownReason::DEFAULT));
  ASSERT_EQ(1u, power_manager_->shutdown_reasons().size());
  EXPECT_EQ("", power_manager_->shutdown_reasons()[0]);

  EXPECT_TRUE(client_.ShutDown(ShutdownReason::USER_REQUESTED));
  ASSERT_EQ(2u, power_manager_->shutdown_reasons().size());
  EXPECT_EQ(kShutdownReasonUserRequested,
            power_manager_->shutdown_reasons()[1]);
}

TEST_F(PowerManagerClientTest, Reboot) {
  EXPECT_TRUE(client_.Reboot(RebootReason::DEFAULT));
  ASSERT_EQ(1u, power_manager_->reboot_reasons().size());
  EXPECT_EQ("", power_manager_->reboot_reasons()[0]);

  EXPECT_TRUE(client_.Reboot(RebootReason::RECOVERY));
  ASSERT_EQ(2u, power_manager_->reboot_reasons().size());
  EXPECT_EQ(kRebootReasonRecovery, power_manager_->reboot_reasons()[1]);
}

}  // namespace android
