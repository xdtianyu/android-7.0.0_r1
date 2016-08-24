//
// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License") override;
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

#ifndef PROXY_DBUS_SHILL_WIFI_CLIENT_H
#define PROXY_DBUS_SHILL_WIFI_CLIENT_H

#include "proxy_dbus_client.h"
#include "proxy_shill_wifi_client.h"

// This is the class implementing the ShillWifiClient abstract interface for a Dbus
// Client.
class ProxyDbusShillWifiClient : public ProxyShillWifiClient {
 public:
  ProxyDbusShillWifiClient(scoped_refptr<dbus::Bus> dbus_bus);
  ~ProxyDbusShillWifiClient() override = default;
  bool SetLogging() override;
  bool RemoveAllWifiEntries() override;
  bool ConfigureServiceByGuid(const std::string& guid,
                              AutoConnectType autoconnect,
                              const std::string& passphrase) override;
  bool ConfigureWifiService(const std::string& ssid,
                            const std::string& security,
                            const brillo::VariantDictionary& security_params,
                            bool save_credentials,
                            StationType station_type,
                            bool hidden_network,
                            const std::string& guid,
                            AutoConnectType autoconnect) override;
  bool ConnectToWifiNetwork(const std::string& ssid,
                            const std::string& security,
                            const brillo::VariantDictionary& security_params,
                            bool save_credentials,
                            StationType station_type,
                            bool hidden_network,
                            const std::string& guid,
                            AutoConnectType autoconnect,
                            long discovery_timeout_milliseconds,
                            long association_timeout_milliseconds,
                            long configuration_timeout_milliseconds,
                            long* discovery_time_milliseconds,
                            long* association_time_milliseconds,
                            long* configuration_time_milliseconds,
                            std::string* failure_reason) override;
  bool DisconnectFromWifiNetwork(const std::string& ssid,
                                 long disconnect_timeout_milliseconds,
                                 long* disconnect_time_milliseconds,
                                 std::string* failure_reason) override;
  bool ConfigureBgScan(const std::string& interface_name,
                       const std::string& method_name,
                       uint16_t short_interval,
                       uint16_t long_interval,
                       int signal_threshold) override;
  bool GetActiveWifiSsids(std::vector<std::string>* ssids) override;
  bool WaitForServiceStates(const std::string& ssid,
                            const std::vector<std::string>& expected_states,
                            long wait_timeout_milliseconds,
                            std::string* final_state,
                            long* wait_time_milliseconds) override;
  bool CreateProfile(const std::string& profile_name) override;
  bool PushProfile(const std::string& profile_name) override;
  bool PopProfile(const std::string& profile_name) override;
  bool RemoveProfile(const std::string& profile_name) override;
  bool CleanProfiles() override;
  bool DeleteEntriesForSsid(const std::string& ssid) override;
  bool ListControlledWifiInterfaces(std::vector<std::string>* interface_names) override;
  bool Disconnect(const std::string& ssid) override;
  bool GetServiceOrder(std::string* service_order) override;
  bool SetServiceOrder(const std::string& service_order) override;
  bool GetServiceProperties(const std::string& ssid,
                            brillo::VariantDictionary* properties) override;
  bool SetSchedScan(bool enable) override;
  bool GetPropertyOnDevice(const std::string& interface_name,
                           const std::string& property_name,
                           brillo::Any* property_value) override;
  bool SetPropertyOnDevice(const std::string& interface_name,
                           const std::string& property_name,
                           const brillo::Any& property_value) override;
  bool RequestRoam(const std::string& interface_name, const std::string& bssid) override;
  bool SetDeviceEnabled(const std::string& interface_name, bool enable) override;
  bool DiscoverTdlsLink(const std::string& interface_name,
                        const std::string& peer_mac_address) override;
  bool EstablishTdlsLink(const std::string& interface_name,
                         const std::string& peer_mac_address) override;
  bool QueryTdlsLink(const std::string& interface_name,
                     const std::string& peer_mac_address,
                     std::string* status) override;
  bool AddWakePacketSource(const std::string& interface_name,
                           const std::string& source_ip_address) override;
  bool RemoveWakePacketSource(const std::string& interface_name,
                              const std::string& source_ip_address) override;
  bool RemoveAllWakePacketSources(const std::string& interface_name) override;

 private:
  void SetAutoConnectInServiceParams(AutoConnectType autoconnect,
                                     brillo::VariantDictionary* service_params);
  bool PerformTdlsOperation(const std::string& interface_name,
                            const std::string& operation,
                            const std::string& peer_mac_address,
                            std::string* out_params);
  std::unique_ptr<ProxyDbusClient> dbus_client_;
};

#endif // PROXY_DBUS_SHILL_WIFI_CLIENT_H
