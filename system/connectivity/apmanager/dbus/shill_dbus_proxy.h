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

#ifndef APMANAGER_DBUS_SHILL_DBUS_PROXY_H_
#define APMANAGER_DBUS_SHILL_DBUS_PROXY_H_

#include <string>

#include <base/macros.h>
#include <shill/dbus-proxies.h>

#include "apmanager/shill_proxy_interface.h"

namespace apmanager {

class EventDispatcher;

class ShillDBusProxy : public ShillProxyInterface {
 public:
  ShillDBusProxy(const scoped_refptr<dbus::Bus>& bus,
                 const base::Closure& service_appeared_callback,
                 const base::Closure& service_vanished_callback);
  ~ShillDBusProxy() override;

  // Implementation of ShillProxyInterface.
  bool ClaimInterface(const std::string& interface_name) override;
  bool ReleaseInterface(const std::string& interface_name) override;
#if defined(__BRILLO__)
  bool SetupApModeInterface(std::string* interface_name) override;
  bool SetupStationModeInterface(std::string* interface_name) override;
#endif  // __BRILLO__

 private:
  void OnServiceAvailable(bool service_available);
  void OnServiceOwnerChanged(const std::string& old_owner,
                             const std::string& new_owner);

  // DBus proxy for shill's manager interface.
  std::unique_ptr<org::chromium::flimflam::ManagerProxy> manager_proxy_;
  EventDispatcher* dispatcher_;
  base::Closure service_appeared_callback_;
  base::Closure service_vanished_callback_;
  bool service_available_;

  base::WeakPtrFactory<ShillDBusProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ShillDBusProxy);
};

}  // namespace apmanager

#endif  // APMANAGER_DBUS_SHILL_DBUS_PROXY_H_
