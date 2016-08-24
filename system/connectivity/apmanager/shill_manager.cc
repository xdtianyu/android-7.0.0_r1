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

#include "apmanager/shill_manager.h"

#include <base/bind.h>
#include <brillo/errors/error.h>

#include "apmanager/control_interface.h"

using std::string;

namespace apmanager {

ShillManager::ShillManager() {}

ShillManager::~ShillManager() {}

void ShillManager::Init(ControlInterface* control_interface) {
  CHECK(!shill_proxy_) << "Already init";
  shill_proxy_ =
      control_interface->CreateShillProxy(
          base::Bind(&ShillManager::OnShillServiceAppeared,
                     weak_factory_.GetWeakPtr()),
          base::Bind(&ShillManager::OnShillServiceVanished,
                     weak_factory_.GetWeakPtr()));
}

void ShillManager::ClaimInterface(const string& interface_name) {
  CHECK(shill_proxy_) << "Proxy not initialize yet";
  shill_proxy_->ClaimInterface(interface_name);
  claimed_interfaces_.insert(interface_name);
}

void ShillManager::ReleaseInterface(const string& interface_name) {
  CHECK(shill_proxy_) << "Proxy not initialize yet";
  shill_proxy_->ReleaseInterface(interface_name);
  claimed_interfaces_.erase(interface_name);
}

#if defined(__BRILLO__)
bool ShillManager::SetupApModeInterface(string* interface_name) {
  CHECK(shill_proxy_) << "Proxy not initialized yet";
  return shill_proxy_->SetupApModeInterface(interface_name);
}

bool ShillManager::SetupStationModeInterface(string* interface_name) {
  CHECK(shill_proxy_) << "Proxy not initialized yet";
  return shill_proxy_->SetupStationModeInterface(interface_name);
}
#endif  // __BRILLO__

void ShillManager::OnShillServiceAppeared() {
  LOG(INFO) << __func__;
  // Claim all interfaces from shill service in case this is a new instance.
  for (const auto& interface : claimed_interfaces_) {
    shill_proxy_->ClaimInterface(interface);
  }
}

void ShillManager::OnShillServiceVanished() {
  LOG(INFO) << __func__;
}

}  // namespace apmanager
