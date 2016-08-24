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

#ifndef SHILL_DBUS_CHROMEOS_MM1_MODEM_MODEM3GPP_PROXY_H_
#define SHILL_DBUS_CHROMEOS_MM1_MODEM_MODEM3GPP_PROXY_H_

#include <string>
#include <vector>

#include "cellular/dbus-proxies.h"
#include "shill/cellular/mm1_modem_modem3gpp_proxy_interface.h"

namespace shill {
namespace mm1 {

// A proxy to org.freedesktop.ModemManager1.Modem.Modem3gpp.
class ChromeosModemModem3gppProxy : public ModemModem3gppProxyInterface {
 public:
  // Constructs an org.freedesktop.ModemManager1.Modem.Modem3gpp DBus
  // object proxy at |path| owned by |service|.
  ChromeosModemModem3gppProxy(const scoped_refptr<dbus::Bus>& bus,
                              const std::string& path,
                              const std::string& service);
  ~ChromeosModemModem3gppProxy() override;
  // Inherited methods from ModemModem3gppProxyInterface.
  void Register(const std::string& operator_id,
                Error* error,
                const ResultCallback& callback,
                int timeout) override;
  void Scan(Error* error,
            const KeyValueStoresCallback& callback,
            int timeout) override;

 private:
  // Callbacks for Register async call.
  void OnRegisterSuccess(const ResultCallback& callback);
  void OnRegisterFailure(const ResultCallback& callback,
                         brillo::Error* dbus_error);

  // Callbacks for Scan async call.
  void OnScanSuccess(const KeyValueStoresCallback& callback,
                     const std::vector<brillo::VariantDictionary>& results);
  void OnScanFailure(const KeyValueStoresCallback& callback,
                     brillo::Error* dbus_error);

  std::unique_ptr<org::freedesktop::ModemManager1::Modem::Modem3gppProxy>
      proxy_;

  base::WeakPtrFactory<ChromeosModemModem3gppProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosModemModem3gppProxy);
};

}  // namespace mm1
}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_MM1_MODEM_MODEM3GPP_PROXY_H_
