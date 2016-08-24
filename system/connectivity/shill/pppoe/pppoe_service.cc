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

#include "shill/pppoe/pppoe_service.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>

#include <base/callback.h>
#include <base/logging.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/control_interface.h"
#include "shill/ethernet/ethernet.h"
#include "shill/event_dispatcher.h"
#include "shill/manager.h"
#include "shill/metrics.h"
#include "shill/ppp_daemon.h"
#include "shill/ppp_device.h"
#include "shill/ppp_device_factory.h"
#include "shill/process_manager.h"
#include "shill/store_interface.h"

using base::StringPrintf;
using std::map;
using std::string;
using std::unique_ptr;

namespace shill {

const int PPPoEService::kDefaultLCPEchoInterval = 30;
const int PPPoEService::kDefaultLCPEchoFailure = 3;
const int PPPoEService::kDefaultMaxAuthFailure = 3;

PPPoEService::PPPoEService(ControlInterface* control_interface,
                           EventDispatcher* dispatcher,
                           Metrics* metrics,
                           Manager* manager,
                           base::WeakPtr<Ethernet> ethernet)
    : EthernetService(control_interface, dispatcher, metrics, manager,
                      Technology::kPPPoE, ethernet),
      control_interface_(control_interface),
      ppp_device_factory_(PPPDeviceFactory::GetInstance()),
      process_manager_(ProcessManager::GetInstance()),
      lcp_echo_interval_(kDefaultLCPEchoInterval),
      lcp_echo_failure_(kDefaultLCPEchoFailure),
      max_auth_failure_(kDefaultMaxAuthFailure),
      authenticating_(false),
      weak_ptr_factory_(this) {
  PropertyStore* store = this->mutable_store();
  store->RegisterString(kPPPoEUsernameProperty, &username_);
  store->RegisterString(kPPPoEPasswordProperty, &password_);
  store->RegisterInt32(kPPPoELCPEchoIntervalProperty, &lcp_echo_interval_);
  store->RegisterInt32(kPPPoELCPEchoFailureProperty, &lcp_echo_failure_);
  store->RegisterInt32(kPPPoEMaxAuthFailureProperty, &max_auth_failure_);

  set_friendly_name("PPPoE");
  SetConnectable(true);
  SetAutoConnect(true);
  NotifyPropertyChanges();
}

PPPoEService::~PPPoEService() {}

void PPPoEService::Connect(Error* error, const char* reason) {
  Service::Connect(error, reason);

  CHECK(ethernet());

  if (!ethernet()->link_up()) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kOperationFailed, StringPrintf(
            "PPPoE Service %s does not have Ethernet link.",
            unique_name().c_str()));
    return;
  }

  if (IsConnected()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kAlreadyConnected,
                          StringPrintf("PPPoE service %s already connected.",
                                       unique_name().c_str()));
    return;
  }

  if (IsConnecting()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInProgress,
                          StringPrintf("PPPoE service %s already connecting.",
                                       unique_name().c_str()));
    return;
  }

  PPPDaemon::DeathCallback callback(base::Bind(&PPPoEService::OnPPPDied,
                                               weak_ptr_factory_.GetWeakPtr()));

  PPPDaemon::Options options;
  options.no_detach = true;
  options.no_default_route = true;
  options.use_peer_dns = true;
  options.use_pppoe_plugin = true;
  options.lcp_echo_interval = lcp_echo_interval_;
  options.lcp_echo_failure = lcp_echo_failure_;
  options.max_fail = max_auth_failure_;
  options.use_ipv6 = true;

  pppd_ = PPPDaemon::Start(
      control_interface_, process_manager_, weak_ptr_factory_.GetWeakPtr(),
      options, ethernet()->link_name(), callback, error);
  if (pppd_ == nullptr) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInternalError,
                          StringPrintf("PPPoE service %s can't start pppd.",
                                       unique_name().c_str()));
    return;
  }

  SetState(Service::kStateAssociating);
}

void PPPoEService::Disconnect(Error* error, const char* reason) {
  EthernetService::Disconnect(error, reason);
  if (ppp_device_) {
    ppp_device_->DropConnection();
  } else {
    // If no PPPDevice has been associated with this service then nothing will
    // drive this service's transition into the idle state.  This must be forced
    // here to ensure that the service is not left in any intermediate state.
    SetState(Service::kStateIdle);
  }
  ppp_device_ = nullptr;
  pppd_.reset();
  manager()->OnInnerDevicesChanged();
}

