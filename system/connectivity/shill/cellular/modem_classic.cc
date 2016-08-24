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

#include "shill/cellular/modem.h"

#include <mm/mm-modem.h>

#include "shill/cellular/cellular.h"

using std::string;
using std::vector;

namespace shill {

ModemClassic::ModemClassic(const string& service,
                           const string& path,
                           ModemInfo* modem_info,
                           ControlInterface* control_interface)
    : Modem(service, path, modem_info, control_interface) {}

ModemClassic::~ModemClassic() {}

bool ModemClassic::GetLinkName(const KeyValueStore& modem_properties,
                               string* name) const {
  if (!modem_properties.ContainsString(kPropertyLinkName)) {
    return false;
  }
  *name = modem_properties.GetString(kPropertyLinkName);
  return true;
}

void ModemClassic::CreateDeviceClassic(
    const KeyValueStore& modem_properties) {
  Init();
  uint32_t mm_type = std::numeric_limits<uint32_t>::max();
  if (modem_properties.ContainsUint(kPropertyType)) {
    mm_type = modem_properties.GetUint(kPropertyType);
  }
  switch (mm_type) {
    case MM_MODEM_TYPE_CDMA:
      set_type(Cellular::kTypeCDMA);
      break;
    case MM_MODEM_TYPE_GSM:
      set_type(Cellular::kTypeGSM);
      break;
    default:
      LOG(ERROR) << "Unsupported cellular modem type: " << mm_type;
      return;
  }
  uint32_t ip_method = std::numeric_limits<uint32_t>::max();
  if (!modem_properties.ContainsUint(kPropertyIPMethod) ||
      (ip_method = modem_properties.GetUint(kPropertyIPMethod)) !=
          MM_MODEM_IP_METHOD_DHCP) {
    LOG(ERROR) << "Unsupported IP method: " << ip_method;
    return;
  }

  InterfaceToProperties properties;
  properties[MM_MODEM_INTERFACE] = modem_properties;
  CreateDeviceFromModemProperties(properties);
}

string ModemClassic::GetModemInterface(void) const {
  return string(MM_MODEM_INTERFACE);
}

}  // namespace shill
