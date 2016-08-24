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

#include <memory>

#include <base/at_exit.h>
#include <base/logging.h>
#include <base/message_loop/message_loop.h>
#include <base/sys_info.h>
#include <base/time/time.h>
#include <binderwrapper/binder_wrapper.h>
#include <brillo/flag_helper.h>
#include <nativepower/power_manager_client.h>
#include <nativepower/wake_lock.h>

namespace {

// Seconds to sleep after acquiring a wake lock.
const int kWakeLockSleepSec = 5;

}  // namespace

int main(int argc, char *argv[]) {
  DEFINE_string(action, "",
                "Action to perform (\"reboot\", \"shut_down\", \"suspend\", "
                "\"wake_lock\")");

  brillo::FlagHelper::Init(argc, argv, "Example power-management client.");
  logging::InitLogging(logging::LoggingSettings());
  base::AtExitManager at_exit;
  base::MessageLoopForIO loop;
  android::BinderWrapper::Create();

  android::PowerManagerClient client;
  CHECK(client.Init());

  if (FLAGS_action == "reboot") {
    LOG(INFO) << "Requesting reboot";
    CHECK(client.Reboot(android::RebootReason::DEFAULT));
  } else if (FLAGS_action == "shut_down") {
    LOG(INFO) << "Requesting shutdown";
    CHECK(client.ShutDown(android::ShutdownReason::DEFAULT));
  } else if (FLAGS_action == "suspend") {
    LOG(INFO) << "Requesting suspend";
    CHECK(client.Suspend(base::SysInfo::Uptime(),
                         android::SuspendReason::APPLICATION, 0 /* flags */));
  } else if (FLAGS_action == "wake_lock") {
    LOG(INFO) << "Creating wake lock";
    std::unique_ptr<android::WakeLock> lock(
        client.CreateWakeLock("power_example", "power"));
    CHECK(lock) << "Lock not created";
    LOG(INFO) << "Sleeping for " << kWakeLockSleepSec << " seconds";
    sleep(kWakeLockSleepSec);
  } else {
    LOG(FATAL) << "Unknown action \"" << FLAGS_action << "\"";
  }

  LOG(INFO) << "Exiting";
  return 0;
}
