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

#ifndef SHILL_DBUS_CHROMEOS_MODEM_PROXY_H_
#define SHILL_DBUS_CHROMEOS_MODEM_PROXY_H_

#include <string>
#include <tuple>

#include <base/macros.h>

#include "cellular/dbus-proxies.h"
#include "shill/cellular/modem_proxy_interface.h"

namespace shill {

// A proxy to (old) ModemManager.Modem.
class ChromeosModemProxy : public ModemProxyInterface {
 public:
  // Constructs a ModemManager.Modem DBus object proxy at |path| owned by
  // |service|.
  ChromeosModemProxy(const scoped_refptr<dbus::Bus>& bus,
                     const std::string& path,
                     const std::string& service);
  ~ChromeosModemProxy() override;

  // Inherited from ModemProxyInterface.
  void Enable(bool enable,
              Error* error,
              const ResultCallback& callback,
              int timeout) override;
  void Disconnect(Error* error,
                  const ResultCallback& callback,
                  int timeout) override;
  void GetModemInfo(Error* error,
                    const ModemInfoCallback& callback,
                    int timeout) override;

  void set_state_changed_callback(
      const ModemStateChangedSignalCallback& callback) override {
    state_changed_callback_ = callback;
  }

 private:
  typedef std::tuple<std::string, std::string, std::string> ModemHardwareInfo;

  // Signal handler.
  void StateChanged(uint32_t old, uint32_t _new, uint32_t reason);

  // Callbacks for Enable async call.
  void OnEnableSuccess(const ResultCallback& callback);
  void OnEnableFailure(const ResultCallback& callback,
                       brillo::Error* dbus_error);

  // Callback for GetInfo async call.
  void OnGetInfoSuccess(const ModemInfoCallback& callback,
                        const ModemHardwareInfo& info);
  void OnGetInfoFailure(const ModemInfoCallback& callback,
                        brillo::Error* dbus_error);

  // Callback for Disconnect async call.
  void OnDisconnectSuccess(const ResultCallback& callback);
  void OnDisconnectFailure(const ResultCallback& callback,
                           brillo::Error* dbus_error);

  // Called when signal is connected to the ObjectProxy.
  void OnSignalConnected(const std::string& interface_name,
                         const std::string& signal_name,
                         bool success);

  ModemStateChangedSignalCallback state_changed_callback_;

  std::unique_ptr<org::freedesktop::ModemManager::ModemProxy> proxy_;

  base::WeakPtrFactory<ChromeosModemProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosModemProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_MODEM_PROXY_H_
