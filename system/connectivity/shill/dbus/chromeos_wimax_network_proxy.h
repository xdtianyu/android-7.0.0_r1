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

#ifndef SHILL_DBUS_CHROMEOS_WIMAX_NETWORK_PROXY_H_
#define SHILL_DBUS_CHROMEOS_WIMAX_NETWORK_PROXY_H_

#include <string>

#include <base/callback.h>
#include <wimax_manager/dbus-proxies.h>

#include "shill/wimax/wimax_network_proxy_interface.h"

namespace shill {

class ChromeosWiMaxNetworkProxy : public WiMaxNetworkProxyInterface {
 public:
  // Constructs a WiMaxManager.Network DBus object proxy at |path|.
  ChromeosWiMaxNetworkProxy(const scoped_refptr<dbus::Bus>& bus,
                            const std::string& rpc_identifier);
  ~ChromeosWiMaxNetworkProxy() override;

  // Inherited from WiMaxNetwokProxyInterface.
  RpcIdentifier path() const override;
  void set_signal_strength_changed_callback(
      const SignalStrengthChangedCallback& callback) override;
  uint32_t Identifier(Error* error) override;
  std::string Name(Error* error) override;
  int Type(Error* error) override;
  int CINR(Error* error) override;
  int RSSI(Error* error) override;
  int SignalStrength(Error* error) override;

 private:
  class PropertySet : public dbus::PropertySet {
   public:
    PropertySet(dbus::ObjectProxy* object_proxy,
                const std::string& interface_name,
                const PropertyChangedCallback& callback);
    brillo::dbus_utils::Property<uint32_t> identifier;
    brillo::dbus_utils::Property<std::string> name;
    brillo::dbus_utils::Property<int32_t> type;
    brillo::dbus_utils::Property<int32_t> cinr;
    brillo::dbus_utils::Property<int32_t> rssi;
    brillo::dbus_utils::Property<int32_t> signal_strength;

   private:
    DISALLOW_COPY_AND_ASSIGN(PropertySet);
  };

  static const char kPropertyIdentifier[];
  static const char kPropertyName[];
  static const char kPropertyType[];
  static const char kPropertyCINR[];
  static const char kPropertyRSSI[];
  static const char kPropertySignalStrength[];

  // Signal handler.
  void SignalStrengthChanged(int32_t signal_strength);

  // Callback invoked when the value of property |property_name| is changed.
  void OnPropertyChanged(const std::string& property_name);

  // Called when signal is connected to the ObjectProxy.
  void OnSignalConnected(const std::string& interface_name,
                         const std::string& signal_name,
                         bool success);

  std::unique_ptr<org::chromium::WiMaxManager::NetworkProxy> proxy_;
  std::unique_ptr<PropertySet> properties_;
  SignalStrengthChangedCallback signal_strength_changed_callback_;

  base::WeakPtrFactory<ChromeosWiMaxNetworkProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosWiMaxNetworkProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_WIMAX_NETWORK_PROXY_H_
