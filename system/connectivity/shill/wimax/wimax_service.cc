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

#include "shill/wimax/wimax_service.h"

#include <algorithm>

#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/control_interface.h"
#include "shill/eap_credentials.h"
#include "shill/key_value_store.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/store_interface.h"
#include "shill/technology.h"
#include "shill/wimax/wimax.h"

using std::replace_if;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kWiMax;
static string ObjectID(WiMaxService* w) { return w->GetRpcIdentifier(); }
}

const char WiMaxService::kStorageNetworkId[] = "NetworkId";
const char WiMaxService::kNetworkIdProperty[] = "NetworkId";

WiMaxService::WiMaxService(ControlInterface* control,
                           EventDispatcher* dispatcher,
                           Metrics* metrics,
                           Manager* manager)
    : Service(control, dispatcher, metrics, manager, Technology::kWiMax),
      need_passphrase_(true),
      is_default_(false) {
  PropertyStore* store = this->mutable_store();
  // TODO(benchan): Support networks that require no user credentials or
  // implicitly defined credentials.
  store->RegisterBool(kPassphraseRequiredProperty, &need_passphrase_);
  store->RegisterConstString(kNetworkIdProperty, &network_id_);

  SetEapCredentials(new EapCredentials());

  IgnoreParameterForConfigure(kNetworkIdProperty);

  // Initialize a default storage identifier based on the service's unique
  // name. The identifier most likely needs to be reinitialized by the caller
  // when its components have been set.
  InitStorageIdentifier();

  // Now that |this| is a fully constructed WiMaxService, synchronize observers
  // with our current state, and emit the appropriate change notifications.
  // (Initial observer state may have been set in our base class.)
  NotifyPropertyChanges();
}

WiMaxService::~WiMaxService() {}

void WiMaxService::GetConnectParameters(KeyValueStore* parameters) const {
  CHECK(parameters);
  eap()->PopulateWiMaxProperties(parameters);
}

RpcIdentifier WiMaxService::GetNetworkObjectPath() const {
  CHECK(proxy_.get());
  return proxy_->path();
}

void WiMaxService::Stop() {
  if (!IsStarted()) {
    return;
  }
  LOG(INFO) << "Stopping WiMAX service: " << GetStorageIdentifier();
  proxy_.reset();
  SetStrength(0);
  if (device_) {
    device_->OnServiceStopped(this);
    SetDevice(nullptr);
  }
  UpdateConnectable();
  NotifyPropertyChanges();
}

bool WiMaxService::Start(WiMaxNetworkProxyInterface* proxy) {
  SLOG(this, 2) << __func__;
  CHECK(proxy);
  std::unique_ptr<WiMaxNetworkProxyInterface> local_proxy(proxy);
  if (IsStarted()) {
    return true;
  }
  if (friendly_name().empty()) {
    LOG(ERROR) << "Empty service name.";
    return false;
  }
  Error error;
  network_name_ = proxy->Name(&error);
  if (error.IsFailure()) {
    return false;
  }
  uint32_t identifier = proxy->Identifier(&error);
  if (error.IsFailure()) {
    return false;
  }
  WiMaxNetworkId id = ConvertIdentifierToNetworkId(identifier);
  if (id != network_id_) {
    LOG(ERROR) << "Network identifiers don't match: "
               << id << " != " << network_id_;
    return false;
  }
  int signal_strength = proxy->SignalStrength(&error);
  if (error.IsFailure()) {
    return false;
  }
  SetStrength(signal_strength);
  proxy->set_signal_strength_changed_callback(
      Bind(&WiMaxService::OnSignalStrengthChanged, Unretained(this)));
  proxy_.reset(local_proxy.release());
  UpdateConnectable();
  NotifyPropertyChanges();
  LOG(INFO) << "WiMAX service started: " << GetStorageIdentifier();
  return true;
}

bool WiMaxService::IsStarted() const {
  return proxy_.get();
}

void WiMaxService::Connect(Error* error, const char* reason) {
  SLOG(this, 2) << __func__;
  if (device_) {
    // TODO(benchan): Populate error again after changing the way that
    // Chrome handles Error::kAlreadyConnected situation.
    LOG(WARNING) << "Service " << GetStorageIdentifier()
                 << " is already being connected or already connected.";
    return;
  }
  if (!connectable()) {
    LOG(ERROR) << "Can't connect. Service " << GetStorageIdentifier()
               << " is not connectable.";
    Error::PopulateAndLog(FROM_HERE, error, Error::kOperationFailed,
                          Error::GetDefaultMessage(Error::kOperationFailed));
    return;
  }
  WiMaxRefPtr carrier = manager()->wimax_provider()->SelectCarrier(this);
  if (!carrier) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kNoCarrier,
        "No suitable WiMAX device available.");
    return;
  }
  Service::Connect(error, reason);
  carrier->ConnectTo(this, error);
  if (error->IsSuccess()) {
    // Associate with the carrier device if the connection process has been
    // initiated successfully.
    SetDevice(carrier);
  }
}

