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

#ifndef SHILL_DBUS_CHROMEOS_MODEM_GSM_CARD_PROXY_H_
#define SHILL_DBUS_CHROMEOS_MODEM_GSM_CARD_PROXY_H_

#include <string>

#include <base/macros.h>

#include "cellular/dbus-proxies.h"
#include "shill/cellular/modem_gsm_card_proxy_interface.h"

namespace shill {

// A proxy to (old) ModemManager.Modem.Gsm.Card.
class ChromeosModemGSMCardProxy : public ModemGSMCardProxyInterface {
 public:
  // Constructs a ModemManager.Modem.Gsm.Card DBus
  // object proxy at |path| owned by |service|.
  ChromeosModemGSMCardProxy(const scoped_refptr<dbus::Bus>& bus,
                            const std::string& path,
                            const std::string& service);
  ~ChromeosModemGSMCardProxy() override;

  // Inherited from ModemGSMCardProxyInterface.
  void GetIMEI(Error* error,
               const GSMIdentifierCallback& callback,
               int timeout) override;
  void GetIMSI(Error* error,
               const GSMIdentifierCallback& callback,
               int timeout) override;
  void GetSPN(Error* error,
              const GSMIdentifierCallback& callback,
              int timeout) override;
  void GetMSISDN(Error* error,
                 const GSMIdentifierCallback& callback,
                 int timeout) override;
  void EnablePIN(const std::string& pin,
                 bool enabled,
                 Error* error,
                 const ResultCallback& callback,
                 int timeout) override;
  void SendPIN(const std::string& pin,
               Error* error,
               const ResultCallback& callback,
               int timeout) override;
  void SendPUK(const std::string& puk,
               const std::string& pin,
               Error* error,
               const ResultCallback& callback,
               int timeout) override;
  void ChangePIN(const std::string& old_pin,
                 const std::string& new_pin,
                 Error* error,
                 const ResultCallback& callback,
                 int timeout) override;
  uint32_t EnabledFacilityLocks() override;

 private:
  class PropertySet : public dbus::PropertySet {
   public:
    PropertySet(dbus::ObjectProxy* object_proxy,
                const std::string& interface_name,
                const PropertyChangedCallback& callback);
    brillo::dbus_utils::Property<uint32_t> enabled_facility_locks;

   private:
    DISALLOW_COPY_AND_ASSIGN(PropertySet);
  };

  static const char kPropertyEnabledFacilityLocks[];

  // Callbacks for various GSMIdentifier Get async calls.
  void OnGetGSMIdentifierSuccess(const GSMIdentifierCallback& callback,
                                 const std::string& identifier_name,
                                 const std::string& identifier_value);
  void OnGetGSMIdentifierFailure(const GSMIdentifierCallback& callback,
                                 const std::string& identifier_name,
                                 brillo::Error* dbus_error);
  void OnOperationSuccess(const ResultCallback& callback,
                          const std::string& operation_name);
  void OnOperationFailure(const ResultCallback& callback,
                          const std::string& operation_name,
                          brillo::Error* dbus_error);

  // Callback invoked when the value of property |property_name| is changed.
  void OnPropertyChanged(const std::string& property_name);

  std::unique_ptr<org::freedesktop::ModemManager::Modem::Gsm::CardProxy> proxy_;
  std::unique_ptr<PropertySet> properties_;

  base::WeakPtrFactory<ChromeosModemGSMCardProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosModemGSMCardProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_MODEM_GSM_CARD_PROXY_H_
