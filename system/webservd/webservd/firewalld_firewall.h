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

#ifndef WEBSERVER_WEBSERVD_FIREWALLD_FIREWALL_H_
#define WEBSERVER_WEBSERVD_FIREWALLD_FIREWALL_H_

#include "webservd/firewall_interface.h"

#include <string>

#include <base/macros.h>
#include <base/memory/weak_ptr.h>

#include "firewalld/dbus-proxies.h"

namespace webservd {

class FirewalldFirewall : public FirewallInterface {
 public:
  FirewalldFirewall() = default;
  ~FirewalldFirewall() override = default;

  // Interface overrides.
  void WaitForServiceAsync(dbus::Bus* bus, const base::Closure& callback)
      override;
  void PunchTcpHoleAsync(
      uint16_t port, const std::string& interface_name,
      const base::Callback<void(bool)>& success_cb,
      const base::Callback<void(brillo::Error*)>& failure_cb) override;

 private:
  void OnFirewalldOnline(org::chromium::FirewalldProxyInterface* proxy);

  std::unique_ptr<org::chromium::Firewalld::ObjectManagerProxy> object_manager_;

  // Proxy to the firewall DBus service. Owned by the DBus bindings module.
  org::chromium::FirewalldProxyInterface* proxy_;

  // Callback to use when firewall service comes online.
  base::Closure service_online_cb_;

  base::WeakPtrFactory<FirewalldFirewall> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(FirewalldFirewall);
};

}  // namespace webservd

#endif  // WEBSERVER_WEBSERVD_FIREWALLD_FIREWALL_H_
