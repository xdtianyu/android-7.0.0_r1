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

#include "shill/vpn/vpn_provider.h"

#include <algorithm>
#include <memory>

#include <base/strings/string_util.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/error.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/process_manager.h"
#include "shill/profile.h"
#include "shill/store_interface.h"
#include "shill/vpn/l2tp_ipsec_driver.h"
#include "shill/vpn/openvpn_driver.h"
#include "shill/vpn/third_party_vpn_driver.h"
#include "shill/vpn/vpn_service.h"

using std::set;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kVPN;
static string ObjectID(const VPNProvider* v) { return "(vpn_provider)"; }
}

VPNProvider::VPNProvider(ControlInterface* control_interface,
                         EventDispatcher* dispatcher,
                         Metrics* metrics,
                         Manager* manager)
    : control_interface_(control_interface),
      dispatcher_(dispatcher),
      metrics_(metrics),
      manager_(manager) {}

VPNProvider::~VPNProvider() {}

void VPNProvider::Start() {}

void VPNProvider::Stop() {}

// static
bool VPNProvider::GetServiceParametersFromArgs(const KeyValueStore& args,
                                               string* type_ptr,
                                               string* name_ptr,
                                               string* host_ptr,
                                               Error* error) {
  SLOG(nullptr, 2) << __func__;
  string type = args.LookupString(kProviderTypeProperty, "");
  if (type.empty()) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kNotSupported, "Missing VPN type property.");
    return false;
  }

  string host = args.LookupString(kProviderHostProperty, "");
  if (host.empty()) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kNotSupported, "Missing VPN host property.");
    return false;
  }

  *type_ptr = type,
  *host_ptr = host,
  *name_ptr = args.LookupString(kNameProperty, "");

  return true;
}

// static
bool VPNProvider::GetServiceParametersFromStorage(const StoreInterface* storage,
                                                  const string& entry_name,
                                                  string* vpn_type_ptr,
                                                  string* name_ptr,
                                                  string* host_ptr,
                                                  Error* error) {
  string service_type;
  if (!storage->GetString(entry_name, kTypeProperty, &service_type) ||
      service_type != kTypeVPN) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Unspecified or invalid network type");
    return false;
  }
  if (!storage->GetString(entry_name, kProviderTypeProperty, vpn_type_ptr) ||
      vpn_type_ptr->empty()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "VPN type not specified");
    return false;
  }
  if (!storage->GetString(entry_name, kNameProperty, name_ptr) ||
      name_ptr->empty()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Network name not specified");
    return false;
  }
  if (!storage->GetString(entry_name, kProviderHostProperty, host_ptr) ||
      host_ptr->empty()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Host not specified");
    return false;
  }
  return true;
}

ServiceRefPtr VPNProvider::GetService(const KeyValueStore& args,
                                      Error* error) {
  SLOG(this, 2) << __func__;
  string type;
  string name;
  string host;

  if (!GetServiceParametersFromArgs(args, &type, &name, &host, error)) {
    return nullptr;
  }

  string storage_id = VPNService::CreateStorageIdentifier(args, error);
  if (storage_id.empty()) {
    return nullptr;
  }

  // Find a service in the provider list which matches these parameters.
  VPNServiceRefPtr service = FindService(type, name, host);
  if (service == nullptr) {
    service = CreateService(type, name, storage_id, error);
  }
  return service;
}

ServiceRefPtr VPNProvider::FindSimilarService(const KeyValueStore& args,
                                              Error* error) const {
  SLOG(this, 2) << __func__;
  string type;
  string name;
  string host;

  if (!GetServiceParametersFromArgs(args, &type, &name, &host, error)) {
    return nullptr;
  }

  // Find a service in the provider list which matches these parameters.
  VPNServiceRefPtr service = FindService(type, name, host);
  if (!service) {
    error->Populate(Error::kNotFound, "Matching service was not found");
  }

  return service;
}

bool VPNProvider::OnDeviceInfoAvailable(const string& link_name,
                                        int interface_index) {
  for (const auto& service : services_) {
    if (service->driver()->ClaimInterface(link_name, interface_index)) {
      return true;
    }
  }

  return false;
}

void VPNProvider::RemoveService(VPNServiceRefPtr service) {
  const auto it = std::find(services_.begin(), services_.end(), service);
  if (it != services_.end()) {
    services_.erase(it);
  }
}

