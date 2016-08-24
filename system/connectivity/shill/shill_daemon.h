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

#ifndef SHILL_SHILL_DAEMON_H_
#define SHILL_SHILL_DAEMON_H_

#include <string>

#include <base/callback.h>
#include <brillo/daemons/daemon.h>

#include "shill/daemon_task.h"

namespace shill {

class Config;

// ShillDaemon is the daemon that will be initialized in shill_main.cc. It
// inherits the logic of daemon-related tasks (e.g. init/shutdown, start/stop)
// from DaemonTask, and additionally overrides methods of brillo::Daemon.
class ShillDaemon : public DaemonTask, public brillo::Daemon {
 public:
  ShillDaemon(const base::Closure& startup_callback,
              const shill::DaemonTask::Settings& settings, Config* config);
  virtual ~ShillDaemon();

 protected:
  // Implementation of brillo::Daemon.
  int OnInit() override;
  void OnShutdown(int* return_code) override;

  base::Closure startup_callback_;
};

}  // namespace shill

#endif  // SHILL_SHILL_DAEMON_H_
