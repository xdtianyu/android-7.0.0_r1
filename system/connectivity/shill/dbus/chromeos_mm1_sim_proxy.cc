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

#include "shill/dbus/chromeos_mm1_sim_proxy.h"

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

ChromeosSimProxy::ChromeosSimProxy(const scoped_refptr<dbus::Bus>& bus,
                                   const string& path,
                                   const string& service)
    : proxy_(
        new org::freedesktop::ModemManager1::SimProxy(
            bus, service, dbus::ObjectPath(path))) {}

ChromeosSimProxy::~ChromeosSimProxy() {}


void ChromeosSimProxy::SendPin(const string& pin,
                               Error* error,
                               const ResultCallback& callback,
                               int timeout) {
  // pin is intentionally not logged.
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->SendPinAsync(pin,
                       base::Bind(&ChromeosSimProxy::OnOperationSuccess,
                                  weak_factory_.GetWeakPtr(),
                                  callback,
                                  __func__),
                       base::Bind(&ChromeosSimProxy::OnOperationFailure,
                                  weak_factory_.GetWeakPtr(),
                                  callback,
                                  __func__));
}

void ChromeosSimProxy::SendPuk(const string& puk,
                               const string& pin,
                               Error* error,
                               const ResultCallback& callback,
                               int timeout) {
  // pin and puk are intentionally not logged.
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->SendPukAsync(puk,
                       pin,
                       base::Bind(&ChromeosSimProxy::OnOperationSuccess,
                                  weak_factory_.GetWeakPtr(),
                                  callback,
                                  __func__),
                       base::Bind(&ChromeosSimProxy::OnOperationFailure,
                                  weak_factory_.GetWeakPtr(),
                                  callback,
                                  __func__));
}

void ChromeosSimProxy::EnablePin(const string& pin,
                                 const bool enabled,
                                 Error* error,
                                 const ResultCallback& callback,
                                 int timeout) {
  // pin is intentionally not logged.
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << enabled;
  proxy_->EnablePinAsync(pin,
                         enabled,
                         base::Bind(&ChromeosSimProxy::OnOperationSuccess,
                                    weak_factory_.GetWeakPtr(),
                                    callback,
                                    __func__),
                         base::Bind(&ChromeosSimProxy::OnOperationFailure,
                                    weak_factory_.GetWeakPtr(),
                                    callback,
                                    __func__));
}

void ChromeosSimProxy::ChangePin(const string& old_pin,
                                 const string& new_pin,
                                 Error* error,
                                 const ResultCallback& callback,
                                 int timeout) {
  // old_pin and new_pin are intentionally not logged.
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->ChangePinAsync(old_pin,
                      new_pin,
                      base::Bind(&ChromeosSimProxy::OnOperationSuccess,
                                 weak_factory_.GetWeakPtr(),
                                 callback,
                                 __func__),
                      base::Bind(&ChromeosSimProxy::OnOperationFailure,
                                 weak_factory_.GetWeakPtr(),
                                 callback,
                                 __func__));
}

void ChromeosSimProxy::OnOperationSuccess(const ResultCallback& callback,
                                          const string& operation) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << operation;
  callback.Run(Error());
}

void ChromeosSimProxy::OnOperationFailure(const ResultCallback& callback,
                                          const string& operation,
                                          brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << operation;
  Error error;
  CellularError::FromMM1ChromeosDBusError(dbus_error, &error);
  callback.Run(error);
}

}  // namespace mm1
}  // namespace shill
