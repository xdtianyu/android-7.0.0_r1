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

#ifndef SHILL_DBUS_CHROMEOS_MM1_MODEM_MODEMCDMA_PROXY_H_
#define SHILL_DBUS_CHROMEOS_MM1_MODEM_MODEMCDMA_PROXY_H_

#include <string>

#include "cellular/dbus-proxies.h"
#include "shill/cellular/mm1_modem_modemcdma_proxy_interface.h"

namespace shill {
namespace mm1 {

// A proxy to org.freedesktop.ModemManager1.Modem.ModemCdma.
class ChromeosModemModemCdmaProxy : public ModemModemCdmaProxyInterface {
 public:
  // Constructs a org.freedesktop.ModemManager1.Modem.ModemCdma DBus
  // object proxy at |path| owned by |service|.
  ChromeosModemModemCdmaProxy(const scoped_refptr<dbus::Bus>& bus,
                              const std::string& path,
                              const std::string& service);
  ~ChromeosModemModemCdmaProxy() override;

  // Inherited methods from ModemModemCdmaProxyInterface.
  void Activate(const std::string& carrier,
                Error* error,
                const ResultCallback& callback,
                int timeout) override;
  void ActivateManual(
      const KeyValueStore& properties,
      Error* error,
      const ResultCallback& callback,
      int timeout) override;

  void set_activation_state_callback(
      const ActivationStateSignalCallback& callback) override {
    activation_state_callback_ = callback;
  }

 private:
  // Signal handler.
  void ActivationStateChanged(
        uint32_t activation_state,
        uint32_t activation_error,
        const brillo::VariantDictionary& status_changes);

  // Callbacks for async calls that uses ResultCallback.
  void OnOperationSuccess(const ResultCallback& callback,
                          const std::string& operation);
  void OnOperationFailure(const ResultCallback& callback,
                          const std::string& operation,
                          brillo::Error* dbus_error);

  // Called when signal is connected to the ObjectProxy.
  void OnSignalConnected(const std::string& interface_name,
                         const std::string& signal_name,
                         bool success);

  ActivationStateSignalCallback activation_state_callback_;

  std::unique_ptr<org::freedesktop::ModemManager1::Modem::ModemCdmaProxy>
      proxy_;

  base::WeakPtrFactory<ChromeosModemModemCdmaProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosModemModemCdmaProxy);
};

}  // namespace mm1
}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_MM1_MODEM_MODEMCDMA_PROXY_H_
