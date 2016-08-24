//
// Copyright (C) 2014 The Android Open Source Project
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

#include "shill/vpn/third_party_vpn_driver.h"

#include <algorithm>

#include <fcntl.h>
#include <unistd.h>

#include <base/posix/eintr_wrapper.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_split.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/connection.h"
#include "shill/control_interface.h"
#include "shill/device_info.h"
#include "shill/error.h"
#include "shill/file_io.h"
#include "shill/ipconfig.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/property_accessor.h"
#include "shill/store_interface.h"
#include "shill/virtual_device.h"
#include "shill/vpn/vpn_service.h"

namespace shill {

namespace Logging {

static auto kModuleLogScope = ScopeLogger::kVPN;
static std::string ObjectID(const ThirdPartyVpnDriver* v) {
  return "(third_party_vpn_driver)";
}

}  // namespace Logging

namespace {

const int32_t kConstantMaxMtu = (1 << 16) - 1;
const int32_t kConnectTimeoutSeconds = 60*5;

std::string IPAddressFingerprint(const IPAddress& address) {
  static const std::string hex_to_bin[] = {
      "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111",
      "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"};
  std::string fingerprint;
  const size_t address_length = address.address().GetLength();
  const uint8_t* raw_address = address.address().GetConstData();
  for (size_t i = 0; i < address_length; ++i) {
    fingerprint += hex_to_bin[raw_address[i] >> 4];
    fingerprint += hex_to_bin[raw_address[i] & 0xf];
  }
  return fingerprint.substr(0, address.prefix());
}

}  // namespace

const VPNDriver::Property ThirdPartyVpnDriver::kProperties[] = {
  { kProviderHostProperty, 0 },
  { kProviderTypeProperty, 0 },
  { kExtensionNameProperty, 0 },
  { kConfigurationNameProperty, 0 }
};

ThirdPartyVpnDriver* ThirdPartyVpnDriver::active_client_ = nullptr;

ThirdPartyVpnDriver::ThirdPartyVpnDriver(ControlInterface* control,
                                         EventDispatcher* dispatcher,
                                         Metrics* metrics, Manager* manager,
                                         DeviceInfo* device_info)
    : VPNDriver(dispatcher, manager, kProperties, arraysize(kProperties)),
      control_(control),
      dispatcher_(dispatcher),
      metrics_(metrics),
      device_info_(device_info),
      tun_fd_(-1),
      parameters_expected_(false) {
  file_io_ = FileIO::GetInstance();
}

ThirdPartyVpnDriver::~ThirdPartyVpnDriver() {
  Cleanup(Service::kStateIdle, Service::kFailureUnknown,
          Service::kErrorDetailsNone);
}

void ThirdPartyVpnDriver::InitPropertyStore(PropertyStore* store) {
  VPNDriver::InitPropertyStore(store);
  store->RegisterDerivedString(
      kObjectPathSuffixProperty,
      StringAccessor(
          new CustomWriteOnlyAccessor<ThirdPartyVpnDriver, std::string>(
              this, &ThirdPartyVpnDriver::SetExtensionId,
              &ThirdPartyVpnDriver::ClearExtensionId, nullptr)));
}

bool ThirdPartyVpnDriver::Load(StoreInterface* storage,
                               const std::string& storage_id) {
  bool return_value = VPNDriver::Load(storage, storage_id);
  if (adaptor_interface_ == nullptr) {
    storage->GetString(storage_id, kObjectPathSuffixProperty,
                       &object_path_suffix_);
    adaptor_interface_.reset(control_->CreateThirdPartyVpnAdaptor(this));
  }
  return return_value;
}

bool ThirdPartyVpnDriver::Save(StoreInterface* storage,
                               const std::string& storage_id,
                               bool save_credentials) {
  bool return_value = VPNDriver::Save(storage, storage_id, save_credentials);
  storage->SetString(storage_id, kObjectPathSuffixProperty,
                     object_path_suffix_);
  return return_value;
}

void ThirdPartyVpnDriver::ClearExtensionId(Error* error) {
  error->Populate(Error::kNotSupported,
                  "Clearing extension id is not supported.");
}

bool ThirdPartyVpnDriver::SetExtensionId(const std::string& value,
                                         Error* error) {
  if (adaptor_interface_ == nullptr) {
    object_path_suffix_ = value;
    adaptor_interface_.reset(control_->CreateThirdPartyVpnAdaptor(this));
    return true;
  }
  error->Populate(Error::kAlreadyExists, "Extension ID is set");
  return false;
}

void ThirdPartyVpnDriver::UpdateConnectionState(
    Service::ConnectState connection_state, std::string* error_message) {
  if (active_client_ != this) {
    error_message->append("Unexpected call");
    return;
  }
  if (service_ && connection_state == Service::kStateFailure) {
    service_->SetState(connection_state);
    Cleanup(Service::kStateFailure, Service::kFailureUnknown,
            Service::kErrorDetailsNone);
  } else if (!service_ || connection_state != Service::kStateOnline) {
    // We expect "failure" and "connected" messages from the client, but we
    // only set state for these "failure" messages. "connected" message (which
    // is corresponding to kStateOnline here) will simply be ignored.
    error_message->append("Invalid argument");
  }
}

void ThirdPartyVpnDriver::SendPacket(const std::vector<uint8_t>& ip_packet,
                                     std::string* error_message) {
  if (active_client_ != this) {
    error_message->append("Unexpected call");
    return;
  } else if (tun_fd_ < 0) {
    error_message->append("Device not open");
    return;
  } else if (file_io_->Write(tun_fd_, ip_packet.data(), ip_packet.size()) !=
             static_cast<ssize_t>(ip_packet.size())) {
    error_message->append("Partial write");
    adaptor_interface_->EmitPlatformMessage(
        static_cast<uint32_t>(PlatformMessage::kError));
  }
}

void ThirdPartyVpnDriver::ProcessIp(
    const std::map<std::string, std::string>& parameters, const char* key,
    std::string* target, bool mandatory, std::string* error_message) {
  // TODO(kaliamoorthi): Add IPV6 support.
  auto it = parameters.find(key);
  if (it != parameters.end()) {
    if (IPAddress(parameters.at(key)).family() == IPAddress::kFamilyIPv4) {
      *target = parameters.at(key);
    } else {
      error_message->append(key).append(" is not a valid IP;");
    }
  } else if (mandatory) {
    error_message->append(key).append(" is missing;");
  }
}

void ThirdPartyVpnDriver::ProcessIPArray(
    const std::map<std::string, std::string>& parameters, const char* key,
    char delimiter, std::vector<std::string>* target, bool mandatory,
    std::string* error_message, std::string* warning_message) {
  std::vector<std::string> string_array;
  auto it = parameters.find(key);
  if (it != parameters.end()) {
    string_array = base::SplitString(
        parameters.at(key), std::string{delimiter}, base::TRIM_WHITESPACE,
        base::SPLIT_WANT_ALL);

    // Eliminate invalid IPs
    for (auto value = string_array.begin(); value != string_array.end();) {
      if (IPAddress(*value).family() != IPAddress::kFamilyIPv4) {
        warning_message->append(*value + " for " + key + " is invalid;");
        value = string_array.erase(value);
      } else {
        ++value;
      }
    }

    if (!string_array.empty()) {
      target->swap(string_array);
    } else {
      error_message->append(key).append(" has no valid values or is empty;");
    }
  } else if (mandatory) {
    error_message->append(key).append(" is missing;");
  }
}

void ThirdPartyVpnDriver::ProcessIPArrayCIDR(
    const std::map<std::string, std::string>& parameters, const char* key,
    char delimiter, std::vector<std::string>* target, bool mandatory,
    std::string* error_message, std::string* warning_message) {
  std::vector<std::string> string_array;
  IPAddress address(IPAddress::kFamilyIPv4);
  auto it = parameters.find(key);
  if (it != parameters.end()) {
    string_array = base::SplitString(
        parameters.at(key), std::string{delimiter}, base::TRIM_WHITESPACE,
        base::SPLIT_WANT_ALL);

    // Eliminate invalid IPs
    for (auto value = string_array.begin(); value != string_array.end();) {
      if (!address.SetAddressAndPrefixFromString(*value)) {
        warning_message->append(*value + " for " + key + " is invalid;");
        value = string_array.erase(value);
        continue;
      }
      const std::string cidr_key = IPAddressFingerprint(address);
      if (known_cidrs_.find(cidr_key) != known_cidrs_.end()) {
        warning_message->append("Duplicate entry for " + *value + " in " + key +
                                " found;");
        value = string_array.erase(value);
      } else {
        known_cidrs_.insert(cidr_key);
        ++value;
      }
    }

    if (!string_array.empty()) {
      target->swap(string_array);
    } else {
      error_message->append(key).append(" has no valid values or is empty;");
    }
  } else if (mandatory) {
    error_message->append(key).append(" is missing;");
  }
}

void ThirdPartyVpnDriver::ProcessSearchDomainArray(
    const std::map<std::string, std::string>& parameters, const char* key,
    char delimiter, std::vector<std::string>* target, bool mandatory,
    std::string* error_message) {
  std::vector<std::string> string_array;
  auto it = parameters.find(key);
  if (it != parameters.end()) {
    string_array = base::SplitString(
        parameters.at(key), std::string{delimiter}, base::TRIM_WHITESPACE,
        base::SPLIT_WANT_ALL);

    if (!string_array.empty()) {
      target->swap(string_array);
    } else {
      error_message->append(key).append(" has no valid values or is empty;");
    }
  } else if (mandatory) {
    error_message->append(key).append(" is missing;");
  }
}

void ThirdPartyVpnDriver::ProcessInt32(
    const std::map<std::string, std::string>& parameters, const char* key,
    int32_t* target, int32_t min_value, int32_t max_value, bool mandatory,
    std::string* error_message) {
  int32_t value = 0;
  auto it = parameters.find(key);
  if (it != parameters.end()) {
    if (base::StringToInt(parameters.at(key), &value) && value >= min_value &&
        value <= max_value) {
      *target = value;
    } else {
      error_message->append(key).append(" not in expected range;");
    }
  } else if (mandatory) {
    error_message->append(key).append(" is missing;");
  }
}

void ThirdPartyVpnDriver::SetParameters(
    const std::map<std::string, std::string>& parameters,
    std::string* error_message, std::string* warning_message) {
  // TODO(kaliamoorthi): Add IPV6 support.
  if (!parameters_expected_ || active_client_ != this) {
    error_message->append("Unexpected call");
    return;
  }

  ip_properties_ = IPConfig::Properties();
  ip_properties_.address_family = IPAddress::kFamilyIPv4;

  ProcessIp(parameters, kAddressParameterThirdPartyVpn, &ip_properties_.address,
            true, error_message);
  ProcessIp(parameters, kBroadcastAddressParameterThirdPartyVpn,
            &ip_properties_.broadcast_address, false, error_message);

  ip_properties_.gateway = ip_properties_.address;

  ProcessInt32(parameters, kSubnetPrefixParameterThirdPartyVpn,
               &ip_properties_.subnet_prefix, 0, 32, true, error_message);
  ProcessInt32(parameters, kMtuParameterThirdPartyVpn, &ip_properties_.mtu,
               IPConfig::kMinIPv4MTU, kConstantMaxMtu, false, error_message);

  ProcessSearchDomainArray(parameters, kDomainSearchParameterThirdPartyVpn,
                           kNonIPDelimiter, &ip_properties_.domain_search,
                           false, error_message);
  ProcessIPArray(parameters, kDnsServersParameterThirdPartyVpn, kIPDelimiter,
                 &ip_properties_.dns_servers, true, error_message,
                 warning_message);

  known_cidrs_.clear();

  ProcessIPArrayCIDR(parameters, kExclusionListParameterThirdPartyVpn,
                     kIPDelimiter, &ip_properties_.exclusion_list, true,
                     error_message, warning_message);
  if (!ip_properties_.exclusion_list.empty()) {
    // The first excluded IP is used to find the default gateway. The logic that
    // finds the default gateway does not work for default route "0.0.0.0/0".
    // Hence, this code ensures that the first IP is not default.
    IPAddress address(ip_properties_.address_family);
    address.SetAddressAndPrefixFromString(ip_properties_.exclusion_list[0]);
    if (address.IsDefault() && !address.prefix()) {
      if (ip_properties_.exclusion_list.size() > 1) {
        swap(ip_properties_.exclusion_list[0],
             ip_properties_.exclusion_list[1]);
      } else {
        // When there is only a single entry which is a default address, it can
        // be cleared since the default behavior is to not route any traffic to
        // the tunnel interface.
        ip_properties_.exclusion_list.clear();
      }
    }
  }

  std::vector<std::string> inclusion_list;
  ProcessIPArrayCIDR(parameters, kInclusionListParameterThirdPartyVpn,
                     kIPDelimiter, &inclusion_list, true, error_message,
                     warning_message);

  IPAddress ip_address(ip_properties_.address_family);
  IPConfig::Route route;
  route.gateway = ip_properties_.gateway;
  for (auto value = inclusion_list.begin(); value != inclusion_list.end();
       ++value) {
    ip_address.SetAddressAndPrefixFromString(*value);
    ip_address.IntoString(&route.host);
    IPAddress::GetAddressMaskFromPrefix(
        ip_address.family(), ip_address.prefix()).IntoString(&route.netmask);
    ip_properties_.routes.push_back(route);
  }

  if (error_message->empty()) {
    ip_properties_.user_traffic_only = true;
    ip_properties_.default_route = false;
    ip_properties_.blackhole_ipv6 = true;
    device_->SelectService(service_);
    device_->UpdateIPConfig(ip_properties_);
    device_->SetLooseRouting(true);
    StopConnectTimeout();
    parameters_expected_ = false;
  }
}

void ThirdPartyVpnDriver::OnInput(InputData* data) {
  // TODO(kaliamoorthi): This is not efficient, transfer the descriptor over to
  // chrome browser or use a pipe in between. Avoid using DBUS for packet
  // transfer.
  std::vector<uint8_t> ip_packet(data->buf, data->buf + data->len);
  adaptor_interface_->EmitPacketReceived(ip_packet);
}

void ThirdPartyVpnDriver::OnInputError(const std::string& error) {
  LOG(ERROR) << error;
  CHECK_EQ(active_client_, this);
  adaptor_interface_->EmitPlatformMessage(
      static_cast<uint32_t>(PlatformMessage::kError));
}

void ThirdPartyVpnDriver::Cleanup(Service::ConnectState state,
                                  Service::ConnectFailure failure,
                                  const std::string& error_details) {
  SLOG(this, 2) << __func__ << "(" << Service::ConnectStateToString(state)
                << ", " << error_details << ")";
  StopConnectTimeout();
  int interface_index = -1;
  if (device_) {
    interface_index = device_->interface_index();
    device_->DropConnection();
    device_->SetEnabled(false);
    device_ = nullptr;
  }
  if (interface_index >= 0) {
    device_info_->DeleteInterface(interface_index);
  }
  tunnel_interface_.clear();
  if (service_) {
    if (state == Service::kStateFailure) {
      service_->SetErrorDetails(error_details);
      service_->SetFailure(failure);
    } else {
      service_->SetState(state);
    }
    service_ = nullptr;
  }
  if (tun_fd_ > 0) {
    file_io_->Close(tun_fd_);
    tun_fd_ = -1;
  }
  io_handler_.reset();
  if (active_client_ == this) {
    adaptor_interface_->EmitPlatformMessage(
        static_cast<uint32_t>(PlatformMessage::kDisconnected));
    active_client_ = nullptr;
  }
  parameters_expected_ = false;
}

void ThirdPartyVpnDriver::Connect(const VPNServiceRefPtr& service,
                                  Error* error) {
  SLOG(this, 2) << __func__;
  CHECK(adaptor_interface_);
  CHECK(!active_client_);
  StartConnectTimeout(kConnectTimeoutSeconds);
  ip_properties_ = IPConfig::Properties();
  service_ = service;
  service_->SetState(Service::kStateConfiguring);
  if (!device_info_->CreateTunnelInterface(&tunnel_interface_)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInternalError,
                          "Could not create tunnel interface.");
    Cleanup(Service::kStateFailure, Service::kFailureInternal,
            "Unable to create tun interface");
  }
  // Wait for the ClaimInterface callback to continue the connection process.
}

