// Copyright 2014 The Android Open Source Project
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

#include "firewall_service.h"

#include "dbus_interface.h"
#include "iptables.h"

namespace firewalld {

FirewallService::FirewallService(
    brillo::dbus_utils::ExportedObjectManager* object_manager)
    : org::chromium::FirewalldAdaptor(&iptables_),
      dbus_object_{object_manager, object_manager->GetBus(),
                   org::chromium::FirewalldAdaptor::GetObjectPath()} {}

void FirewallService::RegisterAsync(const CompletionAction& callback) {
  RegisterWithDBusObject(&dbus_object_);

#if !defined(__ANDROID__)
  // Track permission_broker's lifetime so that we can close firewall holes
  // if/when permission_broker exits.
  permission_broker_.reset(
      new org::chromium::PermissionBroker::ObjectManagerProxy(
          dbus_object_.GetBus()));
  permission_broker_->SetPermissionBrokerRemovedCallback(
      base::Bind(&FirewallService::OnPermissionBrokerRemoved,
                 weak_ptr_factory_.GetWeakPtr()));
#endif  // __ANDROID__

  dbus_object_.RegisterAsync(callback);
}

#if !defined(__ANDROID__)
void FirewallService::OnPermissionBrokerRemoved(const dbus::ObjectPath& path) {
  LOG(INFO) << "permission_broker died, plugging all firewall holes";
  iptables_.PlugAllHoles();
}
#endif  // __ANDROID__

}  // namespace firewalld
