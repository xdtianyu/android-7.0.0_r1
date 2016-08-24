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

#ifndef SHILL_DBUS_CHROMEOS_MODEM_GOBI_PROXY_H_
#define SHILL_DBUS_CHROMEOS_MODEM_GOBI_PROXY_H_

#include <string>

#include <base/macros.h>

#include "cellular/dbus-proxies.h"
#include "shill/cellular/modem_gobi_proxy_interface.h"

namespace shill {

// A proxy to (old) ModemManager.Modem.Gobi.
class ChromeosModemGobiProxy : public ModemGobiProxyInterface {
 public:
  // Constructs a ModemManager.Modem.Gobi DBus object proxy at |path| owned by
  // |service|.
  ChromeosModemGobiProxy(const scoped_refptr<dbus::Bus>& bus,
                         const std::string& path,
                         const std::string& service);
  ~ChromeosModemGobiProxy() override;

  // Inherited from ModemGobiProxyInterface.
  void SetCarrier(const std::string& carrier,
                  Error* error,
                  const ResultCallback& callback,
                  int timeout) override;

 private:
  // Callbacks for SetCarrier async call.
  void OnSetCarrierSuccess(const ResultCallback& callback);
  void OnSetCarrierFailure(const ResultCallback& callback,
                           brillo::Error* dbus_error);

  std::unique_ptr<org::chromium::ModemManager::Modem::GobiProxy> proxy_;

  base::WeakPtrFactory<ChromeosModemGobiProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosModemGobiProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_MODEM_GOBI_PROXY_H_