bool ThirdPartyVpnDriver::ClaimInterface(const std::string& link_name,
                                         int interface_index) {
  if (link_name != tunnel_interface_) {
    return false;
  }
  CHECK(!active_client_);

  SLOG(this, 2) << "Claiming " << link_name << " for third party VPN tunnel";

  CHECK(!device_);
  device_ = new VirtualDevice(control_, dispatcher(), metrics_, manager(),
                              link_name, interface_index, Technology::kVPN);
  device_->SetEnabled(true);

  tun_fd_ = device_info_->OpenTunnelInterface(tunnel_interface_);
  if (tun_fd_ < 0) {
    Cleanup(Service::kStateFailure, Service::kFailureInternal,
            "Unable to open tun interface");
  } else {
    io_handler_.reset(dispatcher_->CreateInputHandler(
        tun_fd_,
        base::Bind(&ThirdPartyVpnDriver::OnInput, base::Unretained(this)),
        base::Bind(&ThirdPartyVpnDriver::OnInputError,
                   base::Unretained(this))));
    active_client_ = this;
    parameters_expected_ = true;
    adaptor_interface_->EmitPlatformMessage(
        static_cast<uint32_t>(PlatformMessage::kConnected));
  }
  return true;
}

void ThirdPartyVpnDriver::Disconnect() {
  SLOG(this, 2) << __func__;
  CHECK(adaptor_interface_);
  if (active_client_ == this) {
    Cleanup(Service::kStateIdle, Service::kFailureUnknown,
            Service::kErrorDetailsNone);
  }
}

std::string ThirdPartyVpnDriver::GetProviderType() const {
  return std::string(kProviderThirdPartyVpn);
}

void ThirdPartyVpnDriver::OnConnectionDisconnected() {
  Cleanup(Service::kStateFailure, Service::kFailureInternal,
          "Underlying network disconnected.");
}

void ThirdPartyVpnDriver::OnConnectTimeout() {
  SLOG(this, 2) << __func__;
  VPNDriver::OnConnectTimeout();
  adaptor_interface_->EmitPlatformMessage(
      static_cast<uint32_t>(PlatformMessage::kError));
  Cleanup(Service::kStateFailure, Service::kFailureConnect,
          "Connection timed out");
}

}  // namespace shill
