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

#include "shill/dbus/chromeos_supplicant_bss_proxy.h"

#include <string>

#include <base/bind.h>

#include "shill/logging.h"
#include "shill/supplicant/wpa_supplicant.h"
#include "shill/wifi/wifi_endpoint.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(const dbus::ObjectPath* p) { return p->value(); }
}

ChromeosSupplicantBSSProxy::ChromeosSupplicantBSSProxy(
    const scoped_refptr<dbus::Bus>& bus,
    const std::string& object_path,
    WiFiEndpoint* wifi_endpoint)
    : bss_proxy_(
        new fi::w1::wpa_supplicant1::BSSProxy(
            bus,
            WPASupplicant::kDBusAddr,
            dbus::ObjectPath(object_path))),
      wifi_endpoint_(wifi_endpoint) {
  // Register signal handler.
  bss_proxy_->RegisterPropertiesChangedSignalHandler(
      base::Bind(&ChromeosSupplicantBSSProxy::PropertiesChanged,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosSupplicantBSSProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));
}

ChromeosSupplicantBSSProxy::~ChromeosSupplicantBSSProxy() {
  bss_proxy_->ReleaseObjectProxy(base::Bind(&base::DoNothing));
}

void ChromeosSupplicantBSSProxy::PropertiesChanged(
    const brillo::VariantDictionary& properties) {
  SLOG(&bss_proxy_->GetObjectPath(), 2) << __func__;
  KeyValueStore store;
  KeyValueStore::ConvertFromVariantDictionary(properties, &store);
  wifi_endpoint_->PropertiesChanged(store);
}

// Called when signal is connected to the ObjectProxy.
void ChromeosSupplicantBSSProxy::OnSignalConnected(
    const std::string& interface_name,
    const std::string& signal_name,
    bool success) {
  SLOG(&bss_proxy_->GetObjectPath(), 2) << __func__
      << "interface: " << interface_name << " signal: " << signal_name
      << "success: " << success;
  if (!success) {
    LOG(ERROR) << "Failed to connect signal " << signal_name
        << " to interface " << interface_name;
  }
}

}  // namespace shill
