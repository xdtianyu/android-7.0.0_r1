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

#ifndef SHILL_CELLULAR_CELLULAR_BEARER_H_
#define SHILL_CELLULAR_CELLULAR_BEARER_H_

#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>
#include <gtest/gtest_prod.h>

#include "shill/ipconfig.h"

namespace shill {

class DBusPropertiesProxyInterface;
class ControlInterface;

// A class for observing property changes of a bearer object exposed by
// ModemManager.
class CellularBearer {
 public:
  // Constructs a cellular bearer for observing property changes of a
  // corresponding bearer object, at the DBus path |dbus_path| of DBus service
  // |dbus_service|,  exposed by ModemManager. The ownership of |proxy_factory|
  // is not transferred, and should outlive this object.
  //
  // TODO(benchan): Use a context object approach to pass objects like
  // ControlInterface through constructor.
  CellularBearer(ControlInterface* control_interface,
                 const std::string& dbus_path,
                 const std::string& dbus_service);
  ~CellularBearer();

  // Initializes this object by creating a DBus properties proxy to observe
  // property changes of the corresponding bearer object exposed by ModemManager
  // and also fetching the current properties of the bearer.  Returns true on
  // success or false if it fails to the DBus properties proxy.
  bool Init();

  // Callback upon property changes of the bearer.
  void OnPropertiesChanged(
      const std::string& interface,
      const KeyValueStore& changed_properties,
      const std::vector<std::string>& invalidated_properties);

  const std::string& dbus_path() const { return dbus_path_; }
  const std::string& dbus_service() const { return dbus_service_; }

  bool connected() const { return connected_; }
  const std::string& data_interface() const { return data_interface_; }
  IPConfig::Method ipv4_config_method() const { return ipv4_config_method_; }
  const IPConfig::Properties* ipv4_config_properties() const {
    return ipv4_config_properties_.get();
  }
  IPConfig::Method ipv6_config_method() const { return ipv6_config_method_; }
  const IPConfig::Properties* ipv6_config_properties() const {
    return ipv6_config_properties_.get();
  }

 private:
  friend class CellularBearerTest;
  FRIEND_TEST(CellularTest, EstablishLinkDHCP);
  FRIEND_TEST(CellularTest, EstablishLinkPPP);
  FRIEND_TEST(CellularTest, EstablishLinkStatic);

  // Gets the IP configuration method and properties from |properties|.
  // |address_family| specifies the IP address family of the configuration.
  // |ipconfig_method| and |ipconfig_properties| are used to return the IP
  // configuration method and properties and should be non-NULL.
  void GetIPConfigMethodAndProperties(
      const KeyValueStore& properties,
      IPAddress::Family address_family,
      IPConfig::Method* ipconfig_method,
      std::unique_ptr<IPConfig::Properties>* ipconfig_properties) const;

  // Resets bearer properties.
  void ResetProperties();

  // Updates bearer properties by fetching the current properties of the
  // corresponding bearer object exposed by ModemManager over DBus.
  void UpdateProperties();

  // Setters for unit tests.
  void set_connected(bool connected) { connected_ = connected; }
  void set_data_interface(const std::string& data_interface) {
    data_interface_ = data_interface;
  }
  void set_ipv4_config_method(IPConfig::Method ipv4_config_method) {
    ipv4_config_method_ = ipv4_config_method;
  }
  void set_ipv4_config_properties(
      std::unique_ptr<IPConfig::Properties> ipv4_config_properties) {
    ipv4_config_properties_ = std::move(ipv4_config_properties);
  }
  void set_ipv6_config_method(IPConfig::Method ipv6_config_method) {
    ipv6_config_method_ = ipv6_config_method;
  }
  void set_ipv6_config_properties(
      std::unique_ptr<IPConfig::Properties> ipv6_config_properties) {
    ipv6_config_properties_ = std::move(ipv6_config_properties);
  }

  ControlInterface* control_interface_;
  std::string dbus_path_;
  std::string dbus_service_;
  std::unique_ptr<DBusPropertiesProxyInterface> dbus_properties_proxy_;
  bool connected_;
  std::string data_interface_;

  // If |ipv4_config_method_| is set to |IPConfig::kMethodStatic|,
  // |ipv4_config_properties_| is guaranteed to contain valid IP configuration
  // properties. Otherwise, |ipv4_config_properties_| is set to nullptr.
  // |ipv6_config_properties_| is handled similarly.
  IPConfig::Method ipv4_config_method_;
  std::unique_ptr<IPConfig::Properties> ipv4_config_properties_;
  IPConfig::Method ipv6_config_method_;
  std::unique_ptr<IPConfig::Properties> ipv6_config_properties_;

  DISALLOW_COPY_AND_ASSIGN(CellularBearer);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_CELLULAR_BEARER_H_
