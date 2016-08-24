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

#include <string>

#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/time/time.h>
#include <nativepower/wake_lock.h>
#include <powermanager/IPowerManager.h>
#include <utils/StrongPointer.h>

namespace android {

// Reasons that can be passed to PowerManagerClient::Suspend().
enum class SuspendReason {
  // These values must match the ones in android.os.PowerManager.
  APPLICATION  = 0,
  DEVICE_ADMIN = 1,
  TIMEOUT      = 2,
  LID_SWITCH   = 3,
  POWER_BUTTON = 4,
  HDMI         = 5,
  SLEEP_BUTTON = 6,
};

enum class SuspendFlags {
  // Corresponds to GO_TO_SLEEP_FLAG_NO_DOZE in android.os.PowerManager.
  NO_DOZE = 1 << 0,
};

// Reasons that can be passed to PowerManagerClient::ShutDown().
enum class ShutdownReason {
  DEFAULT,
  USER_REQUESTED,
};

// Reasons that can be passed to PowerManagerClient::Reboot().
enum class RebootReason {
  DEFAULT,
  RECOVERY,
};

// Class used to communicate with the system power manager.
//
// android::BinderWrapper must be initialized before constructing this class.
class PowerManagerClient {
 public:
  PowerManagerClient();
  ~PowerManagerClient();

  // This should not be used directly; it's just exposed for WakeLock.
  const sp<IPowerManager>& power_manager() { return power_manager_; }

  // Initializes the object, returning true on success. Must be called before
  // any other methods.
  bool Init();

  // Creates and returns a wake lock identified by |tag| and |package|. The
  // returned WakeLock object will block power management until it is destroyed.
  // An empty pointer is returned on failure (e.g. due to issues communicating
  // with the power manager).
  std::unique_ptr<WakeLock> CreateWakeLock(const std::string& tag,
                                           const std::string& package);

  // Suspends the system immediately, returning true on success.
  //
  // |event_uptime| contains the time since the system was booted (e.g.
  // base::SysInfo::Uptime()) of the event that triggered the suspend request.
  // It is used to avoid acting on stale suspend requests that are sent before
  // the currently-active suspend request completes.
  // |reason| is currently only used by android.view.WindowManagerPolicy.
  // |flags| is a bitfield of SuspendFlag values.
  bool Suspend(base::TimeDelta event_uptime, SuspendReason reason, int flags);

  // Shuts down or reboots the system, returning true on success.
  bool ShutDown(ShutdownReason reason);
  bool Reboot(RebootReason reason);

 private:
  // Called in response to |power_manager_|'s binder dying.
  void OnPowerManagerDied();

  // Interface for communicating with the power manager.
  sp<IPowerManager> power_manager_;

  // Keep this member last.
  base::WeakPtrFactory<PowerManagerClient> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PowerManagerClient);
};

}  // namespace android
