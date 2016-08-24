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

#include "shill/dhcp/dhcpv6_config.h"

#include <base/files/file_util.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/dhcp/dhcp_provider.h"
#include "shill/logging.h"
#include "shill/net/ip_address.h"

using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDHCP;
static string ObjectID(DHCPv6Config* d) {
  if (d == nullptr)
    return "(DHCPv6_config)";
  else
    return d->device_name();
}
}

// static
const char DHCPv6Config::kDHCPCDPathFormatPID[] =
    "var/run/dhcpcd/dhcpcd-%s-6.pid";

const char DHCPv6Config::kConfigurationKeyDelegatedPrefix[] =
    "DHCPv6DelegatedPrefix";
const char DHCPv6Config::kConfigurationKeyDelegatedPrefixLength[] =
    "DHCPv6DelegatedPrefixLength";
const char DHCPv6Config::kConfigurationKeyDelegatedPrefixLeaseTime[] =
    "DHCPv6DelegatedPrefixLeaseTime";
const char DHCPv6Config::kConfigurationKeyDNS[] = "DHCPv6NameServers";
const char DHCPv6Config::kConfigurationKeyDomainSearch[] = "DHCPv6DomainSearch";
const char DHCPv6Config::kConfigurationKeyIPAddress[] = "DHCPv6Address";
const char DHCPv6Config::kConfigurationKeyIPAddressLeaseTime[] =
    "DHCPv6AddressLeaseTime";
const char DHCPv6Config::kConfigurationKeyServerIdentifier[] =
    "DHCPv6ServerIdentifier";

const char DHCPv6Config::kReasonBound[] = "BOUND6";
const char DHCPv6Config::kReasonFail[] = "FAIL6";
const char DHCPv6Config::kReasonRebind[] = "REBIND6";
const char DHCPv6Config::kReasonReboot[] = "REBOOT6";
const char DHCPv6Config::kReasonRenew[] = "RENEW6";

const char DHCPv6Config::kType[] = "dhcp6";

DHCPv6Config::DHCPv6Config(ControlInterface* control_interface,
                           EventDispatcher* dispatcher,
                           DHCPProvider* provider,
                           const string& device_name,
                           const string& lease_file_suffix)
    : DHCPConfig(control_interface,
                 dispatcher,
                 provider,
                 device_name,
                 kType,
                 lease_file_suffix) {
  SLOG(this, 2) << __func__ << ": " << device_name;
}

DHCPv6Config::~DHCPv6Config() {
  SLOG(this, 2) << __func__ << ": " << device_name();
}

void DHCPv6Config::ProcessEventSignal(const string& reason,
                                      const KeyValueStore& configuration) {
  LOG(INFO) << "Event reason: " << reason;
  if (reason == kReasonFail) {
    LOG(ERROR) << "Received failure event from DHCPv6 client.";
    NotifyFailure();
    return;
  } else if (reason != kReasonBound &&
             reason != kReasonRebind &&
             reason != kReasonReboot &&
             reason != kReasonRenew) {
    LOG(WARNING) << "Event ignored.";
    return;
  }

  CHECK(ParseConfiguration(configuration));

  // This needs to be set before calling UpdateProperties() below since
  // those functions may indirectly call other methods like ReleaseIP that
  // depend on or change this value.
  set_is_lease_active(true);

  DHCPConfig::UpdateProperties(properties_, true);
}

void DHCPv6Config::ProcessStatusChangeSignal(const string& status) {
  SLOG(this, 2) << __func__ << ": " << status;
  // TODO(zqiu): metric reporting for status.
}

void DHCPv6Config::CleanupClientState() {
  DHCPConfig::CleanupClientState();

  // Delete lease file if it is ephemeral.
  if (IsEphemeralLease()) {
    base::DeleteFile(root().Append(
        base::StringPrintf(DHCPProvider::kDHCPCDPathFormatLease6,
                           device_name().c_str())), false);
  }
  base::DeleteFile(root().Append(
      base::StringPrintf(kDHCPCDPathFormatPID, device_name().c_str())), false);

  // Reset configuration data.
  properties_ = IPConfig::Properties();
}

vector<string> DHCPv6Config::GetFlags() {
  // Get default flags first.
  vector<string> flags = DHCPConfig::GetFlags();

  flags.push_back("-6");  // IPv6 only.
  flags.push_back("-a");  // Request ia_na and ia_pd.
  return flags;
}

bool DHCPv6Config::ParseConfiguration(const KeyValueStore& configuration) {
  SLOG(nullptr, 2) << __func__;
  properties_.method = kTypeDHCP6;
  properties_.address_family = IPAddress::kFamilyIPv6;
  for (const auto it :  configuration.properties()) {
    const string& key = it.first;
    const brillo::Any& value = it.second;
    SLOG(nullptr, 2) << "Processing key: " << key;
    if (key == kConfigurationKeyIPAddress) {
      properties_.address = value.Get<string>();
    } else if (key == kConfigurationKeyDNS) {
      properties_.dns_servers = value.Get<vector<string>>();
    } else if (key == kConfigurationKeyDomainSearch) {
      properties_.domain_search = value.Get<vector<string>>();
    } else if (key == kConfigurationKeyIPAddressLeaseTime ||
               key == kConfigurationKeyDelegatedPrefixLeaseTime) {
      UpdateLeaseTime(value.Get<uint32_t>());
    } else if (key == kConfigurationKeyDelegatedPrefix) {
      properties_.delegated_prefix = value.Get<string>();
    } else if (key == kConfigurationKeyDelegatedPrefixLength) {
      properties_.delegated_prefix_length = value.Get<uint32_t>();
    } else {
      SLOG(nullptr, 2) << "Key ignored.";
    }
  }
  return true;
}

void DHCPv6Config::UpdateLeaseTime(uint32_t lease_time) {
  // IP address and delegated prefix are provided as separate lease. Use
  // the shorter time of the two lease as the lease time.
  if (properties_.lease_duration_seconds == 0 ||
      lease_time < properties_.lease_duration_seconds) {
    properties_.lease_duration_seconds = lease_time;
  }
}

}  // namespace shill
