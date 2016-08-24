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

#include "shill/dbus/chromeos_modem_simple_proxy.h"

#include <memory>

#include <base/bind.h>

#include "shill/cellular/cellular_error.h"
#include "shill/error.h"
#include "shill/logging.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(const dbus::ObjectPath* p) { return p->value(); }
}

ChromeosModemSimpleProxy::ChromeosModemSimpleProxy(
    const scoped_refptr<dbus::Bus>& bus,
    const string& path,
    const string& service)
    : proxy_(
        new org::freedesktop::ModemManager::Modem::SimpleProxy(
            bus, service, dbus::ObjectPath(path))) {}

ChromeosModemSimpleProxy::~ChromeosModemSimpleProxy() {}

void ChromeosModemSimpleProxy::GetModemStatus(
    Error* error, const KeyValueStoreCallback& callback, int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->GetStatusAsync(
      base::Bind(&ChromeosModemSimpleProxy::OnGetStatusSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback),
      base::Bind(&ChromeosModemSimpleProxy::OnGetStatusFailure,
                 weak_factory_.GetWeakPtr(),
                 callback));
}

void ChromeosModemSimpleProxy::Connect(const KeyValueStore& properties,
                                       Error* error,
                                       const ResultCallback& callback,
                                       int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  brillo::VariantDictionary properties_dict;
  KeyValueStore::ConvertToVariantDictionary(properties, &properties_dict);
  proxy_->ConnectAsync(
      properties_dict,
      base::Bind(&ChromeosModemSimpleProxy::OnConnectSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback),
      base::Bind(&ChromeosModemSimpleProxy::OnConnectFailure,
                 weak_factory_.GetWeakPtr(),
                 callback));
}

void ChromeosModemSimpleProxy::OnGetStatusSuccess(
    const KeyValueStoreCallback& callback,
    const brillo::VariantDictionary& props) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  KeyValueStore props_store;
  KeyValueStore::ConvertFromVariantDictionary(props, &props_store);
  callback.Run(props_store, Error());
}

void ChromeosModemSimpleProxy::OnGetStatusFailure(
    const KeyValueStoreCallback& callback, brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(KeyValueStore(), error);
}

void ChromeosModemSimpleProxy::OnConnectSuccess(
    const ResultCallback& callback) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  callback.Run(Error());
}

void ChromeosModemSimpleProxy::OnConnectFailure(
    const ResultCallback& callback, brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(error);
}

}  // namespace shill
