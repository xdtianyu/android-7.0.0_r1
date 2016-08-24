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

#ifndef SHILL_DBUS_CHROMEOS_MODEM_SIMPLE_PROXY_H_
#define SHILL_DBUS_CHROMEOS_MODEM_SIMPLE_PROXY_H_

#include <string>

#include <base/macros.h>

#include "cellular/dbus-proxies.h"
#include "shill/cellular/modem_simple_proxy_interface.h"

namespace shill {

// A proxy to (old) ModemManager.Modem.Simple.
class ChromeosModemSimpleProxy : public ModemSimpleProxyInterface {
 public:
  // Constructs a ModemManager.Modem.Simple DBus object proxy at
  // |path| owned by |service|.
  ChromeosModemSimpleProxy(const scoped_refptr<dbus::Bus>& bus,
                           const std::string& path,
                           const std::string& service);
  ~ChromeosModemSimpleProxy() override;

  // Inherited from ModemSimpleProxyInterface.
  void GetModemStatus(Error* error,
                      const KeyValueStoreCallback& callback,
                      int timeout) override;
  void Connect(const KeyValueStore& properties,
               Error* error,
               const ResultCallback& callback,
               int timeout) override;

 private:
  // Callbacks for GetStatus async call.
  void OnGetStatusSuccess(const KeyValueStoreCallback& callback,
                          const brillo::VariantDictionary& props);
  void OnGetStatusFailure(const KeyValueStoreCallback& callback,
                          brillo::Error* dbus_error);

  // Callbacks for Connect async call.
  void OnConnectSuccess(const ResultCallback& callback);
  void OnConnectFailure(const ResultCallback& callback,
                        brillo::Error* dbus_error);

  std::unique_ptr<org::freedesktop::ModemManager::Modem::SimpleProxy> proxy_;

  base::WeakPtrFactory<ChromeosModemSimpleProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosModemSimpleProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_MODEM_SIMPLE_PROXY_H_
