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

#ifndef SHILL_DBUS_CHROMEOS_MODEM_CDMA_PROXY_H_
#define SHILL_DBUS_CHROMEOS_MODEM_CDMA_PROXY_H_

#include <string>

#include "cellular/dbus-proxies.h"
#include "shill/cellular/modem_cdma_proxy_interface.h"

namespace shill {

// A proxy to (old) ModemManager.Modem.CDMA.
class ChromeosModemCDMAProxy : public ModemCDMAProxyInterface {
 public:
  // Constructs a ModemManager.Modem.CDMA DBus object proxy at |path| owned by
  // |service|.
  ChromeosModemCDMAProxy(const scoped_refptr<dbus::Bus>& bus,
                         const std::string& path,
                         const std::string& service);
  ~ChromeosModemCDMAProxy() override;

  // Inherited from ModemCDMAProxyInterface.
  void Activate(const std::string& carrier,
                Error* error,
                const ActivationResultCallback& callback,
                int timeout) override;
  void GetRegistrationState(Error* error,
                            const RegistrationStateCallback& callback,
                            int timeout) override;
  void GetSignalQuality(Error* error,
                        const SignalQualityCallback& callback,
                        int timeout) override;
  const std::string MEID() override;

  void set_activation_state_callback(
      const ActivationStateSignalCallback& callback) override {
    activation_state_callback_ = callback;
  }
  void set_signal_quality_callback(
      const SignalQualitySignalCallback& callback) override {
    signal_quality_callback_ = callback;
  }
  void set_registration_state_callback(
      const RegistrationStateSignalCallback& callback) override {
    registration_state_callback_ = callback;
  }

 private:
  class PropertySet : public dbus::PropertySet {
   public:
    PropertySet(dbus::ObjectProxy* object_proxy,
                const std::string& interface_name,
                const PropertyChangedCallback& callback);
    brillo::dbus_utils::Property<std::string> meid;

   private:
    DISALLOW_COPY_AND_ASSIGN(PropertySet);
  };

  static const char kPropertyMeid[];

  // Signal handlers.
  void ActivationStateChanged(
      uint32_t activation_state,
      uint32_t activation_error,
      const brillo::VariantDictionary& status_changes);
  void SignalQuality(uint32_t quality);
  void RegistrationStateChanged(uint32_t cdma_1x_state,
                                uint32_t evdo_state);

  // Callbacks for Activate async call.
  void OnActivateSuccess(const ActivationResultCallback& callback,
                         uint32_t status);
  void OnActivateFailure(const ActivationResultCallback& callback,
                         brillo::Error* dbus_error);

  // Callbacks for GetRegistrationState async call.
  void OnGetRegistrationStateSuccess(const RegistrationStateCallback& callback,
                                     uint32_t state_1x,
                                     uint32_t state_evdo);
  void OnGetRegistrationStateFailure(const RegistrationStateCallback& callback,
                                     brillo::Error* dbus_error);

  // Callbacks for GetSignalQuality async call.
  void OnGetSignalQualitySuccess(const SignalQualityCallback& callback,
                                 uint32_t quality);
  void OnGetSignalQualityFailure(const SignalQualityCallback& callback,
                                 brillo::Error* dbus_error);

  // Called when signal is connected to the ObjectProxy.
  void OnSignalConnected(const std::string& interface_name,
                         const std::string& signal_name,
                         bool success);

  // Callback invoked when the value of property |property_name| is changed.
  void OnPropertyChanged(const std::string& property_name);

  ActivationStateSignalCallback activation_state_callback_;
  SignalQualitySignalCallback signal_quality_callback_;
  RegistrationStateSignalCallback registration_state_callback_;

  std::unique_ptr<org::freedesktop::ModemManager::Modem::CdmaProxy> proxy_;
  std::unique_ptr<PropertySet> properties_;

  base::WeakPtrFactory<ChromeosModemCDMAProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosModemCDMAProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_MODEM_CDMA_PROXY_H_
