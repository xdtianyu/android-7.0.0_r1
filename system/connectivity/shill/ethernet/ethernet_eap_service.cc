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

#include "shill/ethernet/ethernet_eap_service.h"

#include <string>

#include <base/strings/stringprintf.h>

#include "shill/eap_credentials.h"
#include "shill/ethernet/ethernet_eap_provider.h"
#include "shill/manager.h"
#include "shill/technology.h"

using std::string;

namespace shill {

EthernetEapService::EthernetEapService(ControlInterface* control_interface,
                                       EventDispatcher* dispatcher,
                                       Metrics* metrics,
                                       Manager* manager)
    : Service(control_interface, dispatcher, metrics, manager,
              Technology::kEthernetEap) {
  SetEapCredentials(new EapCredentials());
  set_friendly_name("Ethernet EAP Parameters");
}

EthernetEapService::~EthernetEapService() {}

string EthernetEapService::GetStorageIdentifier() const {
  return base::StringPrintf(
      "%s_all", Technology::NameFromIdentifier(technology()).c_str());
}

string EthernetEapService::GetDeviceRpcId(Error* /*error*/) const {
  return "/";
}

void EthernetEapService::OnEapCredentialsChanged(
    Service::UpdateCredentialsReason reason) {
  if (reason == Service::kReasonPropertyUpdate) {
    // Although the has_ever_connected_ field is not used in the
    // same manner as the other services, we still make this call
    // to maintain consistent behavior by the EAP Credential Change
    // call.
    SetHasEverConnected(false);
  }
  manager()->ethernet_eap_provider()->OnCredentialsChanged();
}

bool EthernetEapService::Unload() {
  Service::Unload();
  manager()->ethernet_eap_provider()->OnCredentialsChanged();
  return false;
}

}  // namespace shill
