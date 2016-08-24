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

#ifndef SYSTEM_NATIVEPOWER_DAEMON_POWER_MANAGER_H_
#define SYSTEM_NATIVEPOWER_DAEMON_POWER_MANAGER_H_

#include <memory>

#include <base/files/file_path.h>
#include <base/macros.h>
#include <base/time/time.h>
#include <nativepower/BnPowerManager.h>

#include "system_property_setter.h"
#include "wake_lock_manager.h"

namespace android {

class PowerManager : public BnPowerManager {
 public:
  // The part of the reboot or shutdown system properties' values that appears
  // before the reason. These strings are hardcoded in
  // system/core/init/builtins.cpp.
  static const char kRebootPrefix[];
  static const char kShutdownPrefix[];

  // Value written to |power_state_path_| to suspend the system to memory.
  static const char kPowerStateSuspend[];

  PowerManager();
  ~PowerManager() override;

  // Must be called before Init().
  void set_property_setter_for_testing(
      std::unique_ptr<SystemPropertySetterInterface> setter) {
    property_setter_ = std::move(setter);
  }

  // Must be called before Init().
  void set_wake_lock_manager_for_testing(
      std::unique_ptr<WakeLockManagerInterface> manager) {
    wake_lock_manager_ = std::move(manager);
  }

  void set_power_state_path_for_testing(const base::FilePath& path) {
    power_state_path_ = path;
  }

  // Initializes the object, returning true on success.
  bool Init();

  // BnPowerManager:
  status_t acquireWakeLock(int flags,
                           const sp<IBinder>& lock,
                           const String16& tag,
                           const String16& packageName,
                           bool isOneWay=false) override;
  status_t acquireWakeLockWithUid(int flags,
                                  const sp<IBinder>& lock,
                                  const String16& tag,
                                  const String16& packageName,
                                  int uid,
                                  bool isOneWay=false) override;
  status_t releaseWakeLock(const sp<IBinder>& lock,
                           int flags,
                           bool isOneWay=false) override;
  status_t updateWakeLockUids(const sp<IBinder>& lock,
                              int len,
                              const int* uids,
                              bool isOneWay=false) override;
  status_t powerHint(int hintId, int data) override;
  status_t goToSleep(int64_t event_time_ms, int reason, int flags) override;
  status_t reboot(bool confirm, const String16& reason, bool wait) override;
  status_t shutdown(bool confirm, const String16& reason, bool wait) override;
  status_t crash(const String16& message) override;

 private:
  // Helper method for acquireWakeLock*(). Returns true on success.
  bool AddWakeLockRequest(const sp<IBinder>& lock,
                          const String16& tag,
                          const String16& packageName,
                          int uid);

  std::unique_ptr<SystemPropertySetterInterface> property_setter_;
  std::unique_ptr<WakeLockManagerInterface> wake_lock_manager_;

  // Path to sysfs file that can be written to change the power state.
  base::FilePath power_state_path_;

  // System uptime (as duration since boot) when userspace was last resumed from
  // suspend. Initially unset.
  base::TimeDelta last_resume_uptime_;

  DISALLOW_COPY_AND_ASSIGN(PowerManager);
};

}  // namespace android

#endif  // SYSTEM_NATIVEPOWER_DAEMON_POWER_MANAGER_H_