bool PPPoEService::Load(StoreInterface* storage) {
  if (!Service::Load(storage)) {
    return false;
  }

  const string id = GetStorageIdentifier();
  storage->GetString(id, kPPPoEUsernameProperty, &username_);
  storage->GetString(id, kPPPoEPasswordProperty, &password_);
  storage->GetInt(id, kPPPoELCPEchoIntervalProperty, &lcp_echo_interval_);
  storage->GetInt(id, kPPPoELCPEchoFailureProperty, &lcp_echo_failure_);
  storage->GetInt(id, kPPPoEMaxAuthFailureProperty, &max_auth_failure_);

  return true;
}

bool PPPoEService::Save(StoreInterface* storage) {
  if (!Service::Save(storage)) {
    return false;
  }

  const string id = GetStorageIdentifier();
  storage->SetString(id, kPPPoEUsernameProperty, username_);
  storage->SetString(id, kPPPoEPasswordProperty, password_);
  storage->SetInt(id, kPPPoELCPEchoIntervalProperty, lcp_echo_interval_);
  storage->SetInt(id, kPPPoELCPEchoFailureProperty, lcp_echo_failure_);
  storage->SetInt(id, kPPPoEMaxAuthFailureProperty, max_auth_failure_);

  return true;
}

bool PPPoEService::Unload() {
  username_.clear();
  password_.clear();
  return Service::Unload();
}

string PPPoEService::GetInnerDeviceRpcIdentifier() const {
  return ppp_device_ ? ppp_device_->GetRpcIdentifier() : "";
}

void PPPoEService::GetLogin(string* user, string* password) {
  CHECK(user && password);
  *user = username_;
  *password = password_;
}

void PPPoEService::Notify(const string& reason,
                          const map<string, string>& dict) {
  if (reason == kPPPReasonAuthenticating) {
    OnPPPAuthenticating();
  } else if (reason == kPPPReasonAuthenticated) {
    OnPPPAuthenticated();
  } else if (reason == kPPPReasonConnect) {
    OnPPPConnected(dict);
  } else if (reason == kPPPReasonDisconnect) {
    OnPPPDisconnected();
  } else {
    NOTREACHED();
  }
}

void PPPoEService::OnPPPDied(pid_t pid, int exit) {
  OnPPPDisconnected();
}

void PPPoEService::OnPPPAuthenticating() {
  authenticating_ = true;
}

void PPPoEService::OnPPPAuthenticated() {
  authenticating_ = false;
}

void PPPoEService::OnPPPConnected(const map<string, string>& params) {
  const string interface_name = PPPDevice::GetInterfaceName(params);

  DeviceInfo* device_info = manager()->device_info();
  const int interface_index = device_info->GetIndex(interface_name);
  if (interface_index < 0) {
    NOTIMPLEMENTED() << ": No device info for " << interface_name;
    return;
  }

  if (ppp_device_) {
    ppp_device_->SelectService(nullptr);
  }

  ppp_device_ = ppp_device_factory_->CreatePPPDevice(
      control_interface_, dispatcher(), metrics(), manager(), interface_name,
      interface_index);
  device_info->RegisterDevice(ppp_device_);
  ppp_device_->SetEnabled(true);
  ppp_device_->SelectService(this);
  ppp_device_->UpdateIPConfigFromPPP(params, false);
#ifndef DISABLE_DHCPV6
  // Acquire DHCPv6 configurations through the PPPoE (virtual) interface
  // if it is enabled for DHCPv6.
  if (manager()->IsDHCPv6EnabledForDevice(ppp_device_->link_name())) {
    ppp_device_->AcquireIPv6Config();
  }
#endif
  manager()->OnInnerDevicesChanged();
}

void PPPoEService::OnPPPDisconnected() {
  pppd_.release()->DestroyLater(dispatcher());

  Error unused_error;
  Disconnect(&unused_error, __func__);

  if (authenticating_) {
    SetFailure(Service::kFailurePPPAuth);
  } else {
    SetFailure(Service::kFailureUnknown);
  }
}

}  // namespace shill
