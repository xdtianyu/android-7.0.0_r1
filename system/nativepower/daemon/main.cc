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

#include <sysexits.h>

#include <base/logging.h>
#include <base/macros.h>
#include <binderwrapper/binder_wrapper.h>
#include <brillo/binder_watcher.h>
#include <brillo/daemons/daemon.h>
#include <brillo/flag_helper.h>

#include "power_manager.h"

namespace {

class PowerManagerDaemon : public brillo::Daemon {
 public:
  PowerManagerDaemon() = default;
  ~PowerManagerDaemon() override = default;

 private:
  // brillo::Daemon:
  int OnInit() override {
    int result = brillo::Daemon::OnInit();
    if (result != EX_OK)
      return result;

    android::BinderWrapper::Create();
    if (!binder_watcher_.Init())
      return EX_OSERR;
    if (!power_manager_.Init())
      return EX_OSERR;

    LOG(INFO) << "Initialization complete";
    return EX_OK;
  }

  brillo::BinderWatcher binder_watcher_;
  android::PowerManager power_manager_;

  DISALLOW_COPY_AND_ASSIGN(PowerManagerDaemon);
};

}  // namespace

int main(int argc, char *argv[]) {
  // This also initializes base::CommandLine(), which is needed for logging.
  brillo::FlagHelper::Init(argc, argv, "Power management daemon");
  logging::InitLogging(logging::LoggingSettings());
  return PowerManagerDaemon().Run();
}
