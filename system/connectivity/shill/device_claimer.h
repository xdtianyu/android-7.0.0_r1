//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef SHILL_DEVICE_CLAIMER_H_
#define SHILL_DEVICE_CLAIMER_H_

#include <set>
#include <string>

#include <base/callback.h>

#include "shill/error.h"
#include "shill/rpc_service_watcher_interface.h"

namespace shill {

class ControlInterface;
class DeviceInfo;

// Provide an abstraction for remote service to claim/release devices
// from/to shill.
class DeviceClaimer {
 public:
  DeviceClaimer(const std::string& service_name,
                DeviceInfo* device_info,
                bool default_claimer);
  virtual ~DeviceClaimer();

  virtual bool Claim(const std::string& device_name, Error* error);
  virtual bool Release(const std::string& device_name, Error* error);

  // Return true if there are devices claimed by this claimer, false
  // otherwise.
  virtual bool DevicesClaimed();

  // Return true if the specified device is released by this claimer, false
  // otherwise.
  virtual bool IsDeviceReleased(const std::string& device_name);

  const std::string& name() const { return service_name_; }

  virtual bool default_claimer() const { return default_claimer_; }

  const std::set<std::string>& claimed_device_names() const {
    return claimed_device_names_;
  }

 private:
  // Watcher for monitoring the remote RPC service of the claimer.
  std::unique_ptr<RPCServiceWatcherInterface> service_watcher_;
  // The name of devices that have been claimed by this claimer.
  std::set<std::string> claimed_device_names_;
  // The name of devices that have been released by this claimer.
  std::set<std::string> released_device_names_;
  // Service name of the claimer.
  std::string service_name_;

  DeviceInfo* device_info_;

  // Flag indicating if this is the default claimer. When set to true, this
  // claimer will only be deleted when shill terminates.
  bool default_claimer_;

  DISALLOW_COPY_AND_ASSIGN(DeviceClaimer);
};

}  // namespace shill

#endif  // SHILL_DEVICE_CLAIMER_H_
