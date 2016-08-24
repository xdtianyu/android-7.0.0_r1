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

#include "apmanager/daemon.h"

#include <sysexits.h>

#include <base/logging.h>

#include "apmanager/dbus/dbus_control.h"

namespace apmanager {

// static
#if !defined(__ANDROID__)
const char Daemon::kAPManagerGroupName[] = "apmanager";
const char Daemon::kAPManagerUserName[] = "apmanager";
#else
const char Daemon::kAPManagerGroupName[] = "system";
const char Daemon::kAPManagerUserName[] = "system";
#endif  // __ANDROID__

Daemon::Daemon(const base::Closure& startup_callback)
    : startup_callback_(startup_callback) {
}

int Daemon::OnInit() {
  int return_code = brillo::Daemon::OnInit();
  if (return_code != EX_OK) {
    return return_code;
  }

  // Setup control interface. The control interface will expose
  // our service (Manager) through its RPC interface.
  control_interface_.reset(new DBusControl());
  control_interface_->Init();

  // Signal that we've acquired all resources.
  startup_callback_.Run();

  return EX_OK;
}

void Daemon::OnShutdown(int* return_code) {
  control_interface_->Shutdown();
}

}  // namespace apmanager
