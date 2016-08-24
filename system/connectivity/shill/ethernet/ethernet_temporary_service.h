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

#ifndef SHILL_ETHERNET_ETHERNET_TEMPORARY_SERVICE_H_
#define SHILL_ETHERNET_ETHERNET_TEMPORARY_SERVICE_H_

#include <string>

#include "shill/service.h"

namespace shill {

class ControlInterface;
class EventDispatcher;
class Manager;
class Metrics;

// This is only use for loading non-active Ethernet service entries from the
// profile.
class EthernetTemporaryService : public Service {
 public:
  EthernetTemporaryService(ControlInterface* control_interface,
                           EventDispatcher* dispatcher,
                           Metrics* metrics,
                           Manager* manager,
                           const std::string& storage_identifier);
  ~EthernetTemporaryService() override;

  // Inherited from Service.
  std::string GetDeviceRpcId(Error* error) const override;
  std::string GetStorageIdentifier() const override;
  bool IsVisible() const override;

 private:
  std::string storage_identifier_;
  DISALLOW_COPY_AND_ASSIGN(EthernetTemporaryService);
};

}  // namespace shill

#endif  // SHILL_ETHERNET_ETHERNET_TEMPORARY_SERVICE_H_
