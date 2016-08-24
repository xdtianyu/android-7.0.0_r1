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

#include "shill/dbus/chromeos_modem_gsm_network_proxy.h"

#include <memory>

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
const char ChromeosModemGSMNetworkProxy::kPropertyAccessTechnology[] =
    "AccessTechnology";

ChromeosModemGSMNetworkProxy::PropertySet::PropertySet(
    dbus::ObjectProxy* object_proxy,
    const std::string& interface_name,
    const PropertyChangedCallback& callback)
    : dbus::PropertySet(object_proxy, interface_name, callback) {
  RegisterProperty(kPropertyAccessTechnology, &access_technology);
}

ChromeosModemGSMNetworkProxy::ChromeosModemGSMNetworkProxy(
    const scoped_refptr<dbus::Bus>& bus,
    const string& path,
    const string& service)
    : proxy_(
        new org::freedesktop::ModemManager::Modem::Gsm::NetworkProxy(
            bus, service, dbus::ObjectPath(path))) {
  // Register signal handlers.
  proxy_->RegisterSignalQualitySignalHandler(
      base::Bind(&ChromeosModemGSMNetworkProxy::SignalQuality,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosModemGSMNetworkProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));
  proxy_->RegisterRegistrationInfoSignalHandler(
      base::Bind(&ChromeosModemGSMNetworkProxy::RegistrationInfo,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosModemGSMNetworkProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));
  proxy_->RegisterNetworkModeSignalHandler(
      base::Bind(&ChromeosModemGSMNetworkProxy::NetworkMode,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosModemGSMNetworkProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));

  // Register properties.
  properties_.reset(
      new PropertySet(
          proxy_->GetObjectProxy(),
          cromo::kModemGsmNetworkInterface,
          base::Bind(&ChromeosModemGSMNetworkProxy::OnPropertyChanged,
                     weak_factory_.GetWeakPtr())));

  // Connect property signals and initialize cached values. Based on
  // recommendations from src/dbus/property.h.
  properties_->ConnectSignals();
  properties_->GetAll();
}

ChromeosModemGSMNetworkProxy::~ChromeosModemGSMNetworkProxy() {}

void ChromeosModemGSMNetworkProxy::GetRegistrationInfo(
    Error* error,
    const RegistrationInfoCallback& callback,
    int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->GetRegistrationInfoAsync(
      base::Bind(&ChromeosModemGSMNetworkProxy::OnGetRegistrationInfoSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback),
      base::Bind(&ChromeosModemGSMNetworkProxy::OnGetRegistrationInfoFailure,
                 weak_factory_.GetWeakPtr(),
                 callback));
}

void ChromeosModemGSMNetworkProxy::GetSignalQuality(
    Error* error,
    const SignalQualityCallback& callback,
    int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->GetSignalQualityAsync(
      base::Bind(&ChromeosModemGSMNetworkProxy::OnGetSignalQualitySuccess,
                 weak_factory_.GetWeakPtr(),
                 callback),
      base::Bind(&ChromeosModemGSMNetworkProxy::OnGetSignalQualityFailure,
                 weak_factory_.GetWeakPtr(),
                 callback));
}

void ChromeosModemGSMNetworkProxy::Register(const string& network_id,
                                            Error* error,
                                            const ResultCallback& callback,
                                            int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << network_id;
  proxy_->RegisterAsync(
      network_id,
      base::Bind(&ChromeosModemGSMNetworkProxy::OnRegisterSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback),
      base::Bind(&ChromeosModemGSMNetworkProxy::OnRegisterFailure,
                 weak_factory_.GetWeakPtr(),
                 callback));
}

void ChromeosModemGSMNetworkProxy::Scan(Error* error,
                                        const ScanResultsCallback& callback,
                                        int timeout) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  proxy_->ScanAsync(
      base::Bind(&ChromeosModemGSMNetworkProxy::OnScanSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback),
      base::Bind(&ChromeosModemGSMNetworkProxy::OnScanFailure,
                 weak_factory_.GetWeakPtr(),
                 callback));
}

uint32_t ChromeosModemGSMNetworkProxy::AccessTechnology() {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  if (!properties_->access_technology.GetAndBlock()) {
    LOG(ERROR) << "Failed to get AccessTechnology";
    return 0;
  }
  return properties_->access_technology.value();
}

void ChromeosModemGSMNetworkProxy::SignalQuality(uint32_t quality) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "(" << quality << ")";
  if (signal_quality_callback_.is_null()) {
    return;
  }
  signal_quality_callback_.Run(quality);
}

void ChromeosModemGSMNetworkProxy::RegistrationInfo(
    uint32_t status, const string& operator_code, const string& operator_name) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "(" << status << ", "
      << operator_code << ", " << operator_name << ")";
  if (registration_info_callback_.is_null()) {
    return;
  }
  registration_info_callback_.Run(status, operator_code, operator_name);
}

void ChromeosModemGSMNetworkProxy::NetworkMode(uint32_t mode) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "(" << mode << ")";
  if (network_mode_callback_.is_null()) {
    return;
  }
  network_mode_callback_.Run(mode);
}

void ChromeosModemGSMNetworkProxy::OnRegisterSuccess(
    const ResultCallback& callback) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  callback.Run(Error());
}

void ChromeosModemGSMNetworkProxy::OnRegisterFailure(
    const ResultCallback& callback, brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(error);
}

void ChromeosModemGSMNetworkProxy::OnGetRegistrationInfoSuccess(
    const RegistrationInfoCallback& callback,
    const GSMRegistrationInfo& info) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  callback.Run(
      std::get<0>(info), std::get<1>(info), std::get<2>(info), Error());
}

void ChromeosModemGSMNetworkProxy::OnGetRegistrationInfoFailure(
    const RegistrationInfoCallback& callback,
    brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(0, "", "", error);
}

void ChromeosModemGSMNetworkProxy::OnGetSignalQualitySuccess(
    const SignalQualityCallback& callback, uint32_t quality) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "(" << quality << ")";
  callback.Run(quality, Error());
}

void ChromeosModemGSMNetworkProxy::OnGetSignalQualityFailure(
    const SignalQualityCallback& callback, brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(0, error);
}

void ChromeosModemGSMNetworkProxy::OnScanSuccess(
    const ScanResultsCallback& callback, const GSMScanResults& results) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  callback.Run(results, Error());
}

void ChromeosModemGSMNetworkProxy::OnScanFailure(
    const ScanResultsCallback& callback, brillo::Error* dbus_error) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(GSMScanResults(), error);
}

void ChromeosModemGSMNetworkProxy::OnSignalConnected(
    const string& interface_name, const string& signal_name, bool success) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__
      << "interface: " << interface_name
             << " signal: " << signal_name << "success: " << success;
  if (!success) {
    LOG(ERROR) << "Failed to connect signal " << signal_name
        << " to interface " << interface_name;
  }
}

void ChromeosModemGSMNetworkProxy::OnPropertyChanged(
    const string& property_name) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << ": " << property_name;
}

}  // namespace shill
