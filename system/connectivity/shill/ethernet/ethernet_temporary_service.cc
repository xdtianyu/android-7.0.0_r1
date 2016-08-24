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

#include "shill/ethernet/ethernet_temporary_service.h"

#include <string>

#include "shill/control_interface.h"
#include "shill/event_dispatcher.h"
#include "shill/manager.h"
#include "shill/metrics.h"

using std::string;

namespace shill {

EthernetTemporaryService::EthernetTemporaryService(
    ControlInterface* control_interface,
    EventDispatcher* dispatcher,
    Metrics* metrics,
    Manager* manager,
    const string& storage_identifier)
  : Service(control_interface,
            dispatcher, metrics,
            manager,
            Technology::kEthernet),
    storage_identifier_(storage_identifier) {
  set_friendly_name("Ethernet");
}

EthernetTemporaryService::~EthernetTemporaryService() {}

std::string EthernetTemporaryService::GetDeviceRpcId(Error* /*error*/) const {
  return control_interface()->NullRPCIdentifier();
}

string EthernetTemporaryService::GetStorageIdentifier() const {
  return storage_identifier_;
}

bool EthernetTemporaryService::IsVisible() const {
  return false;
}

}  // namespace shill
