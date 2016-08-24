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

#ifndef SHILL_DBUS_CHROMEOS_MODEM_MANAGER_PROXY_H_
#define SHILL_DBUS_CHROMEOS_MODEM_MANAGER_PROXY_H_

#include <string>
#include <vector>

#include <base/macros.h>

#include "cellular/dbus-proxies.h"
#include "shill/cellular/modem_manager_proxy_interface.h"

namespace shill {

class EventDispatcher;
class ModemManagerClassic;

// There's a single proxy per (old) ModemManager service identified by
// its DBus |path| and owner name |service|.
class ChromeosModemManagerProxy : public ModemManagerProxyInterface {
 public:
  ChromeosModemManagerProxy(
      EventDispatcher* dispatcher,
      const scoped_refptr<dbus::Bus>& bus,
      ModemManagerClassic* manager,
      const std::string& path,
      const std::string& service,
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback);
  ~ChromeosModemManagerProxy() override;

  // Inherited from ModemManagerProxyInterface.
  std::vector<std::string> EnumerateDevices() override;

 private:
  // Signal handlers.
  void DeviceAdded(const dbus::ObjectPath& device);
  void DeviceRemoved(const dbus::ObjectPath& device);

  // Called when service appeared or vanished.
  void OnServiceAvailable(bool available);

  // Service name owner changed handler.
  void OnServiceOwnerChanged(const std::string& old_owner,
                             const std::string& new_owner);

  // Called when signal is connected to the ObjectProxy.
  void OnSignalConnected(const std::string& interface_name,
                         const std::string& signal_name,
                         bool success);

  std::unique_ptr<org::freedesktop::ModemManagerProxy> proxy_;
  EventDispatcher* dispatcher_;
  ModemManagerClassic* manager_;      // The owner of this proxy.
  base::Closure service_appeared_callback_;
  base::Closure service_vanished_callback_;
  bool service_available_;

  base::WeakPtrFactory<ChromeosModemManagerProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosModemManagerProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_MODEM_MANAGER_PROXY_H_
