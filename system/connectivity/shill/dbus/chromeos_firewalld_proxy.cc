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

#include "shill/dbus/chromeos_firewalld_proxy.h"

#include <string>
#include <vector>

#include "shill/logging.h"

namespace shill {

ChromeosFirewalldProxy::ChromeosFirewalldProxy(
    const scoped_refptr<dbus::Bus>& bus)
    : proxy_(new org::chromium::FirewalldProxy(bus)) {
  // TODO(zqiu): register handler for service name owner changes, to
  // automatically re-request VPN setup when firewalld is restarted.
}

ChromeosFirewalldProxy::~ChromeosFirewalldProxy() {}

bool ChromeosFirewalldProxy::RequestVpnSetup(
    const std::vector<std::string>& user_names,
    const std::string& interface) {
  // VPN already setup.
  if (!user_names_.empty() || !interface_name_.empty()) {
    LOG(ERROR) << "Already setup?";
    return false;
  }

  bool success = false;
  brillo::ErrorPtr error;
  if (!proxy_->RequestVpnSetup(user_names, interface, &success, &error)) {
    LOG(ERROR) << "Failed to request VPN setup: " << error->GetCode()
               << " " << error->GetMessage();
  }
  return success;
}

bool ChromeosFirewalldProxy::RemoveVpnSetup() {
  // No VPN setup.
  if (user_names_.empty() && interface_name_.empty()) {
    return true;
  }

  brillo::ErrorPtr error;
  bool success = false;
  if (!proxy_->RemoveVpnSetup(user_names_, interface_name_, &success, &error)) {
    LOG(ERROR) << "Failed to remove VPN setup: " << error->GetCode()
               << " " << error->GetMessage();
  }
  user_names_.clear();
  interface_name_ = "";
  return success;
}

}  // namespace shill
