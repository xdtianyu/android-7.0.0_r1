// Copyright 2015 The Android Open Source Project
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

#ifndef BUFFET_SHILL_CLIENT_H_
#define BUFFET_SHILL_CLIENT_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/cancelable_callback.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <dbus/bus.h>
#include <shill/dbus-proxies.h>
#include <weave/provider/network.h>
#include <weave/provider/wifi.h>

namespace buffet {

class ApManagerClient;

class ShillClient final : public weave::provider::Network,
                          public weave::provider::Wifi {
 public:
  ShillClient(const scoped_refptr<dbus::Bus>& bus,
              const std::set<std::string>& device_whitelist,
              bool disable_xmpp);
  ~ShillClient();

  // NetworkProvider implementation.
  void AddConnectionChangedCallback(
      const ConnectionChangedCallback& listener) override;
  State GetConnectionState() const override;
  void OpenSslSocket(const std::string& host,
                     uint16_t port,
                     const OpenSslSocketCallback& callback) override;

  // WifiProvider implementation.
  void Connect(const std::string& ssid,
               const std::string& passphrase,
               const weave::DoneCallback& callback) override;
  void StartAccessPoint(const std::string& ssid) override;
  void StopAccessPoint() override;
  bool IsWifi24Supported() const override { return true; }
  // TODO(avakulenko): See if we can get appropriate information from Shill
  // regarding 5.0 GHz support.
  bool IsWifi50Supported() const override { return false; }

 private:
  struct DeviceState {
    std::unique_ptr<org::chromium::flimflam::DeviceProxy> device;
    // ServiceProxy objects are shared because the connecting service will
    // also be the selected service for a device, but is not always the selected
    // service (for instance, in the period between configuring a WiFi service
    // with credentials, and when Connect() is called.)
    std::shared_ptr<org::chromium::flimflam::ServiceProxy> selected_service;
    State service_state{State::kOffline};
  };

  void Init();

  bool IsMonitoredDevice(org::chromium::flimflam::DeviceProxy* device);
  void OnShillServiceOwnerChange(const std::string& old_owner,
                                 const std::string& new_owner);
  void OnManagerPropertyChangeRegistration(const std::string& interface,
                                           const std::string& signal_name,
                                           bool success);
  void OnManagerPropertyChange(const std::string& property_name,
                               const brillo::Any& property_value);
  void OnDevicePropertyChangeRegistration(const dbus::ObjectPath& device_path,
                                          const std::string& interface,
                                          const std::string& signal_name,
                                          bool success);
  void OnDevicePropertyChange(const dbus::ObjectPath& device_path,
                              const std::string& property_name,
                              const brillo::Any& property_value);
  void OnServicePropertyChangeRegistration(const dbus::ObjectPath& path,
                                           const std::string& interface,
                                           const std::string& signal_name,
                                           bool success);
  void OnServicePropertyChange(const dbus::ObjectPath& service_path,
                               const std::string& property_name,
                               const brillo::Any& property_value);

  void OnStateChangeForConnectingService(const std::string& state);
  void OnErrorChangeForConnectingService(const std::string& error);
  void OnStrengthChangeForConnectingService(uint8_t signal_strength);
  void OnStateChangeForSelectedService(const dbus::ObjectPath& service_path,
                                       const std::string& state);
  void UpdateConnectivityState();
  void NotifyConnectivityListeners(bool am_online);
  // Clean up state related to a connecting service.
  void CleanupConnectingService();

  void ConnectToServiceError(
      std::shared_ptr<org::chromium::flimflam::ServiceProxy>
          connecting_service);

  const scoped_refptr<dbus::Bus> bus_;
  org::chromium::flimflam::ManagerProxy manager_proxy_;
  // There is logic that assumes we will never change this device list
  // in OnManagerPropertyChange.  Do not be tempted to remove this const.
  const std::set<std::string> device_whitelist_;
  bool disable_xmpp_{false};
  std::vector<ConnectionChangedCallback> connectivity_listeners_;

  // State for tracking where we are in our attempts to connect to a service.
  bool have_called_connect_{false};
  std::shared_ptr<org::chromium::flimflam::ServiceProxy> connecting_service_;
  std::string connecting_service_error_;
  weave::DoneCallback connect_done_callback_;

  // State for tracking our online connectivity.
  std::map<dbus::ObjectPath, DeviceState> devices_;
  State connectivity_state_{State::kOffline};

  std::unique_ptr<ApManagerClient> ap_manager_client_;

  base::WeakPtrFactory<ShillClient> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ShillClient);
};

}  // namespace buffet

#endif  // BUFFET_SHILL_CLIENT_H_
