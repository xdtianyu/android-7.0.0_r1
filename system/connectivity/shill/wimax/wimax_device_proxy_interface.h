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

#ifndef SHILL_WIMAX_WIMAX_DEVICE_PROXY_INTERFACE_H_
#define SHILL_WIMAX_WIMAX_DEVICE_PROXY_INTERFACE_H_

#include <string>

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/callbacks.h"

namespace shill {

class Error;
class KeyValueStore;

// These are the methods that a WiMaxManager.Device proxy must support. The
// interface is provided so that it can be mocked in tests.
class WiMaxDeviceProxyInterface {
 public:
  typedef base::Callback<void(const RpcIdentifiers&)> NetworksChangedCallback;
  typedef base::Callback<void(
      wimax_manager::DeviceStatus)> StatusChangedCallback;

  virtual ~WiMaxDeviceProxyInterface() {}

  virtual void Enable(Error* error,
                      const ResultCallback& callback,
                      int timeout) = 0;
  virtual void Disable(Error* error,
                       const ResultCallback& callback,
                       int timeout) = 0;
  virtual void ScanNetworks(Error* error,
                            const ResultCallback& callback,
                            int timeout) = 0;
  virtual void Connect(const RpcIdentifier& network,
                       const KeyValueStore& parameters,
                       Error* error,
                       const ResultCallback& callback,
                       int timeout) = 0;
  virtual void Disconnect(Error* error,
                          const ResultCallback& callback,
                          int timeout) = 0;

  virtual void set_networks_changed_callback(
      const NetworksChangedCallback& callback) = 0;
  virtual void set_status_changed_callback(
      const StatusChangedCallback& callback) = 0;

  // Properties.
  virtual uint8_t Index(Error* error) = 0;
  virtual std::string Name(Error* error) = 0;
  virtual RpcIdentifiers Networks(Error* error) = 0;
};

}  // namespace shill

#endif  // SHILL_WIMAX_WIMAX_DEVICE_PROXY_INTERFACE_H_
