//
// Copyright (C) 2016 The Android Open Source Project
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

#include "shill/shill_daemon.h"

#include <sysexits.h>

#include <base/bind.h>

using base::Bind;
using base::Unretained;

namespace shill {

ShillDaemon::ShillDaemon(const base::Closure& startup_callback,
                         const shill::DaemonTask::Settings& settings,
                         Config* config)
    : DaemonTask(settings, config), startup_callback_(startup_callback) {}

ShillDaemon::~ShillDaemon() {}

int ShillDaemon::OnInit() {
  // Manager DBus interface will get registered as part of this init call.
  int return_code = brillo::Daemon::OnInit();
  if (return_code != EX_OK) {
    return return_code;
  }

  Init();

  // Signal that we've acquired all resources.
  startup_callback_.Run();

  return EX_OK;
}

void ShillDaemon::OnShutdown(int* return_code) {
  if (!DaemonTask::Quit(base::Bind(&DaemonTask::BreakTerminationLoop,
                                   base::Unretained(this)))) {
    // Run a message loop to allow shill to complete its termination
    // procedures. This is different from the secondary loop in
    // brillo::Daemon. This loop will run until we explicitly
    // breakout of the loop, whereas the secondary loop in
    // brillo::Daemon will run until no more tasks are posted on the
    // loop.  This allows asynchronous D-Bus method calls to complete
    // before exiting.
    brillo::MessageLoop::current()->Run();
  }

  brillo::Daemon::OnShutdown(return_code);
}

}  // namespace shill
