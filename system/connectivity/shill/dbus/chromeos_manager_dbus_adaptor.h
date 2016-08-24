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

#ifndef SHILL_DBUS_CHROMEOS_MANAGER_DBUS_ADAPTOR_H_
#define SHILL_DBUS_CHROMEOS_MANAGER_DBUS_ADAPTOR_H_

#include <map>
#include <string>
#include <vector>

#include <base/macros.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "dbus_bindings/org.chromium.flimflam.Manager.h"
#include "shill/adaptor_interfaces.h"
#include "shill/dbus/chromeos_dbus_adaptor.h"
#include "shill/dbus/chromeos_dbus_service_watcher.h"

namespace shill {

class DBusServiceWatcherFactory;
class Manager;

// Subclass of DBusAdaptor for Manager objects
// There is a 1:1 mapping between Manager and ChromeosManagerDBusAdaptor
// instances.  Furthermore, the Manager owns the ChromeosManagerDBusAdaptor
// and manages its lifetime, so we're OK with ChromeosManagerDBusAdaptor
// having a bare pointer to its owner manager.
class ChromeosManagerDBusAdaptor
    : public org::chromium::flimflam::ManagerAdaptor,
      public org::chromium::flimflam::ManagerInterface,
      public ChromeosDBusAdaptor,
      public ManagerAdaptorInterface {
 public:
  static const char kPath[];

  ChromeosManagerDBusAdaptor(const scoped_refptr<dbus::Bus>& adaptor_bus,
                             const scoped_refptr<dbus::Bus> proxy_bus,
                             Manager* manager);
  ~ChromeosManagerDBusAdaptor() override;

  // Implementation of ManagerAdaptorInterface.
  void RegisterAsync(
      const base::Callback<void(bool)>& completion_callback) override;
  const std::string& GetRpcIdentifier() override { return dbus_path().value(); }
  void EmitBoolChanged(const std::string& name, bool value) override;
  void EmitUintChanged(const std::string& name, uint32_t value) override;
  void EmitIntChanged(const std::string& name, int value) override;
  void EmitStringChanged(const std::string& name,
                         const std::string& value) override;
  void EmitStringsChanged(const std::string& name,
                          const std::vector<std::string>& value) override;
  void EmitRpcIdentifierChanged(
      const std::string& name, const std::string& value) override;
  void EmitRpcIdentifierArrayChanged(
      const std::string& name, const std::vector<std::string>& value) override;

  // Implementation of Manager_adaptor
  bool GetProperties(brillo::ErrorPtr* error,
                     brillo::VariantDictionary* properties) override;
  bool SetProperty(brillo::ErrorPtr* error,
                   const std::string& name,
                   const brillo::Any& value) override;
  bool GetState(brillo::ErrorPtr* error, std::string* state) override;
  bool CreateProfile(brillo::ErrorPtr* error,
                     const std::string& name,
                     dbus::ObjectPath* profile_path) override;
  bool RemoveProfile(brillo::ErrorPtr* error,
                     const std::string& name) override;
  bool PushProfile(brillo::ErrorPtr* error,
                   const std::string& name,
                   dbus::ObjectPath* profile_path) override;
  bool InsertUserProfile(brillo::ErrorPtr* error,
                         const std::string& name,
                         const std::string& user_hash,
                         dbus::ObjectPath* profile_path) override;
  bool PopProfile(brillo::ErrorPtr* error, const std::string& name) override;
  bool PopAnyProfile(brillo::ErrorPtr* error) override;
  bool PopAllUserProfiles(brillo::ErrorPtr* error) override;
  bool RecheckPortal(brillo::ErrorPtr* error) override;
  bool RequestScan(brillo::ErrorPtr* error,
                   const std::string& technology) override;
  void EnableTechnology(DBusMethodResponsePtr<> response,
                        const std::string& technology_namer) override;
  void DisableTechnology(DBusMethodResponsePtr<> response,
                         const std::string& technology_name) override;
  bool GetService(brillo::ErrorPtr* error,
                  const brillo::VariantDictionary& args,
                  dbus::ObjectPath* service_path) override;
  bool GetVPNService(brillo::ErrorPtr* error,
                     const brillo::VariantDictionary& args,
                     dbus::ObjectPath* service_path) override;
  bool GetWifiService(brillo::ErrorPtr* error,
                      const brillo::VariantDictionary& args,
                      dbus::ObjectPath* service_path) override;
  bool ConfigureService(brillo::ErrorPtr* error,
                        const brillo::VariantDictionary& args,
                        dbus::ObjectPath* service_path) override;
  bool ConfigureServiceForProfile(brillo::ErrorPtr* error,
                                  const dbus::ObjectPath& profile_rpcid,
                                  const brillo::VariantDictionary& args,
                                  dbus::ObjectPath* service_path) override;
  bool FindMatchingService(brillo::ErrorPtr* error,
                           const brillo::VariantDictionary& args,
                           dbus::ObjectPath* service_path) override;
  bool GetDebugLevel(brillo::ErrorPtr* error,
                     int32_t* level) override;
  bool SetDebugLevel(brillo::ErrorPtr* error, int32_t level) override;
  bool GetServiceOrder(brillo::ErrorPtr* error, std::string* order) override;
  bool SetServiceOrder(brillo::ErrorPtr* error,
                       const std::string& order) override;
  bool GetDebugTags(brillo::ErrorPtr* error, std::string* tags) override;
  bool SetDebugTags(brillo::ErrorPtr* error,
                    const std::string& tags) override;
  bool ListDebugTags(brillo::ErrorPtr* error, std::string* tags) override;
  bool GetNetworksForGeolocation(
      brillo::ErrorPtr* error,
      brillo::VariantDictionary* networks) override;
  void VerifyDestination(DBusMethodResponsePtr<bool> response,
                         const std::string& certificate,
                         const std::string& public_key,
                         const std::string& nonce,
                         const std::string& signed_data,
                         const std::string& destination_udn,
                         const std::string& hotspot_ssid,
                         const std::string& hotspot_bssid) override;
  void VerifyAndEncryptCredentials(DBusMethodResponsePtr<std::string> response,
                                   const std::string& certificate,
                                   const std::string& public_key,
                                   const std::string& nonce,
                                   const std::string& signed_data,
                                   const std::string& destination_udn,
                                   const std::string& hotspot_ssid,
                                   const std::string& hotspot_bssid,
                                   const dbus::ObjectPath& network) override;
  void VerifyAndEncryptData(DBusMethodResponsePtr<std::string> response,
                            const std::string& certificate,
                            const std::string& public_key,
                            const std::string& nonce,
                            const std::string& signed_data,
                            const std::string& destination_udn,
                            const std::string& hotspot_ssid,
                            const std::string& hotspot_bssid,
                            const std::string& data) override;
  bool ConnectToBestServices(brillo::ErrorPtr* error) override;
  bool CreateConnectivityReport(brillo::ErrorPtr* error) override;
  bool ClaimInterface(brillo::ErrorPtr* error,
                      dbus::Message* message,
                      const std::string& claimer_name,
                      const std::string& interface_name) override;
  bool ReleaseInterface(brillo::ErrorPtr* error,
                        dbus::Message* message,
                        const std::string& claimer_name,
                        const std::string& interface_name) override;
  bool SetSchedScan(brillo::ErrorPtr* error, bool enable) override;
  bool SetupApModeInterface(brillo::ErrorPtr* error,
                            dbus::Message* message,
                            std::string* out_interface_name) override;
  bool SetupStationModeInterface(brillo::ErrorPtr* error,
                                 std::string* out_interface_name) override;

 private:
  friend class ChromeosManagerDBusAdaptorTest;
  // Tests that require access to |watcher_for_device_claimer_|.
  FRIEND_TEST(ChromeosManagerDBusAdaptorTest, ClaimInterface);
  FRIEND_TEST(ChromeosManagerDBusAdaptorTest, OnDeviceClaimerVanished);
  FRIEND_TEST(ChromeosManagerDBusAdaptorTest, ReleaseInterface);
  // Tests that require access to |watcher_for_ap_mode_setter_|.
  FRIEND_TEST(ChromeosManagerDBusAdaptorTest, OnApModeSetterVanished);
  FRIEND_TEST(ChromeosManagerDBusAdaptorTest, SetupApModeInterface);
  FRIEND_TEST(ChromeosManagerDBusAdaptorTest, SetupStationModeInterface);

  void OnApModeSetterVanished();
  void OnDeviceClaimerVanished();

  Manager* manager_;
  // We store a pointer to |proxy_bus_| in order to create a
  // ChromeosDBusServiceWatcher objects.
  scoped_refptr<dbus::Bus> proxy_bus_;
  DBusServiceWatcherFactory* dbus_service_watcher_factory_;
  std::unique_ptr<ChromeosDBusServiceWatcher> watcher_for_device_claimer_;
  std::unique_ptr<ChromeosDBusServiceWatcher> watcher_for_ap_mode_setter_;

  DISALLOW_COPY_AND_ASSIGN(ChromeosManagerDBusAdaptor);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_MANAGER_DBUS_ADAPTOR_H_
