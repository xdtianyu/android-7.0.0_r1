// Copyright 2015 The Android Open Source Project
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

#ifndef WEBSERVER_WEBSERVD_FIREWALL_INTERFACE_H_
#define WEBSERVER_WEBSERVD_FIREWALL_INTERFACE_H_

#include <inttypes.h>

#include <string>

#include <base/callback.h>
#include <base/macros.h>
#include <brillo/dbus/dbus_object.h>

namespace webservd {

class FirewallInterface {
 public:
  virtual ~FirewallInterface() = default;

  // Wait for the firewall DBus service to be up.
  virtual void WaitForServiceAsync(dbus::Bus* bus,
                                   const base::Closure& callback) = 0;

  // Methods for managing firewall ports.
  virtual void PunchTcpHoleAsync(
      uint16_t port,
      const std::string& interface_name,
      const base::Callback<void(bool)>& success_cb,
      const base::Callback<void(brillo::Error*)>& failure_cb) = 0;

 protected:
  FirewallInterface() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(FirewallInterface);
};

}  // namespace webservd

#endif  // WEBSERVER_WEBSERVD_FIREWALL_INTERFACE_H_
