//
// Copyright (C) 2012 The Android Open Source Project
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

#ifndef SHILL_WIMAX_WIMAX_PROVIDER_H_
#define SHILL_WIMAX_WIMAX_PROVIDER_H_

#include <map>
#include <memory>
#include <string>

#include <base/macros.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/accessor_interface.h"
#include "shill/provider_interface.h"
#include "shill/refptr_types.h"
#include "shill/wimax/wimax_network_proxy_interface.h"

namespace shill {

class ControlInterface;
class EventDispatcher;
class KeyValueStore;
class Manager;
class Metrics;
class StoreInterface;
class WiMaxManagerProxyInterface;

class WiMaxProvider : public ProviderInterface {
 public:
  WiMaxProvider(ControlInterface* control,
                EventDispatcher* dispatcher,
                Metrics* metrics,
                Manager* manager);
  ~WiMaxProvider() override;

  // Called by Manager as a part of the Provider interface.  The attributes
  // used for matching services for the WiMax provider are the NetworkId,
  // mode and Name parameters.
  void CreateServicesFromProfile(const ProfileRefPtr& profile) override;
  ServiceRefPtr FindSimilarService(
      const KeyValueStore& args, Error* error) const override;
  ServiceRefPtr GetService(const KeyValueStore& args, Error* error) override;
  ServiceRefPtr CreateTemporaryService(
      const KeyValueStore& args, Error* error) override;
  ServiceRefPtr CreateTemporaryServiceFromProfile(
      const ProfileRefPtr& profile,
      const std::string& entry_name,
      Error* error) override;
  void Start() override;
  void Stop() override;

  // Signaled by DeviceInfo when a new WiMAX device becomes available.
  virtual void OnDeviceInfoAvailable(const std::string& link_name);

  // Signaled by a WiMAX device when its set of live networks changes.
  virtual void OnNetworksChanged();

  // Signaled by |service| when it's been unloaded by Manager. Returns true if
  // this provider has released ownership of the service, and false otherwise.
  virtual bool OnServiceUnloaded(const WiMaxServiceRefPtr& service);

  // Selects and returns a WiMAX device to connect |service| through.
  virtual WiMaxRefPtr SelectCarrier(const WiMaxServiceConstRefPtr& service);

 private:
  friend class WiMaxProviderTest;
  FRIEND_TEST(WiMaxProviderTest, ConnectDisconnectWiMaxManager);
  FRIEND_TEST(WiMaxProviderTest, CreateDevice);
  FRIEND_TEST(WiMaxProviderTest, CreateServicesFromProfile);
  FRIEND_TEST(WiMaxProviderTest, DestroyAllServices);
  FRIEND_TEST(WiMaxProviderTest, DestroyDeadDevices);
  FRIEND_TEST(WiMaxProviderTest, FindService);
  FRIEND_TEST(WiMaxProviderTest, GetLinkName);
  FRIEND_TEST(WiMaxProviderTest, GetUniqueService);
  FRIEND_TEST(WiMaxProviderTest, OnDeviceInfoAvailable);
  FRIEND_TEST(WiMaxProviderTest, OnDevicesChanged);
  FRIEND_TEST(WiMaxProviderTest, OnNetworksChanged);
  FRIEND_TEST(WiMaxProviderTest, OnServiceUnloaded);
  FRIEND_TEST(WiMaxProviderTest, RetrieveNetworkInfo);
  FRIEND_TEST(WiMaxProviderTest, SelectCarrier);
  FRIEND_TEST(WiMaxProviderTest, StartLiveServices);
  FRIEND_TEST(WiMaxProviderTest, StartStop);
  FRIEND_TEST(WiMaxProviderTest, StopDeadServices);

  struct NetworkInfo {
    WiMaxNetworkId id;
    std::string name;
  };

  void ConnectToWiMaxManager();
  void DisconnectFromWiMaxManager();
  void OnWiMaxManagerAppeared();
  void OnWiMaxManagerVanished();

  void OnDevicesChanged(const RpcIdentifiers& devices);

  void CreateDevice(const std::string& link_name, const RpcIdentifier& path);
  void DestroyDeadDevices(const RpcIdentifiers& live_devices);

  std::string GetLinkName(const RpcIdentifier& path);

  // Retrieves network info for a network at RPC |path| into |networks_| if it's
  // not already available.
  void RetrieveNetworkInfo(const RpcIdentifier& path);

  // Finds and returns the service identified by |storage_id|. Returns nullptr
  // if the service is not found.
  WiMaxServiceRefPtr FindService(const std::string& storage_id) const;

  // Finds or creates a service with the given parameters. The parameters
  // uniquely identify a service so no duplicate services will be created.
  // The service will be registered and a memeber of the provider's
  // |services_| map.
  WiMaxServiceRefPtr GetUniqueService(const WiMaxNetworkId& id,
                                      const std::string& name);

  // Allocates a service with the given parameters.
  WiMaxServiceRefPtr CreateService(const WiMaxNetworkId& id,
                                   const std::string& name);

  // Populates the |id_ptr| and |name_ptr| from the parameters in |args|.
  // Returns true on success, otheriwse populates |error| and returns false.
  static bool GetServiceParametersFromArgs(const KeyValueStore& args,
                                           WiMaxNetworkId* id_ptr,
                                           std::string* name_ptr,
                                           Error* error);

  static bool GetServiceParametersFromStorage(const StoreInterface* storage,
                                              const std::string& entry_name,
                                              WiMaxNetworkId* id_ptr,
                                              std::string* name_ptr,
                                              Error* error);

  // Starts all services with network ids in the current set of live
  // networks. This method also creates, registers and starts the default
  // service for each live network.
  void StartLiveServices();

  // Stops all services with network ids that are not in the current set of live
  // networks.
  void StopDeadServices();

  // Stops, deregisters and destroys all services.
  void DestroyAllServices();

  ControlInterface* control_;
  EventDispatcher* dispatcher_;
  Metrics* metrics_;
  Manager* manager_;

  std::unique_ptr<WiMaxManagerProxyInterface> wimax_manager_proxy_;

  // Key is the interface link name.
  std::map<std::string, RpcIdentifier> pending_devices_;
  std::map<std::string, WiMaxRefPtr> devices_;
  // Key is service's storage identifier.
  std::map<std::string, WiMaxServiceRefPtr> services_;
  std::map<RpcIdentifier, NetworkInfo> networks_;

  DISALLOW_COPY_AND_ASSIGN(WiMaxProvider);
};

}  // namespace shill

#endif  // SHILL_WIMAX_WIMAX_PROVIDER_H_
