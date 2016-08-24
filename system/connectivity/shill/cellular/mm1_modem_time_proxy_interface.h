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

#ifndef SHILL_CELLULAR_MM1_MODEM_TIME_PROXY_INTERFACE_H_
#define SHILL_CELLULAR_MM1_MODEM_TIME_PROXY_INTERFACE_H_

#include <string>

#include "shill/callbacks.h"

namespace shill {
class Error;

namespace mm1 {

typedef base::Callback<void(const std::string&)>
    NetworkTimeChangedSignalCallback;

// These are the methods that an org.freedesktop.ModemManager1.Modem.Time
// proxy must support. The interface is provided so that it can be mocked
// in tests. All calls are made asynchronously. Call completion is signalled
// via callbacks passed to the methods.
class ModemTimeProxyInterface {
 public:
  virtual ~ModemTimeProxyInterface() {}

  virtual void GetNetworkTime(Error* error,
                              const StringCallback& callback,
                              int timeout) = 0;

  virtual void set_network_time_changed_callback(
      const NetworkTimeChangedSignalCallback& callback) = 0;
};

}  // namespace mm1
}  // namespace shill

#endif  // SHILL_CELLULAR_MM1_MODEM_TIME_PROXY_INTERFACE_H_
