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

#include "webservd/permission_broker_firewall.h"

#include <unistd.h>

#include <string>

#include <base/bind.h>
#include <base/macros.h>

namespace webservd {

PermissionBrokerFirewall::PermissionBrokerFirewall() {
  int fds[2];
  PCHECK(pipe(fds) == 0) << "Failed to create firewall lifeline pipe";
  lifeline_read_fd_ = fds[0];
  lifeline_write_fd_ = fds[1];
}

PermissionBrokerFirewall::~PermissionBrokerFirewall() {
  close(lifeline_read_fd_);
  close(lifeline_write_fd_);
}

void PermissionBrokerFirewall::WaitForServiceAsync(
    dbus::Bus* bus,
    const base::Closure& callback) {
  service_online_cb_ = callback;
  object_manager_.reset(
      new org::chromium::PermissionBroker::ObjectManagerProxy{bus});
  object_manager_->SetPermissionBrokerAddedCallback(
      base::Bind(&PermissionBrokerFirewall::OnPermissionBrokerOnline,
                 weak_ptr_factory_.GetWeakPtr()));
}

void PermissionBrokerFirewall::PunchTcpHoleAsync(
    uint16_t port,
    const std::string& interface_name,
    const base::Callback<void(bool)>& success_cb,
    const base::Callback<void(brillo::Error*)>& failure_cb) {
  dbus::FileDescriptor dbus_fd{lifeline_read_fd_};
  dbus_fd.CheckValidity();
  proxy_->RequestTcpPortAccessAsync(port, interface_name, dbus_fd, success_cb,
                                    failure_cb);
}

void PermissionBrokerFirewall::OnPermissionBrokerOnline(
    org::chromium::PermissionBrokerProxyInterface* proxy) {
  proxy_ = proxy;
  service_online_cb_.Run();
}

}  // namespace webservd
