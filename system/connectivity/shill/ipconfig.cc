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

#include "shill/ipconfig.h"

#include <sys/time.h>

#include <limits>

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/adaptor_interfaces.h"
#include "shill/control_interface.h"
#include "shill/error.h"
#include "shill/logging.h"
#include "shill/net/shill_time.h"
#include "shill/static_ip_parameters.h"

using base::Callback;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kInet;
static string ObjectID(IPConfig* i) { return i->GetRpcIdentifier(); }
}

namespace {

const time_t kDefaultLeaseExpirationTime = std::numeric_limits<long>::max();

}  // namespace

// static
const int IPConfig::kDefaultMTU = 1500;
const int IPConfig::kMinIPv4MTU = 576;
const int IPConfig::kMinIPv6MTU = 1280;
const int IPConfig::kUndefinedMTU = 0;
const char IPConfig::kType[] = "ip";

// static
uint IPConfig::global_serial_ = 0;

IPConfig::IPConfig(ControlInterface* control_interface,
                   const std::string& device_name)
    : device_name_(device_name),
      type_(kType),
      serial_(global_serial_++),
      adaptor_(control_interface->CreateIPConfigAdaptor(this)) {
  Init();
}

IPConfig::IPConfig(ControlInterface* control_interface,
                   const std::string& device_name,
                   const std::string& type)
    : device_name_(device_name),
      type_(type),
      serial_(global_serial_++),
      adaptor_(control_interface->CreateIPConfigAdaptor(this)) {
  Init();
}

void IPConfig::Init() {
  store_.RegisterConstString(kAddressProperty, &properties_.address);
  store_.RegisterConstString(kBroadcastProperty,
                             &properties_.broadcast_address);
  store_.RegisterConstString(kDomainNameProperty, &properties_.domain_name);
  store_.RegisterConstString(kAcceptedHostnameProperty,
                             &properties_.accepted_hostname);
  store_.RegisterConstString(kGatewayProperty, &properties_.gateway);
  store_.RegisterConstString(kMethodProperty, &properties_.method);
  store_.RegisterConstInt32(kMtuProperty, &properties_.mtu);
  store_.RegisterConstStrings(kNameServersProperty, &properties_.dns_servers);
  store_.RegisterConstString(kPeerAddressProperty, &properties_.peer_address);
  store_.RegisterConstInt32(kPrefixlenProperty, &properties_.subnet_prefix);
  store_.RegisterConstStrings(kSearchDomainsProperty,
                              &properties_.domain_search);
  store_.RegisterConstByteArray(kVendorEncapsulatedOptionsProperty,
                                &properties_.vendor_encapsulated_options);
  store_.RegisterConstString(kWebProxyAutoDiscoveryUrlProperty,
                             &properties_.web_proxy_auto_discovery);
  store_.RegisterConstString(kDelegatedPrefixProperty,
                             &properties_.delegated_prefix);
  store_.RegisterConstInt32(kDelegatedPrefixLengthProperty,
                            &properties_.delegated_prefix_length);
  store_.RegisterConstUint32(kLeaseDurationSecondsProperty,
                             &properties_.lease_duration_seconds);
  time_ = Time::GetInstance();
  current_lease_expiration_time_ = {kDefaultLeaseExpirationTime, 0};
  SLOG(this, 2) << __func__ << " device: " << device_name();
}

IPConfig::~IPConfig() {
  SLOG(this, 2) << __func__ << " device: " << device_name();
}

string IPConfig::GetRpcIdentifier() {
  return adaptor_->GetRpcIdentifier();
}

bool IPConfig::RequestIP() {
  return false;
}

bool IPConfig::RenewIP() {
  return false;
}

bool IPConfig::ReleaseIP(ReleaseReason reason) {
  return false;
}

void IPConfig::Refresh(Error* /*error*/) {
  if (!refresh_callback_.is_null()) {
    refresh_callback_.Run(this);
  }
  RenewIP();
}

void IPConfig::ApplyStaticIPParameters(
    StaticIPParameters* static_ip_parameters) {
  static_ip_parameters->ApplyTo(&properties_);
  EmitChanges();
}

void IPConfig::RestoreSavedIPParameters(
    StaticIPParameters* static_ip_parameters) {
  static_ip_parameters->RestoreTo(&properties_);
  EmitChanges();
}

void IPConfig::UpdateLeaseExpirationTime(uint32_t new_lease_duration) {
  struct timeval new_expiration_time;
  time_->GetTimeBoottime(&new_expiration_time);
  new_expiration_time.tv_sec += new_lease_duration;
  current_lease_expiration_time_ = new_expiration_time;
}

void IPConfig::ResetLeaseExpirationTime() {
  current_lease_expiration_time_ = {kDefaultLeaseExpirationTime, 0};
}

bool IPConfig::TimeToLeaseExpiry(uint32_t* time_left) {
  if (current_lease_expiration_time_.tv_sec == kDefaultLeaseExpirationTime) {
    SLOG(this, 2) << __func__ << ": No current DHCP lease";
    return false;
  }
  struct timeval now;
  time_->GetTimeBoottime(&now);
  if (now.tv_sec > current_lease_expiration_time_.tv_sec) {
    SLOG(this, 2) << __func__ << ": Current DHCP lease has already expired";
    return false;
  }
  *time_left = current_lease_expiration_time_.tv_sec - now.tv_sec;
  return true;
}

void IPConfig::UpdateProperties(const Properties& properties,
                                bool new_lease_acquired) {
  // Take a reference of this instance to make sure we don't get destroyed in
  // the middle of this call. (The |update_callback_| may cause a reference
  // to be dropped. See, e.g., EthernetService::Disconnect and
  // Ethernet::DropConnection.)
  IPConfigRefPtr me = this;

  properties_ = properties;

  if (!update_callback_.is_null()) {
    update_callback_.Run(this, new_lease_acquired);
  }
  EmitChanges();
}

void IPConfig::UpdateDNSServers(const std::vector<std::string>& dns_servers) {
  properties_.dns_servers = dns_servers;
  EmitChanges();
}

void IPConfig::NotifyFailure() {
  // Take a reference of this instance to make sure we don't get destroyed in
  // the middle of this call. (The |update_callback_| may cause a reference
  // to be dropped. See, e.g., EthernetService::Disconnect and
  // Ethernet::DropConnection.)
  IPConfigRefPtr me = this;

  if (!failure_callback_.is_null()) {
    failure_callback_.Run(this);
  }
}

void IPConfig::NotifyExpiry() {
  if (!expire_callback_.is_null()) {
    expire_callback_.Run(this);
  }
}

void IPConfig::RegisterUpdateCallback(const UpdateCallback& callback) {
  update_callback_ = callback;
}

void IPConfig::RegisterFailureCallback(const Callback& callback) {
  failure_callback_ = callback;
}

void IPConfig::RegisterRefreshCallback(const Callback& callback) {
  refresh_callback_ = callback;
}

void IPConfig::RegisterExpireCallback(const Callback& callback) {
  expire_callback_ = callback;
}

void IPConfig::ResetProperties() {
  properties_ = Properties();
  EmitChanges();
}

void IPConfig::EmitChanges() {
  adaptor_->EmitStringChanged(kAddressProperty, properties_.address);
  adaptor_->EmitStringsChanged(kNameServersProperty, properties_.dns_servers);
}

}  // namespace shill
