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

#ifndef SHILL_CELLULAR_MM1_MODEM_PROXY_INTERFACE_H_
#define SHILL_CELLULAR_MM1_MODEM_PROXY_INTERFACE_H_

#include <string>
#include <vector>

#include "shill/callbacks.h"

namespace shill {
class Error;

namespace mm1 {

typedef base::Callback<void(int32_t,
                            int32_t, uint32_t)> ModemStateChangedSignalCallback;

// These are the methods that a org.freedesktop.ModemManager1.Modem
// proxy must support. The interface is provided so that it can be
// mocked in tests. All calls are made asynchronously. Call completion
// is signalled via the callbacks passed to the methods.
class ModemProxyInterface {
 public:
  virtual ~ModemProxyInterface() {}

  virtual void Enable(bool enable,
                      Error* error,
                      const ResultCallback& callback,
                      int timeout) = 0;
  virtual void CreateBearer(const KeyValueStore& properties,
                            Error* error,
                            const RpcIdentifierCallback& callback,
                            int timeout) = 0;
  virtual void DeleteBearer(const std::string& bearer,
                            Error* error,
                            const ResultCallback& callback,
                            int timeout) = 0;
  virtual void Reset(Error* error,
                     const ResultCallback& callback,
                     int timeout) = 0;
  virtual void FactoryReset(const std::string& code,
                            Error* error,
                            const ResultCallback& callback,
                            int timeout) = 0;
  virtual void SetCurrentCapabilities(uint32_t capabilities,
                                      Error* error,
                                      const ResultCallback& callback,
                                      int timeout) = 0;
  virtual void SetCurrentModes(uint32_t allowed_modes,
                               uint32_t preferred_mode,
                               Error* error,
                               const ResultCallback& callback,
                               int timeout) = 0;
  virtual void SetCurrentBands(const std::vector<uint32_t>& bands,
                               Error* error,
                               const ResultCallback& callback,
                               int timeout) = 0;
  virtual void Command(const std::string& cmd,
                       uint32_t user_timeout,
                       Error* error,
                       const StringCallback& callback,
                       int timeout) = 0;
  virtual void SetPowerState(uint32_t power_state,
                             Error* error,
                             const ResultCallback& callback,
                             int timeout) = 0;


  virtual void set_state_changed_callback(
      const ModemStateChangedSignalCallback& callback) = 0;
};

}  // namespace mm1
}  // namespace shill

#endif  // SHILL_CELLULAR_MM1_MODEM_PROXY_INTERFACE_H_
