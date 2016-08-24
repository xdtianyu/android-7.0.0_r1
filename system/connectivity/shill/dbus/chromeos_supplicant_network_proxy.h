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

#ifndef SHILL_DBUS_CHROMEOS_SUPPLICANT_NETWORK_PROXY_H_
#define SHILL_DBUS_CHROMEOS_SUPPLICANT_NETWORK_PROXY_H_

#include <map>
#include <string>

#include <base/macros.h>

#include "shill/refptr_types.h"
#include "shill/supplicant/supplicant_network_proxy_interface.h"
#include "supplicant/dbus-proxies.h"

namespace shill {

// ChromeosSupplicantNetworkProxy. provides access to wpa_supplicant's
// network-interface APIs via D-Bus.
class ChromeosSupplicantNetworkProxy
    : public SupplicantNetworkProxyInterface {
 public:
  ChromeosSupplicantNetworkProxy(const scoped_refptr<dbus::Bus>& bus,
                                 const std::string& object_path);
  ~ChromeosSupplicantNetworkProxy() override;

  // Implementation of SupplicantNetworkProxyInterface.
  // This function will always return true, since PropertySet::Set is an
  // async method. Failures will be logged in the callback.
  bool SetEnabled(bool enabled) override;

 private:
  class PropertySet : public dbus::PropertySet {
   public:
    PropertySet(dbus::ObjectProxy* object_proxy,
                const std::string& interface_name,
                const PropertyChangedCallback& callback);
    brillo::dbus_utils::Property<bool> enabled;

   private:
    DISALLOW_COPY_AND_ASSIGN(PropertySet);
  };

  static const char kInterfaceName[];
  static const char kPropertyEnabled[];
  static const char kPropertyProperties[];

  // Signal handlers.
  void PropertiesChanged(const brillo::VariantDictionary& properties);

  // Callback invoked when the value of property |property_name| is changed.
  void OnPropertyChanged(const std::string& property_name);

  // Called when signal is connected to the ObjectProxy.
  void OnSignalConnected(const std::string& interface_name,
                         const std::string& signal_name,
                         bool success);

  std::unique_ptr<fi::w1::wpa_supplicant1::NetworkProxy> network_proxy_;
  std::unique_ptr<PropertySet> properties_;

  base::WeakPtrFactory<ChromeosSupplicantNetworkProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosSupplicantNetworkProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_SUPPLICANT_NETWORK_PROXY_H_
