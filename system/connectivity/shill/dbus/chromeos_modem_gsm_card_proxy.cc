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

#include "shill/dbus/chromeos_modem_gsm_card_proxy.h"

#include <memory>

#include <base/bind.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/cellular/cellular_error.h"
#include "shill/error.h"
#include "shill/logging.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(const dbus::ObjectPath* p) { return p->value(); }
}  // namespace Logging

// static.
const char ChromeosModemGSMCardProxy::kPropertyEnabledFacilityLocks[] =
    "EnabledFacilityLocks";

ChromeosModemGSMCardProxy::PropertySet::PropertySet(
    dbus::ObjectProxy* object_proxy,
    const std::string& interface_name,
    const PropertyChangedCallback& callback)
    : dbus::PropertySet(object_proxy, interface_name, callback) {
  RegisterProperty(kPropertyEnabledFacilityLocks, &enabled_facility_locks);
}

ChromeosModemGSMCardProxy::ChromeosModemGSMCardProxy(
    const scoped_refptr<dbus::Bus>& bus,
    const string& path,
    const string& service)
    : proxy_(
        new org::freedesktop::ModemManager::Modem::Gsm::CardProxy(
            bus, service, dbus::ObjectPath(path))) {
  // Register properties.
  properties_.reset(
      new PropertySet(
          proxy_->GetObjectProxy(),
          cromo::kModemGsmCardInterface,
          base::Bind(&ChromeosModemGSMCardProxy::OnPropertyChanged,
                     weak_factory_.GetWeakPtr())));

  // Connect property signals and initialize cached values. Based on
  // recommendations from src/dbus/property.h.
  properties_->ConnectSignals();
  properties_->GetAll();
}

ChromeosModemGSMCardProxy::~ChromeosModemGSMCardProxy() {}

void ChromeosModemGSMCardProxy::GetIMEI(
    Error* error, const GSMIdentifierCallback& callback, int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->GetImeiAsync(
      base::Bind(&ChromeosModemGSMCardProxy::OnGetGSMIdentifierSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 "IMEI"),
      base::Bind(&ChromeosModemGSMCardProxy::OnGetGSMIdentifierFailure,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 "IMEI"));
}

void ChromeosModemGSMCardProxy::GetIMSI(Error* error,
                                        const GSMIdentifierCallback& callback,
                                        int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->GetImsiAsync(
      base::Bind(&ChromeosModemGSMCardProxy::OnGetGSMIdentifierSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 "IMSI"),
      base::Bind(&ChromeosModemGSMCardProxy::OnGetGSMIdentifierFailure,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 "IMSI"));
}

void ChromeosModemGSMCardProxy::GetSPN(Error* error,
                                       const GSMIdentifierCallback& callback,
                                       int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->GetSpnAsync(
      base::Bind(&ChromeosModemGSMCardProxy::OnGetGSMIdentifierSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 "SPN"),
      base::Bind(&ChromeosModemGSMCardProxy::OnGetGSMIdentifierFailure,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 "SPN"));
}

void ChromeosModemGSMCardProxy::GetMSISDN(Error* error,
                                          const GSMIdentifierCallback& callback,
                                          int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->GetMsIsdnAsync(
      base::Bind(&ChromeosModemGSMCardProxy::OnGetGSMIdentifierSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 "MSIDN"),
      base::Bind(&ChromeosModemGSMCardProxy::OnGetGSMIdentifierFailure,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 "MSIDN"));
}

void ChromeosModemGSMCardProxy::EnablePIN(const string& pin, bool enabled,
                                          Error* error,
                                          const ResultCallback& callback,
                                          int timeout) {
  // pin is intentionally not logged.
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << enabled;
  proxy_->EnablePinAsync(
      pin,
      enabled,
      base::Bind(&ChromeosModemGSMCardProxy::OnOperationSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 __func__),
      base::Bind(&ChromeosModemGSMCardProxy::OnOperationFailure,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 __func__));
}

void ChromeosModemGSMCardProxy::SendPIN(const string& pin,
                                        Error* error,
                                        const ResultCallback& callback,
                                        int timeout) {
  // pin is intentionally not logged.
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->SendPinAsync(
      pin,
      base::Bind(&ChromeosModemGSMCardProxy::OnOperationSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 __func__),
      base::Bind(&ChromeosModemGSMCardProxy::OnOperationFailure,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 __func__));
}

void ChromeosModemGSMCardProxy::SendPUK(const string& puk, const string& pin,
                                        Error* error,
                                        const ResultCallback& callback,
                                        int timeout) {
  // pin is intentionally not logged.
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->SendPukAsync(
      puk,
      pin,
      base::Bind(&ChromeosModemGSMCardProxy::OnOperationSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 __func__),
      base::Bind(&ChromeosModemGSMCardProxy::OnOperationFailure,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 __func__));
}

void ChromeosModemGSMCardProxy::ChangePIN(const string& old_pin,
                                          const string& new_pin,
                                          Error* error,
                                          const ResultCallback& callback,
                                          int timeout) {
  // pin is intentionally not logged.
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->SendPukAsync(
      old_pin,
      new_pin,
      base::Bind(&ChromeosModemGSMCardProxy::OnOperationSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 __func__),
      base::Bind(&ChromeosModemGSMCardProxy::OnOperationFailure,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 __func__));
}

uint32_t ChromeosModemGSMCardProxy::EnabledFacilityLocks() {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  if (!properties_->enabled_facility_locks.GetAndBlock()) {
    LOG(ERROR) << "Faild to get EnableFacilityLocks";
    return 0;
  }
  return properties_->enabled_facility_locks.value();
}

void ChromeosModemGSMCardProxy::OnGetGSMIdentifierSuccess(
    const GSMIdentifierCallback& callback,
    const string& identifier_name,
    const string& identifier_value) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << identifier_name
                                    << " " << identifier_value;
  callback.Run(identifier_value, Error());
}

void ChromeosModemGSMCardProxy::OnGetGSMIdentifierFailure(
    const GSMIdentifierCallback& callback,
    const string& identifier_name,
    brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << identifier_name;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run("", error);
}

void ChromeosModemGSMCardProxy::OnOperationSuccess(
    const ResultCallback& callback,
    const string& operation_name) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << operation_name;
  callback.Run(Error());
}

void ChromeosModemGSMCardProxy::OnOperationFailure(
    const ResultCallback& callback,
    const string& operation_name,
    brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << operation_name;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(error);
}

void ChromeosModemGSMCardProxy::OnPropertyChanged(
    const string& property_name) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << property_name;
}

}  // namespace shill
