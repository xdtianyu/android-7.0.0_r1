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

#ifndef SHILL_CELLULAR_MODEM_PROXY_INTERFACE_H_
#define SHILL_CELLULAR_MODEM_PROXY_INTERFACE_H_

#include <string>

#include "shill/callbacks.h"

namespace shill {

class CallContext;
class Error;

typedef base::Callback<void(uint32_t, uint32_t, uint32_t)>
    ModemStateChangedSignalCallback;
typedef base::Callback<void(const std::string& manufacturer,
                            const std::string& modem,
                            const std::string& version,
                            const Error&)> ModemInfoCallback;

// These are the methods that a ModemManager.Modem proxy must support. The
// interface is provided so that it can be mocked in tests. All calls are
// made asynchronously.
class ModemProxyInterface {
 public:
  virtual ~ModemProxyInterface() {}

  virtual void Enable(bool enable, Error* error,
                      const ResultCallback& callback, int timeout) = 0;
  virtual void Disconnect(Error* error, const ResultCallback& callback,
                          int timeout) = 0;
  virtual void GetModemInfo(Error* error, const ModemInfoCallback& callback,
                            int timeout) = 0;

  virtual void set_state_changed_callback(
      const ModemStateChangedSignalCallback& callback) = 0;
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MODEM_PROXY_INTERFACE_H_
