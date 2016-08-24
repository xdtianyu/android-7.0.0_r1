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

#ifndef SHILL_DBUS_CHROMEOS_MODEM_GSM_NETWORK_PROXY_H_
#define SHILL_DBUS_CHROMEOS_MODEM_GSM_NETWORK_PROXY_H_

#include <string>
#include <tuple>

#include "cellular/dbus-proxies.h"
#include "shill/cellular/modem_gsm_network_proxy_interface.h"

namespace shill {

// A proxy to (old) ModemManager.Modem.Gsm.Network.
class ChromeosModemGSMNetworkProxy : public ModemGSMNetworkProxyInterface {
 public:
  // Constructs a ModemManager.Modem.Gsm.Network DBus object proxy at |path|
  // owned by |service|.
  ChromeosModemGSMNetworkProxy(const scoped_refptr<dbus::Bus>& bus,
                               const std::string& path,
                               const std::string& service);
  ~ChromeosModemGSMNetworkProxy() override;

  // Inherited from ModemGSMNetworkProxyInterface.
  void GetRegistrationInfo(Error* error,
                           const RegistrationInfoCallback& callback,
                           int timeout) override;
  void GetSignalQuality(Error* error,
                        const SignalQualityCallback& callback,
                        int timeout) override;
  void Register(const std::string& network_id,
                Error* error,
                const ResultCallback& callback,
                int timeout) override;
  void Scan(Error* error,
            const ScanResultsCallback& callback,
            int timeout) override;
  uint32_t AccessTechnology() override;

  void set_signal_quality_callback(
      const SignalQualitySignalCallback& callback) override {
    signal_quality_callback_ = callback;
  }

  void set_network_mode_callback(
      const NetworkModeSignalCallback& callback) override {
    network_mode_callback_ = callback;
  }

  void set_registration_info_callback(
      const RegistrationInfoSignalCallback& callback) override {
    registration_info_callback_ = callback;
  }

 private:
  typedef std::tuple<uint32_t, std::string, std::string> GSMRegistrationInfo;

  class PropertySet : public dbus::PropertySet {
   public:
    PropertySet(dbus::ObjectProxy* object_proxy,
                const std::string& interface_name,
                const PropertyChangedCallback& callback);
    brillo::dbus_utils::Property<uint32_t> access_technology;

   private:
    DISALLOW_COPY_AND_ASSIGN(PropertySet);
  };

  static const char kPropertyAccessTechnology[];

  // Signal handlers.
  void SignalQuality(uint32_t quality);
  void RegistrationInfo(uint32_t status,
                        const std::string& operator_code,
                        const std::string& operator_name);
  void NetworkMode(uint32_t mode);

  // Callbacks for Register async call.
  void OnRegisterSuccess(const ResultCallback& callback);
  void OnRegisterFailure(const ResultCallback& callback,
                         brillo::Error* dbus_error);

  // Callbacks for GetRegistrationInfo async call.
  void OnGetRegistrationInfoSuccess(
      const RegistrationInfoCallback& callback,
      const GSMRegistrationInfo& info);
  void OnGetRegistrationInfoFailure(
      const RegistrationInfoCallback& callback, brillo::Error* dbus_error);

  // Callbacks for GetSignalQuality async call.
  void OnGetSignalQualitySuccess(const SignalQualityCallback& callback,
                                 uint32_t quality);
  void OnGetSignalQualityFailure(const SignalQualityCallback& callback,
                                 brillo::Error* dbus_error);

  // Callbacks for Scan async call.
  void OnScanSuccess(const ScanResultsCallback& callback,
                     const GSMScanResults& results);
  void OnScanFailure(const ScanResultsCallback& callback,
                     brillo::Error* dbus_error);

  // Called when signal is connected to the ObjectProxy.
  void OnSignalConnected(const std::string& interface_name,
                         const std::string& signal_name,
                         bool success);

  // Callback invoked when the value of property |property_name| is changed.
  void OnPropertyChanged(const std::string& property_name);

  SignalQualitySignalCallback signal_quality_callback_;
  RegistrationInfoSignalCallback registration_info_callback_;
  NetworkModeSignalCallback network_mode_callback_;

  std::unique_ptr<org::freedesktop::ModemManager::Modem::Gsm::NetworkProxy>
      proxy_;
  std::unique_ptr<PropertySet> properties_;

  base::WeakPtrFactory<ChromeosModemGSMNetworkProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosModemGSMNetworkProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_MODEM_GSM_NETWORK_PROXY_H_
