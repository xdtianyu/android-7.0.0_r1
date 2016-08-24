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

#include "apmanager/firewall_manager.h"

#include <base/bind.h>
#include <brillo/errors/error.h>

#include "apmanager/control_interface.h"

using std::string;

namespace apmanager {

namespace {
const uint16_t kDhcpServerPort = 67;
}  // namespace

FirewallManager::FirewallManager() {}

FirewallManager::~FirewallManager() {}

void FirewallManager::Init(ControlInterface* control_interface) {
  CHECK(!firewall_proxy_) << "Already started";
  firewall_proxy_ =
      control_interface->CreateFirewallProxy(
          base::Bind(&FirewallManager::OnFirewallServiceAppeared,
                     weak_factory_.GetWeakPtr()),
          base::Bind(&FirewallManager::OnFirewallServiceVanished,
                     weak_factory_.GetWeakPtr()));
}

void FirewallManager::RequestDHCPPortAccess(const std::string& interface) {
  CHECK(firewall_proxy_) << "Proxy not initialized yet";
  if (dhcp_access_interfaces_.find(interface) !=
      dhcp_access_interfaces_.end()) {
    LOG(ERROR) << "DHCP access already requested for interface: " << interface;
    return;
  }
  firewall_proxy_->RequestUdpPortAccess(interface, kDhcpServerPort);
  dhcp_access_interfaces_.insert(interface);
}

void FirewallManager::ReleaseDHCPPortAccess(const std::string& interface) {
  CHECK(firewall_proxy_) << "Proxy not initialized yet";
  if (dhcp_access_interfaces_.find(interface) ==
      dhcp_access_interfaces_.end()) {
    LOG(ERROR) << "DHCP access has not been requested for interface: "
               << interface;
    return;
  }
  firewall_proxy_->ReleaseUdpPortAccess(interface, kDhcpServerPort);
  dhcp_access_interfaces_.erase(interface);
}

void FirewallManager::OnFirewallServiceAppeared() {
  LOG(INFO) << __func__;
  RequestAllPortsAccess();
}

void FirewallManager::OnFirewallServiceVanished() {
  // Nothing need to be done.
  LOG(INFO) << __func__;
}

void FirewallManager::RequestAllPortsAccess() {
  // Request access to DHCP port for all specified interfaces.
  for (const auto& dhcp_interface : dhcp_access_interfaces_) {
    firewall_proxy_->RequestUdpPortAccess(dhcp_interface, kDhcpServerPort);
  }
}

}  // namespace apmanager
