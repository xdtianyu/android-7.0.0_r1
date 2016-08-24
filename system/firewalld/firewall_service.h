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

#ifndef FIREWALLD_FIREWALL_SERVICE_H_
#define FIREWALLD_FIREWALL_SERVICE_H_

#include <base/callback.h>
#include <base/macros.h>
#include <base/memory/scoped_ptr.h>
#include <base/memory/weak_ptr.h>
#include <brillo/dbus/dbus_object.h>

#include "dbus_bindings/org.chromium.Firewalld.h"
#if !defined(__ANDROID__)
# include "permission_broker/dbus-proxies.h"
#endif  // __ANDROID__

#include "iptables.h"

using CompletionAction =
    brillo::dbus_utils::AsyncEventSequencer::CompletionAction;

namespace firewalld {

class FirewallService : public org::chromium::FirewalldAdaptor {
 public:
  explicit FirewallService(
      brillo::dbus_utils::ExportedObjectManager* object_manager);
  virtual ~FirewallService() = default;

  // Connects to D-Bus system bus and exports methods.
  void RegisterAsync(const CompletionAction& callback);

 private:
#if !defined(__ANDROID__)
  void OnPermissionBrokerRemoved(const dbus::ObjectPath& path);
#endif  // __ANDROID__

  brillo::dbus_utils::DBusObject dbus_object_;
#if !defined(__ANDROID__)
  std::unique_ptr<org::chromium::PermissionBroker::ObjectManagerProxy>
      permission_broker_;
#endif  // __ANDROID__
  IpTables iptables_;

  base::WeakPtrFactory<FirewallService> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(FirewallService);
};

}  // namespace firewalld

#endif  // FIREWALLD_FIREWALL_SERVICE_H_
