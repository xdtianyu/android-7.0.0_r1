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

#include "shill/ppp_device.h"

#include <base/stl_util.h>
#include <base/strings/string_number_conversions.h>

#include "shill/logging.h"
#include "shill/metrics.h"
#include "shill/technology.h"

using std::map;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kPPP;
static string ObjectID(PPPDevice* p) { return p->link_name(); }
}

PPPDevice::PPPDevice(ControlInterface* control,
                     EventDispatcher* dispatcher,
                     Metrics* metrics,
                     Manager* manager,
                     const string& link_name,
                     int interface_index)
    : VirtualDevice(control, dispatcher, metrics, manager, link_name,
                    interface_index, Technology::kPPP) {}

PPPDevice::~PPPDevice() {}

void PPPDevice::UpdateIPConfigFromPPP(const map<string, string>& configuration,
                                      bool blackhole_ipv6) {
  SLOG(this, 2) << __func__ << " on " << link_name();
  IPConfig::Properties properties =
      ParseIPConfiguration(link_name(), configuration);
  properties.blackhole_ipv6 = blackhole_ipv6;
  UpdateIPConfig(properties);
}

void PPPDevice::UpdateIPConfigFromPPPWithMTU(
    const map<string, string>& configuration,
    bool blackhole_ipv6,
    int32_t mtu) {
  SLOG(this, 2) << __func__ << " on " << link_name();
  IPConfig::Properties properties =
      ParseIPConfiguration(link_name(), configuration);
  properties.blackhole_ipv6 = blackhole_ipv6;
  properties.mtu = mtu;
  UpdateIPConfig(properties);
}

#ifndef DISABLE_DHCPV6
bool PPPDevice::AcquireIPv6Config() {
  return AcquireIPv6ConfigWithLeaseName(string());
}
#endif

// static
string PPPDevice::GetInterfaceName(const map<string, string>& configuration) {
  if (ContainsKey(configuration, kPPPInterfaceName)) {
    return configuration.find(kPPPInterfaceName)->second;
  }
  return string();
}

IPConfig::Properties PPPDevice::ParseIPConfiguration(
    const string& link_name, const map<string, string>& configuration) {
  SLOG(PPP, nullptr, 2) << __func__ << " on " << link_name;
  IPConfig::Properties properties;
  properties.address_family = IPAddress::kFamilyIPv4;
  properties.subnet_prefix = IPAddress::GetMaxPrefixLength(
      properties.address_family);
  for (const auto& it : configuration)  {
    const string& key = it.first;
    const string& value = it.second;
    SLOG(PPP, nullptr, 2) << "Processing: " << key << " -> " << value;
    if (key == kPPPInternalIP4Address) {
      properties.address = value;
    } else if (key == kPPPExternalIP4Address) {
      properties.peer_address = value;
    } else if (key == kPPPGatewayAddress) {
      properties.gateway = value;
    } else if (key == kPPPDNS1) {
      properties.dns_servers.insert(properties.dns_servers.begin(), value);
    } else if (key == kPPPDNS2) {
      properties.dns_servers.push_back(value);
    } else if (key == kPPPLNSAddress) {
      // This is really a L2TPIPSec property. But it's sent to us by
      // our PPP plugin.
      size_t prefix = IPAddress::GetMaxPrefixLength(properties.address_family);
      properties.exclusion_list.push_back(value + "/" +
                                          base::SizeTToString(prefix));
    } else if (key == kPPPMRU) {
      int mru;
      if (!base::StringToInt(value, &mru)) {
        LOG(WARNING) << "Failed to parse MRU: " << value;
        continue;
      }
      properties.mtu = mru;
      metrics()->SendSparseToUMA(Metrics::kMetricPPPMTUValue, mru);
    } else {
      SLOG(PPP, nullptr, 2) << "Key ignored.";
    }
  }
  if (properties.gateway.empty()) {
    // The gateway may be unspecified, since this is a point-to-point
    // link. Set to the peer's address, so that Connection can set the
    // routing table.
    properties.gateway = properties.peer_address;
  }
  return properties;
}

}  // namespace shill
