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

#ifndef SHILL_WIMAX_WIMAX_MANAGER_PROXY_INTERFACE_H_
#define SHILL_WIMAX_WIMAX_MANAGER_PROXY_INTERFACE_H_

#include <vector>

#include <base/callback.h>

#include "shill/accessor_interface.h"

namespace shill {

class Error;

// These are the methods that a WiMaxManager proxy must support. The interface
// is provided so that it can be mocked in tests.
class WiMaxManagerProxyInterface {
 public:
  typedef base::Callback<void(const RpcIdentifiers&)> DevicesChangedCallback;

  virtual ~WiMaxManagerProxyInterface() {}

  virtual void set_devices_changed_callback(
      const DevicesChangedCallback& callback) = 0;

  // Properties.
  virtual RpcIdentifiers Devices(Error* error) = 0;
};

}  // namespace shill

#endif  // SHILL_WIMAX_WIMAX_MANAGER_PROXY_INTERFACE_H_
