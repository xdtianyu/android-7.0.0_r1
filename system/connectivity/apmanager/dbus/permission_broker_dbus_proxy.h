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

#ifndef APMANAGER_DBUS_PERMISSION_BROKER_DBUS_PROXY_H_
#define APMANAGER_DBUS_PERMISSION_BROKER_DBUS_PROXY_H_

#include <string>

#include <base/macros.h>
#include <base/memory/scoped_ptr.h>
#include <permission_broker/dbus-proxies.h>

#include "apmanager/firewall_proxy_interface.h"

namespace apmanager {

class EventDispatcher;

class PermissionBrokerDBusProxy : public FirewallProxyInterface {
 public:
  PermissionBrokerDBusProxy(const scoped_refptr<dbus::Bus>& bus,
                            const base::Closure& service_appeared_callback,
                            const base::Closure& service_vanished_callback);
  ~PermissionBrokerDBusProxy() override;

  // Request/release access for an UDP port |port} on an interface |interface|.
  bool RequestUdpPortAccess(const std::string& interface,
                            uint16_t port) override;
  bool ReleaseUdpPortAccess(const std::string& interface,
                            uint16_t port) override;

 private:
  // Called when service appeared or vanished.
  void OnServiceAvailable(bool service_available);

  // Service name owner changed handler.
  void OnServiceOwnerChanged(const std::string& old_owner,
                            const std::string& new_owner);

  std::unique_ptr<org::chromium::PermissionBrokerProxy> proxy_;
  EventDispatcher* dispatcher_;

  // File descriptors for the two end of the pipe use for communicating with
  // permission_broker, where it will use the read end of the pipe to detect
  // when this process exits.
  int lifeline_read_fd_;
  int lifeline_write_fd_;

  base::Closure service_appeared_callback_;
  base::Closure service_vanished_callback_;
  bool service_available_;

  base::WeakPtrFactory<PermissionBrokerDBusProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(PermissionBrokerDBusProxy);
};

}  // namespace apmanager

#endif  // APMANAGER_DBUS_PERMISSION_BROKER_DBUS_PROXY_H_
