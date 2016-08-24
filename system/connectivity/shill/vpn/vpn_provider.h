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

#ifndef SHILL_VPN_VPN_PROVIDER_H_
#define SHILL_VPN_VPN_PROVIDER_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/provider_interface.h"
#include "shill/refptr_types.h"

namespace shill {

class ControlInterface;
class Error;
class EventDispatcher;
class KeyValueStore;
class Manager;
class Metrics;
class StoreInterface;

class VPNProvider : public ProviderInterface {
 public:
  VPNProvider(ControlInterface* control_interface,
              EventDispatcher* dispatcher,
              Metrics* metrics,
              Manager* manager);
  ~VPNProvider() override;

  // Called by Manager as a part of the Provider interface.  The attributes
  // used for matching services for the VPN provider are the ProviderType,
  // ProviderHost mode and Name parameters.
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

  // Offers an unclaimed interface to VPN services.  Returns true if this
  // device has been accepted by a service.
  virtual bool OnDeviceInfoAvailable(const std::string& link_name,
                                     int interface_index);

  // Clean up a VPN services that has been unloaded and will be deregistered.
  // This removes the VPN provider's reference to this service in its
  // services_ vector.
  void RemoveService(VPNServiceRefPtr service);

  // Returns true if any of the managed VPN services is connecting or connected.
  virtual bool HasActiveService() const;

  // Disconnect any other active VPN services.
  virtual void DisconnectAll();

 private:
  friend class VPNProviderTest;
  FRIEND_TEST(VPNProviderTest, CreateService);
  FRIEND_TEST(VPNProviderTest, OnDeviceInfoAvailable);
  FRIEND_TEST(VPNProviderTest, RemoveService);
  FRIEND_TEST(VPNServiceTest, Unload);

  // Create a service of type |type| and storage identifier |storage_id|
  // and initial parameters |args|.  Returns a service reference pointer
  // to the newly created service, or populates |error| with an the error
  // that caused this to fail.
  VPNServiceRefPtr CreateServiceInner(const std::string& type,
                                      const std::string& name,
                                      const std::string& storage_id,
                                      Error* error);

  // Calls CreateServiceInner above, and on success registers and adds this
  // service to the provider's list.
  VPNServiceRefPtr CreateService(const std::string& type,
                                 const std::string& name,
                                 const std::string& storage_id,
                                 Error* error);

  // Finds a service of type |type| with its Name property set to |name| and its
  // Provider.Host property set to |host|.
  VPNServiceRefPtr FindService(const std::string& type,
                               const std::string& name,
                               const std::string& host) const;

  // Populates |type_ptr|, |name_ptr| and |host_ptr| with the appropriate
  // values from |args|.  Returns True on success, otherwise if any of
  // these arguments are not available, |error| is populated and False is
  // returned.
  static bool GetServiceParametersFromArgs(const KeyValueStore& args,
                                           std::string* type_ptr,
                                           std::string* name_ptr,
                                           std::string* host_ptr,
                                           Error* error);
  // Populates |vpn_type_ptr|, |name_ptr| and |host_ptr| with the appropriate
  // values from profile storgae.  Returns True on success, otherwise if any of
  // these arguments are not available, |error| is populated and False is
  // returned.
  static bool GetServiceParametersFromStorage(const StoreInterface* storage,
                                              const std::string& entry_name,
                                              std::string* vpn_type_ptr,
                                              std::string* name_ptr,
                                              std::string* host_ptr,
                                              Error* error);

  ControlInterface* control_interface_;
  EventDispatcher* dispatcher_;
  Metrics* metrics_;
  Manager* manager_;
  std::vector<VPNServiceRefPtr> services_;

  DISALLOW_COPY_AND_ASSIGN(VPNProvider);
};

}  // namespace shill

#endif  // SHILL_VPN_VPN_PROVIDER_H_
