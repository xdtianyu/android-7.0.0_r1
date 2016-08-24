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

#include "shill/cellular/modem.h"

#include <ModemManager/ModemManager.h>

#include "shill/cellular/cellular.h"
#include "shill/device_info.h"

using std::string;
using std::vector;

namespace shill {

Modem1::Modem1(const string& service,
               const string& path,
               ModemInfo* modem_info,
               ControlInterface* control_interface)
    : Modem(service, path, modem_info, control_interface) {}

Modem1::~Modem1() {}

bool Modem1::GetLinkName(const KeyValueStore& modem_props,
                         string* name) const {
  if (!modem_props.Contains(MM_MODEM_PROPERTY_PORTS)) {
    LOG(ERROR) << "Device missing property: " << MM_MODEM_PROPERTY_PORTS;
    return false;
  }

  auto ports = modem_props.Get(MM_MODEM_PROPERTY_PORTS).
      Get<vector<std::tuple<string, uint32_t>>>();
  string net_port;
  for (const auto& port_pair : ports) {
    if (std::get<1>(port_pair) == MM_MODEM_PORT_TYPE_NET) {
      net_port = std::get<0>(port_pair);
      break;
    }
  }

  if (net_port.empty()) {
    LOG(ERROR) << "Could not find net port used by the device.";
    return false;
  }

  *name = net_port;
  return true;
}

void Modem1::CreateDeviceMM1(const InterfaceToProperties& properties) {
  Init();
  uint32_t capabilities = std::numeric_limits<uint32_t>::max();
  InterfaceToProperties::const_iterator it =
      properties.find(MM_DBUS_INTERFACE_MODEM);
  if (it == properties.end()) {
    LOG(ERROR) << "Cellular device with no modem properties";
    return;
  }
  const KeyValueStore& modem_props = it->second;
  if (modem_props.ContainsUint(MM_MODEM_PROPERTY_CURRENTCAPABILITIES)) {
    capabilities =
      modem_props.GetUint(MM_MODEM_PROPERTY_CURRENTCAPABILITIES);
  }

  if ((capabilities & MM_MODEM_CAPABILITY_LTE) ||
      (capabilities & MM_MODEM_CAPABILITY_GSM_UMTS)) {
    set_type(Cellular::kTypeUniversal);
  } else if (capabilities & MM_MODEM_CAPABILITY_CDMA_EVDO) {
    set_type(Cellular::kTypeUniversalCDMA);
  } else {
    LOG(ERROR) << "Unsupported capabilities: " << capabilities;
    return;
  }

  // We cannot check the IP method to make sure it's not PPP. The IP
  // method will be checked later when the bearer object is fetched.
  CreateDeviceFromModemProperties(properties);
}

string Modem1::GetModemInterface(void) const {
  return string(MM_DBUS_INTERFACE_MODEM);
}

}  // namespace shill
