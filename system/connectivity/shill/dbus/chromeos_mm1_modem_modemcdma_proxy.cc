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

#include "shill/dbus/chromeos_mm1_modem_modemcdma_proxy.h"

#include <memory>

#include "shill/cellular/cellular_error.h"
#include "shill/logging.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(const dbus::ObjectPath* p) { return p->value(); }
}  // namespace Logging

namespace mm1 {

ChromeosModemModemCdmaProxy::ChromeosModemModemCdmaProxy(
    const scoped_refptr<dbus::Bus>& bus,
    const string& path,
    const string& service)
    : proxy_(
        new org::freedesktop::ModemManager1::Modem::ModemCdmaProxy(
            bus, service, dbus::ObjectPath(path))) {
  // Register signal handlers.
  proxy_->RegisterActivationStateChangedSignalHandler(
      base::Bind(&ChromeosModemModemCdmaProxy::ActivationStateChanged,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosModemModemCdmaProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));
}

ChromeosModemModemCdmaProxy::~ChromeosModemModemCdmaProxy() {}

void ChromeosModemModemCdmaProxy::Activate(const std::string& carrier,
                                           Error* error,
                                           const ResultCallback& callback,
                                           int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << carrier;
  proxy_->ActivateAsync(
      carrier,
      base::Bind(&ChromeosModemModemCdmaProxy::OnOperationSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 __func__),
      base::Bind(&ChromeosModemModemCdmaProxy::OnOperationFailure,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 __func__));
}

void ChromeosModemModemCdmaProxy::ActivateManual(
    const KeyValueStore& properties,
    Error* error,
    const ResultCallback& callback,
    int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  brillo::VariantDictionary properties_dict;
  KeyValueStore::ConvertToVariantDictionary(properties, &properties_dict);
  proxy_->ActivateManualAsync(
      properties_dict,
      base::Bind(&ChromeosModemModemCdmaProxy::OnOperationSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 __func__),
      base::Bind(&ChromeosModemModemCdmaProxy::OnOperationFailure,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 __func__));
}

void ChromeosModemModemCdmaProxy::ActivationStateChanged(
    uint32_t activation_state,
    uint32_t activation_error,
    const brillo::VariantDictionary& status_changes) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  if (activation_state_callback_.is_null()) {
    return;
  }
  KeyValueStore status_store;
  KeyValueStore::ConvertFromVariantDictionary(status_changes, &status_store);
  activation_state_callback_.Run(activation_state,
                                 activation_error,
                                 status_store);
}

void ChromeosModemModemCdmaProxy::OnOperationSuccess(
    const ResultCallback& callback, const string& operation) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << operation;
  callback.Run(Error());
}

void ChromeosModemModemCdmaProxy::OnOperationFailure(
    const ResultCallback& callback,
    const string& operation,
    brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << operation;
  Error error;
  CellularError::FromMM1ChromeosDBusError(dbus_error, &error);
  callback.Run(Error());
}

void ChromeosModemModemCdmaProxy::OnSignalConnected(
    const string& interface_name, const string& signal_name, bool success) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__
      << "interface: " << interface_name
             << " signal: " << signal_name << "success: " << success;
  if (!success) {
    LOG(ERROR) << "Failed to connect signal " << signal_name
        << " to interface " << interface_name;
  }
}

}  // namespace mm1
}  // namespace shill