void VPNProvider::CreateServicesFromProfile(const ProfileRefPtr& profile) {
  SLOG(this, 2) << __func__;
  const StoreInterface* storage = profile->GetConstStorage();
  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeVPN);
  for (const auto& group : storage->GetGroupsWithProperties(args)) {
    string type;
    string name;
    string host;
    if (!GetServiceParametersFromStorage(storage,
                                         group,
                                         &type,
                                         &name,
                                         &host,
                                         nullptr)) {
      continue;
    }

    VPNServiceRefPtr service = FindService(type, name, host);
    if (service != nullptr) {
      // If the service already exists, it does not need to be configured,
      // since PushProfile would have already called ConfigureService on it.
      SLOG(this, 2) << "Service already exists " << group;
      continue;
    }

    Error error;
    service = CreateService(type, name, group, &error);

    if (service == nullptr) {
      LOG(ERROR) << "Could not create service for " << group;
      continue;
    }

    if (!profile->ConfigureService(service)) {
      LOG(ERROR) << "Could not configure service for " << group;
      continue;
    }
  }
}

VPNServiceRefPtr VPNProvider::CreateServiceInner(const string& type,
                                                 const string& name,
                                                 const string& storage_id,
                                                 Error* error) {
  SLOG(this, 2) << __func__ << " type " << type << " name " << name
                << " storage id " << storage_id;
#if defined(DISABLE_VPN)

  Error::PopulateAndLog(
      FROM_HERE, error, Error::kNotSupported, "VPN is not supported.");
  return nullptr;

#else

  std::unique_ptr<VPNDriver> driver;
  if (type == kProviderOpenVpn) {
    driver.reset(new OpenVPNDriver(
        control_interface_, dispatcher_, metrics_, manager_,
        manager_->device_info(), ProcessManager::GetInstance()));
  } else if (type == kProviderL2tpIpsec) {
    driver.reset(new L2TPIPSecDriver(
        control_interface_, dispatcher_, metrics_, manager_,
        manager_->device_info(), ProcessManager::GetInstance()));
  } else if (type == kProviderThirdPartyVpn) {
    // For third party VPN host contains extension ID
    driver.reset(new ThirdPartyVpnDriver(
        control_interface_, dispatcher_, metrics_, manager_,
        manager_->device_info()));
  } else {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kNotSupported,
        "Unsupported VPN type: " + type);
    return nullptr;
  }

  VPNServiceRefPtr service = new VPNService(
      control_interface_, dispatcher_, metrics_, manager_, driver.release());
  service->set_storage_id(storage_id);
  service->InitDriverPropertyStore();
  if (!name.empty()) {
    service->set_friendly_name(name);
  }
  return service;

#endif  // DISABLE_VPN
}

VPNServiceRefPtr VPNProvider::CreateService(const string& type,
                                            const string& name,
                                            const string& storage_id,
                                            Error* error) {
  VPNServiceRefPtr service = CreateServiceInner(type, name, storage_id, error);
  if (service) {
    services_.push_back(service);
    manager_->RegisterService(service);
  }

  return service;
}

VPNServiceRefPtr VPNProvider::FindService(const string& type,
                                          const string& name,
                                          const string& host) const {
  for (const auto& service : services_) {
    if (type == service->driver()->GetProviderType() &&
        name == service->friendly_name() &&
        host == service->driver()->GetHost()) {
      return service;
    }
  }
  return nullptr;
}

ServiceRefPtr VPNProvider::CreateTemporaryService(
      const KeyValueStore& args, Error* error) {
  string type;
  string name;
  string host;

  if (!GetServiceParametersFromArgs(args, &type, &name, &host, error)) {
    return nullptr;
  }

  string storage_id = VPNService::CreateStorageIdentifier(args, error);
  if (storage_id.empty()) {
    return nullptr;
  }

  return CreateServiceInner(type, name, storage_id, error);
}

ServiceRefPtr VPNProvider::CreateTemporaryServiceFromProfile(
    const ProfileRefPtr& profile, const std::string& entry_name, Error* error) {
  string type;
  string name;
  string host;
  if (!GetServiceParametersFromStorage(profile->GetConstStorage(),
                                       entry_name,
                                       &type,
                                       &name,
                                       &host,
                                       error)) {
    return nullptr;
  }

  return CreateServiceInner(type, name, entry_name, error);
}

bool VPNProvider::HasActiveService() const {
  for (const auto& service : services_) {
    if (service->IsConnecting() || service->IsConnected()) {
      return true;
    }
  }
  return false;
}

void VPNProvider::DisconnectAll() {
  for (const auto& service : services_) {
    if (service->IsConnecting() || service->IsConnected()) {
      service->Disconnect(nullptr, "user selected new config");
    }
  }
}

}  // namespace shill
