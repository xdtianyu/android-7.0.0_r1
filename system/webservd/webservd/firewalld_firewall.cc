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

#include "webservd/firewalld_firewall.h"

#include <string>

#include <base/bind.h>

namespace webservd {

void FirewalldFirewall::WaitForServiceAsync(dbus::Bus* bus,
                                            const base::Closure& callback) {
  service_online_cb_ = callback;
  object_manager_.reset(new org::chromium::Firewalld::ObjectManagerProxy{bus});
  object_manager_->SetFirewalldAddedCallback(
      base::Bind(&FirewalldFirewall::OnFirewalldOnline,
                 weak_ptr_factory_.GetWeakPtr()));
}

void FirewalldFirewall::PunchTcpHoleAsync(
    uint16_t port,
    const std::string& interface_name,
    const base::Callback<void(bool)>& success_cb,
    const base::Callback<void(brillo::Error*)>& failure_cb) {
  proxy_->PunchTcpHoleAsync(port, interface_name, success_cb, failure_cb);
}

void FirewalldFirewall::OnFirewalldOnline(
    org::chromium::FirewalldProxyInterface* proxy) {
  proxy_ = proxy;
  service_online_cb_.Run();
}

}  // namespace webservd
