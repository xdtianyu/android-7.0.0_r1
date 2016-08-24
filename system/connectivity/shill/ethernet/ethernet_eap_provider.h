//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_ETHERNET_ETHERNET_EAP_PROVIDER_H_
#define SHILL_ETHERNET_ETHERNET_EAP_PROVIDER_H_

#include <map>
#include <string>

#include <base/callback.h>

#include "shill/provider_interface.h"
#include "shill/refptr_types.h"

namespace shill {

class ControlInterface;
class Error;
class Ethernet;
class EventDispatcher;
class KeyValueStore;
class Manager;
class Metrics;

class EthernetEapProvider : public ProviderInterface {
 public:
  typedef base::Callback<void()> CredentialChangeCallback;

  EthernetEapProvider(ControlInterface* control_interface,
                      EventDispatcher* dispatcher,
                      Metrics* metrics,
                      Manager* manager);
  ~EthernetEapProvider() override;

  // Called by Manager as a part of the Provider interface.
  void CreateServicesFromProfile(const ProfileRefPtr& profile) override;
  ServiceRefPtr GetService(const KeyValueStore& args, Error* error) override;
  ServiceRefPtr FindSimilarService(
      const KeyValueStore& args, Error* error) const override;
  ServiceRefPtr CreateTemporaryService(
      const KeyValueStore& args, Error* error) override;
  ServiceRefPtr CreateTemporaryServiceFromProfile(
      const ProfileRefPtr& profile,
      const std::string& entry_name,
      Error* error) override;
  void Start() override;
  void Stop() override;

  virtual const ServiceRefPtr& service() const { return service_; }

  // Notify |device| via |callback| when EAP credentials have changed.
  // Any previous callbacks for |device| are removed.  |device| is
  // only used a key to a map and is never dereferenced.
  virtual void SetCredentialChangeCallback(Ethernet* device,
                                           CredentialChangeCallback callback);

  // Clear any previously registered callback for |device|.
  virtual void ClearCredentialChangeCallback(Ethernet* device);

  // Called by |service_| when EAP credentials are changed.  Notify all
  // listening Ethernet devices.
  virtual void OnCredentialsChanged() const;

 private:
  friend class EthernetEapProviderTest;
  friend class EthernetTest;
  friend class ManagerTest;

  // Used only in Ethernet and Manager unit tests.
  // TODO(gauravsh): Remove this and allow mocks to work correctly
  // crbug.com/232134
  void set_service(const ServiceRefPtr& service) { service_ = service; }

  typedef std::map<Ethernet*, CredentialChangeCallback> CallbackMap;

  // Representative service on which EAP credentials are configured.
  ServiceRefPtr service_;

  // Keyed set of notifiers to call when the EAP credentials for |service_|
  // have changed.
  CallbackMap callback_map_;

  ControlInterface* control_interface_;
  EventDispatcher* dispatcher_;
  Metrics* metrics_;
  Manager* manager_;
};

}  // namespace shill

#endif  // SHILL_ETHERNET_ETHERNET_EAP_PROVIDER_H_
