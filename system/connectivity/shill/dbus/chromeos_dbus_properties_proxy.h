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

#ifndef SHILL_DBUS_CHROMEOS_DBUS_PROPERTIES_PROXY_H_
#define SHILL_DBUS_CHROMEOS_DBUS_PROPERTIES_PROXY_H_

#include <string>
#include <vector>

#include <base/macros.h>

#include "cellular/dbus-proxies.h"
#include "shill/dbus_properties_proxy_interface.h"

namespace shill {

// DBusPropertiesProxyInterface is a cellular-specific interface, refer to its
// header for more info.
class ChromeosDBusPropertiesProxy : public DBusPropertiesProxyInterface {
 public:
  ChromeosDBusPropertiesProxy(const scoped_refptr<dbus::Bus>& bus,
                              const std::string& path,
                              const std::string& service);
  ~ChromeosDBusPropertiesProxy() override;

  // Inherited from DBusPropertiesProxyInterface.
  KeyValueStore GetAll(const std::string& interface_name) override;
  brillo::Any Get(const std::string& interface_name,
                  const std::string& property) override;

  void set_properties_changed_callback(
      const PropertiesChangedCallback& callback) override {
    properties_changed_callback_ = callback;
  }

  void set_modem_manager_properties_changed_callback(
      const ModemManagerPropertiesChangedCallback& callback) override {
    mm_properties_changed_callback_ = callback;
  }

 private:
  // Signal handlers.
  void MmPropertiesChanged(
      const std::string& interface,
      const brillo::VariantDictionary& properties);
  void PropertiesChanged(
      const std::string& interface,
      const brillo::VariantDictionary& changed_properties,
      const std::vector<std::string>& invalidated_properties);

  // Called when signal is connected to the ObjectProxy.
  void OnSignalConnected(const std::string& interface_name,
                         const std::string& signal_name,
                         bool success);

  PropertiesChangedCallback properties_changed_callback_;
  ModemManagerPropertiesChangedCallback mm_properties_changed_callback_;

  std::unique_ptr<org::freedesktop::DBus::PropertiesProxy> proxy_;

  base::WeakPtrFactory<ChromeosDBusPropertiesProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosDBusPropertiesProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_DBUS_PROPERTIES_PROXY_H_