void WiMaxService::Disconnect(Error* error, const char* reason) {
  SLOG(this, 2) << __func__;
  if (!device_) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kNotConnected, "Not connected.");
    return;
  }
  Service::Disconnect(error, reason);
  device_->DisconnectFrom(this, error);
  SetDevice(nullptr);
}

string WiMaxService::GetStorageIdentifier() const {
  return storage_id_;
}

string WiMaxService::GetDeviceRpcId(Error* error) const {
  if (!device_) {
    error->Populate(Error::kNotFound, "Not associated with a device");
    return control_interface()->NullRPCIdentifier();
  }
  return device_->GetRpcIdentifier();
}

bool WiMaxService::IsAutoConnectable(const char** reason) const {
  if (!Service::IsAutoConnectable(reason)) {
    return false;
  }
  WiMaxRefPtr device = manager()->wimax_provider()->SelectCarrier(this);
  DCHECK(device);
  if (!device->IsIdle()) {
    *reason = kAutoConnBusy;
    return false;
  }
  return true;
}

bool WiMaxService::Is8021x() const {
  return true;
}

bool WiMaxService::IsVisible() const {
  // WiMAX services should be displayed only if they are in range (i.e.
  // a corresponding network is exposed by WiMAX manager).
  return IsStarted();
}

void WiMaxService::OnEapCredentialsChanged(
    Service::UpdateCredentialsReason reason) {
  need_passphrase_ = !eap()->IsConnectableUsingPassphrase();
  if (reason == Service::kReasonPropertyUpdate)
    SetHasEverConnected(false);
  UpdateConnectable();
}

void WiMaxService::UpdateConnectable() {
  SLOG(this, 2) << __func__ << "(started: " << IsStarted()
                << ", need passphrase: " << need_passphrase_ << ")";
  SetConnectableFull(IsStarted() && !need_passphrase_);
}

void WiMaxService::OnSignalStrengthChanged(int strength) {
  SLOG(this, 2) << __func__ << "(" << strength << ")";
  SetStrength(strength);
}

void WiMaxService::SetDevice(WiMaxRefPtr new_device) {
  if (device_ == new_device)
    return;
  if (new_device) {
    adaptor()->EmitRpcIdentifierChanged(kDeviceProperty,
                                        new_device->GetRpcIdentifier());
  } else {
    adaptor()->EmitRpcIdentifierChanged(
        kDeviceProperty, control_interface()->NullRPCIdentifier());
  }
  device_ = new_device;
}

bool WiMaxService::Save(StoreInterface* storage) {
  SLOG(this, 2) << __func__;
  if (!Service::Save(storage)) {
    return false;
  }
  const string id = GetStorageIdentifier();
  storage->SetString(id, kStorageNetworkId, network_id_);

  return true;
}

bool WiMaxService::Unload() {
  SLOG(this, 2) << __func__;
  // The base method also disconnects the service.
  Service::Unload();
  ClearPassphrase();
  // Notify the WiMAX provider that this service has been unloaded. If the
  // provider releases ownership of this service, it needs to be deregistered.
  return manager()->wimax_provider()->OnServiceUnloaded(this);
}

void WiMaxService::SetState(ConnectState state) {
  Service::SetState(state);
  if (!IsConnecting() && !IsConnected()) {
    // Disassociate from any carrier device if it's not connected anymore.
    SetDevice(nullptr);
  }
}

// static
WiMaxNetworkId WiMaxService::ConvertIdentifierToNetworkId(uint32_t identifier) {
  return base::StringPrintf("%08x", identifier);
}

void WiMaxService::InitStorageIdentifier() {
  storage_id_ = CreateStorageIdentifier(network_id_, friendly_name());
}

// static
string WiMaxService::CreateStorageIdentifier(const WiMaxNetworkId& id,
                                             const string& name) {
  string storage_id = base::ToLowerASCII(
      base::StringPrintf("%s_%s_%s",
                         kTypeWimax, name.c_str(), id.c_str()));
  replace_if(storage_id.begin(), storage_id.end(), &Service::IllegalChar, '_');
  return storage_id;
}

void WiMaxService::ClearPassphrase() {
  SLOG(this, 2) << __func__;
  mutable_eap()->set_password("");
  OnEapCredentialsChanged(Service::kReasonPropertyUpdate);
}

}  // namespace shill
