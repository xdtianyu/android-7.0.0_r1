//
// Copyright (C) 2012 The Android Open Source Project
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

#ifndef SHILL_CELLULAR_MODEM_GSM_CARD_PROXY_INTERFACE_H_
#define SHILL_CELLULAR_MODEM_GSM_CARD_PROXY_INTERFACE_H_

#include <string>

#include "shill/callbacks.h"

namespace shill {

class Error;
typedef base::Callback<void(const std::string&,
                            const Error&)> GSMIdentifierCallback;

// These are the methods that a ModemManager.Modem.Gsm.Card proxy must
// support. The interface is provided so that it can be mocked in tests.
// All calls are made asynchronously.
class ModemGSMCardProxyInterface {
 public:
  virtual ~ModemGSMCardProxyInterface() {}

  virtual void GetIMEI(Error* error, const GSMIdentifierCallback& callback,
                       int timeout) = 0;
  virtual void GetIMSI(Error* error, const GSMIdentifierCallback& callback,
                       int timeout) = 0;
  virtual void GetSPN(Error* error, const GSMIdentifierCallback& callback,
                      int timeout) = 0;
  virtual void GetMSISDN(Error* error, const GSMIdentifierCallback& callback,
                         int timeout) = 0;
  virtual void EnablePIN(const std::string& pin, bool enabled,
                         Error* error, const ResultCallback& callback,
                         int timeout) = 0;
  virtual void SendPIN(const std::string& pin,
                       Error* error, const ResultCallback& callback,
                       int timeout) = 0;
  virtual void SendPUK(const std::string& puk, const std::string& pin,
                       Error* error, const ResultCallback& callback,
                       int timeout) = 0;
  virtual void ChangePIN(const std::string& old_pin,
                         const std::string& new_pin,
                         Error* error, const ResultCallback& callback,
                         int timeout) = 0;

  // Properties.
  virtual uint32_t EnabledFacilityLocks() = 0;
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MODEM_GSM_CARD_PROXY_INTERFACE_H_
