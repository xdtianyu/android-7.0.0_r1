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

#include "shill/dbus/chromeos_supplicant_network_proxy.h"

#include <string>

#include <base/bind.h>

#include "shill/logging.h"
#include "shill/supplicant/wpa_supplicant.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(const dbus::ObjectPath* p) { return p->value(); }
}

// static.
const char ChromeosSupplicantNetworkProxy::kInterfaceName[] =
    "fi.w1.wpa_supplicant1.Network";
const char ChromeosSupplicantNetworkProxy::kPropertyEnabled[] = "Enabled";
const char ChromeosSupplicantNetworkProxy::kPropertyProperties[] =
    "Properties";

ChromeosSupplicantNetworkProxy::PropertySet::PropertySet(
    dbus::ObjectProxy* object_proxy,
    const std::string& interface_name,
    const PropertyChangedCallback& callback)
    : dbus::PropertySet(object_proxy, interface_name, callback) {
  RegisterProperty(kPropertyEnabled, &enabled);
}

ChromeosSupplicantNetworkProxy::ChromeosSupplicantNetworkProxy(
    const scoped_refptr<dbus::Bus>& bus,
    const std::string& object_path)
    : network_proxy_(
        new fi::w1::wpa_supplicant1::NetworkProxy(
            bus,
            WPASupplicant::kDBusAddr,
            dbus::ObjectPath(object_path))) {
  // Register properties.
  properties_.reset(
      new PropertySet(
          network_proxy_->GetObjectProxy(),
          kInterfaceName,
          base::Bind(&ChromeosSupplicantNetworkProxy::OnPropertyChanged,
                     weak_factory_.GetWeakPtr())));

  // Register signal handler.
  network_proxy_->RegisterPropertiesChangedSignalHandler(
      base::Bind(&ChromeosSupplicantNetworkProxy::PropertiesChanged,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosSupplicantNetworkProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));

  // Connect property signals and initialize cached values. Based on
  // recommendations from src/dbus/property.h.
  properties_->ConnectSignals();
  properties_->GetAll();
}

ChromeosSupplicantNetworkProxy::~ChromeosSupplicantNetworkProxy() {
  network_proxy_->ReleaseObjectProxy(base::Bind(&base::DoNothing));
}

bool ChromeosSupplicantNetworkProxy::SetEnabled(bool enabled) {
  SLOG(&network_proxy_->GetObjectPath(), 2) << __func__;
  if(!properties_->enabled.SetAndBlock(enabled)) {
    LOG(ERROR) << "Failed to SetEnabled: " << enabled;
    return false;
  }
  return true;
}

void ChromeosSupplicantNetworkProxy::PropertiesChanged(
    const brillo::VariantDictionary& /*properties*/) {
  SLOG(&network_proxy_->GetObjectPath(), 2) << __func__;
}

void ChromeosSupplicantNetworkProxy::OnPropertyChanged(
    const std::string& property_name) {
  SLOG(&network_proxy_->GetObjectPath(), 2) << __func__ << ": "
      << property_name;
}

// Called when signal is connected to the ObjectProxy.
void ChromeosSupplicantNetworkProxy::OnSignalConnected(
    const std::string& interface_name,
    const std::string& signal_name,
    bool success) {
  SLOG(&network_proxy_->GetObjectPath(), 2) << __func__
      << "interface: " << interface_name << " signal: " << signal_name
      << "success: " << success;
  if (!success) {
    LOG(ERROR) << "Failed to connect signal " << signal_name
        << " to interface " << interface_name;
  }
}

}  // namespace shill
