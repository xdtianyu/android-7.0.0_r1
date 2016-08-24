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

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/macros.h>
#include <base/sys_info.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binderwrapper/binder_test_base.h>
#include <binderwrapper/stub_binder_wrapper.h>
#include <cutils/android_reboot.h>
#include <nativepower/constants.h>
#include <powermanager/PowerManager.h>

#include "power_manager.h"
#include "system_property_setter_stub.h"
#include "wake_lock_manager_stub.h"

namespace android {

class PowerManagerTest : public BinderTestBase {
 public:
  PowerManagerTest()
      : power_manager_(new PowerManager()),
        interface_(interface_cast<IPowerManager>(power_manager_)),
        property_setter_(new SystemPropertySetterStub()),
        wake_lock_manager_(new WakeLockManagerStub()) {
    CHECK(temp_dir_.CreateUniqueTempDir());

    power_state_path_ = temp_dir_.path().Append("power_state");
    power_manager_->set_power_state_path_for_testing(power_state_path_);
    ClearPowerState();

    power_manager_->set_property_setter_for_testing(
        std::unique_ptr<SystemPropertySetterInterface>(property_setter_));
    power_manager_->set_wake_lock_manager_for_testing(
        std::unique_ptr<WakeLockManagerInterface>(wake_lock_manager_));

    CHECK(power_manager_->Init());
  }
  ~PowerManagerTest() override = default;

 protected:
  // Returns the value in |power_state_path_|.
  std::string ReadPowerState() {
    std::string state;
    PCHECK(base::ReadFileToString(power_state_path_, &state))
        << "Failed to read " << power_state_path_.value();
    return state;
  }

  // Clears |power_state_path_|.
  void ClearPowerState() {
    PCHECK(base::WriteFile(power_state_path_, "", 0) == 0)
        << "Failed to write " << power_state_path_.value();
  }

  base::ScopedTempDir temp_dir_;
  sp<PowerManager> power_manager_;
  sp<IPowerManager> interface_;
  SystemPropertySetterStub* property_setter_;  // Owned by |power_manager_|.
  WakeLockManagerStub* wake_lock_manager_;  // Owned by |power_manager_|.

  // File under |temp_dir_| used in place of /sys/power/state.
  base::FilePath power_state_path_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PowerManagerTest);
};

TEST_F(PowerManagerTest, RegisterService) {
  EXPECT_EQ(power_manager_,
            binder_wrapper()->GetRegisteredService(kPowerManagerServiceName));
}

TEST_F(PowerManagerTest, AcquireAndReleaseWakeLock) {
  const char kTag[] = "foo";
  const char kPackage[] = "bar";
  sp<BBinder> binder = binder_wrapper()->CreateLocalBinder();

  // Check that PowerManager looks up the calling UID when necessary.
  const uid_t kCallingUid = 100;
  binder_wrapper()->set_calling_uid(kCallingUid);
  EXPECT_EQ(OK, interface_->acquireWakeLock(0, binder, String16(kTag),
                                            String16(kPackage)));
  EXPECT_EQ(1, wake_lock_manager_->num_requests());
  EXPECT_EQ(
      WakeLockManagerStub::ConstructRequestString(kTag, kPackage, kCallingUid),
      wake_lock_manager_->GetRequestString(
          binder_wrapper()->local_binders()[0]));

  EXPECT_EQ(OK, interface_->releaseWakeLock(binder, 0));
  EXPECT_EQ(0, wake_lock_manager_->num_requests());

  // If a UID is passed, it should be used instead.
  const uid_t kPassedUid = 200;
  EXPECT_EQ(OK, interface_->acquireWakeLockWithUid(
                    0, binder, String16(kTag), String16(kPackage), kPassedUid));
  EXPECT_EQ(1, wake_lock_manager_->num_requests());
  EXPECT_EQ(
      WakeLockManagerStub::ConstructRequestString(kTag, kPackage, kPassedUid),
      wake_lock_manager_->GetRequestString(
          binder_wrapper()->local_binders()[0]));
}

TEST_F(PowerManagerTest, GoToSleep) {
  EXPECT_EQ("", ReadPowerState());

  const int64_t kStartTime = base::SysInfo::Uptime().InMilliseconds();
  EXPECT_EQ(OK,
            interface_->goToSleep(kStartTime, 0 /* reason */, 0 /* flags */));
  EXPECT_EQ(PowerManager::kPowerStateSuspend, ReadPowerState());

  // A request with a timestamp preceding the last resume should be ignored.
  ClearPowerState();
  EXPECT_EQ(BAD_VALUE, interface_->goToSleep(kStartTime - 1, 0, 0));
  EXPECT_EQ("", ReadPowerState());

  // A second attempt with a timestamp occurring after the last
  // resume should be honored.
  ClearPowerState();
  EXPECT_EQ(
      OK,
      interface_->goToSleep(base::SysInfo::Uptime().InMilliseconds(), 0, 0));
  EXPECT_EQ(PowerManager::kPowerStateSuspend, ReadPowerState());
}

TEST_F(PowerManagerTest, Reboot) {
  EXPECT_EQ(OK, interface_->reboot(false, String16(), false));
  EXPECT_EQ(PowerManager::kRebootPrefix,
            property_setter_->GetProperty(ANDROID_RB_PROPERTY));

  EXPECT_EQ(OK, interface_->reboot(false, String16(kRebootReasonRecovery),
                                   false));
  EXPECT_EQ(std::string(PowerManager::kRebootPrefix) + kRebootReasonRecovery,
            property_setter_->GetProperty(ANDROID_RB_PROPERTY));

  // Invalid values should be rejected.
  ASSERT_TRUE(property_setter_->SetProperty(ANDROID_RB_PROPERTY, ""));
  EXPECT_EQ(BAD_VALUE, interface_->reboot(false, String16("foo"), false));
  EXPECT_EQ("", property_setter_->GetProperty(ANDROID_RB_PROPERTY));
}

TEST_F(PowerManagerTest, Shutdown) {
  EXPECT_EQ(OK, interface_->shutdown(false, String16(), false));
  EXPECT_EQ(PowerManager::kShutdownPrefix,
            property_setter_->GetProperty(ANDROID_RB_PROPERTY));

  EXPECT_EQ(OK, interface_->shutdown(false,
                                     String16(kShutdownReasonUserRequested),
                                     false));
  EXPECT_EQ(std::string(PowerManager::kShutdownPrefix) +
            kShutdownReasonUserRequested,
            property_setter_->GetProperty(ANDROID_RB_PROPERTY));

  // Invalid values should be rejected.
  ASSERT_TRUE(property_setter_->SetProperty(ANDROID_RB_PROPERTY, ""));
  EXPECT_EQ(BAD_VALUE, interface_->shutdown(false, String16("foo"), false));
  EXPECT_EQ("", property_setter_->GetProperty(ANDROID_RB_PROPERTY));
}

}  // namespace android
