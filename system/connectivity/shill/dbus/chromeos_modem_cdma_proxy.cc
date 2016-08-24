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

#include "shill/dbus/chromeos_modem_cdma_proxy.h"

#include <memory>

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/cellular/cellular_error.h"
#include "shill/logging.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(const dbus::ObjectPath* p) { return p->value(); }
}  // namespace Logging

// static.
const char ChromeosModemCDMAProxy::kPropertyMeid[] = "Meid";

ChromeosModemCDMAProxy::PropertySet::PropertySet(
    dbus::ObjectProxy* object_proxy,
    const std::string& interface_name,
    const PropertyChangedCallback& callback)
    : dbus::PropertySet(object_proxy, interface_name, callback) {
  RegisterProperty(kPropertyMeid, &meid);
}

ChromeosModemCDMAProxy::ChromeosModemCDMAProxy(
    const scoped_refptr<dbus::Bus>& bus,
    const string& path,
    const string& service)
    : proxy_(
        new org::freedesktop::ModemManager::Modem::CdmaProxy(
            bus, service, dbus::ObjectPath(path))) {
  // Register signal handlers.
  proxy_->RegisterActivationStateChangedSignalHandler(
      base::Bind(&ChromeosModemCDMAProxy::ActivationStateChanged,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosModemCDMAProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));
  proxy_->RegisterSignalQualitySignalHandler(
      base::Bind(&ChromeosModemCDMAProxy::SignalQuality,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosModemCDMAProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));
  proxy_->RegisterRegistrationStateChangedSignalHandler(
      base::Bind(&ChromeosModemCDMAProxy::RegistrationStateChanged,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosModemCDMAProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));

  // Register properties.
  properties_.reset(
      new PropertySet(
          proxy_->GetObjectProxy(),
          cromo::kModemCdmaInterface,
          base::Bind(&ChromeosModemCDMAProxy::OnPropertyChanged,
                     weak_factory_.GetWeakPtr())));

  // Connect property signals and initialize cached value. Based on
  // recommendations from src/dbus/property.h.
  properties_->ConnectSignals();
  properties_->GetAll();
}

ChromeosModemCDMAProxy::~ChromeosModemCDMAProxy() {}

void ChromeosModemCDMAProxy::Activate(const string& carrier,
                                      Error* error,
                                      const ActivationResultCallback& callback,
                                      int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << carrier;
  proxy_->ActivateAsync(
      carrier,
      base::Bind(&ChromeosModemCDMAProxy::OnActivateSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback),
      base::Bind(&ChromeosModemCDMAProxy::OnActivateFailure,
                 weak_factory_.GetWeakPtr(),
                 callback));
}

void ChromeosModemCDMAProxy::GetRegistrationState(
    Error* error,
    const RegistrationStateCallback& callback,
    int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->GetRegistrationStateAsync(
      base::Bind(&ChromeosModemCDMAProxy::OnGetRegistrationStateSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback),
      base::Bind(&ChromeosModemCDMAProxy::OnGetRegistrationStateFailure,
                 weak_factory_.GetWeakPtr(),
                 callback));
}

void ChromeosModemCDMAProxy::GetSignalQuality(
    Error* error, const SignalQualityCallback& callback, int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->GetSignalQualityAsync(
      base::Bind(&ChromeosModemCDMAProxy::OnGetSignalQualitySuccess,
                 weak_factory_.GetWeakPtr(),
                 callback),
      base::Bind(&ChromeosModemCDMAProxy::OnGetSignalQualityFailure,
                 weak_factory_.GetWeakPtr(),
                 callback));
}

const string ChromeosModemCDMAProxy::MEID() {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  if (!properties_->meid.GetAndBlock()) {
    LOG(ERROR) << "Failed to get MEID";
    return string();
  }
  return properties_->meid.value();
}

void ChromeosModemCDMAProxy::ActivationStateChanged(
    uint32_t activation_state,
    uint32_t activation_error,
    const brillo::VariantDictionary& status_changes) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "(" << activation_state
                                    << ", " << activation_error << ")";
  if (activation_state_callback_.is_null()) {
    return;
  }
  KeyValueStore status_changes_store;
  KeyValueStore::ConvertFromVariantDictionary(status_changes,
                                              &status_changes_store);
  activation_state_callback_.Run(activation_state,
                                 activation_error,
                                 status_changes_store);
}

void ChromeosModemCDMAProxy::SignalQuality(uint32_t quality) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "(" << quality << ")";
  if (signal_quality_callback_.is_null()) {
    return;
  }
  signal_quality_callback_.Run(quality);
}

void ChromeosModemCDMAProxy::RegistrationStateChanged(
    uint32_t cdma_1x_state,
    uint32_t evdo_state) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "(" << cdma_1x_state << ", "
                                    << evdo_state << ")";
  if (registration_state_callback_.is_null()) {
    return;
  }
  registration_state_callback_.Run(cdma_1x_state, evdo_state);
}

void ChromeosModemCDMAProxy::OnActivateSuccess(
    const ActivationResultCallback& callback, uint32_t status) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "(" << status << ")";
  callback.Run(status, Error());
}

void ChromeosModemCDMAProxy::OnActivateFailure(
    const ActivationResultCallback& callback, brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(0, error);
}

void ChromeosModemCDMAProxy::OnGetRegistrationStateSuccess(
    const RegistrationStateCallback& callback,
    uint32_t state_1x,
    uint32_t state_evdo) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "(" << state_1x
                                    << ", " << state_evdo << ")";
  callback.Run(state_1x, state_evdo, Error());
}

void ChromeosModemCDMAProxy::OnGetRegistrationStateFailure(
    const RegistrationStateCallback& callback, brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(0, 0, error);
}

void ChromeosModemCDMAProxy::OnGetSignalQualitySuccess(
    const SignalQualityCallback& callback, uint32_t quality) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "(" << quality << ")";
  callback.Run(quality, Error());
}

void ChromeosModemCDMAProxy::OnGetSignalQualityFailure(
    const SignalQualityCallback& callback, brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(0, error);
}

void ChromeosModemCDMAProxy::OnSignalConnected(
    const string& interface_name, const string& signal_name, bool success) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__
      << "interface: " << interface_name
             << " signal: " << signal_name << "success: " << success;
  if (!success) {
    LOG(ERROR) << "Failed to connect signal " << signal_name
        << " to interface " << interface_name;
  }
}

void ChromeosModemCDMAProxy::OnPropertyChanged(
    const string& property_name) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << property_name;
}

}  // namespace shill
