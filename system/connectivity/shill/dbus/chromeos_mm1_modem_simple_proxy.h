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

#ifndef SHILL_DBUS_CHROMEOS_MM1_MODEM_SIMPLE_PROXY_H_
#define SHILL_DBUS_CHROMEOS_MM1_MODEM_SIMPLE_PROXY_H_

#include <string>

#include "cellular/dbus-proxies.h"
#include "shill/cellular/mm1_modem_simple_proxy_interface.h"

namespace shill {
namespace mm1 {

// A proxy to org.freedesktop.ModemManager1.Modem.Simple.
class ChromeosModemSimpleProxy : public ModemSimpleProxyInterface {
 public:
  // Constructs a org.freedesktop.ModemManager1.Modem.Simple DBus
  // object proxy at |path| owned by |service|.
  ChromeosModemSimpleProxy(const scoped_refptr<dbus::Bus>& bus,
                           const std::string& path,
                           const std::string& service);
  ~ChromeosModemSimpleProxy() override;

  // Inherited methods from SimpleProxyInterface.
  void Connect(
      const KeyValueStore& properties,
      Error* error,
      const RpcIdentifierCallback& callback,
      int timeout) override;
  void Disconnect(const std::string& bearer,
                  Error* error,
                  const ResultCallback& callback,
                  int timeout) override;
  void GetStatus(Error* error,
                 const KeyValueStoreCallback& callback,
                 int timeout) override;

 private:
  // Callbacks for Connect async call.
  void OnConnectSuccess(const RpcIdentifierCallback& callback,
                        const dbus::ObjectPath& path);
  void OnConnectFailure(const RpcIdentifierCallback& callback,
                        brillo::Error* error);

  // Callbacks for Disconnect async call.
  void OnDisconnectSuccess(const ResultCallback& callback);
  void OnDisconnectFailure(const ResultCallback& callbac,
                           brillo::Error* dbus_error);

  // Callbacks for GetStatus async call.
  void OnGetStatusSuccess(const KeyValueStoreCallback& callback,
                          const brillo::VariantDictionary& status);
  void OnGetStatusFailure(const KeyValueStoreCallback& callback,
                          brillo::Error* error);

  std::unique_ptr<org::freedesktop::ModemManager1::Modem::SimpleProxy> proxy_;

  base::WeakPtrFactory<ChromeosModemSimpleProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosModemSimpleProxy);
};

}  // namespace mm1
}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_MM1_MODEM_SIMPLE_PROXY_H_
