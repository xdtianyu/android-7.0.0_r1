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

#include "shill/vpn/vpn_service.h"

#include <algorithm>

#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/key_value_store.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/profile.h"
#include "shill/property_accessor.h"
#include "shill/technology.h"
#include "shill/vpn/vpn_driver.h"
#include "shill/vpn/vpn_provider.h"

using base::Bind;
using base::StringPrintf;
using base::Unretained;
using std::replace_if;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kVPN;
static string ObjectID(const VPNService* s) { return s->GetRpcIdentifier(); }
}

const char VPNService::kAutoConnNeverConnected[] = "never connected";
const char VPNService::kAutoConnVPNAlreadyActive[] = "vpn already active";

VPNService::VPNService(ControlInterface* control,
                       EventDispatcher* dispatcher,
                       Metrics* metrics,
                       Manager* manager,
                       VPNDriver* driver)
    : Service(control, dispatcher, metrics, manager, Technology::kVPN),
      driver_(driver) {
  SetConnectable(true);
  set_save_credentials(false);
  mutable_store()->RegisterString(kVPNDomainProperty, &vpn_domain_);
  mutable_store()->RegisterDerivedString(
          kPhysicalTechnologyProperty,
          StringAccessor(
              new CustomAccessor<VPNService, string>(
                  this,
                  &VPNService::GetPhysicalTechnologyProperty,
                  nullptr)));
}

VPNService::~VPNService() {}

void VPNService::Connect(Error* error, const char* reason) {
  if (IsConnected()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kAlreadyConnected,
                          StringPrintf("VPN service %s already connected.",
                                       unique_name().c_str()));
    return;
  }
  if (IsConnecting()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInProgress,
                          StringPrintf("VPN service %s already connecting.",
                                       unique_name().c_str()));
    return;
  }
  manager()->vpn_provider()->DisconnectAll();
  Service::Connect(error, reason);
  driver_->Connect(this, error);
}

void VPNService::Disconnect(Error* error, const char* reason) {
  SLOG(this, 1) << "Disconnect from service " << unique_name();
  Service::Disconnect(error, reason);
  driver_->Disconnect();
}

string VPNService::GetStorageIdentifier() const {
  return storage_id_;
}

// static
string VPNService::CreateStorageIdentifier(const KeyValueStore& args,
                                           Error* error) {
  string host = args.LookupString(kProviderHostProperty, "");
  if (host.empty()) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kInvalidProperty, "Missing VPN host.");
    return "";
  }
  string name = args.LookupString(kNameProperty, "");
  if (name.empty()) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kNotSupported, "Missing VPN name.");
    return "";
  }
  string id = StringPrintf("vpn_%s_%s", host.c_str(), name.c_str());
  replace_if(id.begin(), id.end(), &Service::IllegalChar, '_');
  return id;
}

string VPNService::GetDeviceRpcId(Error* error) const {
  error->Populate(Error::kNotSupported);
  return "/";
}

bool VPNService::Load(StoreInterface* storage) {
  return Service::Load(storage) &&
      driver_->Load(storage, GetStorageIdentifier());
}

bool VPNService::Save(StoreInterface* storage) {
  return Service::Save(storage) &&
      driver_->Save(storage, GetStorageIdentifier(), save_credentials());
}

bool VPNService::Unload() {
  // The base method also disconnects the service.
  Service::Unload();

  set_save_credentials(false);
  driver_->UnloadCredentials();

  // Ask the VPN provider to remove us from its list.
  manager()->vpn_provider()->RemoveService(this);

  return true;
}

void VPNService::InitDriverPropertyStore() {
  driver_->InitPropertyStore(mutable_store());
}

void VPNService::EnableAndRetainAutoConnect() {
  // The base EnableAndRetainAutoConnect method also sets auto_connect_ to true
  // which is not desirable for VPN services.
  RetainAutoConnect();
}

void VPNService::SetConnection(const ConnectionRefPtr& connection) {
  // Construct the connection binder here rather than in the constructor because
  // there's really no reason to construct a binder if we never connect to this
  // service. It's safe to use an unretained callback to driver's method because
  // both the binder and the driver will be destroyed when this service is
  // destructed.
  if (!connection_binder_.get()) {
    connection_binder_.reset(
        new Connection::Binder(unique_name(),
                               Bind(&VPNDriver::OnConnectionDisconnected,
                                    Unretained(driver_.get()))));
  }
  // Note that |connection_| is a reference-counted pointer and is always set
  // through this method. This means that the connection binder will not be
  // notified when the connection is destructed (because we will unbind it first
  // here when it's set to NULL, or because the binder will already be destroyed
  // by ~VPNService) -- it will be notified only if the connection disconnects
  // (e.g., because an underlying connection is destructed).
  connection_binder_->Attach(connection);
  Service::SetConnection(connection);
}

bool VPNService::IsAutoConnectable(const char** reason) const {
  if (!Service::IsAutoConnectable(reason)) {
    return false;
  }
  // Don't auto-connect VPN services that have never connected. This improves
  // the chances that the VPN service is connectable and avoids dialog popups.
  if (!has_ever_connected()) {
    *reason = kAutoConnNeverConnected;
    return false;
  }
  // Don't auto-connect a VPN service if another VPN service is already active.
  if (manager()->vpn_provider()->HasActiveService()) {
    *reason = kAutoConnVPNAlreadyActive;
    return false;
  }
  return true;
}

string VPNService::GetTethering(Error* error) const {
  ConnectionRefPtr conn = connection();
  if (conn)
    conn = conn->GetCarrierConnection();

  string tethering;
  if (conn) {
    tethering = conn->tethering();
    if (!tethering.empty()) {
      return tethering;
    }
    // The underlying service may not have a Tethering property.  This is
    // not strictly an error, so we don't print an error message.  Populating
    // an error here just serves to propagate the lack of a property in
    // GetProperties().
    error->Populate(Error::kNotSupported);
  } else {
    error->Populate(Error::kOperationFailed);
  }
  return "";
}

bool VPNService::SetNameProperty(const string& name, Error* error) {
  if (name == friendly_name()) {
    return false;
  }
  LOG(INFO) << "Renaming service " << unique_name() << ": "
            << friendly_name() << " -> " << name;

  KeyValueStore* args = driver_->args();
  args->SetString(kNameProperty, name);
  string new_storage_id = CreateStorageIdentifier(*args, error);
  if (new_storage_id.empty()) {
    return false;
  }
  string old_storage_id = storage_id_;
  DCHECK_NE(old_storage_id, new_storage_id);

  SetFriendlyName(name);

  // Update the storage identifier before invoking DeleteEntry to prevent it
  // from unloading this service.
  storage_id_ = new_storage_id;
  profile()->DeleteEntry(old_storage_id, nullptr);
  profile()->UpdateService(this);
  return true;
}

string VPNService::GetPhysicalTechnologyProperty(Error* error) {
  ConnectionRefPtr conn = connection();
  if (conn)
    conn = conn->GetCarrierConnection();

  if (!conn) {
    error->Populate(Error::kOperationFailed);
    return "";
  }

  return Technology::NameFromIdentifier(conn->technology());
}

}  // namespace shill
