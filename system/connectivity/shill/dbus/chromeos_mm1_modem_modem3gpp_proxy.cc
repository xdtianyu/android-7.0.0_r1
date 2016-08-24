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

#include "shill/dbus/chromeos_mm1_modem_modem3gpp_proxy.h"

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

ChromeosModemModem3gppProxy::ChromeosModemModem3gppProxy(
    const scoped_refptr<dbus::Bus>& bus,
    const string& path,
    const string& service)
    : proxy_(
        new org::freedesktop::ModemManager1::Modem::Modem3gppProxy(
            bus, service, dbus::ObjectPath(path))) {}

ChromeosModemModem3gppProxy::~ChromeosModemModem3gppProxy() {}

void ChromeosModemModem3gppProxy::Register(const std::string& operator_id,
                                           Error* error,
                                           const ResultCallback& callback,
                                           int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << operator_id;
  proxy_->RegisterAsync(
      operator_id,
      base::Bind(&ChromeosModemModem3gppProxy::OnRegisterSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback),
      base::Bind(&ChromeosModemModem3gppProxy::OnRegisterFailure,
                 weak_factory_.GetWeakPtr(),
                 callback));
}

void ChromeosModemModem3gppProxy::Scan(Error* error,
                                       const KeyValueStoresCallback& callback,
                                       int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->ScanAsync(base::Bind(&ChromeosModemModem3gppProxy::OnScanSuccess,
                               weak_factory_.GetWeakPtr(),
                               callback),
                    base::Bind(&ChromeosModemModem3gppProxy::OnScanFailure,
                               weak_factory_.GetWeakPtr(),
                               callback));
}

void ChromeosModemModem3gppProxy::OnRegisterSuccess(
    const ResultCallback& callback) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  callback.Run(Error());
}

void ChromeosModemModem3gppProxy::OnRegisterFailure(
    const ResultCallback& callback, brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromMM1ChromeosDBusError(dbus_error, &error);
  callback.Run(error);
}

void ChromeosModemModem3gppProxy::OnScanSuccess(
    const KeyValueStoresCallback& callback,
    const std::vector<brillo::VariantDictionary>& results) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  std::vector<KeyValueStore> result_stores;
  for (const auto& result : results) {
    KeyValueStore result_store;
    KeyValueStore::ConvertFromVariantDictionary(result, &result_store);
    result_stores.push_back(result_store);
  }
  callback.Run(result_stores, Error());
}

void ChromeosModemModem3gppProxy::OnScanFailure(
    const KeyValueStoresCallback& callback, brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromMM1ChromeosDBusError(dbus_error, &error);
  callback.Run(std::vector<KeyValueStore>(), error);
}

}  // namespace mm1
}  // namespace shill
