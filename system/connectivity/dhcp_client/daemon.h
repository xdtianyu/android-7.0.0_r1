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

#ifndef DHCP_CLIENT_DAEMON_H_
#define DHCP_CLIENT_DAEMON_H_

#include <base/callback_forward.h>
#include <brillo/daemons/dbus_daemon.h>

namespace dhcp_client {

class Daemon : public brillo::Daemon {
 public:
  explicit Daemon(const base::Closure& startup_callback);
  ~Daemon() = default;

 protected:
  int OnInit() override;
  void OnShutdown(int* return_code) override;

 private:
  base::Closure startup_callback_;

  DISALLOW_COPY_AND_ASSIGN(Daemon);
};

}  // namespace dhcp_client

#endif  // DHCP_CLIENT_DAEMON_H_
