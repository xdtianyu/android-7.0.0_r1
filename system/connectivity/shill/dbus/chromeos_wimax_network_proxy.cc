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

#include "shill/dbus/chromeos_wimax_network_proxy.h"

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/error.h"
#include "shill/logging.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(const dbus::ObjectPath* p) {
  return "wimax_network_proxy (" + p->value() + ")";
}
}

// static.
const char ChromeosWiMaxNetworkProxy::kPropertyIdentifier[] = "Identifier";
const char ChromeosWiMaxNetworkProxy::kPropertyName[] = "Name";
const char ChromeosWiMaxNetworkProxy::kPropertyType[] = "Type";
const char ChromeosWiMaxNetworkProxy::kPropertyCINR[] = "CINR";
const char ChromeosWiMaxNetworkProxy::kPropertyRSSI[] = "RSSI";
const char ChromeosWiMaxNetworkProxy::kPropertySignalStrength[] =
    "SignalStrength";

ChromeosWiMaxNetworkProxy::PropertySet::PropertySet(
    dbus::ObjectProxy* object_proxy,
    const std::string& interface_name,
    const PropertyChangedCallback& callback)
    : dbus::PropertySet(object_proxy, interface_name, callback) {
  RegisterProperty(kPropertyIdentifier, &identifier);
  RegisterProperty(kPropertyName, &name);
  RegisterProperty(kPropertyType, &type);
  RegisterProperty(kPropertyCINR, &cinr);
  RegisterProperty(kPropertyRSSI, &rssi);
  RegisterProperty(kPropertySignalStrength, &signal_strength);
}

ChromeosWiMaxNetworkProxy::ChromeosWiMaxNetworkProxy(
    const scoped_refptr<dbus::Bus>& bus,
    const std::string& rpc_identifier)
    : proxy_(
        new org::chromium::WiMaxManager::NetworkProxy(
            bus,
            wimax_manager::kWiMaxManagerServiceName,
            dbus::ObjectPath(rpc_identifier))) {
  // Register signal handlers.
  proxy_->RegisterSignalStrengthChangedSignalHandler(
      base::Bind(&ChromeosWiMaxNetworkProxy::SignalStrengthChanged,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosWiMaxNetworkProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));

  // Register properties.
  properties_.reset(
      new PropertySet(
          proxy_->GetObjectProxy(),
          wimax_manager::kWiMaxManagerNetworkInterface,
          base::Bind(&ChromeosWiMaxNetworkProxy::OnPropertyChanged,
                     weak_factory_.GetWeakPtr())));
  properties_->ConnectSignals();
  properties_->GetAll();
}


ChromeosWiMaxNetworkProxy::~ChromeosWiMaxNetworkProxy() {
  proxy_->ReleaseObjectProxy(base::Bind(&base::DoNothing));
}

RpcIdentifier ChromeosWiMaxNetworkProxy::path() const {
  return proxy_->GetObjectPath().value();
}

void ChromeosWiMaxNetworkProxy::set_signal_strength_changed_callback(
    const SignalStrengthChangedCallback& callback) {
  signal_strength_changed_callback_ = callback;
}

uint32_t ChromeosWiMaxNetworkProxy::Identifier(Error* /*error*/) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  if (!properties_->identifier.GetAndBlock()) {
    LOG(ERROR) << "Failed to get Identifier";
    return 0;
  }
  return properties_->identifier.value();
}

string ChromeosWiMaxNetworkProxy::Name(Error* /*error*/) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  if (!properties_->name.GetAndBlock()) {
    LOG(ERROR) << "Failed to get Name";
    return string();
  }
  return properties_->name.value();
}

int ChromeosWiMaxNetworkProxy::Type(Error* /*error*/) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  if (!properties_->type.GetAndBlock()) {
    LOG(ERROR) << "Failed to get Type";
    return 0;
  }
  return properties_->type.value();
}

int ChromeosWiMaxNetworkProxy::CINR(Error* /*error*/) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  if (!properties_->cinr.GetAndBlock()) {
    LOG(ERROR) << "Failed to get CINR";
    return 0;
  }
  return properties_->cinr.value();
}

int ChromeosWiMaxNetworkProxy::RSSI(Error* /*error*/) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  if (!properties_->rssi.GetAndBlock()) {
    LOG(ERROR) << "Failed to get RSSI";
    return 0;
  }
  return properties_->rssi.value();
}

int ChromeosWiMaxNetworkProxy::SignalStrength(Error* /*error*/) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  if (!properties_->signal_strength.GetAndBlock()) {
    LOG(ERROR) << "Faild to get SignalStrength";
    return 0;
  }
  return properties_->signal_strength.value();
}

void ChromeosWiMaxNetworkProxy::SignalStrengthChanged(int32_t signal_strength) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__
      << "(" << signal_strength << ")";
  if (!signal_strength_changed_callback_.is_null()) {
    signal_strength_changed_callback_.Run(signal_strength);
  }
}

void ChromeosWiMaxNetworkProxy::OnSignalConnected(
    const string& interface_name, const string& signal_name, bool success) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__
      << "interface: " << interface_name << " signal: " << signal_name
      << "success: " << success;
  if (!success) {
    LOG(ERROR) << "Failed to connect signal " << signal_name
        << " to interface " << interface_name;
  }
}

void ChromeosWiMaxNetworkProxy::OnPropertyChanged(
    const std::string& property_name) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << property_name;
}

}  // namespace shill
