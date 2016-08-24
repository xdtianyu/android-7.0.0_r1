//
// Copyright (C) 2015 The Android Open Source Project
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

#include "dhcp_client/daemon.h"

#include <sysexits.h>

#include <base/logging.h>
#include <base/run_loop.h>

namespace dhcp_client {

Daemon::Daemon(const base::Closure& startup_callback)
    : startup_callback_(startup_callback) {
}

int Daemon::OnInit() {
  int return_code = brillo::Daemon::OnInit();
  if (return_code != EX_OK) {
    return return_code;
  }

  startup_callback_.Run();

  return EX_OK;
}

void Daemon::OnShutdown(int* return_code) {
}

}  // namespace dhcp_client
