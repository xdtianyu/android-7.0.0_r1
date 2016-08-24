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

#ifndef APMANAGER_DAEMON_H_
#define APMANAGER_DAEMON_H_

#include <base/callback_forward.h>
#include <brillo/daemons/daemon.h>

#include "apmanager/control_interface.h"

namespace apmanager {

class Daemon : public brillo::Daemon {
 public:
  // User and group to run the apmanager process.
  static const char kAPManagerGroupName[];
  static const char kAPManagerUserName[];

  explicit Daemon(const base::Closure& startup_callback);
  ~Daemon() = default;

 protected:
  int OnInit() override;
  void OnShutdown(int* return_code) override;

 private:
  friend class DaemonTest;

  std::unique_ptr<ControlInterface> control_interface_;
  base::Closure startup_callback_;

  DISALLOW_COPY_AND_ASSIGN(Daemon);
};

}  // namespace apmanager

#endif  // APMANAGER_DAEMON_H_
