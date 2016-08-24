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

#include "shill/dbus/chromeos_permission_broker_proxy.h"

#include <string>
#include <vector>

#include "shill/logging.h"

namespace shill {

// static
const int ChromeosPermissionBrokerProxy::kInvalidHandle = -1;

ChromeosPermissionBrokerProxy::ChromeosPermissionBrokerProxy(
    const scoped_refptr<dbus::Bus>& bus)
    : proxy_(new org::chromium::PermissionBrokerProxy(bus)),
      lifeline_read_fd_(kInvalidHandle),
      lifeline_write_fd_(kInvalidHandle) {
  // TODO(zqiu): register handler for service name owner changes, to
  // automatically re-request VPN setup when permission broker is restarted.
}

ChromeosPermissionBrokerProxy::~ChromeosPermissionBrokerProxy() {}

bool ChromeosPermissionBrokerProxy::RequestVpnSetup(
    const std::vector<std::string>& user_names,
    const std::string& interface) {
  if (lifeline_read_fd_ != kInvalidHandle ||
      lifeline_write_fd_ != kInvalidHandle) {
    LOG(ERROR) << "Already setup?";
    return false;
  }

  // TODO(zqiu): move pipe creation/cleanup to the constructor and destructor.
  // No need to recreate pipe for each request.
  int fds[2];
  if (pipe(fds) != 0) {
    LOG(ERROR) << "Failed to create lifeline pipe";
    return false;
  }
  lifeline_read_fd_ = fds[0];
  lifeline_write_fd_ = fds[1];

  dbus::FileDescriptor dbus_fd(lifeline_read_fd_);
  dbus_fd.CheckValidity();
  brillo::ErrorPtr error;
  bool success = false;
  if (!proxy_->RequestVpnSetup(
      user_names, interface, dbus_fd, &success, &error)) {
    LOG(ERROR) << "Failed to request VPN setup: " << error->GetCode()
               << " " << error->GetMessage();
  }
  return success;
}

bool ChromeosPermissionBrokerProxy::RemoveVpnSetup() {
  if (lifeline_read_fd_ == kInvalidHandle &&
      lifeline_write_fd_ == kInvalidHandle) {
    return true;
  }

  close(lifeline_read_fd_);
  close(lifeline_write_fd_);
  lifeline_read_fd_ = kInvalidHandle;
  lifeline_write_fd_ = kInvalidHandle;
  brillo::ErrorPtr error;
  bool success = false;
  if (!proxy_->RemoveVpnSetup(&success, &error)) {
    LOG(ERROR) << "Failed to remove VPN setup: " << error->GetCode()
               << " " << error->GetMessage();
  }
  return success;
}

}  // namespace shill
