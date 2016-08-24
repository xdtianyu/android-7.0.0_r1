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

#ifndef SHILL_ETHERNET_ETHERNET_EAP_SERVICE_H_
#define SHILL_ETHERNET_ETHERNET_EAP_SERVICE_H_

#include <string>

#include "shill/service.h"

namespace shill {

// The EthernetEapService contains configuraton for any Ethernet interface
// while authenticating or authenticated to a wired 802.1x endpoint.  This
// includes EAP credentials and Static IP configuration.  This service in
// itself is not connectable, but can be used by any Ethernet device during
// authentication.
class EthernetEapService : public Service {
 public:
  EthernetEapService(ControlInterface* control_interface,
                     EventDispatcher* dispatcher,
                     Metrics* metrics,
                     Manager* manager);
  ~EthernetEapService() override;

  // Inherited from Service.
  std::string GetDeviceRpcId(Error* error) const override;
  std::string GetStorageIdentifier() const override;
  bool Is8021x() const override { return true; }
  bool IsVisible() const override { return false; }
  void OnEapCredentialsChanged(
      Service::UpdateCredentialsReason reason) override;
  bool Unload() override;
};

}  // namespace shill

#endif  // SHILL_ETHERNET_ETHERNET_EAP_SERVICE_H_
