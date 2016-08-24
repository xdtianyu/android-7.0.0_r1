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

#include "shill/dbus/chromeos_modem_proxy.h"

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
}  // namespace Logging

ChromeosModemProxy::ChromeosModemProxy(const scoped_refptr<dbus::Bus>& bus,
                                       const string& path,
                                       const string& service)
    : proxy_(
        new org::freedesktop::ModemManager::ModemProxy(
            bus, service, dbus::ObjectPath(path))) {
  // Register signal handlers.
  proxy_->RegisterStateChangedSignalHandler(
      base::Bind(&ChromeosModemProxy::StateChanged,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosModemProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));
}

ChromeosModemProxy::~ChromeosModemProxy() {}

void ChromeosModemProxy::Enable(
    bool enable, Error* error, const ResultCallback& callback, int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << enable;
  proxy_->EnableAsync(enable,
                      base::Bind(&ChromeosModemProxy::OnEnableSuccess,
                                 weak_factory_.GetWeakPtr(),
                                 callback),
                      base::Bind(&ChromeosModemProxy::OnEnableFailure,
                                 weak_factory_.GetWeakPtr(),
                                 callback));
}

void ChromeosModemProxy::Disconnect(
    Error* error, const ResultCallback& callback, int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->DisconnectAsync(base::Bind(&ChromeosModemProxy::OnDisconnectSuccess,
                                     weak_factory_.GetWeakPtr(),
                                     callback),
                          base::Bind(&ChromeosModemProxy::OnDisconnectFailure,
                                     weak_factory_.GetWeakPtr(),
                                     callback));
}

void ChromeosModemProxy::GetModemInfo(
    Error* error, const ModemInfoCallback& callback, int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->GetInfoAsync(base::Bind(&ChromeosModemProxy::OnGetInfoSuccess,
                                  weak_factory_.GetWeakPtr(),
                                  callback),
                       base::Bind(&ChromeosModemProxy::OnGetInfoFailure,
                                  weak_factory_.GetWeakPtr(),
                                  callback));
}

void ChromeosModemProxy::StateChanged(
    uint32_t old, uint32_t _new, uint32_t reason) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "(" << old
      << ", " << _new << ", " << reason << ")";
  if (state_changed_callback_.is_null()) {
    return;
  }
  state_changed_callback_.Run(old, _new, reason);
}

void ChromeosModemProxy::OnEnableSuccess(const ResultCallback& callback) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  callback.Run(Error());
}

void ChromeosModemProxy::OnEnableFailure(const ResultCallback& callback,
                                         brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(error);
}

void ChromeosModemProxy::OnGetInfoSuccess(const ModemInfoCallback& callback,
                                          const ModemHardwareInfo& info) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  callback.Run(
      std::get<0>(info), std::get<1>(info), std::get<2>(info), Error());
}

void ChromeosModemProxy::OnGetInfoFailure(const ModemInfoCallback& callback,
                                          brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run("", "", "", error);
}

void ChromeosModemProxy::OnDisconnectSuccess(const ResultCallback& callback) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  callback.Run(Error());
}

void ChromeosModemProxy::OnDisconnectFailure(const ResultCallback& callback,
                                             brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(error);
}

void ChromeosModemProxy::OnSignalConnected(
    const string& interface_name, const string& signal_name, bool success) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__
      << "interface: " << interface_name
             << " signal: " << signal_name << "success: " << success;
  if (!success) {
    LOG(ERROR) << "Failed to connect signal " << signal_name
        << " to interface " << interface_name;
  }
}

}  // namespace shill
